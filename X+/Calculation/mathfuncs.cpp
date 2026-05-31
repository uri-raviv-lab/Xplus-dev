#define _USE_MATH_DEFINES
#include <cmath>
#include "models.h"
#include "mathfuncs.h"

std::vector <double> xGlob;
void SetX(std::vector <double> xIn) {
	xGlob = xIn;
}

double DenormGaussianFW(double fwhm, double xc, double A, double B, double x) {
	double result = 0.0, sigma;

	sigma = fwhm / (2.0 * sqrt(2.0 * log(2.0)));

	result = exp(-((x - xc) * (x - xc)) / (2.0 * sigma * sigma));

	return (A * result) + B;
}

double gaussianFW(double fwhm, double xc, double A, double B, double x) {
	
	double result = 0.0, sigma;
	
	sigma = fwhm / (2.0 * sqrt(2.0 * log(2.0)));

	result = exp(-((x - xc) * (x - xc)) / (2.0 * sigma * sigma));

	// Normalization
	result /= sqrt(2.0 * M_PI) * sigma;

	return (A * result) + B;
}

double gaussianSig(double fwhm, double xc, double A, double B, double x) {
	
	double result = 0.0, sigma;
	
	sigma = fwhm;

	result = exp(-((x - xc) * (x - xc)) / (2.0 * sigma * sigma));

	// Normalization
	result /= sqrt(2.0 * M_PI) * sigma;

	

	return (A * result) + B;
}

double lorentzian(double fwhm, double xc, double A, double B, double x) {
	double result = 0.0, hwhm = fwhm / 2.0;

	result = hwhm;

	result /= ((x - xc) * (x - xc) + (hwhm * hwhm));

	// Normalization
	result /= M_PI;

	return (A * result) + B;
}

double lorentzian_squared(double fwhm, double xc, double A, double B, double x) {
	double result = 0.0, hwhm = fwhm / 2.0;

	result = 1.0;

	result /= ((x - xc) * (x - xc) + (hwhm * hwhm));

	result *= result;

	// Normalization
	result /= (4.0 * M_PI / (sq(fwhm) * fwhm));

	return (A * result) + B;
}

double Caille_peak(double fwhm, double xc, double A, double B, double x) {
		return A * (pow(fabs(x - xc),(-1 + fwhm)))+ B;
		/**
		**/
}

double exponentDecay(double x, double base, double decay, double xcenter) {
	return base*exp(-(x - xcenter) / decay); 
}

double linearFunction(double x, double base, double decay, double xcenter) {
	return (-base * (x) + decay);
}

double powerFunction(double x, double base, double decay, double xcenter) {
	if(fabs(x - xcenter) <= 1e-6) return 0.0;
	return (base * pow((x - xcenter), (-decay)));
}

inline double vmax(const std::vector<double>& v) {
	if(v.size() == 0)
		return 0.0;
	double val = v.at(0);
	for(unsigned int i = 0; i < v.size(); i++)
		if(v[i] > val)
			val = v[i];
	return val;
}

EXPORTED double WSSR(std::vector<double> first, std::vector<double> second) {
	std::vector<bool> mask;
	mask.resize(first.size(), false);
	return WSSR_Masked(first, second, mask);
}

EXPORTED double WSSR_Masked(std::vector<double> first, std::vector<double> second, const std::vector<bool>& masked) {
	double result = 0.0;

	if(first.size() != second.size())
		return -1;

	// Normalizing both functions
	/*double maxval = vmax(first);
	for(unfwhmned int i = 0; i < first.size(); i++) {
		first[i] /= maxval;
		second[i] /= maxval;
	}*/

	// Poisson statistics for fwhmma^2 (weights): fwhmma(x) = sqrt(f(x)) + 1

	// Actual WSSR calculation ( (f(x) - g(x) / fwhmma(x)) ^ 2
	for(unsigned int i = 0; i < first.size(); i++)
		if(!masked.at(i))
			if(!isLogFitting()) 
				result += sq(first.at(i) - second.at(i)) / sq(sqrt(first.at(i)) + 1.0);
			else 
				result += sq(log10(first.at(i)) - log10(second.at(i))) / sq(sqrt(fabs(log10(first.at(i)))) + 1.0);
	return result;
}

