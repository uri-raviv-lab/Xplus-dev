#include "HelicalModels.h"
#include "Quadrature.h" // For SetupIntegral


#include "GLPreview.h" // For Draw functions
#include "mathfuncs.h" // For bessel functions and square

HelicalModel::HelicalModel(std::string st, int extraParams) : FFModel(st, extraParams, 3, 2, -1,
													 EDProfile(NONE))
{}

ExtraParam HelicalModel::GetExtraParameter(int index) {
	switch (index) {
		case 2:
			return ExtraParam("Height", std::numeric_limits<double>::infinity(), true, true);
		case 3:
			return ExtraParam("Helix Radius", 10.0, false, true);
		case 4:
			return ExtraParam("Pitch", 15.0, false, true);
		default:
			return Model::GetExtraParameter(index);	
	}
	return Model::GetExtraParameter(index);		
}

int HelicalModel::GetNumRelatedModels() {
	return 3;
}

std::string HelicalModel::GetRelatedModelName(int index) {
	switch(index) {
	default:
		return Model::GetRelatedModelName(index);
	case 0:
		return "Helix";
	case 1:
		return "Discrete Helix";
	case 2:
		return "Gaussian Discrete Helix";
	}
}

bool HelicalModel::IsLayerBased() {
	return false;
}

Model *HelicalModel::CreateRelatedModel(int index) {
	switch(index) {
		case 0:
			return new HelixModel();
		case 1:
			return new DelixModel();
		case 2:
			return new GaussianDelixModel();
		default:
			return Model::CreateRelatedModel(index);
	}
}

std::string HelicalModel::GetLayerParamName(int index) {
	switch(index) {
		default:
			return Model::GetLayerParamName(index);
		case 0:
			return "Phase";
		case 1:
			return "E.D.";
		case 2:
			return "Cross Section";
	}
}

bool HelicalModel::IsParamApplicable(int layer, int lpindex) {
	if(layer < 0 || lpindex < 0 || lpindex >= nLayerParams)
		return false;
	//The solvent layer has no cross section or phase
	if ((layer == 0) && ((lpindex == 0) || (lpindex == 2)))
		return false;
	//The first helix's phase is 0 and not mutable
	if ((layer == 1) && (lpindex == 0))
		return false;
	return true;
}

std::string HelicalModel::GetLayerName(int layer) {
	if(layer < 0)
		return "N/A";

	if(layer == 0)
		return "Solvent";

	std::stringstream ss;

	ss << "Helix " << layer;
	return ss.str();
}

double HelicalModel::GetDefaultParamValue(int paramIndex, int layer) {
	switch(paramIndex) {
		default:
		case 0:
			// Phase
			if(layer <= 1)
				return 0.0;

			return 1.0;

		case 1:			
			// Electron Density
			if(layer == 0)
				return 333.0;

			return 400.0;
		case 2:
			// Cross Section
			if(layer == 0)
				return 0.0;

			return 3.0;
	}
}

void HelicalModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	Model::OrganizeParameters(p, nLayers);
	(*parameters)(0,0) = 0.0;	//Solvent phase
	(*parameters)(0,2) = 0.0;	//Solvent cross section
	rHelix = (*extraParams)[3];
	P =  (*extraParams)[4];
	edSolvent = (*parameters)(0,1);
	delta = (*parameters).col(0);
	deltaED = (*parameters).col(1);
	rCs = (*parameters).col(2);

}


HelixModel::HelixModel(int integralStepsIn,int integralStepsOut) : HelicalModel("Helix"),
						steps(integralStepsOut), steps1(integralStepsIn) {}

void HelixModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	HelicalModel::OrganizeParameters(p, nLayers);
	height = (*extraParams)[2];
}

void HelixModel::DrawOpenGLPreview(const paramStruct& p) {
	// Get the electron density of the first helix and display that
	if(p.params[1].size() > 1)
		DrawGLHelix(p.params[1][1].value);
}

void HelixModel::DrawPreviewScene() {
	DrawGLHelix(-1.0);
}

bool HelixModel::IsSlow() {
	return (_finite(height) == 0);
}


