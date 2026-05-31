#include <cmath>
#include <cstdlib>

#include "modelfitting.h"

#include "models.h"
#include "fittingfactory.h"
#include "gpgpu.h"
#include "mathfuncs.h"

inline void ConvertToBool(const std::vector<char> &in, std::vector<bool> &out) {
    for(int i = 0; i < (int)in.size(); i++) {
		if(in[i] == 'Y')
			out.push_back(true);
		else
			out.push_back(false);
	}
}

EXPORTED bool CreateModel(const vector<double> ffx, const vector<double> ffy,
						  vector<double>& resy, const vector<double>& bgy, const vector<bool>& mask, paramStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, int *pStop) {
		return CreateModelU(ffx, ffy, resy, bgy, mask, p, paramErrors, modelErrors, NULL, pStop, NULL);
}

void ParameterToVectorIndex(const Parameter& param, int index, VectorXd& a,
	VectorXi& ia, cons& a_min, cons& a_max) {
	a(index) = param.value;
	ia(index) = param.isMutable;

	if (param.isConstrained) {
		a_min.num(index) = param.consMin;
		a_max.num(index) = param.consMax;

		a_min.index(index) = param.consMinIndex;
		if (a_min.index(index) >= index)
			a_min.index(index)++;

		a_max.index(index) = param.consMaxIndex;
		if (a_max.index(index) >= index)
			a_max.index(index)++;

		a_min.link(index) = a_max.link(index) = param.linkIndex;
	}
}

/**
 *  Fits a form factor with the possibility to update a graph.
 *  ffx:            Input data X vector
 *  ffy:            Input data Y vector
 *  my:             Output model (may also be pre-filled with
 *                  existing structure factor to fit with)
 *  bgy:            Input additive Y background vector
 *  p:              Input/Output parameters data structure
 *  GraphModify:    A callback used to update the graph while fitting
 *  pStop:          A pointer to a stop signal - 0 means run, 1 means stop
 *  gauss:			A flag to indicate use of a gaussian or discrete model
 *  ProgressReport: A callback used to send back the progress
 **/
