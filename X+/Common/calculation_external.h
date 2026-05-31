#pragma once 

#ifndef __CALCULATION_EXT_H
#define __CALCULATION_EXT_H

#ifdef _WIN32
#ifdef CALCULATION
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED __declspec(dllimport)
#endif
#else
#define EXPORTED extern "C"
#endif

#undef min
#undef max

#include <string>
#include <vector>

using std::vector;

#include "../Calculation/Model.h"
#include "../Calculation/ModelContainer.h"

// Special types

enum ModelType { MODEL_ROD, MODEL_SPHERE, MODEL_SLAB,
				 MODEL_ASLAB, MODEL_HELIX, MODEL_RECT, MODEL_CYLINDROID, MODEL_DELIX};

enum PeakType { SHAPE_GAUSSIAN, SHAPE_LORENTZIAN, SHAPE_LORENTZIAN_SQUARED, SHAPE_CAILLE };

enum PhaseType{	PHASE_NONE, PHASE_LAMELLAR_1D, PHASE_2D,PHASE_RECTANGULAR_2D,
PHASE_CENTERED_RECTANGULAR_2D, PHASE_SQUARE_2D, PHASE_HEXAGONAL_2D, PHASE_3D, PHASE_RHOMBO_3D, 
				PHASE_HEXA_3D, PHASE_MONOC_3D,PHASE_ORTHO_3D,PHASE_TETRA_3D,PHASE_CUBIC_3D};

enum QuadratureMethod { QUAD_MONTECARLO, QUAD_GAUSSLEGENDRE, QUAD_SIMPSON };

enum BGFuncType { BG_EXPONENT, BG_LINEAR, BG_POWER };

enum FitMethod { FIT_LM, FIT_DE, FIT_RAINDROP };

typedef struct {
	std::vector<double> x, y, tmpY;
} graphTable;

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
	std::vector<genParameter> amp, eta, N, sig, N_diffused;
} cailleParamStruct;

typedef struct {
	std::vector<BGFuncType> type;
	std::vector<double> base, decay, center;
	std::vector<char> baseMutable, decayMutable, centerMutable;
	std::vector<double> basemin, basemax, decmin, decmax, centermin, centermax;
} bgStruct;

typedef struct {
	double a,b,c,gamma,alpha,beta, amin, bmin, cmin, gammamin,alphamin, betamin, 
		amax, bmax, cmax, gammamax,alphamax, betamax;
	
	char aM,bM,cM,gammaM,alphaM,betaM;

	double um, ummin, ummax;

	char umM;

	double qmax;
} phaseStruct;

typedef void (*progressFunc)(int progress);
typedef void (*plotFunc)(const std::vector<double>& x, const std::vector<double>& y);


EXPORTED void SetPeakType(PeakType type);
EXPORTED void SetUseFrozenFF(bool a);
EXPORTED void SetSigma (bool a);
EXPORTED void SetConsEccentricity(bool ecc);
EXPORTED void setAccuracySettings(bool accurateFitting, bool accurateDerivative, bool wssrFitting, bool islogfitting);
EXPORTED PeakType GetPeakType();
EXPORTED bool isAccurateFitting();
EXPORTED bool isAccurateDerivative();
EXPORTED bool isLogFitting();
EXPORTED bool isWSSRFitting();

EXPORTED double GetMinimumSig();
EXPORTED void SetMinimumSig(double sig);

EXPORTED double GetResolution() ;
EXPORTED void SetResolution(double sig) ;

EXPORTED int GetFitIterations();
EXPORTED void SetFitIterations(int iterations);
EXPORTED FitMethod GetFitMethod();
EXPORTED void SetFitMethod(FitMethod method);

EXPORTED PeakType GetPDFunc();
EXPORTED void SetPDFunc(PeakType shape);
EXPORTED int GetPDResolution();
EXPORTED void SetPDResolution(int resolution);

EXPORTED bool hasGPUBackend();
EXPORTED bool isGPUBackend();
EXPORTED void SetGPUBackend(bool bEnabled);


// Form factor/Structure factor/Background fitting

EXPORTED bool CreateModel(const vector<double> ffx, const vector<double> ffy, 
						  vector<double>& resy, const vector<double>& bgy, const vector<bool>& mask, paramStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, int *pStop);

EXPORTED bool CreateModelU(const vector<double> ffx, const vector<double> ffy, 
						  vector<double>& resy, const vector<double>& bgy, const vector<bool>& mask, paramStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
						  int *pStop, progressFunc ProgressReport);