void HelixModel::PreCalculate(VectorXd& p, int nLayers){
	OrganizeParameters(p, nLayers);
	steps	= int(8.0 * (rHelix + P));		//inner
	steps1	= int(max(1, 15.0 * height));	//outer
	
	if(_finite(height)) {
#pragma omp parallel sections
		{			
#pragma omp section
			{
				SetupIntegral(xIn, wIn, 0.00001, 2.0 * PI + 0.00001, steps);	// x = theta_r
			}
#pragma omp section
			{
				subSteps = int(sqrt((double)steps1));
				SetupIntegral(xOut, wOut, -1.0, 1.0 , steps1); // Orientational Average: x = cos(theta_q)
			}
		}
	}
}

void HelixModel::PreCalculateFF(VectorXd& p, int nLayers){
	OrganizeParameters(p,nLayers);
	steps = int(8.0 * (rHelix + P));	//inner
	#pragma omp parallel sections
		{
			#pragma omp section
					SetupIntegral(xIn, wIn, 0.00001, 2.0 * PI + 0.00001, steps);	// x = theta_r
		}
}

double HelixModel::Calculate(double q, int nLayers, Eigen::VectorXd &a) {
	//ma - size of a
	//nLayers - number of helices
	//a[1]...a[nLayers - 1] - Delta from Helix 1
	//a[nLayers] - solvent ED
	//a[nLayers + 1]...a[2*nLayers - 1] - Helix ED
	//a[2*nLayers + 1]...a[3*nLayers - 1] - Helix Cross section
	// Extra params (in correct order): (0)scale, (1)background, (2)height, (3)R, (4)P
	if(a.size() > 0)
		OrganizeParameters(a, nLayers);

	VectorXd bess1 = VectorXd::Zero(nLayers);
	
	int hMax;
	if(delta[0] < 0.0) 
		delta[0] = 0.0;

	
//	static VectorXd xIn, wIn, xOut, wOut;
//	static int subSteps;
//	int steps, steps1;

	int N = int(height / P);
		
	hMax = int(floor(q * P / (2.0 * PI)));
	
	double intensity = 0.0;
		//Model from 02/02/2010 - Convolution of a thin helix and a disc
	if(!_finite(height)) {	// Infinite model
#pragma omp parallel for reduction(+ : intensity)
		for(int m = 0; m <= hMax; m++) {
			double  root = sqrt(1.0 - sq(2.0 * PI * double(m) / (q * P)));
			std::complex <double> sum(0.0,0.0), i(0.0,1.0);
			for(int j = 1; j < nLayers; j++) {
 					sum += rCs[j] * (bessel_j1(q * rCs[j] * root) / root) * (deltaED[j] - edSolvent) 
						* exp(i * 2.0 * PI * double(m) * delta[j] / P);
			}
			sum *= bessel_jn(m, q * rHelix * root) ;
			intensity += norm(sum);
		}
		intensity *= 8.0 * PI * sq(PI)/ (sq(q) * q);
	}
	else {	// Finite model
		if(height < 1.0e-12)
			return (*extraParams)[1];
		std::complex <double>  i(0.0,1.0);
		
#pragma omp parallel for reduction(+ : intensity)
		for(int outest = 0; outest <= steps1 / subSteps ; outest++) {
			double subIntensity = 0.0;
			for (int ou = outest * subSteps; ou < (subSteps * outest) + subSteps; ou++) {
				if(!(ou < steps1)) continue;
				std::complex <double> sst(0.0,0.0),st(0.0,0.0);
				double outroot = sqrt(1.0 - sq(xOut[ou]));
				for (int s = 1; s < nLayers; s++) 
					st += (deltaED[s] - edSolvent)* exp(i * q * xOut[ou] * delta[s]) 
						* bessel_j1(q * rCs[s] * outroot) * rCs[s] / outroot ;	
				for (int in = 0; in < steps; in++) 
					sst += wIn[in] * exp(i * q * rHelix * outroot * cos(xIn[in]) 
						+ i * q * xOut[ou] * P * xIn[in] / (2.0 * PI)  );
				subIntensity += wOut[ou] * norm((fabs(xOut[ou]) < 1.0e-20) ? (N) : (sin(N * q * xOut[ou] * P / 2.0) / sin( q * xOut[ou] * P / 2.0)) * sst * st);
			}
			intensity += subIntensity;
		}
		intensity *= 2.0 * PI * sq(P) / sq (q);
	
	}

	/* End of 02/02/2010*/

	intensity *= (*extraParams)[0];   // Multiply by scale
	intensity += (*extraParams)[1];  // Add background
	
	return intensity;
}

