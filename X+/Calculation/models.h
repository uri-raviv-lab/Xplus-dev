#ifndef __MODELS_H
#define __MODELS_H

#include "globalsettings.h"

#include <complex>
using std::complex;

#define SOLVENT 0

#include "Eigen/Core"
using namespace Eigen;
#include <math.h>

// Modular implementation of model
/*typedef double (*modelF)(double, double *, int, int);
typedef complex<double> (*ffFunction)(double, double, double, double *, int, 
									  int);
typedef double (*func)(ffFunction, double*, double, int, int);*/
extern int *pStop;
extern bool bPrecalculate;
extern bool bLastQ;

void SetSignal(int *pSignal);

void ClearSignal();

void LastQ();

void GetRandED(VectorXd& a, VectorXd& r, VectorXd& ed, int nd);

void GetR_ZandED_Symmetric(VectorXd& a, VectorXd& r, VectorXd& ed, int nd, VectorXd& z);

void GetR_ZandED(VectorXd& a, VectorXd& r, VectorXd& ed, int nd, VectorXd& z);

double HCIntensity(double q, VectorXd& a, int ma, int nd);

double RectangularIntensity(double q, VectorXd& a, int ma, int nd);

double ShortHelixIntensity(double q, VectorXd& a, int ma, int nd);

double shortHelix (double phi, double theta);

double HelixIntensity(double q, VectorXd& a, int ma, int nd);

double DiscreteHelixIntensity(double q, VectorXd& a, int ma, int nd);

double NLayeredShell(double q, VectorXd& a, int ma, int nd);

double NLayeredAsymSlabs(double q, VectorXd& a, int ma, int nd);

double NLayeredSlabs(double q, VectorXd& a, int ma, int nd);

double NLayeredPabloAsymSlabs(double q, VectorXd& a, int ma, int nd);

double NLayeredPabloSlabs(double q, VectorXd& a, int ma, int nd);

double NLayeredSlabs_GaussED(double q, VectorXd& a, int ma, int nd);

double NLayeredAsymSlabs_GaussED(double q, VectorXd& a, int ma, int nd);

double NlayeredCylindroid (double q, VectorXd& a, int ma, int nd);

double emulsionIntensity(double q, VectorXd& a, int ma, int nd);

double RoG(double q, VectorXd& a);

double NLayeredShell_Gaussian(double q, VectorXd& a, int ma, int nd);

double HCgaussianIntensity(double q, VectorXd& a, int ma, int nd);

double PolyDisperse(double q, VectorXd& a, int ma, int nd);

double FitScaleBG(double q, VectorXd& a, int ma, int nd);

#endif