EXPORTED bool GenerateModel(const std::vector<double> x, std::vector<double>& genY,
				 		    paramStruct *p, int *pStop);

EXPORTED bool GenerateModelU(const std::vector<double> x, std::vector<double>& genY, 
							 const vector<double>& bgy, paramStruct *p, plotFunc GraphModify, int *pStop,
							 progressFunc ProgressReport);

EXPORTED bool GenerateStructureFactor(const std::vector<double> x, std::vector<double>& y, peakStruct *p);
EXPORTED bool GenerateStructureFactorU(const std::vector<double> x, std::vector<double>& y, const vector<double>& bgy, 
									   peakStruct *p, plotFunc GraphModify, int *pStop, progressFunc ProgressReport);

EXPORTED bool FitStructureFactor(const std::vector<double> sfx, const std::vector<double> sfy, 
								 std::vector<double>& my, const vector<double>& bgy, const vector<bool>& mask, peakStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors);
EXPORTED bool FitStructureFactorU(const std::vector<double> sfx, const std::vector<double> sfy, 
								 std::vector<double>& my, const vector<double>& bgy, const vector<bool>& mask, peakStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
								 int *pStop, progressFunc ProgressReport);

EXPORTED bool FitBackground(const vector<double> bgx, const vector<double> bgy, 
						    vector<double>& resy, const vector<double>& signaly, const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors);
EXPORTED bool FitBackgroundU(const vector<double> bgx, const vector<double> bgy, 
						     vector<double>& resy, const vector<double>& signaly, const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
						     int *pStop, progressFunc ProgressReport);

EXPORTED bool GenerateBackground(const std::vector<double> bgx, std::vector<double>& genY,
				 				 bgStruct *p);
EXPORTED bool GenerateBackgroundU(const std::vector<double> x, std::vector<double>& genY, 
								  const vector<double>& signaly, bgStruct *p, plotFunc GraphModify, int *pStop,
								  progressFunc ProgressReport);
EXPORTED std::vector <double> MachineResolution(const std::vector <double> &q ,const std::vector <double> &orig, double width);

// Statistics

EXPORTED double WSSR(std::vector<double> first, std::vector<double> second);

EXPORTED double WSSR_Masked(std::vector<double> first, std::vector<double> second, const std::vector<bool>& masked);

EXPORTED double RSquared(std::vector<double> data, std::vector<double> fit);

EXPORTED double RSquared_Masked(std::vector<double> data, std::vector<double> fit, const std::vector<bool>& masked);

// File management

EXPORTED int CheckSizeOfFile(const wchar_t *filename);

EXPORTED void ReadDataFile(const wchar_t *filename,
						   std::vector<double>& x, 
						   std::vector<double>& y);

EXPORTED void Read1DDataFile(const wchar_t *filename,
							 std::vector<double>& x);

EXPORTED void WriteDataFileWHeader(const wchar_t *filename, vector<double>& x,
				   vector<double>& y, std::stringstream& header);

EXPORTED void WriteDataFile(const wchar_t *filename, std::vector<double>& x, 
							std::vector<double>& y);

EXPORTED void Write3ColDataFile(const wchar_t *filename, std::vector<double>& x, 
							std::vector<double>& y, std::vector<double>& err);

EXPORTED void Write1DDataFile(const wchar_t *filename, std::vector<double>& x);

EXPORTED void GetDirectory(const wchar_t *file, wchar_t *result, int n = 260);
EXPORTED void GetBasename(const wchar_t *file, wchar_t *result, int n = 260);

// Smoothing

EXPORTED void smoothVector(int strength, std::vector<double>& data);

// Quadrature
EXPORTED void SetupIntegral(Eigen::VectorXd& x, Eigen::VectorXd& w, 
				   double s, double e, int steps);

EXPORTED void ClassifyQuadratureMethod(QuadratureMethod method);

EXPORTED void SetQuadResolution(int res);

// Background/Baseline

EXPORTED void GenerateBGLinesandFormFactor(const wchar_t *datafile,
                                           const wchar_t *baselinefile,
                                           std::vector <double>& bglx,
                                           std::vector <double>& bgly,
                                           std::vector <double>& ffy,bool ang);

EXPORTED void ImportBackground(const wchar_t *filename, 
							   const wchar_t *datafile,
							   const wchar_t *savename,
							   bool bFactor);

EXPORTED void AutoBaselineGen(const std::vector<double>& datax,
							  const std::vector<double>& datay, std::vector<double>& bgy);

// Phases

EXPORTED void FitPhases1D (std::vector <double> peaks,
                           std::vector<std::vector<std::vector<int> > > &indices_loc, double &wssr,
                           double &slope);