std::complex<double> HelixModel::CalculateFF(Vector3d qvec, int nLayers, double w, double precision, VectorXd& a) {
	//ma - size of a
	//nLayers - number of helices
	//a[1]...a[nLayers - 1] - Delta from Helix 1
	//a[nLayers] - solvent ED
	//a[nLayers + 1]...a[2*nLayers - 1] - Helix ED
	//a[2*nLayers + 1]...a[3*nLayers - 1] - Helix Cross section
	// Extra params (in correct order): (0)scale, (1)background, (2)height, (3)R, (4)P
	
	int ma = a.size();
	double height = a[ma-nExtraParams+2];
	double rHelix, P, edSolvent; 
	double *delta = &a[1];
	double *deltaED = &a[nLayers+1];
	double *rCs = &a[nLayers+nLayers+1];
	VectorXd bess1 = VectorXd::Zero(nLayers);
	
	int hMax;
	if(delta[0] < 0.0) 
		delta[0] = 0.0;

	edSolvent = a[nLayers];
	rHelix = a[ma-nExtraParams+3];
	P =  a[ma-nExtraParams+4];
	int N = int(height / P);
		
	hMax = int(floor(qvec[2] * P / (2.0 * PI)));
	double qperp = sqrt(qvec[0] * qvec[0] + qvec[1] * qvec[1]);
	double qphi = atan2(qvec[1],qvec[0]);
	
	// Amplitude real and complex parts
	double ampr = 0.0, ampim = 0.0;

	if(height < 0.0) {	// Infinite model
#pragma omp parallel for reduction(+ : ampr, ampim)
		for(int m = 0; m <= hMax; m++) {
			//Dirac Delta Function condition
			if(fabs(qvec[2] - 2.0 * PI * double(m) / P ) < precision) {
				std::complex <double> cof(0.0,0.0), sum(0.0,0.0), i(0.0,1.0);
				//double  root = sqrt(1.0 - sq(2.0 * PI * double(m) / (q * P)));
				cof = 4.0 * PI * PI * exp(-i * double(m) * qphi) * bessel_jn(m, qperp * rHelix) / qperp ;
				if (m % 4 == 0)
					cof *= 1;
				else if (m % 4 == 1)
					cof *= -i;
				else if (m % 4 == 2)
					cof *= -1;
				else 
					cof *= i;
				for(int j = 1; j < nLayers - 1; j++) {
 						//sum += rCs[j] * (bessel_j1(q * rCs[j] * root) / root) * (deltaED[j] - edSolvent) 
						//	* exp(i * 2.0 * PI * double(m) * delta[j] / P);
					sum += (deltaED[j] - edSolvent) * rCs[j] * exp(i * (double)m * delta[j]) * bessel_j1(qperp * rCs[j]);
					sum *= cof;
				}
				ampr += sum.real();
				ampim += sum.imag();
			}
		}	

		return std::complex<double>(ampr, ampim) / w;
	
	
	}
	else {	// Finite model
		if(height < 1.0e-12)
			return a[ma-nExtraParams+1];
		std::complex<double> i(0.0,1.0);
		std::complex<double> pre(0.0,0.0), amplitude(0.0,0.0);

		double st_real = 0.0, st_imag = 0.0, sst_real = 0.0, sst_imag = 0.0;


#pragma omp parallel for reduction(+ : st_real, st_imag)	
		for (int s = 1; s < nLayers; s++) {
			double coef = bessel_j1(qperp * rCs[s]) * rCs[s] * (deltaED[s] - edSolvent)* P;
			double innerExp = qvec[2] * P * delta[s] / (2 * PI) - qvec[2] * P * qphi / (2 * PI);
			st_real += coef * cos(innerExp);
			st_imag += coef * sin(innerExp);
		}

		pre =  sin(qvec[2] * P * (N + 0.5)) / (qperp * sin(qvec[2] * P /2)) ;
#pragma omp parallel for reduction(+ : sst_real, sst_imag)
		for (int in = 0; in < steps; in++) {
			// TODO: Modify to two "+=" lines
			// i.e. sst_real += ...; sst_imag += ...;
			std::complex <double> sst (0.0,0.0);
			sst = wIn[in] * exp(-i * qperp * rHelix * cos(xIn[in]) - i * qvec[2] * P * xIn[in] / (2.0 * PI)  );
			sst_real += sst.real();
			sst_imag += sst.imag();
		}
	
		amplitude = pre * std::complex<double>(st_real, st_imag) * std::complex<double>(sst_real, sst_imag);	
	
		return amplitude;
	}
}

