#include <cmath>
#include <limits>

#include "Model.h"
#include "models.h"
#include "fittingfactory.h"
#include "structurefitting.h"
#include <Eigen/QR>
//#include "Eigen/..."
#include "Quadrature.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
#include "mathfuncs.h"

peakF PeakShape;

// Phase globals
MatrixXi _indicesReservoir;
std::vector <double> _peaks;
double _qmax;
PhaseType _phase;
VectorXd _differences;

void SetPeakShape() {
	switch(GetPeakType()) {
        default:
		case SHAPE_GAUSSIAN:
			if(isSigma())
				PeakShape = gaussianSig;
			else
				PeakShape = gaussianFW;
			break;
		case SHAPE_LORENTZIAN:
			PeakShape = lorentzian;
			break;
		case SHAPE_LORENTZIAN_SQUARED:
			PeakShape = lorentzian_squared;
			break;
		case SHAPE_CAILLE:
			//PeakShape = CailleDummy;
			break;
	}
}

std::string debugMatrixPrint(MatrixXd a) {
	std::stringstream s;
	s<< "\t";
	for (int j = 0; j<a.cols(); j++) 
		s<<"["<<j<<"]\t";
	s<<"\n";
	for (int i = 0; i< a.rows(); i++){
		s<<"["<<i<<"]\t";
		for (int j = 0; j<a.cols(); j++) 
			s<<a(i,j)<<"\t";
		s<<"\n";
	}
	return s.str();
}

// Dummy function for Levenberg-Marquardt so that Structure Factor can fit
double StructureFactorIntensity(double q, VectorXd& a, int ma, int nd) {
	double result = 1.0;
	for(int i = 0; i < nd; i++) {
		if( a[i] < 0.0) a[i] = 0.0;
		if( a[nd + i] <= 0.0) {
			a[nd + i] = 0.0;
			continue;
		}
		
		if( a[2 * nd + i] < 0.0) a[2 * nd + i] = 0.0;
		double dfjkgh;

		if(GetPeakType() == SHAPE_CAILLE)
			dfjkgh = CailleDummy(a[1], a[2], a[0],  a[3], q, a[4]);
		else
			dfjkgh = PeakShape(a[nd + i], a[(2 * nd) + i], a[i], 0.0, q);
		result += dfjkgh;
	}

	return result;
}
// Dummy function for Levenberg-Marquardt so that Phases can fit
//double PhasesIntensity(double q, VectorXd& a, int ma, int nd) {
//	return Phases (a[0], a[1], a[3], a[2], a[4], a[5], q, a[6]);
//}


EXPORTED bool GenerateStructureFactor(const std::vector<double> x, std::vector<double>& y, peakStruct *p) {
	return GenerateStructureFactorU(x, y, std::vector<double>(), p, NULL, NULL, NULL);
}

EXPORTED bool GenerateStructureFactorU(const std::vector<double> x, std::vector<double>& y, const std::vector<double>& bgy, 
									   peakStruct *p, plotFunc GraphModify, int *pStop, progressFunc ProgressReport) {
	// If there is no Y, we generate FF = 1
	if(y.size() == 0 || y.size() != x.size())
		y.resize(x.size(), 1.0);
	
	// No peaks
	if(p->amp.size() == 0) {
		y.clear();
		y.resize(x.size(), 1.0);
		return true;
	}

	std::vector<double> sf (y.size(), 1.0), genX, intermY;

	SetPeakShape();

	// TODO!
	//PreCalculateModel();
	bPrecalculate = true;

	for(int i = 0; i < (int)y.size(); i++) {
		if(i == (int)y.size() - 1)
			LastQ();
		for(int j = 0; j < (int)p->amp.size(); j++) {
			if( p->fwhm.at(j).value <= 0.0) {
				p->fwhm.at(j).value = 0.0;
				continue;
			}
			double hgdrb = 0.0;

			if(GetPeakType() == SHAPE_CAILLE) {
				SetX(x);
				hgdrb = CailleDummy(p->fwhm.at(j).value, p->center.at(j).value, p->amp.at(j).value, p->cailleSigma.at(j).value, x.at(i), p->cailleNDiffused.at(j).value);
			} else
				hgdrb = PeakShape(p->fwhm.at(j).value, p->center.at(j).value, p->amp.at(j).value, 0.0, x.at(i));
			sf.at(i) += hgdrb;
		}
		
		if(pStop) {
			if(*pStop)
				break;
	
			if(GraphModify) {
				genX.push_back(x[i]);
				intermY.push_back((y[i] * sf[i]) + bgy[i]);
				// Modifying the generated graph
				GraphModify(genX, intermY);
			}
	
			if(ProgressReport)
				ProgressReport(int(double(i) / double(x.size()) * 100.0));
		}
		y.at(i) = sf.at(i);

		//if((i == (int)y.size() - 1) && (GetPeakType() == SHAPE_CAILLE)) {
		//	for(int gepr = 0; gepr < (int)y.size(); gepr++)
		//		y[gepr] -= p->amp.at(0).value * p->cailleNDiffused.at(0).value;
		//}
		//
	}
	return true;
}

EXPORTED bool FitStructureFactor(const std::vector<double> sfx, const std::vector<double> sfy, std::vector<double>& my, 
								 const std::vector<double>& bgy, const std::vector<bool>& mask, peakStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors) {
									 return FitStructureFactorU(sfx, sfy, my, bgy, mask, p, paramErrors, modelErrors, NULL, NULL, NULL);
}


