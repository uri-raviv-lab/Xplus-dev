#include "globalsettings.h"
#include "clrun.h"
#include "gpgpu.h"

PeakType g_peakShape = SHAPE_GAUSSIAN;
FitMethod g_fitMethod = FIT_LM;
bool g_gaussianED = false;
bool g_gaussianMembrane = false;
bool g_Pablo = false;
bool g_consEcc = false;
bool g_accurateDerivative = false;
bool g_accurateFitting = true;
bool g_WSSRFitting = false;
bool g_logFitting = false;
bool g_sigma = false;
bool g_gpu = false;
bool g_bFrozen = false;		// Flag to tell if we're trying to fit using a given FF
double _D = 0.0;			// d-spacing for Caille
bool g_bScale = false;		// I forgot...

int g_pdResolution = 15;
PeakType g_pdFunc = SHAPE_GAUSSIAN;

double MINIMUM_SIGNAL = 5.0;
double RESOLUTION_WIDTH = 0.0;

int g_fitIterations = 20;

EXPORTED void SetPeakType(PeakType type) {
	g_peakShape = type;
}

EXPORTED void SetUseFrozenFF(bool a) {
	g_bScale = a;
}

EXPORTED void SetGaussED(bool gaus) {
	g_gaussianED = gaus;
}

EXPORTED void SetSigma (bool a) { 
	g_sigma = a;
}

EXPORTED void SetGaussMembrane(bool mem) {
	g_gaussianMembrane = mem;
}

EXPORTED void SetCuboidModel(bool pablo) {
	g_Pablo = pablo;
}

EXPORTED void SetConsEccentricity(bool ecc) {
	g_consEcc = ecc;
}

EXPORTED PeakType GetPeakType() {
	return g_peakShape;
}

EXPORTED bool isGaussED() {
	return g_gaussianED;
}

EXPORTED bool isSigma() {
	return g_sigma;
}

EXPORTED bool isGaussMembrane() {
	return g_gaussianMembrane;
}

EXPORTED bool isCuboidModel() {
	return g_Pablo;
}

EXPORTED bool isConsEcc() {
	return g_consEcc;
}


EXPORTED double GetMinimumSig() {
	return MINIMUM_SIGNAL;
}

EXPORTED void SetMinimumSig(double sig) {
	MINIMUM_SIGNAL = sig;
}
EXPORTED double GetResolution() {
	return RESOLUTION_WIDTH;
}

EXPORTED void SetResolution(double sig) {
	RESOLUTION_WIDTH = sig;
}

EXPORTED void setAccuracySettings(bool accurateFitting, bool accurateDerivative, bool wssrFitting, bool islogfitting) {
	g_accurateFitting = accurateFitting;
	g_accurateDerivative = accurateDerivative;
	g_WSSRFitting = wssrFitting;
	g_logFitting =  islogfitting;
}

EXPORTED bool isAccurateFitting() {
	return g_accurateFitting;
}

EXPORTED bool isAccurateDerivative() {
	return g_accurateDerivative;
}

EXPORTED bool isWSSRFitting() {
	return g_WSSRFitting;
}
EXPORTED bool isLogFitting() {
	return g_logFitting;
}

EXPORTED int GetFitIterations() {
	return g_fitIterations;
}


EXPORTED void SetFitIterations(int iterations) {
	g_fitIterations = iterations;
}

EXPORTED FitMethod GetFitMethod() {
	return g_fitMethod;
}

EXPORTED void SetFitMethod(FitMethod method) {
	g_fitMethod = method;
}

EXPORTED double GetD() {
	return _D;
}

EXPORTED void SetD(double d) {
	_D = d;
}

EXPORTED PeakType GetPDFunc() {
	return g_pdFunc;
}

EXPORTED void SetPDFunc(PeakType shape) {
	if(shape < SHAPE_GAUSSIAN || shape > SHAPE_LORENTZIAN_SQUARED)
		return;

	g_pdFunc = shape;
}

EXPORTED int GetPDResolution() {
	return g_pdResolution;
}

EXPORTED void SetPDResolution(int resolution) {
	if(resolution <= 0)
		return;

	g_pdResolution = resolution;
}

EXPORTED void SetGPUBackend(bool bEnabled) {
	if(hasGPUBackend()) {
		g_gpu = bEnabled;
		if(bEnabled)
			InitializeOpenCL();
		else
			DestroyOpenCL();
	}
}

EXPORTED bool isGPUBackend() {
	return g_gpu;
}

EXPORTED bool hasGPUBackend() {
	clrInit();
	return (clrHasOpenCL() == 1);
}
