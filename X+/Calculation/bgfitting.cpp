
#include "Eigen/Core"
using namespace Eigen;

#include "bgfitting.h"
#include "models.h"
#include "Model.h"

#include "fittingfactory.h"
#include "mathfuncs.h"

typedef double (*bgFunction)(double x, double base, double decay, double xcenter);

bgFunction GetBackgroundShape(BGFuncType type) {
	switch(type) {
		default:
		case BG_EXPONENT:
			return exponentDecay;
		case BG_LINEAR:
			return linearFunction;
		case BG_POWER:
			return powerFunction;
	}
}

// Dummy function for Levenberg-Marquardt so that Background can be fit
double BackgroundIntensity(double q, VectorXd& a, int ma, int nd) {
	double result = 0.0;
	for(int i = 0; i < nd; i++) {
		bgFunction func = GetBackgroundShape((BGFuncType((int)a[i])));
		result += func(q, a[nd + i], a[2*nd + i], a[3*nd + i]);
	}

	return result; // * a[ma - 1];
}

EXPORTED bool GenerateBackground(const std::vector<double> bgx, std::vector<double>& genY,
				 				 bgStruct *p) {
	return GenerateBackgroundU(bgx, genY, std::vector<double>(), p, NULL, NULL, NULL);
}

EXPORTED bool GenerateBackgroundU(const std::vector<double> x, std::vector<double>& genY, 
								  const std::vector<double>& signaly, bgStruct *p, 
								  plotFunc GraphModify, int *pStop, progressFunc ProgressReport) {
	// If there is no Y, we generate FF = 1
	if(genY.size() == 0 || genY.size() != x.size())
		genY.resize(x.size(), 0.0);
	
	// No functions
	if(p->type.size() == 0) {
		genY.clear();
		genY.resize(x.size(), 0.0);
		return true;
	}

	std::vector<double> bg (genY.size(), 0.0), genX, intermY;

	for(int i = 0; i < (int)genY.size(); i++) {
		for(int j = 0; j < (int)p->type.size(); j++) {
			bgFunction func = GetBackgroundShape(p->type[j]);
			bg.at(i) += func(x[i], p->base.at(j), p->decay.at(j), p->center.at(j));
		}

		//bg.at(i) *= p->amp;
		
		if(pStop) {
			if(*pStop)
				break;
	
			if(GraphModify) {
				genX.push_back(x[i]);
				intermY.push_back(bg[i] + signaly[i]);
				// Modifying the generated graph
				GraphModify(genX, intermY);
			}
	
			if(ProgressReport)
				ProgressReport(int(double(i) / double(x.size()) * 100.0));
		}
		genY.at(i) = bg.at(i);
	}

	return true;
}

EXPORTED bool FitBackground(const vector<double> bgx, const vector<double> bgy, 
							vector<double>& resy, const std::vector<double>& signaly,
							const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors) {
	return FitBackgroundU(bgx, bgy, resy, signaly, mask, p, paramErrors, modelErrors, NULL, NULL, NULL);
}

