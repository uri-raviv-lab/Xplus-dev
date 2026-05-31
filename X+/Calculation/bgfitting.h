#ifndef __BGFITTING_H
#define __BGFITTING_H

#include <vector>
using std::vector;

#include "globalsettings.h"

typedef struct {
	std::vector<BGFuncType> type;
	std::vector<double> base, decay, center;
	std::vector<char> baseMutable, decayMutable, centerMutable;
	std::vector<double> basemin, basemax, decmin, decmax, centermin, centermax;
} bgStruct;

typedef void (*progressFunc)(int progress);
typedef void (*plotFunc)(const std::vector<double>& x, const std::vector<double>& y);

EXPORTED bool FitBackground(const vector<double> bgx, const vector<double> bgy, 
						    vector<double>& resy, const vector<double>& signaly, const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors);
EXPORTED bool FitBackgroundU(const vector<double> bgx, const vector<double> bgy, 
						     vector<double>& resy, const vector<double>& signaly, const vector<bool>& mask, bgStruct *p, std::vector<double>& paramErrors, std::vector<double>& modelErrors, plotFunc GraphModify, 
						     int *pStop, progressFunc ProgressReport);

EXPORTED bool GenerateBackground(const std::vector<double> bgx, std::vector<double>& genY,
				 				 bgStruct *p);
EXPORTED bool GenerateBackgroundU(const std::vector<double> x, std::vector<double>& genY, 
								  const vector<double>& signaly, bgStruct *p, plotFunc GraphModify, int *pStop,
								  progressFunc ProgressReport);

#endif
