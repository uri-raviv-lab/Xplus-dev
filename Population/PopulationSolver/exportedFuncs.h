#include "Fitter.h"
//#include "annoying.h"
#include "Eigen/Core"
using namespace Eigen;



EXPORTED bool FitCoeffs(const ArrayXd& datay, const ArrayXd& datax, const std::string expression,
						const ArrayXXd& curvesX, const ArrayXXd& curvesY, ArrayXd& p, const ArrayXi& pmut,
						cons *pMin, cons *pMax, ArrayXd& paramErrors, ArrayXd& modelErrors);