VectorXd HelixModel::Derivative(const std::vector<double>& x, VectorXd param, 
										int nLayers, int ai) {
	int nParams = (int)param.size();
	// Finite helix is numeric
	if(height >= 0.0)
		return Model::Derivative(x, param, nLayers, ai);
	OrganizeParameters(param, nLayers);
	VectorXd Der(x.size());
	
	if ((ai > 0) && (ai < nLayers)) {//Partial Phase Derivation
#pragma omp parallel for
		for (int c = 0; c < int(x.size()); c++) {
			std::complex<double> sum(0.0, 0.0), sum1(0.0, 0.0), i(0.0,1.0);
			double sq = 0;
			int hMax = int(floor(x[c] * P / (2.0 * PI)));
			for (int i = 0; i < (nLayers-1); i++) {
				for (int m = 0; m < hMax ; m++) {
					sq = (1 - (2 * PI * m / (P * x[c])) * (2 * PI * m / (P * x[c])));
					sum1 += (1 /sq) * (bessel_jn(m, x[c] * rHelix * sqrt(sq))) * (bessel_jn(m, x[c] 
						* rHelix * sqrt(sq))) * 2 * m * sin(m * (delta[ai - 1]-delta[i]))
							* bessel_j1(x[c] * rCs[ai - 1] * sqrt(sq)) * bessel_j1(x[c] * rCs[i] * sqrt(sq));
				}
			sum += rCs[i] * rCs[ai - 1] * (deltaED[i] - edSolvent) * (deltaED[ai - 1] - edSolvent) * sum1;
			}
			Der[c] = 32 * PI * PI * PI * PI * PI / (x[c] * x[c] * x[c]) * norm(sum);
		}
		return ((*extraParams)[0] * Der);
	}
	else if  ((ai > nLayers) && (ai < (2 * nLayers))) {//Partial E.D. Derivation
#pragma omp parallel for
		for (int c = 0; c < int(x.size()); c++) {
			std::complex<double> sum(0.0, 0.0), sum1(0.0, 0.0), i(0.0,1.0);
			double sq = 0;
			int hMax = int(floor(x[c] * P / (2.0 * PI)));
			for (int i = 0; i < (nLayers-1); i++) {
				for (int m=0; m < hMax ; m++) {
					sq = (1 - (2 * PI * m / (P * x[c])) * (2 * PI * m / (P * x[c])));
					sum1 += (1 /sq) * (bessel_jn(m, x[c] * rHelix * sqrt(sq))) * (bessel_jn(m, x[c] 
						* rHelix * sqrt(sq))) * 2 * cos(m * (delta[ai - nLayers - 2]-delta[i]))
							* bessel_j1(x[c] * rCs[ai - nLayers - 2] * sqrt(sq)) * bessel_j1(x[c] * rCs[i] * sqrt(sq));
				}
				sum += rCs[i] * rCs[ai - nLayers - 2] * (deltaED[i] - edSolvent) * sum1;
			}
			Der[c] = 32 * PI * PI * PI * PI * PI / (x[c] * x[c] * x[c]) * norm(sum);
		}
		return ((*extraParams)[0] * Der);
	}
	else if  ((ai > 2 * nLayers) && (ai < (nLayerParams * nLayers))) {//Partial Cross Section Derivation
#pragma omp parallel for
		for (int c = 0; c < int(x.size()); c++) {
			std::complex<double> sum(0.0, 0.0), sum1(0.0, 0.0), i(0.0,1.0);
			double sq = 0;
			int hMax = int(floor(x[c] * P / (2.0 * PI)));
			for (int i = 0; i < (nLayers-1); i++) {
				for (int m=0; m < hMax ; m++) {
					sq = (1 - (2 * PI * m / (P * x[c])) * (2 * PI * m / (P * x[c])));
					sum1 += (1 /sqrt(sq)) * (bessel_jn(m, x[c] * rHelix * sqrt(sq))) * (bessel_jn(m, x[c] 
						* rHelix * sqrt(sq))) * 2 * cos(m * (delta[ai - 2 * nLayers - 3]-delta[i]))
							* x[c] * (bessel_j0(x[c] * rCs[ai - 2 * nLayers - 3] * sqrt(sq)) - bessel_jn(2, x[c] * rCs[ai - 2 * nLayers - 3])) * bessel_j1(x[c] * rCs[i] * sqrt(sq));
				}
				sum += rCs[i] * (deltaED[i] - edSolvent) * (deltaED[ai - 2 * nLayers - 3] - edSolvent) * sum1;
			}
			Der[c] = 32 * PI * PI * PI * PI * PI / (x[c] * x[c] * x[c]) * norm(sum);
		}
		return ((*extraParams)[0] * Der);
	}
	else if (ai == (nParams - nExtraParams) + 3) {//Partial Helix Radius Derivative
#pragma omp parallel for
		for (int c = 0; c < int(x.size()); c++) {
			std::complex<double> sum(0.0, 0.0), sum1(0.0, 0.0), i(0.0,1.0);
			double sq = 0;
			int hMax = int(floor(x[c] * P / (2.0 * PI)));
			for (int i = 0; i < (nLayers-1); i++) {
				for (int j = 0; j < (nLayers-1); j++) {
				for (int m=0; m < hMax ; m++) {
					sq = (1 - (2 * PI * m / (P * x[c])) * (2 * PI * m / (P * x[c])));
					sum1 += (1 / sqrt(sq)) * (bessel_jn(m, x[c] * rHelix * sqrt(sq))) * (bessel_jn(m - 1, x[c] 
						* rHelix * sqrt(sq)) - bessel_jn(m + 1, x[c] * rHelix * sqrt(sq))) * x[c]
							* exp(i * m * (delta[j]-delta[i]))* bessel_j1(x[c] * rCs[j] * sqrt(sq)) * bessel_j1(x[c] * rCs[i] * sqrt(sq));
				}
				sum += rCs[j] * rCs[i] * (deltaED[i] - edSolvent) * (deltaED[j] - edSolvent) * sum1;
				}
			}
			Der[c] = 32 * PI * PI * PI * PI * PI / (x[c] * x[c] * x[c]) * norm(sum);
		}
		return ((*extraParams)[0] * Der);
	}
	else if (ai == (nParams - nExtraParams) + 4) {//Partial Pitch Derivative		
#pragma omp parallel for
		for (int c = 0; c < int(x.size()); c++) {
			std::complex<double> sum(0.0, 0.0), sum1(0.0, 0.0), i(0.0,1.0);
			double bi1, bj1, bRm, bmin1mR, bplus1mR, bi0, bi2, bj0, bj2;
			double sq = 0;
			int hMax = int(floor(x[c] * P / (2.0 * PI)));
			for (int i = 0; i < (nLayers-1); i++) {
				for (int j = 0; j < (nLayers-1); j++) {
				for (int m=0; m < hMax ; m++) {
					sq = (1 - (2 * PI * m / (P * x[c])) * (2 * PI * m / (P * x[c])));
					bRm = bessel_jn(m, x[c] * rHelix * sqrt(sq));
					bi1 = bessel_j1(x[c] * rCs[i] * sqrt(sq));
					bj1 = bessel_j1(x[c] * rCs[j] * sqrt(sq));
					bi0 = bessel_j0(x[c] * rCs[i] * sqrt(sq));
					bj0 = bessel_j0(x[c] * rCs[j] * sqrt(sq));
					bi2 = bessel_jn(2, x[c] * rCs[i] * sqrt(sq));
					bj2 = bessel_jn(2, x[c] * rCs[j] * sqrt(sq));
					bmin1mR = bessel_jn(m - 1, x[c] * rHelix * sqrt(sq));
					bplus1mR = bessel_jn(m + 1, x[c] * rHelix * sqrt(sq));
					sum1 += - (1 / (sq * sq * P * P * P * x[c] * x[c])) * 8 * exp(i * m * (delta[i]-delta[j])) * m * m * PI * PI * bi1 * bj1 * bRm * bRm
						+ (1 / (sq * sqrt(sq) * P * P * P * x[c])) * 4 * exp(i * m * (delta[i]-delta[j])) * m * m * PI * PI * rHelix * bi1 * bj1 * bRm * (bmin1mR - bplus1mR)
						+ (1 / (sq * sqrt(sq) * P * P * P * x[c])) * 2 * exp(i * m * (delta[i]-delta[j])) * m * m * PI * PI * rCs[i] * bj1 * bRm * bRm * (bi0 - bi2)
						+ (1 / (sq * sqrt(sq) * P * P * P * x[c])) * 2 * exp(i * m * (delta[i]-delta[j])) * m * m * PI * PI * rCs[j] * bi1 * bRm * bRm * (bj0 - bj2);
				}
				sum += rCs[j] * rCs[i] * (deltaED[i] - edSolvent) * (deltaED[j] - edSolvent) * sum1;
				}
			}
			Der[c] = 32 * PI * PI * PI * PI * PI / (x[c] * x[c] * x[c]) * norm(sum);
		}
		return ((*extraParams)[0] * Der);
	}
	else //Default Numerical Derivation
		return Model::Derivative(x, param, nLayers, ai);
}

