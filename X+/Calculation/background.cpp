#include "background.h"
#include "filemgt.h"

#include <cmath>
#include <cstring>
#include <limits>

using std::vector;


int selOne = -1, selTwo = -1;



double InterpolatePoint(double x0, const std::vector<double>& x, const std::vector<double>& y) {
    for(int i = 0; i < (int)x.size(); i++) {
		if(x0 <= x[i] && i != 0)
			return y[i - 1] + (x0 - x[i - 1]) * ((y[i] - y[i - 1]) / (x[i] - x[i - 1]));
	}
	return 0.0;
}


int vmin(const std::vector<double>& vec) {
	double y_min = vec[0];
	int res = 0;
	for (int i = 1; i < (int)vec.size(); i++) {
		if (y_min > vec[i]) {
			y_min = vec[i];
			res = i;
		}
	}

	return res;
}

inline double vminfrom(vector<double>& v, int from, int *m) {
	if(v.size() == 0)
		return 0.0;
	double val = v.at(0);
	for(unsigned int i = from; i < v.size(); i++) {
		if(v[i] < val) {
			val = v[i];
			if(m)
				*m = i;
		}
	}
	return val;
}

inline double vmax(vector<double>& v, int *m) {
	if(v.size() == 0)
		return 0.0;
	double val = v.at(0);
	for(unsigned int i = 0; i < v.size(); i++) {
		if(v[i] > val) {
			val = v[i];
			if(m)
				*m = i;
		}
	}
	return val;
}

/**
* Returns the slope and intercept (line equation) from given two points
* (x1,y1) and (x2,y2).
*/
inline void LineFunction(double x1, double y1, double x2, double y2, 
						 double *slope, double *intercept) {
	 double x11 = x1, x22 = x2, y11 = y1, y22 = y2;

	 *slope = ((y22 - y11) / (x22 - x11));
	 *intercept = y11 - ((*slope) * x11);
}

EXPORTED void ImportBackground(const wchar_t *filename, const wchar_t *datafile,
							   const wchar_t *savename, bool bFactor) {
	vector<double> datax, datay, bgx, bgy, bgyTemp, resx, resy;
	
	ReadDataFile(datafile, datax, datay);

	ReadDataFile(filename, bgx, bgy);
	
	// If the data files don't match in size or exact q-range crop and/or interpolate
	if((bgx.size() != datax.size()) || (fabs(bgx.at(0) - datax.at(0)) < 1.0e-8) || (fabs(bgx.at(bgx.size() - 1) - datax.at(datax.size() - 1)) < 1.0e-8)) {
		//Crop
		int j = 0, k = 0;
		for(; j < (int)datax.size() && datax[j] < bgx[0]; j++);
		for(k = int(datax.size()) - 1; k >= j && datax[k] > bgx[bgx.size() - 1]; k--);

		if(k - j < 1) { // less than 2 points left
			;
			return;
		}

		datax.erase(datax.begin() + k + 1, datax.end());
		datay.erase(datay.begin() + k + 1, datay.end());
		datax.erase(datax.begin(), datax.begin() + j);
		datay.erase(datay.begin(), datay.begin() + j);



		//Interpolate
		bgyTemp = datax;
		for(int i = 0; i < (int)datax.size(); i++)
			bgyTemp.at(i) = InterpolatePoint(datax.at(i), bgx, bgy);

		bgx = datax;
		bgy = bgyTemp;

	}

	if(!bFactor) {
		int q2 = 0;
		double fmin, fmax, gmin, gmax;
		gmax = vmax(bgy, &q2);
		fmax = InterpolatePoint(bgx.at(q2), datax, datay);
		fmin = datay[vmin(datay)];
		gmin = bgy[vmin(bgy)];

		resx.resize(datax.size());
		resy.resize(datay.size());
		// Fit background to data
		for(unsigned int i = 0; i < resx.size(); i++) {
			double x = datax[i], fx = datay[i];
			resx[i] = x;
			// We will now allow negative points
			/*bgy[i] = bgy[i] - gmin; /*((InterpolatePoint(x, bgx, bgy) - gmin) *
									((fmax - fmin) / (gmax - gmin)));*/
			resy[i] = fx /*- fmin*/ - bgy[i];
		}

		// Again, allowing negative values (for true background substraction
		//double minff = vminfrom(resy, q2 + 1, NULL);
		//for(unsigned int i = 0; i < resx.size(); i++)
		//	resy[i] = resy[i] /*+ GetMinimumSig()*/ - minff;
	} else { // Find the factor
		// Find the ratios for the second half of the graph
		// Take the lowest ratio (sig/BG)
		// resy.at(i) = sig[i] - (ratio * BG[i]) -->interpolate BG if need be
		std::vector<double> ratio;
		
		ratio.resize(datax.size(), std::numeric_limits<double>::max());
		for(int i = datax.size() / 5; i < (int)datax.size(); i++)
			if(!(fabs(bgy.at(i)) <= 1e-10))
				ratio.at(i) = datay.at(i) / bgy.at(i);
		
		int minRatio = 0;
		for(int i = 0; i < (int)ratio.size(); i++)
			if(ratio.at(i) < ratio.at(minRatio))
				minRatio = i;
		double rat = ratio.at(minRatio);

		// Fill out the result
		resx = datax; 
 		for(int i = 0; i < (int)datay.size(); i++)
			resy.push_back(datay.at(i) - rat * bgy.at(i));

	}

	WriteDataFile(savename, resx, resy);
}

