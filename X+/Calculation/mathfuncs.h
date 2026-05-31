#ifndef __MATHFUNCS_H
#define __MATHFUNCS_H

#include <vector>
#include "globalsettings.h"

// Common numerical factors
#ifndef ln2
#define ln2 0.69314718055995
#endif

#ifndef PI
#define PI 3.1415926
#endif

#ifndef EPS
#define EPS 3.0e-11
#endif

#ifndef max
#define max(a,b)	(a > b) ? a : b
#endif

#ifndef min
#define min(a,b)	(a < b) ? a : b
#endif

// Peak Shapes
EXPORTED double gaussianSig(double fwhm, double xc, double A, double B, double x);
EXPORTED double gaussianFW(double fwhm, double xc, double A, double B, double x);
double DenormGaussianFW(double fwhm, double xc, double A, double B, double x);
double lorentzian (double fwhm, double xc, double A, double B, double x);
double lorentzian_squared (double fwhm, double xc, double A, double B, double x);
double Caille_peak(double fwhm, double xc, double A, double B, double x);
double CailleDummy(double fwhm, double xc, double A, double B, double x, double &N_diff);

// Background Shapes
double exponentDecay(double x, double base, double decay, double xcenter);
double linearFunction(double x, double base, double decay, double xcenter);
double powerFunction(double x, double base, double decay, double xcenter);

// Bessel functions (of first, second and nth order)
double bessel_j0(double x);
double bessel_j1(double x);
double bessel_jn(int n, double x);

template <typename T> inline T sq(T x) { return x * x; }
	
void SetX(std::vector <double> xIn);

double Mean(std::vector<double> data);

EXPORTED double WSSR(std::vector<double> first, std::vector<double> second);

EXPORTED double WSSR_Masked(std::vector<double> first, std::vector<double> second, const std::vector<bool>& masked);

EXPORTED double RSquared(std::vector<double> data, std::vector<double> fit);

EXPORTED double RSquared_Masked(std::vector<double> data, std::vector<double> fit, const std::vector<bool>& masked);

EXPORTED std::vector <double> MachineResolution(const std::vector <double> &q ,const std::vector <double> &orig, double width);
 
#endif