DelixModel::DelixModel(std::string st, int step) : HelicalModel(st, 7), steps(step) {}

double DelixModel::Calculate(double q, int nLayers, Eigen::VectorXd& p) {
	if(p.size() > 0)
		OrganizeParameters(p, nLayers);

	double intensity = 0.0;

	VectorXd ball = VectorXd::Zero(nLayers);

	for(int i = 0; i < nLayers; i++)
		ball[i] = (deltaED[i] - edSolvent) * (sin(rCs[i] * q) - rCs[i] * q * cos(rCs[i] * q ));

	// Infinite beta version
//	double sum = 0.0;
//#pragma omp parallel for reduction(+ : sum)
//	for(int i = 1; i < nLayers; i++) {
//		for(int j = 0; j < steps; j++) {
//			sum += (cos(P * q * (2.0 * rCs[i] + deltaw) * root[j] / (4.0 * PI * rHelix)) / sin(P * q * (2.0 * rCs[i] + deltaw) * root[j] / (4.0 * PI * rHelix))
//					- cos((2.0 * rCs[i] + deltaw) * (4.0 * sq(PI) + P * q * root[j]) / (4.0 * PI * rHelix)) / sin((2.0 * rCs[i] + deltaw) * (4.0 * sq(PI) + P * q * root[j]) / (4.0 * PI * rHelix))) * 
//					sin(x[j]) / (2.0 * rCs[i] + deltaw) * w[j];
//			//sum += 0.25 * ball[i] / sq(sin(P * q * (2.0 * rCs[i] + deltaw) * cos(x[j]) / (4.0 * PI * rHelix))) * w[j];
//		}
//	}
//	sum *= rHelix / (16.0 * PI * sq(PI));
//	intensity = sum;
//	intensity *= (32.0 * sq(PI) / sq(q * sq(q))); 
//
//	intensity *= exp(-sq(q * debyeWaller) / 2.0);   // Debye Waller factor
//	intensity *= (*extraParams)[0];   // Multiply by scale
//	intensity += (*extraParams)[1];  // Add background
//	return intensity;


	// Simpler version 

	// Tried and true version
	for (int i = 0; i < nLayers; i++) {
		for (int j = 0; j < nLayers; j++) {
			double sum1 = 0.0;
#pragma omp parallel for reduction(+ : sum1)
			for(int n = 0; n < Nb; n++) {
				
				if(pStop && *pStop)		// Place these lines strategically in
					continue;			// slow models.

				for (int m = 0; m < Nb; m++) {
					double nimj = n * deltaz[i] - m * deltaz[j];
					double sinG = sin((PI / P) * (nimj + (delta[i] - delta[j])));
					for (int s = 0; s < steps; s++) {
						sum1 += w[s] * bessel_j0(2.0 * q * root[s] * rHelix * sinG) *
							cos(q * x[s] * nimj);
					}
				}
			}
			intensity += sum1 * ball[i] * ball[j]; 
		}
	}
	intensity *= (32.0 * sq(PI) / sq(q * sq(q))); 

	intensity *= exp(-sq(q * debyeWaller) / 2.0);   // Debye Waller factor
	intensity *= (*extraParams)[0];   // Multiply by scale
	intensity += (*extraParams)[1];  // Add background
	return intensity;
}