EXPORTED bool FitStructureFactorU(const std::vector<double> insfx,  const std::vector<double> insfy, 
								  std::vector<double>& my, const std::vector<double>& inbgy, const std::vector<bool>& mask, peakStruct *p,
								  std::vector<double>& paramErrors, std::vector<double>& modelErrors,
							 	 plotFunc GraphModify, int *pStop, progressFunc ProgressReport) {
	bool success = true;
	int peaks = p->amp.size();
	// No peaks
	if(peaks == 0)
		return true;
	
	std::vector<double> sfx = insfx, sfy = insfy, bgy = inbgy;
	// If there is no Y, we generate FF = 1
	if(my.size() == 0)
		my.resize(sfx.size(), 1.0);

	// Check to see if there are masked elements and crop them from relevant vectors
	if(mask.size() == insfx.size()) {
		for(int k = mask.size() - 1; k >= 0; k--) {
			if(mask.at(k)) {
				bgy.erase(bgy.begin() + k);
				sfx.erase(sfx.begin() + k);
				sfy.erase(sfy.begin() + k);
				my.erase(my.begin() + k);
			}	//if
		}	//for
	}	//if

	//////////////////////////////////////////////////////////////////////////
	// Initialization
	
	// Initializing additional GUI parameters
	SetSignal(pStop);

	int caille = p->cailleSigma.size();
	int ma = peaks * (3 + caille * 2), ndata;
	VectorXd a  = VectorXd::Zero(ma); // Parameter vector
	VectorXi ia = VectorXi::Zero(ma); // Mutability vector
	cons a_min(ma); // Fit range/constraint vector
	cons a_max(ma); // Fit range/constraint vector

	for(int i = 0; i < peaks; i++) {
		a[i] = p->amp[i].value;
		a[i + peaks] = p->fwhm[i].value;
		a[i + peaks + peaks] = p->center[i].value;
		
		a_min.num[i] = p->amp[i].min;
		a_min.num[i + peaks] = p->fwhm[i].min;
		a_min.num[i + peaks + peaks] = p->center[i].min;

		a_max.num[i] = p->amp[i].max;
		a_max.num[i + peaks] = p->fwhm[i].max;
		a_max.num[i + peaks + peaks] = p->center[i].max;

		ia[i] = p->amp[i].mut == 'Y';
		ia[i + peaks] = p->fwhm[i].mut == 'Y';
		ia[i + peaks + peaks] = p->center[i].mut == 'Y';
		
		if(caille == 1) {
			a[i + 3]			= p->cailleSigma[i].value;
			a_min.num[i + 3]	= p->cailleSigma[i].min;
			a_max.num[i + 3]	= p->cailleSigma[i].max;
			ia[i + 3]			= p->cailleSigma[i].mut == 'Y';

			a[i + 4]			= p->cailleNDiffused[i].value;
			a_min.num[i + 4]	= p->cailleNDiffused[i].min;
			a_max.num[i + 4]	= p->cailleNDiffused[i].max;
			ia[i + 4]			= p->cailleNDiffused[i].mut == 'Y';


		}
	}

	// Initializing graph vectors
	std::vector<double> x = sfx, y = sfy;
	ndata = x.size();

	if(isLogFitting())
		for(int i = 0; i < (int)y.size(); i++) 
			y[i] = log10(y[i]);


	// Initializing weight function: [w(x) = sqrt(x) + 1]
	std::vector<double> weights (ndata, 0.0);
	for(int i = 0; i < ndata; i++)
		weights[i] = (sqrt(fabs(y[i])) + 1.0);


	// Initializing Levenberg-Marquardt fitter
	Model *model = new FunctionModel(StructureFactorIntensity, 0, 3);
	ModelFitter *fitter = CreateFitter(model, x, y, my, bgy, weights, a, ia, &a_min, &a_max, paramErrors, modelErrors, peaks);

	// No mutables
	if(fitter->GetError()) {
		delete fitter;

		return GenerateStructureFactor(insfx, my, p);
	}

	// Initializing visual objects
	std::vector<double> intermY (ndata, 0.0); // Intermediate model for realtime graph plotting

	//////////////////////////////////////////////////////////////////////////
	// Fitting

	// Setting the peak shape function (gaussian, lorentzian, etc.) and the function to fit
	SetPeakShape();

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
			for(int r = 0; r < (int)sfx.size(); r++) {
				double rr;
				rr = StructureFactorIntensity(sfx[r], a, ma, peaks) * my[r] + bgy[r];
				intermY[r] = rr;
			}

			// Modifying the generated graph
			GraphModify(sfx, intermY);
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
	for(int i = 0; i < peaks; i++) {
		p->amp[i].value    = a[i];
		p->fwhm[i].value   = a[i + peaks];
		p->center[i].value = a[i + peaks + peaks];
	}

	if(pStop && *pStop == 1)	// So that the generate for Caille will work
		*pStop = 0;

	// After fitting the peaks, generate the final graph to display
	GenerateStructureFactor(insfx, my, p);

	return success;
}






//EXPORTED bool GeneratePhases(const std::vector<double> x, std::vector<double>& y, phaseStruct *p) {
//	return GeneratePhasesU(x, y, std::vector<double>(), p, NULL, NULL, NULL);
//}
//
//EXPORTED bool GeneratePhasesU(const std::vector<double> x, std::vector<double>& y, const std::vector<double>& bgy, phaseStruct *p,
//								      plotFunc GraphModify, int *pStop, progressFunc ProgressReport) {
//	// If there is no Y, we generate FF = 1
//	if(y.size() == 0)
//		y.resize(x.size(), 1.0);
//	
//		std::vector<double> sf (y.size(), 1.0), genX, intermY;
//
//	for(int i = 0; i < (int)y.size(); i++) {
//		sf.at(i) += Phases(p->a,p->b,p->c,p->gamma,p->alpha,p->beta,x.at(i), p->um);
//		
//		if(pStop) {
//			if(*pStop)
//				break;
//	
//			if(GraphModify) {
//				genX.push_back(x[i]);
//				intermY.push_back(y[i] * sf[i] + bgy[i]);
//				// Modifying the generated graph
//				GraphModify(genX, intermY);
//			}
//	
//			if(ProgressReport)
//				ProgressReport(int(double(i) / double(x.size()) * 100.0));
//		}
//		y.at(i) = sf.at(i);
//	}
//
//	return true;
//}
// Creating matrix of indices
MatrixXi GenerateIndicesMatrix(int dim, int length) {
	MatrixXi result = MatrixXi::Zero(length, dim);
	


	int help = 0;
	int help1 = 0;
	int base = 2;
	for(int i = 0; i < result.rows(); i++) {
		help1++;
		help = help1;
		for (int j = dim - 1; j >= 0 ; j--) {
			result(i, j) = (help % base);
			help /= base;
		}
		for (int m=0; m < i; m++ ) {
			if (result.row(m) == result.row(i)) {
				i--;
				break;
			}
		}

		bool equals = (result(i, 0) ==  base - 1);
		for (int j = dim - 1; j > 0 ; j--) 
				equals &= ((result(i, j) == result(i, j-1)) &&
						  (result(i, j-1) ==  base - 1));
		if(equals) {
			base++;
			help = 0;
			help1 = 0;
		}
				
	}

	//MessageBoxA(NULL, debugMatrixPrint(result.cast<double>()).c_str(), "indices", NULL);

	return result;
}

