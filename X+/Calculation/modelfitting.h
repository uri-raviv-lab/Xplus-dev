#ifndef __MODELFITTING_H
#define __MODELFITTING_H

#include <vector>
#include "globalsettings.h"
#include "Model.h"

using std::vector;

typedef void (*progressFunc)(int progress);
typedef void (*plotFunc)(const std::vector<double>& x, const std::vector<double>& y);

EXPORTED bool CreateModel(const vector<double> ffx, const vector<double> ffy, 
						  vector<double>& resy, const vector<double>& bgy,
						  const vector<bool> mask, paramStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, int *pStop);

EXPORTED bool CreateModelU(const vector<double> ffx, const vector<double> ffy, 
						  vector<double>& resy, const vector<double>& bgy, 
						  const vector<bool>& mask, paramStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
						  int *pStop, progressFunc ProgressReport);

EXPORTED bool GenerateModel(const std::vector<double> x, std::vector<double>& genY,
				 		    paramStruct *p, int *pStop);

EXPORTED bool GenerateModelU(const std::vector<double> x, std::vector<double>& genY, 
							 const vector<double>& bgy, paramStruct *p, plotFunc GraphModify, int *pStop,
							 progressFunc ProgressReport);


#endif // __MODELFITTING_H