void DelixModel::PreCalculate(VectorXd& p, int nLayers) {
	OrganizeParameters(p, nLayers);

	/************ Tried and true version*/
	SetupIntegral(x, w, 0.0, 1.0, steps);
	root.resize(steps);
	for(int s = 0; s < steps; s++)
		root[s] = sqrt (1.0 - sq(x[s]));

	deltaz.resize(nLayers);
	for(int i = 0; i < nLayers; i++)
		deltaz[i] = (P * (2.0 * rCs[i] + deltaw) / ( 2.0 * PI * rHelix));
	/************************************/
	/************ Very beta version
	steps = 200;
	SetupIntegral(x, w, EPS, PI + EPS, steps);
	root.resize(steps);
	for(int s = 0; s < steps; s++)
		root[s] = cos(x[s]);
	************************************/

}

void DelixModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	HelicalModel::OrganizeParameters(p, nLayers);

	Nb			= int((*extraParams)[2]);
	rHelix		= (*extraParams)[3];
	P			= (*extraParams)[4];
	deltaw		= (*extraParams)[5];
	debyeWaller	= (*extraParams)[6];
	edSolvent	= deltaED(0);
}

std::complex<double> DelixModel::CalculateFF(Vector3d qvec, 
											 int nLayers, double w, double precision, VectorXd& p) {
    // TODO: Implement
	return std::complex<double>(0.0, 1.0);
}

