#ifndef __DEFITTING_H
#define __DEFITTING_H

#include "fitting.h"
#include "DESolver.h"

// TODO: Make a GUI setting
// TODO: Make the strategy a setting too
#define FIT_POPULATION 100

// Differential Evolution Solver Class
class DEModelSolver : public DESolver {
protected:
	int dataSize, nLayers;
	const std::vector<double> &x, &y;
	Model *FitModel;
public:
	// Creates and initializes a new Differential Evolution model fitter
	DEModelSolver(int nParams, int layers, VectorXd& params, int fitPopulation, 
				  Model *model, const std::vector<double>& datax, 
				  const std::vector<double>& datay) : DESolver(nParams, fitPopulation, params),
					nLayers(layers), FitModel(model), x(datax), y(datay) { 
		dataSize = x.size();
	}

	double EnergyFunction(VectorXd& trial);

	//virtual void calcErrors();

private:
	// Will not assign to other DEModelSolvers
	void operator=(DEModelSolver rhs) {}
};

// Differential Evolution Model Fitting Class
class DEFitter : public ModelFitter {
protected:
	DEModelSolver *solver;

public:
	// Creates and initializes a new Differential Evolution fitter
	DEFitter(Model *model, const std::vector<double>& datax, 
                 const std::vector<double>& datay,
                 const std::vector<double>& factor, 
				 const std::vector<double>& bg,
                 const std::vector<double>& fitWeights, VectorXd& p,
                 const VectorXi& pmut, cons *pMin, cons *pMax,
 				 std::vector<double>& paramErrors,
				 std::vector<double>& modelErrors,
                int layers) : 
	ModelFitter(model, datax, datay, factor, bg, fitWeights, p, pmut, pMin, pMax, paramErrors, modelErrors, layers), solver(NULL) {
            
			if(mutables == 0) return;


			// TODO: Fit only mutable parameters
			VectorXd pmin = VectorXd::Zero(nParams);
			VectorXd pmax = VectorXd::Zero(nParams);

			if(pMin)
				pmin = pMin->num;
			if(pMax)
				pmax = pMax->num;
			else {
				for(int i = 0; i < pmax.size(); i++)
					pmax[i] = 1.0e7;
			}

			solver = new DEModelSolver(nParams, nLayers, params, FIT_POPULATION, model, datax, datay);
			solver->Setup(pmin, pmax, stRand2Exp, 0.9, 1.0);

			solver->IncrementalSolveBegin();
		}

	virtual double FitIteration() { return solver->IncrementalSolveGeneration(); }

	virtual VectorXd GetResult() const { return solver->Solution(); }

	virtual void calcErrors() {};

	virtual ~DEFitter() { delete solver; }
};


#endif