EXPORTED bool CreateModelU(const std::vector<double> inffx,  const std::vector<double> inffy, 
						std::vector<double>& my, const std::vector<double>& inbgy, 
						const std::vector<bool>& mask, paramStruct *p, 
						std::vector<double>& paramErrors, 
						std::vector<double>& modelErrors, plotFunc GraphModify,
						int *pStop, progressFunc ProgressReport) {
	bool success = true;
	int layers = p->layers;
	std::vector<double> ffx = inffx, ffy = inffy, bgy = inbgy;

	// If there is no Y, we generate SF = 1
	if(my.size() == 0)
		my.resize(ffx.size(), 1.0);
	
	
	// Check to see if there are masked elements and crop them from relevant vectors
	if(mask.size() == inffx.size()) {
		for(int k = mask.size() - 1; k >= 0; k--) {
			if(mask.at(k)) {
				bgy.erase(bgy.begin() + k);
				ffx.erase(ffx.begin() + k);
				ffy.erase(ffy.begin() + k);
				my.erase(my.begin() + k);
			}	//if
		}	//for
	}	//if


	//////////////////////////////////////////////////////////////////////////
	// Initialization

	// Initializing vectors
	int extraParams = p->extraParams.size();
	int ma = (layers * p->model->GetNumLayerParams()) + extraParams, ndata;
	
	VectorXd a  = VectorXd::Zero(ma); // Parameter vector
	VectorXi ia = VectorXi::Zero(ma); // Mutability vector
	cons a_min(ma); // Fit range/constraint vector
	cons a_max(ma); // Fit range/constraint vector
	Model *finalModel = p->model;

	// Initializing additional GUI parameters
	SetSignal(pStop);
	p->model->SetStop(pStop);
	finalModel->SetStop(pStop);


	// Initializing parameter vector from our input vectors	
	for(int i = 0; i < p->model->GetNumLayerParams(); i++)
		for(int j = 0; j < layers; j++) {
			ParameterToVectorIndex(p->params[i][j], i * layers + j, a, ia, 
								   a_min, a_max);
			// Taking model modifiers into account
			// Polydispersity
			if(p->params[i][j].sigma > 0.0) {
				finalModel = new PolydisperseModel(finalModel, i * layers + j,
												 p->params[i][j].sigma, *p,
												 (finalModel != p->model));
			}
			// END of model modifiers
		}

	// Initializing extra parameters
	for(int i = 0; i < extraParams; i++) {
		ParameterToVectorIndex(p->extraParams[i], ma - extraParams + i, a, ia, 
							   a_min, a_max);
		// Taking model modifiers into account
		// Polydispersity
		if(p->extraParams[i].sigma > 0.0) {
			finalModel = new PolydisperseModel(finalModel, ma - extraParams + i,
											   p->extraParams[i].sigma, *p,
											   (finalModel != p->model));
		}
		// END of model modifiers
	}

	// Initializing graph vectors
	vector<double> x = ffx, y = ffy;
	ndata = x.size();

	if(isLogFitting())
		for(int i = 0; i < (int)y.size(); i++)
			y[i] = log10(y[i]);

	// Initializing weight function: [w(x) = sqrt(x) + 1]
	vector<double> weights (ndata, 0.0);
	for(int i = 0; i < ndata; i++)
		weights[i] = (sqrt(fabs(y[i])) + 1.0);

	// Adjust linked parameter indices and relative constraints
	int nlp = p->model->GetNumLayerParams();
	for(int i = 0; i < nlp; i++) {
		for(int j = 0; j < layers; j++) {
			if(a_max.link[i * layers + j] >= 0)
				a_max.link[i * layers + j] += i * layers;
			if(a_max.index[i * layers + j] >= 0)
				a_max.index[i * layers + j] += i * layers;
			if(a_min.index[i * layers + j] >= 0)
				a_min.index[i * layers + j] += i * layers;
		}
	}

	if(!p->bConstrain) {
		a_min.num	= VectorXd::Constant(a_min.num.size(), -std::numeric_limits<double>::infinity());
		a_max.num	= VectorXd::Constant(a_max.num.size(), std::numeric_limits<double>::infinity());
		a_max.index	= VectorXi::Constant(a_max.num.size(), -1);
		a_min.index	= VectorXi::Constant(a_min.num.size(), -1);

	}

	// Initializing Levenberg-Marquardt fitter
	ModelFitter *fitter = CreateFitter(finalModel, x, y, my, bgy, weights, 
		a, ia, &a_min, &a_max, paramErrors, modelErrors, layers);

	// No mutables
	if(fitter->GetError()) {
		delete fitter;

		my.clear();

		// Destroy any remains of model modifiers
		if(finalModel != p->model)
			delete finalModel;

		return GenerateModel(inffx, my, p, pStop);
	}

	// Initializing visual objects
	vector<double> intermY (ndata, 0.0); // Intermediate model for realtime graph plotting

	//////////////////////////////////////////////////////////////////////////
	// Fitting

	// The main fitting loop (each iteration yields a different parameter structure)
	for(int i = 0; i < GetFitIterations(); i++) {
		
		// Fitting
		fitter->FitIteration();
		a = fitter->GetResult();

		if(pStop && *pStop)
			break;
		if(fitter->GetError()) {
			success = false;
			break;
		}

		// Graph update during fitting
		if(GraphModify) {
			VectorXd tmp = fitter->GetInterimRes();
			for(int r = 0; r < int(my.size()); r++)
				intermY[r] = tmp(r);

			// Modifying the generated graph
			GraphModify(ffx, intermY);
		}

		// Progress Report
		if(ProgressReport)
			ProgressReport(int(double(i) / double(GetFitIterations()) * 100.0));
	}

	//////////////////////////////////////////////////////////////////////////
	// Finalization

	fitter->calcErrors();

	for(int bbq = 0; bbq < ia.size(); bbq++) {
		if (ia(bbq) == 0)
			paramErrors.insert(paramErrors.begin() + bbq, -1.0);
	}

	if(!pStop || (pStop && !*pStop))
		success = !fitter->GetError();

	delete fitter;

	// Clearing the stop signal so it won't interrupt us while we generate the final model
	ClearSignal();
	
	// Saving back the parameters and extra parameters to the paramStruct
	for(int i = 0; i < p->model->GetNumLayerParams(); i++)
		for(int j = 0; j < layers; j++)
			p->params[i][j].value = a(i * layers + j);

	for(int i = 0; i < extraParams; i++)
		p->extraParams[i].value = a(ma - extraParams + i);

	// After fitting the model, generate the final graph to show to the user
	my.clear();
	if(pStop && *pStop != 2) {	// == 2 is set only when the FF window is closed
		*pStop = 0;		// So that the generate will work for the slower models

		GenerateModel(inffx, my, p, pStop);
	}

	// Destroy any remains of model modifiers
	if(finalModel != p->model)
		delete finalModel;

	return success;
}