EXPORTED void GenerateBGLinesandFormFactor(const wchar_t *workspace, 
								  const wchar_t *datafile,
								  std::vector <double>& bglx,
								  std::vector <double>& bgly,
								  std::vector <double>& ffy, bool ang) {
	vector<double> sx, datax, datay;
	double slope, intercept;
	
	vector<int> a;

	Read1DDataFile(datafile, sx);
	ReadDataFile(workspace, datax, datay);
	if(ang) {
		for (unsigned int i=0; i < datax.size(); i++) 
			datax[i] *= 10.0;
	}

	int ctr = 0;
	for (int i = 0; i < (int)datax.size(); i++){
		if (fabs(  datax.at(i) - sx.at(ctr))<=0.0001){
			a.push_back(i); 
			ctr++;
			if(ctr == (int)sx.size())
				break;
        }
    }

	vector<double> bx (a.back(), 0.0), by (a.back(), 0.0);

	double sya = InterpolatePoint(sx.at(0), datax, datay), 
		   syb = InterpolatePoint(sx.at(1), datax, datay);
	for (int j = 0; j < (int)sx.size() - 1; j++) {
        slope = ( log10(syb) - log10(sya) ) / ( log10(sx[j+1]) - log10(sx[j]) );

        intercept = log10(sya) - slope*log10(sx[j]);

        for (int i = a[j]; i < a[j+1]; i++) {
			bx[i] = datax[i];
			by[i] = pow(10.0, intercept) * pow(datax[i],slope);
        }

		sya = syb;
		if(j + 2 < (int)sx.size())
			syb = InterpolatePoint(sx.at(j + 2), datax, datay);
    }

	for (int i = a[0]; i < a[a.size() - 1]; i++) {
		bglx.push_back(bx[i]);
		bgly.push_back(by[i]);
		ffy.push_back(GetMinimumSig() + datay[i] - by[i]);
	}
}

void interpolate(const std::vector<double>& x, std::vector<double>& vec, int x1, int x2, double y1, double y2) {
	for(int i = x1; i < x2; i++)
		vec[i] = y1 + (x[i] - x[x1]) * ((y2 - y1) / (x[x2] - x[x1]));
}

// Returns -1 when there is no intersection
int findIntersection(const std::vector<double>& data, const std::vector<double>& bg, int x1, int x2) {
	
	for(int i = x1; i < x2; i++)
		if(bg[i] >= data[i])
			return i;
	/*
	for(int i = x2 - 1; i >= x1; i--)
		if(bg[i] >= data[i])
			return i;
	*/

	return -1;
}

EXPORTED void AutoBaselineGen(const std::vector<double>& datax, const std::vector<double>& datay, std::vector<double>& bgy) {
	// PSEUDOCODE:
	/*
	1. xmax = [find global minimum]
	2. we would like to find the first intersection of the line with increasing slope from xmin to xmax
	3. from intersection to xmax, we interpolate a line
	4. xmax = intersection
	5. do this (steps 3-6) until intersection = 0
	*/

	int minpos = vmin(datay);
	int xmax = minpos, xmin = 0, intersection = xmax;
	const double EPSILON = 1e-4;

	std::vector<double> logy;

	// Use log-scale y
	for(int i = 0; i < (int)datay.size(); i++)
		logy.push_back(log10((datay[minpos] <= 0.0) ? (datay[i] - datay[minpos] + EPSILON) : datay[i]));

	bgy.clear();
	bgy.resize(datax.size(), logy[minpos]);

	double cury = logy[xmax], height = -EPSILON;

	do {
		interpolate(datax, bgy, xmin, xmax, cury + height, bgy[xmax]);

		// While intersection is -1 (no intersection), we should keep going up
		do {
			height += EPSILON;

			interpolate(datax, bgy, xmin, xmax, cury + height, bgy[xmax]);

			// We aim to increase the slope until we find the point of the intersection
			intersection = findIntersection(logy, bgy, xmin, xmax);

			if(xmin >= xmax)
				intersection = 0;

		} while (intersection < 0);

		// Here we undo the last subiteration, so that the baseline won't pass the data
		height -= EPSILON;
		interpolate(datax, bgy, xmin, xmax, cury + height, bgy[xmax]);
		
		xmax = intersection;

	} while (intersection > 0);

	// Un-logscale the background
	for(int i = 0; i < (int)bgy.size(); i++)
		bgy[i] = pow(10, bgy[i]);

	if(datay[minpos] <= 0.0)
		for(int i = 0; i < (int)bgy.size(); i++)
			bgy[i] += datay[minpos] - EPSILON;
}