//documentation :: 
// q is the peak index we are dealing with
// &a are the params, ma is the phase and nd the dimention
void BuildAMatrixForPhases(VectorXd  &a, MatrixXd &g, PhaseType phase) {
	double sqroot;

	switch (phase) {
		case PHASE_RHOMBO_3D:
			a[1] = a[3] = a[0];
			a[4] = a[5] = a[2];
			
			sqroot = sqrt( sin(a[4])*sin(a[4]) - 
				( cos(a[5]) - cos(a[4]) * cos (a[2])) / sin(a[2]) 
			  * ( cos(a[5]) - cos(a[4]) * cos (a[2]) )/ sin(a[2]) );

			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(0,2) =   2.0 * PI / (a[0] * sin (a[2]) * sqroot ) *   
				( (cos(a[5]) - cos(a[4]) * cos (a[2]) )/ tan (a[2]) - cos(a[4]) * sin (a[2]) )  ;
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]) );
			g(1,2) = - 2.0 * PI / (a[1] * sin (a[2]) * sin (a[2]) * sqroot ) * (cos(a[5]) - cos(a[4]) *cos (a[2]));
			g(2,0) =   0;
			g(2,1) =   0;
			g(2,2) =   2.0 * PI / (a[3] * sqroot );
			break;
		
		
		case PHASE_HEXA_3D:
			a[1] = a[0];
			a[4] = a[5] = PI / 2.0;
			a[2] = 2.0 * PI / 3.0;
		
			sqroot = sqrt( sin(a[4])*sin(a[4]) - 
				( cos(a[5]) - cos(a[4]) * cos (a[2])) / sin(a[2]) 
			  * ( cos(a[5]) - cos(a[4]) * cos (a[2]) )/ sin(a[2]) );

			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(0,2) =   2.0 * PI / (a[0] * sin (a[2]) * sqroot ) *   
				( (cos(a[5]) - cos(a[4]) * cos (a[2]) )/ tan (a[2]) - cos(a[4]) * sin (a[2]) )  ;
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]) );
			g(1,2) = - 2.0 * PI / (a[1] * sin (a[2]) * sin (a[2]) * sqroot ) * (cos(a[5]) - cos(a[4]) *cos (a[2]));
			g(2,0) =   0;
			g(2,1) =   0;
			g(2,2) =   2.0 * PI / (a[3] * sqroot );
			break;

		
		
		case PHASE_CUBIC_3D:
			a[3] = a[1] = a[0];
			a[4] = a[5] = a[2] = PI / 2.0;
		case PHASE_TETRA_3D:
			a[4] = a[5] = a[2] = PI / 2.0;
			a[1] = a[0];
		case PHASE_ORTHO_3D:
			a[4] = a[5] = a[2] = PI / 2.0;
		case PHASE_MONOC_3D:
			a[2] = a[5] = PI / 2.0;
		case PHASE_3D:
		case PHASE_NONE:
		default:	
			sqroot = sqrt( sin(a[4])*sin(a[4]) - 
				( cos(a[5]) - cos(a[4]) * cos (a[2])) / sin(a[2]) 
			  * ( cos(a[5]) - cos(a[4]) * cos (a[2]) )/ sin(a[2]) );

			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(0,2) =   2.0 * PI / (a[0] * sin (a[2]) * sqroot ) *   
				( (cos(a[5]) - cos(a[4]) * cos (a[2]) )/ tan (a[2]) - cos(a[4]) * sin (a[2]) )  ;
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]) );
			g(1,2) = - 2.0 * PI / (a[1] * sin (a[2]) * sin (a[2]) * sqroot ) * (cos(a[5]) - cos(a[4]) *cos (a[2]));
			g(2,0) =   0;
			g(2,1) =   0;
			g(2,2) =   2.0 * PI / (a[3] * sqroot );
			break;
		case PHASE_LAMELLAR_1D:
			g(0,0) =   2.0 * PI / (a[0]);
			break;
		case PHASE_HEXAGONAL_2D:
			a[1] = a[0];
			a[2] = PI / 1.5;			
		case PHASE_2D:
			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]));
			break; 
		case PHASE_SQUARE_2D:
			a[1] = a[0];
		case PHASE_RECTANGULAR_2D:
			a[2] = PI / 2.0;
			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]));
			break; 
		case PHASE_CENTERED_RECTANGULAR_2D:
			a[2] = PI / 4.0;			
			g(0,0) =   2.0 * PI / (a[0]);
			g(0,1) = - 2.0 * PI / (a[0] * tan (a[2]));
			g(1,0) =   0;
			g(1,1) =   2.0 * PI / (a[1] * sin (a[2]));
			break; 
	}

	//MessageBoxA(NULL, debugMatrixPrint(g).c_str(), "G", NULL);
}

int phaseDimensions(PhaseType phase) {
	switch (phase) {
		case PHASE_LAMELLAR_1D:
			return 1;
		case PHASE_2D:
		case PHASE_HEXAGONAL_2D:
		case PHASE_CENTERED_RECTANGULAR_2D:
		case PHASE_RECTANGULAR_2D:
		case PHASE_SQUARE_2D:
			return 2;
	
		default:
		case PHASE_NONE:
		case PHASE_3D:
		case PHASE_RHOMBO_3D:
		case PHASE_HEXA_3D:
		case PHASE_MONOC_3D:
		case PHASE_ORTHO_3D:
		case PHASE_TETRA_3D:
		case PHASE_CUBIC_3D:
			return 3;
	}
}

