#include "smooth.h"

using std::vector;

void smoothVector(int strength, vector<double>& data) {
	vector<double> newy = data;

	for(int iter = 0; iter < strength; iter++) {
		newy[0] = data[0];
		newy[data.size() - 1] = data[data.size() - 1];
		for(int i = 1; i < (int)data.size() - 1; i++)
			newy[i] = 0.25 * (data[i - 1] + (2.0 * data[i]) + data[i + 1]);
		
		data = newy;
	}
}
