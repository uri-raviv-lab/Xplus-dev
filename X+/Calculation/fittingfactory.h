#ifndef __FITTINGFACTORY_H
#define __FITTINGFACTORY_H

#include "fitting.h"
#include "DEFitting.h"
#include "MCFitting.h"
#include "globalsettings.h"

inline static ModelFitter *CreateFitter(Model *model, const std::vector<double>& datax, 
										const std::vector<double>& datay,
										const std::vector<double>& factor, 
										const std::vector<double>& bg,
										const std::vector<double>& fitWeights, VectorXd& p,
										const VectorXi& pmut, cons *pMin, cons *pMax,
										std::vector<double>& paramErrors,
										std::vector<double>& modelErrors,
										int layers) {
	switch(GetFitMethod()) {
		default:
		case FIT_LM:
			return new LMFitter(model, datax, datay, factor, bg, fitWeights, p, pmut, pMin, pMax, paramErrors, modelErrors, layers);
		case FIT_DE:
			return new DEFitter(model, datax, datay, factor, bg, fitWeights, p, pmut, pMin, pMax, paramErrors, modelErrors, layers);
		case FIT_RAINDROP:
			return new MCLMFitter(model, datax, datay, factor, bg, fitWeights, p, pmut, pMin, pMax, paramErrors, modelErrors, layers);
	}
}

#endif