//EXPORTED void FitPhaseIndices2D(std::vector <double> peaks ,double a,
//                                double b, double gamma,
//                                std::vector<std::vector<std::vector<int> > > &indices_loc,
//                                double &wssr,double &slope);
//
//EXPORTED double Phases   (double a, double b, double c, double gamma, double phi, double theta, double q, double um);
//EXPORTED bool GeneratePhases (const std::vector<double> x, std::vector<double>& y, phaseStruct *p);
//EXPORTED bool GeneratePhasesU(const std::vector<double> x, std::vector<double>& y, const std::vector<double>& bgy, phaseStruct *p,
//								      plotFunc GraphModify, int *pStop, progressFunc ProgressReport);
//EXPORTED double GenerateNextPhasePeak(const std::vector<double> peakPositions, const phaseStruct *p);

EXPORTED bool FitPhases (PhaseType phase , std::vector<double>& peaks, phaseStruct *p, std::vector<double>& paramErrors,
						 std::vector<std::string> &locs);
EXPORTED bool FitPhasesU (PhaseType phase, std::vector<double>& peaks, phaseStruct *p, std::vector<double>& paramErrors,
						  std::vector<std::string> &locs,
						  int *pStop, progressFunc ProgressReport);
EXPORTED std::vector <double> GenPhases (PhaseType phase , phaseStruct *p,
								std::vector<std::string> &locs);
/*

EXPORTED double Phases (double a, double b, double c, double gamma, double phi, double theta, double q);

EXPORTED bool GeneratePhases(const std::vector<double> x, std::vector<double>& y, phaseStruct *p);

EXPORTED bool GeneratePhasesU(const std::vector<double> x, std::vector<double>& y, phaseStruct *p,
								      plotFunc GraphModify, int *pStop, progressFunc ProgressReport);
*/
// INI Management

EXPORTED double GetIniDouble (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED void   SetIniDouble (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
						     double value, int precision = 6);

EXPORTED int  GetIniNoOfParameters (const std::wstring& file, const std::string& object, const std::string& param, void* ini);

EXPORTED int  GetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED int  GetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini, int defval);
EXPORTED void SetIniInt (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
					    int value);

EXPORTED char GetIniChar (const std::wstring& file, const std::string& object, const std::string& param, void* ini);
EXPORTED void SetIniChar (const std::wstring& file, const std::string& object, const std::string& param, void* ini,
						 char value);

EXPORTED void GetIniString(const std::wstring& file, const std::string& section, const std::string& key, 
				 		   std::string& result, void* ini);
EXPORTED void SetIniString (const std::wstring& file, const std::string& object, const std::string& param,
							void* ini, const std::string& value);

EXPORTED void ReadParameters(const std::wstring &filename, std::string obj, paramStruct *p, void* ini);
EXPORTED void WriteParameters(const std::wstring &filename, std::string obj, paramStruct *p, void* ini);

EXPORTED void ReadPeaks(const std::wstring &filename, std::string obj, peakStruct *peaks, void* ini);
EXPORTED void WritePeaks(const std::wstring &filename, std::string obj, const peakStruct *peaks, void* ini);

EXPORTED void WriteBG(const std::wstring &filename, std::string obj, const bgStruct *BGs, void* ini);
EXPORTED void ReadBG(const std::wstring &filename, std::string obj, bgStruct *BGs, void* ini);

EXPORTED void WriteCaille(const std::wstring &filename, std::string obj, const graphTable *Cailles, const cailleParamStruct *Caillesdrawing, void* ini);
EXPORTED void ReadCaille(const std::wstring &filename, std::string obj, graphTable *Cailles, cailleParamStruct *Caillesdrawing, void* ini);

EXPORTED void WritePhases(const std::wstring &filename, std::string obj, const phaseStruct *ph, int pt, void* ini);
EXPORTED void ReadPhases(const std::wstring &filename, std::string obj, phaseStruct *ph, int *pt, void* ini);

EXPORTED bool IniHasModelType(const std::wstring& file, const std::string& object, void* ini);

// Instantiates ini as a CSimpleIniA
EXPORTED void *NewIniFile();
// Deletes ini
EXPORTED void CloseIniFile(void* ini);

EXPORTED void SetD(double d);

//Linear Fit for Caille
EXPORTED std::pair <double, double> LinearFit(std::vector <double> x, std::vector <double> y);


// Peak shape
EXPORTED double gaussianSig(double fwhm, double xc, double A, double B, double x);
EXPORTED double gaussianFW(double fwhm, double xc, double A, double B, double x);

#endif
