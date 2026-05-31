#ifndef __BACKGROUND_H
#define __BACKGROUND_H

#include "globalsettings.h"
#include <string>
#include <vector>
#include <map>

typedef struct {
	std::vector<double> x;
	std::vector<double> y;
} graphStruct;

EXPORTED void ImportBackground(const char *filename, 
							   const char *datafile,
							   const char *savename,
							   bool bFactor);

EXPORTED void GenerateBGLinesandFormFactor(const char *datafile, 
										   const char *baselinefile,
				 						   std::vector <double>& bglx,
										   std::vector <double>& bgly,
										   std::vector <double>& ffy);

EXPORTED void AutoBaselineGen(const std::vector<double>& datax,
							  const std::vector<double>& datay, std::vector<double>& bgy);

inline void LineFunction(double, double, double, double, double *, double *);

#endif // __BACKGROUND_H
