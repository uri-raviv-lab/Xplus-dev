#ifndef __STRUCTUREFITTING_H
#define __STRUCTUREFITTING_H

#include <vector>
#include "Eigen/Core"

#include "globalsettings.h"


typedef struct ParamCons {
	double value, min, max;
	char mut;
	int minInd, maxInd, linkInd;

	ParamCons(double val = 0.0) : value(val), min(0.0), max(0.0), mut('N'), minInd(-1), maxInd(-1), linkInd(-1) {}
} genParameter;

typedef struct {
	std::vector<genParameter> amp, fwhm, center, cailleSigma, cailleNDiffused;
} peakStruct;

typedef struct {
	std::vector<double> x, y;
} graphTable;

typedef struct {
	std::vector<genParameter> amp, eta, N, sig, N_diffused;
} cailleParamStruct;

typedef struct {
	double a,b,c,gamma,alpha,beta, amin, bmin, cmin, gammamin,alphamin, betamin, 
		amax, bmax, cmax, gammamax,alphamax, betamax;
	
	char aM,bM,cM,gammaM,alphaM,betaM;

	double um, ummin, ummax;

	char umM;

	double qmax;
} phaseStruct;

typedef double (*peakF)(double sig, double xc, double A, double B, double x);
extern peakF PeakShape;

typedef void (*progressFunc)(int progress);
typedef void (*plotFunc)(const std::vector<double>& x, const std::vector<double>& y);


EXPORTED bool GenerateStructureFactor(const std::vector<double> x, std::vector<double>& y, peakStruct *p);
EXPORTED bool GenerateStructureFactorU(const std::vector<double> x, std::vector<double>& y, const std::vector<double>& bgy, 
									   peakStruct *p, plotFunc GraphModify, int *pStop, progressFunc ProgressReport);

EXPORTED bool FitStructureFactor(const std::vector<double> sfx, const std::vector<double> sfy, 
								 std::vector<double>& my, const std::vector<double>& bgy, const std::vector<bool>& mask, peakStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors);
EXPORTED bool FitStructureFactorU(const std::vector<double> sfx, const std::vector<double> sfy, 
								  std::vector<double>& my, const std::vector<double>& bgy, const std::vector<bool>& mask, peakStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
								 int *pStop, progressFunc ProgressReport);

double StructureFactorIntensity(double q, double* a, int ma, int nd);

EXPORTED void FitPhases1D (std::vector <double> peaks,
                           std::vector<std::vector<std::vector<int> > > &indices_loc, double &wssr,
                           double &slope);

//EXPORTED void FitPhaseIndices2D(std::vector <double> peaks ,double a,
//                                double b, double gamma,
//                                std::vector<std::vector<std::vector<int> > > &indices_loc,
//                                double &wssr,double &slope);
EXPORTED bool FitPhases (PhaseType phase , std::vector<double>& peaks, phaseStruct *p, std::vector<double>& paramErrors,
						 std::vector<std::string> &locs);
EXPORTED bool FitPhasesU (PhaseType phase, std::vector<double>& peaks, phaseStruct *p, std::vector<double>& paramErrors,
						  std::vector<std::string> &locs,
						  int *pStop, progressFunc ProgressReport);
//EXPORTED double Phases (double a, double b, double c, double gamma, double phi, double theta, double q, double um);
//EXPORTED bool GeneratePhases(const std::vector<double> x, std::vector<double>& y, phaseStruct *p);
//EXPORTED bool GeneratePhasesU(const std::vector<double> x, std::vector<double>& y, const std::vector<double>& bgy, phaseStruct *p,
//								      plotFunc GraphModify, int *pStop, progressFunc ProgressReport);
Eigen::VectorXd CalculatePhaseDifference(std::vector <double> peakCenters, Eigen::MatrixXd& g,
										 int dim);
//EXPORTED double GenerateNextPhasePeak(const std::vector<double> peakPositions, const phaseStruct *p);
EXPORTED std::pair <double, double> LinearFit(std::vector <double> x, std::vector <double> y);
EXPORTED std::vector <double> GenPhases (PhaseType phase , phaseStruct *p,
								std::vector<std::string> &locs);
void uniqueIndices(const Eigen::VectorXd& G_norm, std::vector<double>& result, std::vector<std::string> &locs);
int phaseDimensions(PhaseType phase);
#endif