ExtraParam DelixModel::GetExtraParameter(int index) {
	switch (index) {
		case 2:
			return ExtraParam("Number of Spheres", 100, false, true);
		case 5:
			return ExtraParam("Water Spacing", 0.0, false, true);
		case 6:
			return ExtraParam("Debye Waller", 0.0, false, true);
		default:
			return HelicalModel::GetExtraParameter(index);	
	}
	return Model::GetExtraParameter(index);		
}
	
void DelixModel::DrawOpenGLPreview(const paramStruct& p) {
	// Get the electron density of the first Delix and display that
	if(p.params[1].size() > 1)
		DrawGLHelix(p.params[1][1].value);
	// We can draw a better one: multiple helices; spheres
}

void DelixModel::DrawPreviewScene() {
	DrawGLMicrotubule(0.1f, 15);
}

bool DelixModel::IsSlow() {
	return true;
}
	
void DelixModel::PreCalculateFF(VectorXd& p, int nLayers) {
}

VectorXd DelixModel::Derivative(const std::vector<double>& x, VectorXd param, 
								int nLayers, int ai) {
	// TODO...
	return Model::Derivative(x, param, nLayers, ai);
}

GaussianDelixModel::GaussianDelixModel(std::string st, int step) : DelixModel(st, step){}

double GaussianDelixModel::Calculate(double q, int nLayers, Eigen::VectorXd& p) {
	if(p.size() > 0)
		OrganizeParameters(p, nLayers);

	double intensity = 0.0;

	VectorXd deltaz = VectorXd::Zero(nLayers), ball = VectorXd::Zero(nLayers);

	for (int i = 0; i < nLayers; i++)
		deltaz[i] =  (P * (rCs[i] + deltaw) / ( 2.0 * PI * rHelix));

	for (int i = 0; i < nLayers; i++) {
		ball[i] = (deltaED[i] - edSolvent) * (sq(rCs[i]) * rCs[i] * exp(-sq(rCs[i] * q / 4.0 ) / ln2 ));
	}

	for (int i = 0; i < nLayers; i++) {
		for (int j = 0; j < nLayers; j++) {
			double sum1 = 0.0;
#pragma omp parallel for reduction(+ : sum1)
			for(int n = 0; n < Nb; n++) {
				
				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for (int m = 0; m < Nb; m++) {
					double sinG = sin (PI * (double(n) * deltaz[i] - double(m) * deltaz[j]) / P + (delta[i] - delta[j]) * PI / P);
					for (int s = 0; s < steps; s++) {
						sum1 += w[s] * bessel_j0( 2.0 * q * root[s] * rHelix * sinG) *
							cos(q * x[s] * (double(n) * deltaz[i] - double(m) * deltaz[j]));
					}
				}
			}
			intensity += sum1 * ball[i] * ball[j] ; 
		}
	}
	intensity *= (sqrt(PI/ln2)*(PI/ln2) / 8.0); 

	intensity *= exp(-sq(q * debyeWaller) / 2.0);   // Debye Waller factor
	intensity *= (*extraParams)[0];   // Multiply by scale
	intensity += (*extraParams)[1];  // Add background
	return intensity;
}