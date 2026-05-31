#ifndef __FITTING_H
#define __FITTING_H

#include "Eigen/Core"
#include "Eigen/Geometry"

using namespace Eigen;

#include <vector>

class Model;

// Cons: Constraint vector description
//       index - The layer index to compare to (-1 if doesn't exist)
//		 link  - The layer index to link a value to (-1 if doesn't exist)
//       num   - The value to compare to (0.0 if doesn't exist)
typedef struct ConsStruct {
	VectorXi index, link;
	VectorXd num;

	ConsStruct(int m) : index(VectorXi::Constant(m, -1)), link(VectorXi::Constant(m, -1)), num(VectorXd::Zero(m)) {}
	ConsStruct()  {}  
} cons;

class ModelFitter {
protected:
    // Model to fit
	Model *FitModel;
        
	// Parameters
	const std::vector<double> &x, &y, &mult, &add;
	VectorXd interimResY;
	VectorXd sqWeights;
	VectorXd params;
	MatrixXd J;
	std::vector<double> *err, *errY;
	double mse;
	const VectorXi& paramMut;
	int mutables, nParams, nLayers;
	cons *p_min, *p_max;

	bool error;

public:
	/** 
	 * Creates and initializes a new fitter.
	 * Parameters:
	 * FitModel - The corresponding model function to fit to
	 * datax, datay - The data to fit to (must have same size)
	 * factor - Multiplicative factor to multiply the model by (must have the same size as datax)
	 * bg - Additive factor to add to the model (must have the same size as datax)
	 * fitWeights - Weights for the Chi-Squared error function
	 * p - Parameter vector
	 * pmut - Parameter mutability vector (0 = immutable, 1 = mutable)
	 * pMin, pMax - The constraint vector pointers
	 * layers - The number of layers in the data (input for the FitModel function)
	 *
	 * Output: The 'p' parameter vector is modified with each fitting iteration
	 */
 ModelFitter(Model *model, const std::vector<double>& datax, 
             const std::vector<double>& datay,
             const std::vector<double>& factor, 
             const std::vector<double>& bg,
             const std::vector<double>& fitWeights, VectorXd& p,
             const VectorXi& pmut, cons *pMin, cons *pMax,
			 std::vector<double>& paramErrors,
			 std::vector<double>& modelErrors,
             int layers) :
        FitModel(model), x(datax), y(datay),
            mult(factor), add(bg), params(p), paramMut(pmut),
            nParams(params.size()), nLayers(layers), p_min(pMin), p_max(pMax), error(false)
        { 

            mutables = 0;
            for(int i = 0; i < nParams; i++)
                if(paramMut[i])
                    mutables++;

            sqWeights = VectorXd::Zero(fitWeights.size());
            for(int i = 0; i < (int)fitWeights.size(); i++)
                sqWeights[i] = fitWeights[i] * fitWeights[i];
	}

	bool GetError() const { return error; }

	virtual ~ModelFitter() {}

	// Performs one fitting iteration, modifies the parameter vector and returns the current WSSR
	virtual double FitIteration() = 0;

	virtual void calcErrors() = 0;

	virtual VectorXd GetResult() const { return params; }

	virtual VectorXd GetInterimRes() {return interimResY;}

private:
	// Will not assign to other ModelFitters
	void operator=(ModelFitter& rhs) {}
};

// Levenberg-Marquardt Nonlinear Model Fitting Class
class LMFitter : public ModelFitter {
protected:
	// LM Coefficients
	MatrixXd alpha;
	VectorXd beta;

	double lambda, curWssr;

	// Calculates the current fitting coefficients and returns the WSSR
	double CalculateCoefficients(VectorXd& p, MatrixXd& alphaMat, VectorXd& betaVec);

public:
	// Creates and initializes a new Levenberg-Marquardt fitter
	LMFitter(Model *model, const std::vector<double>& datax, 
                 const std::vector<double>& datay,
                 const std::vector<double>& factor, 
				 const std::vector<double>& bg,
                 const std::vector<double>& fitWeights, VectorXd& p,
                 const VectorXi& pmut, cons *pMin, cons *pMax,
				 std::vector<double>& paramErrors,
				 std::vector<double>& modelErrors,
                 int layers) : 
		ModelFitter(model, datax, datay, factor, bg, fitWeights, p, pmut, pMin, pMax, paramErrors, modelErrors, layers) {
            // TODO: Modify CreateFitter, Parameter and Fitter constructor to support PD
			if(mutables == 0) {
				error = true; 
				return;
			}

			alpha = MatrixXd::Zero(mutables, mutables);
			beta  = VectorXd::Zero(mutables);
			err = &paramErrors;
			errY = &modelErrors;

            lambda = 0.001;
            
            curWssr = CalculateCoefficients(params, alpha, beta);
	}

	// Performs one fitting iteration, modifies the parameter vector and returns the current WSSR
	virtual double FitIteration();

	virtual void calcErrors();

	virtual ~LMFitter() { }
};

#endif // __FITTING_H
