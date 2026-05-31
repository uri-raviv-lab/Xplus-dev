#ifndef __QUADRATURE_H
#define __QUADRATURE_H

#include "Eigen/Core"

enum QuadratureMethod { QUAD_MONTECARLO, QUAD_GAUSSLEGENDRE, QUAD_SIMPSON };

// Helper function to set up 1D quadrature
EXPORTED void SetupIntegral(Eigen::VectorXd& x, Eigen::VectorXd& w, 
				   double s, double e, int steps);

#endif