double Mean(std::vector<double> data) {
	double result = 0.0;

	for(int i = 0; i < (int)data.size(); i++)
		result += data[i];

	return (result / data.size());
}

EXPORTED double RSquared(std::vector<double> data, std::vector<double> fit) {
	std::vector<bool> mask;
	mask.resize(data.size(), false);
	return RSquared_Masked(data, fit, mask);
}

EXPORTED double RSquared_Masked(std::vector<double> data, std::vector<double> fit, const std::vector<bool>& masked) {
	double mean, sstot = 0.0, sserr = 0.0; // need to move mask to mean
	
	if(data.size() != fit.size())
		return -1;

	if(isLogFitting()) {
		for (unsigned int i = 0; i < data.size(); i++) {
			data[i] = log10(data[i]);
			fit[i] = log10(fit[i]);
		}
	}

	mean = Mean(data);

	for(int i = 0; i < (int)data.size(); i++)
		if(!masked[i])
			sstot += (data[i] - mean) * (data[i] - mean);

	for(int i = 0; i < (int)fit.size(); i++)
		if(!masked[i])
			sserr += (data[i] - fit[i]) * (data[i] - fit[i]);

	return 1.0 - (sserr / sstot);
}
double cailleZhang(double D, double eta, double amp, double q, double h, double Na, double sigma, double N_Diff) {
		/* Constants:	D - d spacing
		   Variables:	amp - amplitude
						q - take a guess
						eta - take another guess
						N - N0 from Zhang
						h - I have no idea
						sigma - the measure of polydispersity of N0
		*/

	double tot = 0.0, wtTot = 0.0;
	double N0 = Na;
	//Parameters:
	//double D, eta, amp, q;
	//int h, N;

#pragma omp parallel for reduction( + : tot, wtTot)
	for(int N = int(max(0.0, N0 - 3.0 * sigma)); N <= int(N0 + 3.0 * sigma); N++) {
		double tmp = 0.0;

		if(pStop && *pStop)
			continue;

		for(int n1 = 1; n1 < N; n1++) {
			double ex = 0.0;
			for(int n2 = 1; n2 < N; n2++)
				ex += (1.0 - cos((double)n2 * M_PI * (double)n1  / double(N))) / (double)n2;
			ex = exp(-eta * double(h) * double(h) * ex);
			tmp += (1.0 - double(n1)/double(N)) * cos(q * double(n1) * D) * ex;
		}
		double wt = (sigma > 0.0) ? gaussianSig(sigma, N0, 1.0, 0.0, N) : 1.0;
		wtTot += wt;

		tot += amp * (double(N) * wt * (2.0 * tmp + 1.0) / (q * q) - N_Diff);
	}

	return tot / wtTot;
}
double CailleDummy(double fwhm, double xc, double A, double B, double x, double &N_diff) {
	//int h, i = 2;
	
	static double a = 0.0;
	static double fix;
	double temp= 0.0;
//	static int H = 1;
	
	static std::vector <double> res;
	
	double Q = 2.0 * M_PI / GetD();
	//if(x < 0.5 * Q)
	//	return 0.0;

	//while(Q < 1.0e300) {
	//	if(fabs(x - double(i) * Q) > fabs(x - double(i - 1) * Q))
	//		break;
	//	i++;
	//}
	//h = i - 1;

	//if(h < H) {
	//	a = 0.0;
	//	H = 1;
	//}

	//if(h - H > 0) {
	//	a += cailleZhang(GetD() ,xc, A, x, h, fwhm, B) - res;
	//	H = h;
	//}
	double h = x / Q  ;
	double mini = std::numeric_limits<double>::infinity();//1e300;
	static int pos; 

	/*
	_D (GetD()) global d-spacing
	xc -> eta
	A -> amp
	x -> q
	h -> locally calculated
	fwhm -> Na
	B -> sigma
	N_diff -> Pabst's N diffused
	*/
	if(pos >= (int)xGlob.size() - 1)
		pos = 0;
	if(bPrecalculate) {
		fix = 1.0e300;	
		pos = 0 ;
		res.resize(xGlob.size());
		
//#pragma omp parallel for
		for (int i = 0; i < (int)res.size(); i++) {
			res[i] = cailleZhang(GetD(), xc, A, xGlob[i], h, fwhm, B, 0.0);
			mini = min (res[i],mini);
		}

		if (mini - A * N_diff < 0.0) {
			N_diff = (mini - 0.0) / A;
		}


		bPrecalculate = false;
		//return res[pos] - A * N_diff;
	}
	
	//if (res < 1.0 )
	//	fix = min(fix,(res - 1.0)/A);
	//if(bLastQ) {
	//	temp = N_diff;
	//	//N_diff = min(N_diff,fix);	
	//}
	//pos++;
	if(x == xGlob[pos])
		return res[pos++] - A * N_diff;// + A * (bLastQ) ? (temp) : (N_diff);// - a;
	else
		for(pos = 0; pos < (int)res.size(); pos++)
			if (fabs(x - xGlob[pos]) < 1.0e-9)
				return res[pos++] - A * N_diff;
	return -1.0;
		
}
EXPORTED std::vector <double> MachineResolution(const std::vector <double> &q, const std::vector <double> &orig, double sig) {
	if(fabs(sig) < 1e-12) return orig;
	int size = orig.size();
	std::vector <double> res(size);
	
	res[0] = orig[0];
	res[size - 1] = orig[size - 1];
	for(int i = 1; i < size - 1; i++) {
		if((fabs(q[i] - q[i+1]) > 3.0 * sig) ){
			res[i] = orig[i];
			continue;
		}

		double val = 0.0;
		double norm = 0.0;
		
		// Worse case scenario values
		int start = 0;
		int end = size - 1;
		double quan1 = max(q[0], q[i] - 3.0 * sig),		// Lower limit
			quan2 = min(q[i] + 3.0 * sig, q[size - 1]);	// Upper limit
		// Find the upper limit in the array
		for (int j = i; j < size; j++) {
			if (q[j] >= quan2) {
				end = j;
				break;
			}
		}
		// Find the lower limit in the array
		for (int j = i; j >= 0; j--) {
			if (q[j] <= quan1) {
				start = j;
				break;
			}
		}

		// Symmetrize the limits 
		if(fabs(fabs( q[i] - q[start]) - fabs( q[i] - q[end])) > 1e-4) {
			
			if(fabs(q[i] - q[start])<fabs( q[i] - q[end]))
				while (( q[i] - q[start]) + ( q[i] - q[end]) > -1e-4) end--;
			else
				while (( q[i] - q[start]) + ( q[i] - q[end]) < 1e-4) start++;
		}
	

		for (int j = start; j <= end; j++) {
			double g = gaussianSig(sig,  q[i],  1.0,  0.0, q[j]);
			val += orig[j] * g;
			norm += g;
		}

		res[i] = val / norm;
	}
	return res;

	/****Avi's version****/
	/****Would work fine if we didn't want to crop the Gaussian symmetrically****/
	//int j;
	//double wt = 0.0, wtot = 0.0;
	//res.resize(size, 0.0);

	//for(int i = 0; i < size; i++) {
	//	wtot = 0.0;

	//	// Find the first point within 3 sigma
	//	for(j = i; j >= 0; j--) {
	//		if(fabs(q[i] - q[j]) < 3.0 * sig) {
	//			j++;
	//			break;
	//		}
	//	}
	//	// Find the last point within 3 sigma
	//	for(end = i; end < size; end++) {
	//		if(fabs(q[i] - q[end]) < 3.0 * sig) {
	//			end--; //?
	//			break;
	//		}
	//	}
	//	end = (end < j) ? end : j;

	//	// Average weighted adjacent points
	//	for(j = i - end; j < i + end; j++) {
	//		if(fabs(q[i] - q[j]) < 3.0 * sig)
	//			break;
	//		wt = gaussianSig(sig, q[i], 1.0, 0.0, q[j]);
	//		wtot += wt;
	//		res[i] += wt * orig[j];
	//	}
	//	res[i] /= wtot;
	//}
	/****End of Avi's version****/
}