EXPORTED std::vector <double> GenPhases (PhaseType phase , phaseStruct *p,
										 std::vector<std::string> &locs) {
	// Initialization
	int ma,dim;
	// Initializing vectors
	VectorXd a;  // Parameter vector
	
	VectorXd params; 
	VectorXi ia; // Mutability vector
	cons a_min(7); // Fit range/constraint vector
	cons a_max(7); // Fit range/constraint vector
	
	// a paramter to pass to ma 
	// of the dummy function that should be like a numerator for the different phases
	
	ma =7;
	a  = VectorXd::Zero(ma);  
	ia = VectorXi::Zero(ma);
	a[0] = p->a;
	a[1] = p->b;
	a[2] = p->gamma;
	a[3] = p->c;
	a[4] = p->alpha;
	a[5] = p->beta;
	//a[6] = p->um;

	ia[0] = p->aM == 'Y';
	ia[1] = p->bM == 'Y';
	ia[2] = p->gammaM == 'Y';
	ia[3] = p->cM == 'Y';
	ia[4] = p->alphaM == 'Y';
	ia[5] = p->betaM == 'Y';
	ia[6] = p->umM == 'Y';

	a_min.num[0] = p->amin;
	a_min.num[1] = p->bmin;
	a_min.num[2] = p->gammamin;
	a_min.num[3] = p->cmin;
	a_min.num[4] = p->alphamin;
	a_min.num[5] = p->betamin;
	a_min.num[6] = p->ummin;

	a_max.num[0] = p->amax;
	a_max.num[1] = p->bmax;
	a_max.num[2] = p->gammamax;
	a_max.num[3] = p->cmax;
	a_max.num[4] = p->alphamax;
	a_max.num[5] = p->betamax;
	a_max.num[6] = p->ummax;

	dim = phaseDimensions(phase);

	params = VectorXd::Zero(ma);

	MatrixXd g = MatrixXd::Zero(dim,dim);
	BuildAMatrixForPhases(a,g, phase);
	
	MatrixXi ind = GenerateIndicesMatrix(dim, dim * dim * 100);

	MatrixXd G = ind.cast<double>()*g;
	VectorXd G_norm = VectorXd::Zero(G.rows());
	for (int i = 0; i<G.rows(); i++) 
		G_norm[i] = G.row(i).norm();
		
	//Sort the rows of G according to the norm
	for (int a = 0; a < G.rows(); a++) {
		for (int b = a + 1; b < G.rows(); b++) {
			if(G_norm[a] > G_norm[b]) {
				double c = G_norm[a];
				G_norm[a] = G_norm[b];
				G_norm[b] = c;
				G.row(a).swap(G.row(b));
				ind.row(a).swap(ind.row(b));
			}
		}
		std::stringstream s;
		
		s << "(" <<ind(a,0);
		for (int d = 1; d < dim; d++) 
			s << ","<< ind(a,d);
		s << ")";
		locs.push_back(s.str());
	}

	std::vector <double> G_N;	
	uniqueIndices(G_norm, G_N, locs);
	
	return G_N;
	
}


void uniqueIndices(const VectorXd& G_norm, std::vector<double>& result, std::vector<std::string> &locs) {
	result.clear();

	int s = 1;
	for (int i = 0; i<G_norm.size(); i++)
		result.push_back(G_norm[i]);
	
	while (s < (int)result.size()) 	{
		if ( fabs(result[s-1] - result[s]) <= 1e-7) {		
			result.erase(result.begin() + s);
			locs[s - 1].append(" , " + locs[s]);
			locs.erase(locs.begin() + s);
			continue;
			
		}
		s++;
	}

}
	

double PhaseDummy(double q, VectorXd &a, int ma, int nd) {
	MatrixXd g = MatrixXd::Zero(nd, nd);
	BuildAMatrixForPhases(a, g, _phase);
	_differences = CalculatePhaseDifference(_peaks, g, nd);
	
	return _differences[(int)q];
}



EXPORTED bool FitPhases(PhaseType phase, std::vector<double>& peaks, phaseStruct *p,
						std::vector<double>& paramErrors, std::vector<std::string> &locs) {
	return FitPhasesU(phase, peaks, p, paramErrors, locs, NULL, NULL);
}