EXPORTED bool FitBackgroundU(const vector<double> inbgx, const vector<double> inbgy, 
						     vector<double>& my, const std::vector<double>& insignaly, 
							 const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
						     int *pStop, progressFunc ProgressReport) {
	bool success = true;
	int funcs = p->type.size();
	// No functions
	if(funcs == 0)
		return true;

	std::vector<double> bgx = inbgx, bgy = inbgy, signaly = insignaly;
	// If there is no Y, we generate FF = 1
	if(my.size() == 0)
		my.resize(bgx.size(), 0.0);

	std::vector<double> thisVectorHasOnlyOnesInIt (bgx.size(), 1.0);

	// Check to see if there are masked elements and crop them from relevant vectors
	if(mask.size() == inbgx.size()) {
		for(int k = mask.size() - 1; k >= 0; k--) {
			if(mask.at(k)) {
				signaly.erase(signaly.begin() + k);
				bgx.erase(bgx.begin() + k);
				bgy.erase(bgy.begin() + k);
				my.erase(my.begin() + k);
			}	//if
		}	//for
	}	//if

	//////////////////////////////////////////////////////////////////////////
	// Initialization
	
	// Initializing additional GUI parameters
	SetSignal(pStop);

	// Initializing vectors
	int ma = funcs * 4 + 1, ndata;
	VectorXd a  = VectorXd::Zero(ma); // Parameter vector
	VectorXi ia = VectorXi::Zero(ma); // Mutability vector
	cons a_min(ma), a_max(ma);

	for(int i = 0; i < funcs; i++) {
		a[i] = p->type[i];
		a[i + funcs] = p->base[i];
		a[i + funcs + funcs] = p->decay[i];
		a[i + funcs + funcs + funcs] = p->center[i];

		ia[i] = 0;
		ia[i + funcs] = p->baseMutable[i] == 'Y';
		ia[i + funcs + funcs] = p->decayMutable[i] == 'Y';
		ia[i + funcs + funcs + funcs] = p->centerMutable[i] == 'Y';

		a_min.num[i + funcs] = p->basemin[i];
		a_max.num[i + funcs] = p->basemax[i];
		a_min.num[i + funcs + funcs] = p->decmin[i];
		a_max.num[i + funcs + funcs] = p->decmax[i];
		a_min.num[i + funcs + funcs + funcs] = p->centermin[i];
		a_max.num[i + funcs + funcs + funcs] = p->centermax[i];
	}

	// Initializing graph vectors
	std::vector<double> x = bgx, y = bgy;
	ndata = x.size();

	if(isLogFitting())
		for(int i = 0; i < (int)y.size(); i++)
			y[i] = log10(y[i]);


	// Initializing weight function: [w(x) = sqrt(x) + 1]
	std::vector<double> weights (ndata, 0.0);
	for(int i = 0; i < ndata; i++)
		weights[i] = (sqrt(fabs(y[i])) + 1.0);

	// Initializing Levenberg-Marquardt fitter
	Model *model = new FunctionModel(BackgroundIntensity, 0, 3);
	ModelFitter *fitter = CreateFitter(model, x, y, thisVectorHasOnlyOnesInIt, signaly, weights, a, ia, &a_min, &a_max, paramErrors, modelErrors, funcs);

	// No mutables
	if(fitter->GetError()) {
		delete fitter;
		delete model;

		return GenerateBackground(inbgx, my, p);
	}

	// Initializing visual objects
	std::vector<double> intermY (ndata, 0.0); // Intermediate model for realtime graph plotting

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
			for(int r = 0; r < (int)bgx.size(); r++) {
				double rr;
				rr = BackgroundIntensity(bgx[r], a, ma, funcs) + signaly[r];

				intermY[r] = rr;
			}

			// Modifying the generated graph
			GraphModify(bgx, intermY);
		}

		// Progress Report
		if(ProgressReport)
			ProgressReport(int(double(i) / double(GetFitIterations()) * 100.0));
	}


	//////////////////////////////////////////////////////////////////////////
	// Finalization

	fitter->calcErrors();
	for(int bbq = 0; bbq < ia.size(); bbq++) {
		if(ia[bbq] == 0)
			paramErrors.insert(paramErrors.begin() + bbq, -1.0);
	}
	if(!pStop || (pStop && !*pStop))
		success = !fitter->GetError();

	delete fitter;
	delete model;

	// Clearing the stop signal so it won't interrupt us while we generate the final model
	ClearSignal();

	// Saving back the parameters to the peakStruct
	for(int i = 0; i < funcs; i++) {
		p->base[i]    = a[i + funcs];
		p->decay[i]   = a[i + funcs + funcs];
		p->center[i]  = a[i + funcs + funcs + funcs];
	}

	// After fitting the peaks, generate the final graph to display
	GenerateBackground(inbgx, my, p);

	return success;
}