EXPORTED bool GenerateModel(const std::vector<double> x, std::vector<double>& genY,
							paramStruct *p, int *pStop) {
	return GenerateModelU(x, genY, std::vector<double>(), p, NULL, pStop, NULL);
}

EXPORTED bool GenerateModelU(const vector<double> x, vector<double>& genY, const std::vector<double>& bgy, paramStruct *p,
							plotFunc GraphModify, int *pStop, progressFunc ProgressReport) {

	int layers = p->layers;
	int extraParams = p->extraParams.size();
	int ma = (layers * p->model->GetNumLayerParams()) + extraParams;

	VectorXd a = VectorXd::Zero(ma); // Parameter Vector
	VectorXi ia = VectorXi::Zero(ma); // Mutability Vector
	Model *finalModel = p->model;

	if(genY.size() == 0 || genY.size() != x.size())
		genY.resize(x.size(), 1.0);

	for(int i = 0; i < p->model->GetNumLayerParams(); i++)
		for(int j = 0; j < layers; j++) {
			a(i * layers + j) = p->params[i][j].value;

			// Taking model modifiers into account
			// Polydispersity
			if(p->params[i][j].sigma > 0.0) {
				finalModel = new PolydisperseModel(finalModel, i * layers + j,
													p->params[i][j].sigma, *p,
													(finalModel != p->model));
			}
			// END of model modifiers
		}

	for(int i = 0; i < extraParams; i++) {
		a(ma - extraParams + i) = p->extraParams[i].value;

		// Taking model modifiers into account
		// Polydispersity
		if(p->extraParams[i].sigma > 0.0) {
			finalModel = new PolydisperseModel(finalModel, ma - extraParams + i,
											   p->extraParams[i].sigma, *p,
											   (finalModel != p->model));
		}
		// END of model modifiers
	}

	VectorXd guess = a;
	int guessLayers = layers;

	// The new layer model is created based on the Electron Density Profile
	EDPFunction *edp = finalModel->GetEDProfile().func;
	if(finalModel->IsLayerBased() && edp && x.size() > 1)
		guess = edp->ComputeParamVector(finalModel, a, x, layers, guessLayers);
	// END of profile reshaping

	vector<double> genX, intermY;
	
	// GPU generation overrides loop
//	if(isGPUBackend()) {
//		VectorXd& y = finalModel->GPUCalculate(x, guessLayers, guess);
//
//#pragma omp parallel for
//		for(int i = 0; i < y.size(); i++)
//			genY[i] = y[i];
//
//		// Destroy any remains of model modifiers
//		if(finalModel != p->model)
//			delete finalModel;
//
//		return true;
//	}

	// If we are in live generation mode (doesn't work for PD models, so it's disabled)
	if(GraphModify && (finalModel->GetName()).compare("Polydisperse Model") != 0) {
		finalModel->PreCalculate(guess, guessLayers);
		VectorXd dummy;

		for (unsigned int i = 0; i < x.size(); i++) {
			double intensity;
			
			// NOTE: We don't multiply with SF here, we do the multiplication in the SF generation
			intensity = finalModel->LiveCalculate(x[i], guessLayers, dummy);
	        
			if(pStop)
				if(*pStop) {
					// Destroy any remains of model modifiers
					if(finalModel != p->model)
						delete finalModel;

					return false;
				}
			
			genX.push_back(x[i]);
			intermY.push_back((genY[i] * intensity) + bgy[i]);
			// Modifying the generated graph
			GraphModify(genX, intermY);
			
			if(ProgressReport)
				ProgressReport(int(float(i) / float(x.size()) * 100.0));
			
			genY[i] = intensity;
		}

		GraphModify(x, genY);
	} else {
		VectorXd& y = finalModel->CalculateVector(x, guessLayers, guess);

#pragma omp parallel for
		for(int i = 0; i < y.size(); i++)
			genY[i] = y[i];
	}

	// Destroy any remains of model modifiers
	if(finalModel != p->model)
		delete finalModel;
	
	return true;
}
