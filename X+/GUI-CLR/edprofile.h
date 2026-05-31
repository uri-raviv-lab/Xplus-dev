#ifndef __EDPROFILE_H
#define __EDPROFILE_H

#include <vector>

#include "calculation_external.h"

void generateEDProfile(std::vector< std::vector<Parameter> > p,
					   struct graphLine *graphs, EDProfile profile);

std::pair<double, double> calcEDIntegral(std::vector<Parameter>& r, std::vector<Parameter>& ed);

#endif
