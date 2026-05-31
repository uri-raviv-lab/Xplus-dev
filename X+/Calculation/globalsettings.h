#ifndef __GLOBALSETTINGS_H
#define __GLOBALSETTINGS_H

#ifndef EXPORTED
#ifdef _WIN32
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED extern "C"
#endif
#endif

// Defines the current version of INI files
#define INI_VERSION 1

enum PeakType { SHAPE_GAUSSIAN, SHAPE_LORENTZIAN, SHAPE_LORENTZIAN_SQUARED, SHAPE_CAILLE };

enum BGFuncType { BG_EXPONENT, BG_LINEAR, BG_POWER };

enum PhaseType{	PHASE_NONE, PHASE_LAMELLAR_1D, PHASE_2D,PHASE_RECTANGULAR_2D,
PHASE_CENTERED_RECTANGULAR_2D, PHASE_SQUARE_2D, PHASE_HEXAGONAL_2D, PHASE_3D, PHASE_RHOMBO_3D, 
				PHASE_HEXA_3D, PHASE_MONOC_3D,PHASE_ORTHO_3D,PHASE_TETRA_3D,PHASE_CUBIC_3D};


enum FitMethod { FIT_LM, FIT_DE, FIT_RAINDROP };


EXPORTED void SetPeakType(PeakType type);
EXPORTED void SetGaussED(bool gaus);
EXPORTED void SetSigma (bool a);
EXPORTED void SetUseFrozenFF(bool a);
EXPORTED void SetGaussMembrane(bool mem);
EXPORTED void SetCuboidModel(bool pablo);
EXPORTED void SetConsEccentricity(bool ecc);
EXPORTED void setAccuracySettings(bool accurateFitting, bool accurateDerivative, bool wssrFitting);
EXPORTED PeakType GetPeakType();
EXPORTED bool isGaussED();
EXPORTED bool isSigma();
EXPORTED bool isGaussMembrane();
EXPORTED bool isCuboidModel();
EXPORTED bool isConsEcc();
EXPORTED bool isAccurateFitting();
EXPORTED bool isAccurateDerivative();
EXPORTED bool isWSSRFitting();
EXPORTED bool isLogFitting();

EXPORTED double GetMinimumSig();
EXPORTED void SetMinimumSig(double sig);

EXPORTED double GetResolution() ;
EXPORTED void SetResolution(double sig) ;

EXPORTED int GetFitIterations();
EXPORTED void SetFitIterations(int iterations);
EXPORTED FitMethod GetFitMethod();
EXPORTED void SetFitMethod(FitMethod method);
EXPORTED double GetD();
EXPORTED void SetD(double d);

EXPORTED PeakType GetPDFunc();
EXPORTED void SetPDFunc(PeakType shape);
EXPORTED int GetPDResolution();
EXPORTED void SetPDResolution(int resolution);


EXPORTED bool hasGPUBackend();
EXPORTED bool isGPUBackend();
EXPORTED void SetGPUBackend(bool bEnabled);



#endif