EXPORTED bool FitPhasesU (PhaseType phase, std::vector<double>& peaks, phaseStruct *p, 
						  std::vector<double>& paramErrors, std::vector<std::string> &locs,
						  int *pStop, progressFunc ProgressReport) {
	
	std::vector<double> modelErrors;
	
	//////////////////////////////////////////////////////////////////////////
	// Initialization
	bool success = true;
	int ma=7,ndata,dim;
	// Initializing vectors
	VectorXd a;  // Parameter vector 
	VectorXi ia; // Mutability vector
	cons a_min(ma); // Fit range/constraint vector
	cons a_max(ma); // Fit range/constraint vector
	
	
	std::vector<std::string> genLoc;
	
	a  = VectorXd::Zero(ma);  
	ia = VectorXi::Zero(ma);
	a[0] = p->a;
	a[1] = p->b;
	a[2] = p->gamma;
	a[3] = p->c;
	a[4] = p->alpha;
	a[5] = p->beta;
	//a[6] = p->um;

	ia[0] = p->aM == 'Y';
	ia[1] = p->bM == 'Y';
	ia[2] = p->gammaM == 'Y';
	ia[3] = p->cM == 'Y';
	ia[4] = p->alphaM == 'Y';
	ia[5] = p->betaM == 'Y';
	ia[6] = p->umM == 'Y';

	a_min.num[0] = p->amin;
	a_min.num[1] = p->bmin;
	a_min.num[2] = p->gammamin;
	a_min.num[3] = p->cmin;
	a_min.num[4] = p->alphamin;
	a_min.num[5] = p->betamin;
	a_min.num[6] = p->ummin;

	a_max.num[0] = p->amax;
	a_max.num[1] = p->bmax;
	a_max.num[2] = p->gammamax;
	a_max.num[3] = p->cmax;
	a_max.num[4] = p->alphamax;
	a_max.num[5] = p->betamax;
	a_max.num[6] = p->ummax;

	dim = phaseDimensions(phase);

	MatrixXd g = MatrixXd::Zero(dim, dim);

	// Initializing additional GUI parameters
	SetSignal(pStop);

	// Initializing graph vectors
	std::vector<double> x, y;
	for (int i = 0; i < (int)peaks.size(); i++) {
		x.push_back(double(i));
		y.push_back(0.0);
	}
	
	ndata = x.size();
	if(ndata < 1) {
		success = false;
		return false;
	}

	// Initializing weight function: [w(x) = sqrt(x) + 1]
	std::vector<double> weights (ndata, 0.0);
	for(int i = 0; i < ndata; i++)
		weights[i] = (sqrt(y[i]) + 1.0);
	std::vector <double> bgy (ndata,0.0), my (ndata, 1.0);

	// Setting the global parameters
	_phase = phase;
	_qmax = p->qmax;
	_peaks = peaks;
	_indicesReservoir = GenerateIndicesMatrix(dim, dim * dim * 100);

	// Initializing Levenberg-Marquardt fitter
	Model *model = new FunctionModel(PhaseDummy, 0, 7); // TODO: PhaseModel with phase,qmax and so on
	ModelFitter *fitter = CreateFitter(model, x, y, my, bgy, weights, a, ia, &a_min, &a_max, paramErrors, modelErrors, dim);

	// No mutables
	if(fitter->GetError()) {
		delete fitter;
		delete model;
		return success;
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

	// Saving back the parameters to the phaseStruct
	p->a     = a[0]; 
	p->b     = a[1]; 
	p->gamma = a[2]; 
	p->c     = a[3]; 
	p->alpha = a[4]; 
	p->beta  = a[5]; 
	//p->um    = a[6];

	// Generating the new indices
	GenPhases(phase, p, locs);

	return success;
}


static double _q;
static Vector3d _a1, _a2, _a3;
static int _N0, _N1, _N2;
//double innerPhases(double phiQ, double thetaQ) {
//	double qx = _q * sin(thetaQ) * cos(phiQ),
//		   qy = _q * sin(thetaQ) * sin(phiQ),
//		   qz = _q * cos(thetaQ);
//	std::complex<double> result = 0.0;
//	Vector3d q = Vector3d::Zero();
//	Vector3d n = Vector3d::Zero();
//	const std::complex<double> im(0.0, 1.0);
//
//	q[0] = qx; q[1] = qy; q[2] = qz;
//
//	for (n[0] = 0; n[0] < _N0; n[0]++) {
//		for (n[1] = 0; n[1] < _N1; n[1]++) {
//			for (n[2] = 0; n[2] < _N2; n[2]++){
//				Vector3d Rn = n[0]*_a1+n[1]*_a2+n[2]*_a3;
//				result += exp(im * q.dot(Rn));
//			}
//		}
//	}
//
//
//	return norm(result) * sin(thetaQ);
//}
//
//EXPORTED double Phases (double a, double b, double c, double gamma, double phi, double theta, double q, double um) {
//	/* the vectors two recieve are:
//	a1 = (a,0,0) , a2 = b(cos gamma, sin gamma, 0), a3 = c(sin theta cos phi, sin theta sin phi, cos theta)
//	there can't be a a3 without a2.
//	*/
//	
//	//from spherical coordinates to vectors
//	int dim = 3;
//	Vector3d a1 = Vector3d::Zero() , a2 = Vector3d::Zero() , a3 = Vector3d::Zero() ;
//	a1[0] = a;
//	if (fabs(b) > 1e-7) {
//		a2[0] = cos(gamma);
//		a2[1] = sin(gamma);
//		a2 *= b;
//		
//		if (fabs(c) > 1e-7) {
//			a3[0] = sin(theta) * cos(phi);
//			a3[1] = sin(theta) * sin(phi);
//			a3[2] = cos(theta);
//			a3 *= c;
//		}
//		else {
//			dim = 2;
//			a3[2] = 1; 
//		}
//	}
//	else {
//		dim = 1; 
//		a2[1] = 1;
//		a3[2] = 1;
//	}
//	
//
//
//	//the function recieves the 3 real lattice vectors and q
//	complex <double> im (0.0, 1.0);
//	double amp = 0.0; 
//	//create reciprocal vectors and more.
//	Vector3d g1 = Vector3d::Zero(), g2 = Vector3d::Zero(), g3 = Vector3d::Zero(), Rn = Vector3d::Zero(), Gn = Vector3d::Zero();
//	//create indices vectors 
//	Vector3i n = Vector3i::Zero(), hkl = Vector3i::Zero();
//	//definition of reciprocal vectors
//	double Volume = a1.dot(a2.cross(a3));
//	//int N0 = 100, N1 = 100, N2 = 100; 
//	g1 = 2.0 * PI / Volume * a2.cross(a3); 
//	g2 = 2.0 * PI / Volume * a3.cross(a1); 
//	g3 = 2.0 * PI / Volume * a1.cross(a2); 
//	//limits of sum
//	int N0, N1, N2;
//	if ( dim == 3 ) {
//		N0 = (int)(q/g1.norm()) + 1;
//		N1 = (int)(q/g2.norm()) + 1;
//		N2 = (int)(q/g3.norm()) + 1;
//	}
//	if ( dim == 2 ) {
//		N0 = (int)(q/g1.norm()) + 1;
//		N1 = (int)(q/g2.norm()) + 1;
//		N2 = 1;
//	}
//	if ( dim == 1 ) {
//		N0 = (int)(q/g1.norm()) + 1; 
//		N1 = 1;
//		N2 = 1;
//	}
//
//	dim == 3 ?	(N0 = N1 = N2 = 10) : (dim == 2 ? (N0 = N1 = 10) : N0 = 100);
//	//find h,k,l that with g1, g2, g3 are close to q; 
//	
//	std::vector <Vector3d> Gns;
///*
//	for (hkl[0] = 0; hkl[0]<N0; hkl[0]++) {
//		for (hkl[1] = 0; hkl[1]<N1; hkl[1]++) {
//			for (hkl[2] = 0; hkl[2]<N2; hkl[2]++){
//				Gn = hkl[0] * g1 + hkl[1] * g2 + hkl[2] * g3;
//				if ((Gn.norm() - q)<1e-3 )
//					Gns.push_back(Gn);
//				if (Gn.norm() > q)
//					break;
//				}
//			}
//		}
//		*/
//
//	/* A bit of thinking:
//	After reading Uri's page I have several problems: 
//	a.	It uses a lineshape for Gaussian and Lorentzian but the program uses already another way to find those. 
//		So that part is Irrelevant and we can just use the Centers as abs(Gn)  we could use it to find the hkl indices.
//	b.	I think we could later, if we want,  fit the amplitude found to the debye waller 
//		factor although we have 2 free factor to fit one number. but maybe with several peaks we could try to fit it. 
//	c.	If we would like to fit the following: SF(q)= sum (exp( i q dot Rn) = sum( exp (i q Rn cos alpha) ) and then we theoretically 
//		could integrate for each q over all the angles alpha and come up with a result but it would probably be very very slow.
//		but  we could maybe skip that if we say that in each exp in the sum we just multiply a vector with an entire shpere and so the result will always be: 
//		integral (exp(i q Rn cos alpha) sin alpha d alpha d phi) = 2 pi integral( exp (i q Rn x) d (-x) ) =  2 pi / (i q Rn)[ exp(i q Rn) - exp(-i q Rn) ] = 
//		= 4 pi sinc (q Rn) -> so if it's correct we just need to change the sum over sincs. 
//
//
//		the code of the following loop would change and be. 
//	 something like 
//	 for 
//	 for
//	 for
//	 Rn definition
//	 amp += sinc(q * Rn) * (4 pi ?)
//		
//
//	*/
//
//
//	// calculate amplitude at point q
//	//old version
//	/*
//	for (int count = 0; count < (int)Gns.size(); count++) {
//		for (n[0] = 0; n[0]<N0; n[0]++) {
//			for (n[1] = 0; n[1]<N1; n[1]++) {
//				for (n[2] = 0; n[2]<N2; n[2]++){
//					Rn = n[0]*a1+n[1]*a2+n[2]*a3;
//					amp += exp(im*Gns[count].dot(Rn)); 
//				}
//			}
//		}
//	}
//	*/
//	
//	// Old version no. 2
//	/*
//	for (n[0] = 0; n[0]<N0; n[0]++) {
//		for (n[1] = 0; n[1]<N1; n[1]++) {
//			for (n[2] = 0; n[2]<N2; n[2]++){
//				Rn = n[0]*a1+n[1]*a2+n[2]*a3;
//				amp +=( (Rn.norm() == 0) ?  1.0 : (sin(q * Rn.norm()) / (q * Rn.norm()))); 
//			}
//		}
//	}
//
//
//	amp *= 4 * PI;
//
//	//returns I
//	return amp * amp;*/
///*
//	_q = q;
//	_a1 = a1;
//	_a2 = a2;
//	_a3 = a3;
//	_N0 = N0;
//	_N1 = N1;
//	_N2 = N2;
//
//	return Quadrature(innerPhases, defaultQuadRes, 0.0 + EPS, 2.0 * PI + EPS, 0.0 + EPS, 
//		              PI + EPS);
//*/
//	std::complex<double> resu(0.0,0.0);
//	const std::complex<double> ima(0.0, 1.0);
//	if(dim == 1) {
//		for (int co = 0; co < N0; co++) 
//			resu += exp(ima* q * (double)co * a);
//	}
//	
//	return (resu.real() * resu.real() + resu.imag() * resu.imag())*exp(-q * q * um);
//
//}

std::pair<double, double> linearfit(std::vector<double> indices,std::vector<double> peaks) {
	
	double slope=0,wssr=0,sumy=0,sumx=0,sumxx=0,sumxy=0;
	for (int i = 0; i < (int)peaks.size(); i++) {
		sumx+=indices[i];
		sumxx+=indices[i]*indices[i];
		sumy+=peaks[i];
	}
		//because I want to force intercept to 0:
	sumxy  = sumy * sumxx / sumx;	
	slope  = sumy * sumx - peaks.size() * sumxy;
	slope /= sumx * sumx - peaks.size() * sumxx;
	//calculating error of line
	for(int i = 0; i < (int)peaks.size();i++)
		wssr += fabs(slope * indices[i] - peaks[i]);
	wssr /= peaks.size();	
	
	return std::pair<double, double> (slope, wssr);
}
double linearwssr(std::vector<double> indices,std::vector<double> peaks) {
	double slope = 1.0;
	double wssr = 0.0;
	//calculating error of line
		for(int i = 0; i < (int)peaks.size();i++)
			wssr += fabs(slope * indices[i] - peaks[i]);
		wssr /= peaks.size();	
		return wssr;

}



VectorXd CalculatePhaseDifference(std::vector <double> peakCenters, MatrixXd& g,
								  int dim) {
	//std::vector <std::string> indices_loc;
	
	if(dim > 3 || dim < 1) return VectorXd::Zero(peakCenters.size()); 
	//going to Eigen
	VectorXd peaks = VectorXd::Zero(peakCenters.size());
	for (int i = 0; i < (int)peakCenters.size(); i++) 
		peaks[i] = peakCenters[i];
	
	//creating indices matrix
	//MatrixXi indices = MatrixXi::Zero(peaks.size(),dim), finalind = MatrixXi::Zero(peaks.size(),dim) ;
	//creating indices not depending on dim
	//MatrixXi _indicesReservoir = GenerateIndicesMatrix(dim, 500);
		
	////defining the reciprocal base vectors
	//MatrixXd g = MatrixXd::Zero(dim,dim);
	//{
	//	int k = 0;
	//	for (int i = 0; i<dim; i++) {
	//		for (int j =0; j<dim; j++) {
	//			if (j < i) continue;
	//			g(i,j) = params[k];
	//			k++;
	//		}
	//	}
	//}

	//defining G;
	MatrixXd G = _indicesReservoir.cast<double>()*g;
	VectorXd G_norm = VectorXd::Zero(G.rows());

	for (int i = 0; i<G.rows(); i++)
		G_norm[i] = G.row(i).norm();


	//MessageBoxA(NULL,debugMatrixPrint(G_norm).c_str(),"G_NORM",NULL);
	//MessageBoxA(NULL,debugMatrixPrint(_indicesReservoir.cast<double>()).c_str(),"indices_loc_AFTER",NULL);

		
	//Sort the rows of G according to the norm
	for (int a = 0; a < G.rows(); a++) {
		for (int b = a + 1; b < G.rows(); b++) {
			if(G_norm[a] > G_norm[b]) {
				//MessageBoxA(NULL,debugMatrixPrint(G_norm).c_str(),"G_NORM_before exchange",NULL);
				//MessageBoxA(NULL,debugMatrixPrint(_indicesReservoir.cast<double>()).c_str(),"indices_loc_before exchange",NULL);
				double c = G_norm[a];
				G_norm[a] = G_norm[b];
				G_norm[b] = c;
				G.row(a).swap(G.row(b));
				//MessageBoxA(NULL,debugMatrixPrint(G_norm).c_str(),"G_NORM_before exchange",NULL);
				///ind.row(a).swap(ind.row(b));
				//MessageBoxA(NULL,debugMatrixPrint(_indicesReservoir.cast<double>()).c_str(),"indices_loc_AFTER exchange",NULL);
			}
		}
	}
	
	//MessageBoxA(NULL,debugMatrixPrint(G_norm).c_str(),"G_NORM after sorting",NULL);
	//MessageBoxA(NULL,debugMatrixPrint(_indicesReservoir.cast<double>()).c_str(),"indices after sorting",NULL);
	std::vector<double> G_N;
	for(int i = 0; i < G_norm.size(); i++)
		G_N.push_back(G_norm[i]);

	int s = 1;
	while (s < (int)G_N.size()) 	{
		if ( fabs(G_N[s-1] - G_N[s]) <= 1e-7) {		
			G_N.erase(G_N.begin() + s);
			continue;		
		}
		s++;
	}


	//	
	//	// Omitting norms that are either larger than qmax or equal to other norms
	//	// by setting the norms to infinity and rerunning the sorting (which will
	//	// effectively send them to the end)
	//	if(G_norm[a] > qmax) {
	//		G_norm[a--] = inf;
	//		indices_loc.pop_back();
	//		if (a < 1) {
	//			return -1.0;
	//		}
	//	}
	//	if(a > 0 && fabs(G_norm[a-1] - G_norm[a]) <= 1e-6) {
	//		G_norm[a--] = inf;	
	//		indices_loc[indices_loc.size() - 2].append(" , " + indices_loc[indices_loc.size() - 1]);
	//		indices_loc.pop_back();
	//	}
	//}

	//MessageBoxA(NULL,debugMatrixPrint(G_norm).c_str(),"G_NORM_AFTER",NULL);
	//MessageBoxA(NULL,debugMatrixPrint(_indicesReservoir.cast<double>()).c_str(),"indices_loc_AFTER",NULL);

	// POSTCONDITION: We have sorted indices and norms in the vectors and norms aren't 
	//                larger than qmax. indices_loc's size is the used height
	// Fitting
	// Calculating differences

	VectorXd differences = VectorXd::Zero(peaks.size());
	
	//std::string aa = debugMatrixPrint(G_norm);
	//std::string b = debugMatrixPrint(peaks);

	//MessageBoxA(NULL, aa.c_str(), "G_Norm", NULL);
	//MessageBoxA(NULL, b.c_str(), "peaks", NULL);

	// PRECONDITION: Both peaks and indices_loc are sorted
	int j = 0;
	for(int i = 0; i < peaks.size(); i++) {
		for(; j < (int)G_N.size(); j++)
			if(G_N[j] > peaks[i])
				break;

		// Exit condition
		if(j >= (int)G_N.size()) {
			differences[i] = fabs(G_N.back() - peaks[i]);
			///indices_finalloc.push_back(indices_loc.back());
			break;
		}

		// The closest Gnorm to the peak is either the one before it or after it
		if(j<1 || fabs(G_N[j] - peaks[i]) < fabs(G_N[j - 1] - peaks[i])) {
			differences[i] = fabs(G_N[j] - peaks[i]);
			///indices_finalloc.push_back(indices_loc[j]);
			j += 2;
		} else {
			differences[i] = fabs(G_N[j - 1] - peaks[i]);
			///indices_finalloc.push_back(indices_loc[j - 1]);
			j++;
		}
		
	}
//	MessageBoxA(NULL,debugMatrixPrint(differences).c_str(),"difference",NULL);
	
	return differences;
}

//EXPORTED double GenerateNextPhasePeak(const std::vector<double> peakPositions, const phaseStruct *p) {
//	// MatrixXi indices = GenerateIndicesMatrix(dim, 50);
//
//	return 0.1337;
//}


//std::vector <double> GeneratePhases1D (int peakamount, double slope) {
//	std::vector <double> peaks (peakamount,0.0) ;
//	
//	for( int i=0; i < (int)peaks.size(); i++) 
//		peaks[i] = slope*(i+1);
//	return peaks;
//}
//
//std::vector<double> GeneratePhases2D (int peakamount, double a,double b, double gamma) {
//	if ((b==0)||(a==0)||(gamma==0)) return GeneratePhases1D(peakamount, (b==0) ? a : b);
//
//	std::vector <double> peaks (peakamount,0.0) ;
//	
//	for (int i = 0; i < (int)peaks.size(); i++) {
//            for (int j = 0; j < (int)peaks.size(); j++) 
//			// (2pi/sin(gamma)^2) * ((i/a)^2 + (j/b)^2))
//			peaks[i+j]+=sqrt((2*PI/sin(gamma)) * (2*PI/sin(gamma)) *((i/a)*(i/a)+(j/b)*(j/b)));
//	}
//	return peaks;
//}


EXPORTED void FitPhases1D (std::vector <double> peaks,
                           std::vector<std::vector<std::vector<int> > > &indices_loc, double &wssr,
                           double &slope) {
	std::vector <double> indices(peaks.size(), 0.0), ind(peaks.size(), 0.0);
	std::pair<double, double> res;
	 
	double oldwssr;
	int j = 0;
	wssr = 100000.0;
	//to decide which index belongs to the first peak.
	
	oldwssr = wssr;
	//to see if index should jump.
	for (j = 0; j < (int)peaks.size() * 4; j++) {
            for (int k = j; k < (int)peaks.size() + j; k++) {
                for (int l = j; l < (int)peaks.size() + j; l++) {
                    for (int i = 0; i < (int)indices.size(); i++) indices.at(i) = 0.0;
				int counter = j;
				for (int i = 0; i < (int)peaks.size(); i++, counter++) {
					//skipping up to 2 peaks in order to find the best peak's series. 
					if (i + j == k + 1 || i + j == l + 1) counter++;
					indices[i] = double(counter + 1);
				}
			
				//we'll fit to a line with indices as x, and centres as y
				//we use the linear least squares	
				//to understand linearfit work go to:
				//http://en.wikipedia.org/wiki/Linear_least_squares subchapter: Computation 
				res = linearfit(indices, peaks);
				if (wssr > res.second) {
					wssr = res.second;
					slope = res.first;
					ind = indices;
				}
			}
		}
	} 
	
	for( int co = 0; co < (int)ind.size(); co++ ) {
		std::vector <int> s(3,0);
		s[0] = (int)ind[co];
		std::vector <std::vector <int> > a;
		a.push_back(s);
		indices_loc.push_back(a);
	}
		
}


//EXPORTED void FitPhaseIndices2D (std::vector <double> peaks ,double a, double b, double gamma, std::vector <std::vector <std::vector <int> > > &indices_loc, double &wssr,double &slope) {
//								 
//	/*
//	build index set according to a, b ,gamma
//		create vector of indices according to the fomula: 2pi/sin(gamma)* sqrt( (h/a)^2 + (k/b)^2 ) h, k are ints.	
//		(the vector at it's end must be 1D vector sorted by their size).
//	index peaks accordingly
//		almost the same as 1d.
//	
//	*/
//	std::vector <double> indices(peaks.size(), 0.0), ind(peaks.size(), 0.0);
//	std::pair<double, double> res;
//	double oldwssr;
//	std::vector < std::vector <double> > Ghk (5.0 * peaks.size() , std::vector <double> (5.0 * peaks.size()) ) ;
//	std::vector < double > fabsGhk (Ghk.size() * Ghk.size() ); 
//	std::vector <int>  peaks_at_index(fabsGhk.size(),1); // number of peaks at loc. 
//	std::vector <int> indices_num(peaks.size(), 0), ind_num(peaks.size(), 0);
//	wssr = 100000.0;
//	//defined bragg peaks locations acording to two indices
//	for (int h=0; h < (int)Ghk.size(); h++) {
//            for (int k=0; k < (int)Ghk.size(); k++) {
//			Ghk[k][h] = fabs(2 * PI / sin(gamma) ) * sqrt((double(k) / a)*(double(k) / a) + (double(h) / b)*(double(h) / b));
//		}
//	}
//	// tranformed the theoretical brag peaks to a 1d set
//	int w = 0; 
//	while( w < (int)(Ghk.size() * Ghk.size()) ) {
//            for (int h=0; h < (int)Ghk.size(); h++) {
//                for (int k=0; k < (int)Ghk.size(); k++ , w++) {
//				fabsGhk[w]=Ghk[k][h];
//				
//			}
//		}
//	}
//	//sorts the array.
//	for (int i = 0; i < (int)fabsGhk.size(); i ++) {
//            for( int j = i; j < (int)fabsGhk.size(); j++) {
//			if (fabsGhk[i] > fabsGhk[j]) {
//				double p;
//				p=fabsGhk[i];
//				fabsGhk[i] = fabsGhk[j];
//				fabsGhk[j] = p;
//			}
//		}
//	}
//	//erased the doubled peaks and counts the peaks . 
//	for( int i = 1; i < (int)fabsGhk.size(); i++ ) {
//		if (fabs(fabsGhk[i-1]-fabsGhk[i])<1E-9) {
//			peaks_at_index[i] ++; 
//			fabsGhk.erase(fabsGhk.begin() + (i - 1));
//			peaks_at_index.erase(peaks_at_index.begin() + (i - 1));
//		}
//
//	}
//	int j = 0;
//	//to decide which index belongs to the first peak.
//	
//	oldwssr = wssr;
//	//to see if index should jump.
//	for (j = 0; j < (int)peaks.size() * 4; j++) {
//            for (int k = j; k < (int)peaks.size() + j; k++) {
//                for (int l = j; l < (int)peaks.size() + j; l++) {
//                    for (int i = 0; i < (int)indices.size(); i++) indices.at(i) = 0.0;
//					int counter = j;
//					for (int i = 0; i < (int)peaks.size(); i++, counter++) {
//						//skipping up to 2 peaks in order to find the best peak's series. 
//						if (i + j == k + 1 || i + j == l + 1) counter++;		
//						indices[i] = fabsGhk[counter + 1];
//						indices_num[i] = peaks_at_index[counter + 1];
//
//					}
//			
//					//we'll fit to a line with indices as x, and centres as y
//					//we use the linear least squares	
//					//to understand linearfit work go to:
//					//http://en.wikipedia.org/wiki/Linear_least_squares subchapter: Computation 
//					res = linearfit(indices, peaks);
//					if (wssr > res.second) {
//						wssr = res.second;
//						slope = res.first;
//						ind = indices;
//						ind_num = indices_num;
//
//					}
//				}
//		}
//	}
//	//retrieve back the h and k from before
//	for (int i = 0; i< (int)ind.size(); i++) {
//		std::vector <std::vector <int> > indices_pair;
//		for (int h = 0; h < (int)Ghk.size(); h++) {
//                    for(int k = 0; k < (int)Ghk.size(); k++) {
//				if(fabs(ind[i] - Ghk[k][h])<1E-6) {
//					std::vector <int> s(3,0);
//					s[0] = k;
//					s[1] = h;
//					indices_pair.push_back(s);
//					ind_num[i] --;
//					if (ind_num[i] == 0 ) break;
//				}
//				
//			}
//			if (ind_num[i] == 0 ) break;
//		
//		}
//		indices_loc.push_back(indices_pair);
//	}
						
	/*
		
	std::vector <double> indices(peaks.size(),0.0),ind(peaks.size(),0.0), theoind(2*peaks.size(),0.0);
	std::pair<double, double> res;
	double oldwssr ,wssr=100000.0,slope=0.0;
	
	
	theoind = GeneratePhases2D(peaks.size(), a, b, gamma);
	
					
	int j=0;
	//to decide which index belongs to the first peak.
	
	do {
		oldwssr=wssr;
		//to see if index should jump.
		for (int k=0;k<peaks.size(); k++) {
			for (int l=0; l<peaks.size(); l++) {
				for (int i=0; i<peaks.size(); i++) {
					if (i==k+1||i==l+1) continue;
//check meaning of this: 				
					indices[i]=theoind[i+j+1];
				}
				//we'll fit to a line with indices as x, and centres as y
				//we use the linear least squares	
				//to understand linearfit work go to: http://en.wikipedia.org/wiki/Linear_least_squares subchapter: Computation 
				res = linearfit(indices, peaks);
				if (wssr>res.second) {
					wssr=res.second;
					slope=res.first;
					ind=indices;
				}
			}
		}	
		j++;
	} while(oldwssr>wssr);
	return std::pair <std::vector <double> , double> (ind,wssr);
	*/
//}
  /*  
	double chisq = 0.0, lastChisq = 0.0; // Temporal Chi-Squared (Coefficient of Determination)
	double lambda = -1.0; // The Lambda (sometimes mu) coefficient


	


		//////////////////////////////////////////////////////////////////////////
	// Fitting

	// The main fitting loop (each iteration yields a different parameter structure)
	std::vector <double> temp(3,0.0);
	for(int i = 0; i < GetFitIterations(); i++) {
		lastChisq = chisq;
		temp = FitPhaseIndices2D(peaks, params[0], params[1], params[2]).first;
		mrqmin(temp, peaks, factor, weights, temp.size(), params, NULL, NULL, ia, 3, covar,
			alpha, &chisq, &lambda, 1 );
		if(getErrorCode())
			break;

	//	if(pStop && *pStop)
	//		break;

	}

	


	//////////////////////////////////////////////////////////////////////////
	// Finalization

	// Clearing the stop signal so it won't interrupt us while we generate the final model
	// Finalizing the Levenberg-Marquardt variables
	lambda = 0.0;
	mrqmin(temp,peaks,factor,weights,temp.size(),params,NULL,NULL,ia,3,covar,alpha,&chisq,&lambda,1);
	// We still have an error, we should probably exit as soon as possible
	if(getErrorCode())
		goto cleanup;

	// Saving back the parameters and extra parameters to the paramStruct
	

	for (int i=0; i<3; i++)  {
		tempo[i]=params[i];
	}
	// After fitting the model, generate the final graph to show to the user
	

cleanup:
	delete[] ia;
	delete[] weights;
	delete[] dyda;
	free_matrix(alpha, 1, ma, 1, ma);
	free_matrix(covar, 1, ma, 1, ma);

	return temp;
	
	
*/	
//}
EXPORTED std::pair<double, double> LinearFit(std::vector<double> x,
	std::vector<double> y) {
	if (x.size() != y.size() || x.empty())
		return std::pair<double, double>(0.0, 0.0);

	int n = (int)x.size();

	MatrixXd A(n, 2);
	VectorXd b(n);

	for (int i = 0; i < n; i++) {
		A(i, 0) = x[i];
		A(i, 1) = 1.0;
		b(i) = y[i];
	}

	Vector2d coeff = A.colPivHouseholderQr().solve(b);

	return std::pair<double, double>(coeff(0), coeff(1));
}