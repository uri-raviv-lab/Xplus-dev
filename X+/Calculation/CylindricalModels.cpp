//#define NOMINMAX
//#include "Windows.h"	//For min/max and messagebox debugging


#include "CylindricalModels.h"
#include "Quadrature.h" // For SetupIntegral

#include "GLPreview.h" // For preview OpenGL draw calls

#include "mathfuncs.h" // For bessel functions and square

using Eigen::VectorXf;

#pragma region Abstract Cylindrical Model

CylindricalModel::CylindricalModel(int integralSteps, std::string str, ProfileShape eds, int nlp) : FFModel(str, 3, nlp, 2, -1, EDProfile(SYMMETRIC, eds)) {
	steps = integralSteps;
	SetupIntegral(xx, ww, 0.0f, 1.0f, steps);
}

ExtraParam CylindricalModel::GetExtraParameter(int index) {
	if(index == 2)
		return ExtraParam("Height", std::numeric_limits<double>::infinity(), 
						  true, true);
	
	// Default extra parameters
	return Model::GetExtraParameter(index);	
}

	int CylindricalModel::GetNumRelatedModels() {
		return 2;	// Uniform and Gaussian (change when the tanh is added)
	}

	std::string CylindricalModel::GetRelatedModelName(int index) {
		switch(index) {
			case 0:
				return "Uniform Hollow Cylinder";
			case 1:
				return "Gaussian Hollow Cylinder";
			default:
				return Model::GetRelatedModelName(index);
		}
	}

	Model *CylindricalModel::CreateRelatedModel(int index) {
		switch(index) {
			case 0:
				return new UniformHCModel();
			case 1:
				return new GaussianHCModel();
			default:
				return Model::CreateRelatedModel(index);
		}
	}

void CylindricalModel::DrawOpenGLPreview(const paramStruct& p) {
	VectorXd rad = VectorXd::Zero(p.layers);
	VectorXd ed = VectorXd::Zero(p.layers);
	
	for(int i = 0; i < p.layers; i++) {
		rad[i] = p.params[0][i].value;
		ed[i]  = p.params[1][i].value;
	}

	DrawGLNLayeredHC(rad.data(), 1.0, ed.data(), p.layers, 1.8);
}

void CylindricalModel::DrawPreviewScene() {
	// Draw a default HC
	double rad[2] = { 0.9, 0.5 };
	double ed[2] = { 0.0, 0.0 };
	DrawGLNLayeredHC(rad, 3.0, ed, 2, 0.5);	
}

void CylindricalModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	Model::OrganizeParameters(p, nLayers);
	t	= (*parameters).col(0);
	ed	= (*parameters).col(1);
	edSolvent = ed[0]; 
}

#pragma endregion

#pragma region Uniform Cylindrical Model

UniformHCModel::UniformHCModel(int integralSteps) : CylindricalModel(integralSteps, "Uniform Hollow Cylinder") {
}

void UniformHCModel::PreCalculate(VectorXd& p, int nLayers){
	OrganizeParameters(p, nLayers);
}

void UniformHCModel::OrganizeParameters(const VectorXd& p, int nLayers) {
	CylindricalModel::OrganizeParameters(p, nLayers);

	for(int i = 1; i < t.size(); i++)
		t[i] = t[i - 1] + t[i];
}

double UniformHCModel::Calculate(double q, int nLayers, VectorXd& a) {
	double H = -1.0; 
    double scale, background;
    double intensity = 0.0;
	//int nParams;

	if(a.size() > 0)
		OrganizeParameters(a, nLayers);
	
	scale = (*extraParams)[0];
    background = (*extraParams)[1];
	H = !_finite((*extraParams)[2]) ? -1.0 :
							(*extraParams)[2]/2.0;
    
   // GetRandED(a, r, ed, nLayers);

	/*Pablo & Avi's model -----  */
	int notZero = 0;
	for(notZero = 0; (notZero < nLayers) && (t[notZero] <= 0.0); notZero++);
	if(notZero == nLayers)
		return 0.0;	//No layer thickness

	for(int i = nLayers - 1; i >= 0; i--)
		ed[i] -= ed[0];
	/*Finite*/
	if(H >= 0.0) {
		
#pragma omp parallel for reduction(+ : intensity)
		for(int i = 0; i < steps; i++) {
			if (xx[i] > 0.0 && xx[i] < 1.0) {			
				double temp = 0.0;

				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for(int j = notZero; j < nLayers - 1; j++) {
					temp += (ed[j] - ed[j + 1])* t[j] * bessel_j1(q * sqrt(1.0 - xx[i] * xx[i]) * t[j]); 								
				}
				temp += ed[nLayers - 1] * t[nLayers- 1] * bessel_j1(q * sqrt(1.0 - xx[i] * xx[i]) * t[nLayers - 1]);
				temp *= 4.0 * PI * sin(q * xx[i] * H) / (q * q * xx[i] * sqrt(1.0 - xx[i] * xx[i]));
				intensity += temp * temp * ww[i];
			}
		}
	}
	
	/*Pablo & Avi's model -----  Infinite*/
	else {
		// "pre"-calculate the bessel coefficients [R * J1(qR)]
		VectorXd besselCoeff = VectorXd::Zero(nLayers);
		for(int i = 0; i < nLayers; i++)
			besselCoeff[i] = t[i] * bessel_j1(q * t[i]);

		double temp = 0.0;
		for(int j = notZero; j < nLayers - 1; j++) {
			temp += (ed[j] - ed[j + 1]) * besselCoeff[j]; 								
		}
		temp *= 2.0 * ed[nLayers - 1] * besselCoeff[nLayers - 1];

#pragma omp parallel for reduction(+ : temp)
		for(int i = 0; i < nLayers - 1; i++) {
			for(int j = 0; j < nLayers - 1; j++)
				temp += (ed[i] - ed[i + 1]) * besselCoeff[i] * (ed[j] - ed[j + 1]) * besselCoeff[j];
		}
		intensity = temp + sq(ed[nLayers - 1] * besselCoeff[nLayers - 1]);
		intensity *= 16.0 * sq(sq(PI)) / (q * q * q);
	}
	   
    intensity *= scale;// * 1.0e-9; // the scale is way too low to use
    intensity += background;
 
	return intensity;   
}

std::complex<double> UniformHCModel::CalculateFF(Vector3d qvec, 
										   int nLayers, double w, double precision, VectorXd& p) {
    // TODO: Implement
	return std::complex<double>(0.0, 1.0);
}

#pragma endregion

#pragma region Gaussian Cylindrical Model

GaussianHCModel::GaussianHCModel(int heightSteps, int radiiSteps) : CylindricalModel(heightSteps, "Gaussian Hollow Cylinder", GAUSSIAN, 3), 
																	steps1(radiiSteps) {
}

bool GaussianHCModel::IsSlow() {
	return (_finite(H) == 0);	// Doesn't work. The ! should not be there, but when it's not, it shows the infinite as 
}


void GaussianHCModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	CylindricalModel::OrganizeParameters(p, nLayers);
	H = (*extraParams)[2] / 2.0;

	r = (*parameters).col(2);
}

void GaussianHCModel::PreCalculate(VectorXd& p, int nLayers) {
	OrganizeParameters(p, nLayers);
	
	for(nonzero = 0; (nonzero < nLayers) && (t[nonzero] <= 0.0); nonzero++);

	if(nonzero >= nLayers)
		return;

	xxR = MatrixXd::Zero(nLayers - nonzero, steps1);
	wwR = MatrixXd::Zero(nLayers - nonzero, steps1);

	#pragma omp parallel for
	for(int i = nonzero; i < nLayers; i++) {
		VectorXd x, w;
		double s = min(0.0f, r[i] - 3.0 * t[i]);
		SetupIntegral(x, w, s, r[i] + 3.0 * t[i], steps1);
		xxR.row(i - nonzero) = x;
		wwR.row(i - nonzero) = w;
	}
}

double GaussianHCModel::Calculate(double q, int nLayers, VectorXd& p) {
	double intensity = 0.0;
	
	if(_finite(H)) {	// Finite Model
		double resouter = 0.0;
#pragma omp parallel for reduction(+ : resouter)
		for (int outer = 0; outer < steps; outer++) {
			double ressum = 0.0;
			for (int sum = nonzero; sum < nLayers; sum++) {
				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				int es = sum - nonzero;
				double resinner = 0.0;
				for (int inner = 0; inner < steps1; inner++) {
					resinner += exp(-4.0 * ln2 * sq(xxR(es,inner)-r[sum])/sq(t[sum])) *
						xxR(es,inner)* bessel_j0(q * sqrt(1-sq(xx[outer]))* xxR(es,inner)) * wwR(es,inner); 
				}
				ressum += resinner * (ed[sum] - edSolvent);
			}
			resouter += sq(ressum * sin(q * xx[outer] * H)/ xx[outer] ) * ww[outer];
		}
		intensity = 2.5 * resouter * 32.0 * sq(PI) * PI / sq(q) ; 
		//the 2.5 is a factor to normalize the ed area between this and the discrete model.
	} else {		// Infinite Model
		double ressum = 0.0;
#pragma omp parallel for reduction(+ : ressum)
		for (int sum = nonzero; sum < nLayers; sum++) {
			int es = sum - nonzero;
			double resinner = 0.0;

			if(pStop && *pStop)		// Place these line strategically in
				continue;			// slow models.

#pragma omp parallel for if(nLayers - nonzero < 2) reduction(+ : resinner)
			for (int inner = 0; inner < steps1; inner++ ) {
				resinner += exp(-4.0 * ln2 * sq(xxR(es,inner)-r[sum])/sq(t[sum])) *
					xxR(es,inner) * bessel_j0(q * xxR(es,inner)) * wwR(es,inner); 
			}
			ressum += resinner * (ed[sum] - edSolvent);
		}

		intensity = 2.0 * sq(ressum * sq(PI)) * 64.0 / q; // single integral
	//the 2.0 is a factor to normalize the ed area between this and the discrete model.
	}

	intensity *= (*extraParams)(0);	// Multiply by scale
	intensity += (*extraParams)(1);	// Add background

	return intensity;
}

std::string GaussianHCModel::GetLayerParamName(int index) {
	switch(index) {
		case 0:
			return "Thickness";
		case 2:
			return "Z_0";
		default:
			return CylindricalModel::GetLayerParamName(index);
	}
}

bool GaussianHCModel::IsParamApplicable(int layer, int lpindex) {
	if(layer == 0 && lpindex != 1)
		return false;
	return true;
}

std::complex<double> GaussianHCModel::CalculateFF(Vector3d qvec, 
														  int nLayers, double w, double precision, VectorXd& p) {
    // TODO: Implement
	return std::complex<double>(0.0, 1.0);
}

#pragma endregion

#pragma region Squished Tubes

Cylindroid::Cylindroid(int integralSteps, std::string str, ProfileShape eds, int ) : FFModel(str, 4, 2, 2, -1, EDProfile(SYMMETRIC, eds)),
		steps1(integralSteps) {}

ExtraParam Cylindroid::GetExtraParameter(int index) {
	switch(index) {
		default:
			return Model::GetExtraParameter(index);
		case 2:
			return ExtraParam("Height", std::numeric_limits<double>::infinity(), true, true);
		case 3:
			return ExtraParam("Short inner radius", 1.0, false, true);
	}
}

void Cylindroid::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	Model::OrganizeParameters(p, nLayers);
	r	= (*parameters).col(0);
	ed	= (*parameters).col(1);
	edSolvent = ed[0];
	b1 = (*extraParams)(3);
	h = (*extraParams)(2);

	for(int i = 1; i < r.size(); i++)
		r[i] += r[i - 1];
}

void Cylindroid::PreCalculate(Eigen::VectorXd &p, int nLayers) {
	OrganizeParameters(p, nLayers);

	b = VectorXd::Zero(nLayers);
	eps = VectorXd::Zero(nLayers);
	
	for(nonzero = 0; (nonzero < nLayers) && (r[nonzero] <= 0.0); nonzero++);

	if(nonzero >= nLayers)
		return;

	bSwitch = r[nonzero] < (*extraParams)(3);
	

	for(int i = nLayers - 1; i >= 0; i--) {
		ed[i] -= edSolvent;
		if (!bSwitch)
			eps[i] = sqrt(1.0 - (b1 * b1) / (r[nonzero] * r[nonzero]));
		else
			eps[i] = sqrt(1.0 - (r[nonzero] * r[nonzero]) / (b1 * b1));
	}

	for(int i = 0; i < nLayers; i++) {
		if(!bSwitch)
			b[i] = r[i] * sqrt(1.0 - eps[i] * eps[i]);
		else {
			b[i] = r[i] / sqrt(1.0 - eps[i] * eps[i]);
			std::swap(b[i], r[i]);
		}
	}
	
	steps = _finite(h) ? 200 : 100;

	steps1  = 1 + int(sqrt(1.0 - sq(b1 / r[nonzero])) * 1000.0);		
#pragma omp parallel sections 
		{
#pragma omp section
			{
				SetupIntegral(xIn, wIn, std::numeric_limits<double>::epsilon(), 2.0 * PI + std::numeric_limits<double>::epsilon(), steps);	// x = theta_r

				cosInner = VectorXd::Zero(steps);
				for(int p = 0; p < steps; p++)
					cosInner[p] = cos(xIn[p]);
			}
#pragma omp section
			{
				if(_finite(h)) {
					thetaSteps = 10 * int(h);
					SetupIntegral(xOut, wOut, 0.0, 2.0 * PI, steps); // Orientational Average: phiQ
					SetupIntegral(xThetaQ, wThetaQ, std::numeric_limits<double>::epsilon(), 1.0 - std::numeric_limits<double>::epsilon(), thetaSteps); // Orientational Average: x = cos(theta_q)
				}
				else
					SetupIntegral(xOut, wOut, 0.0, 2.0 * PI, steps1); // test
			}
		}

}

int Cylindroid::GetNumRelatedModels() {
	return 2;
}

std::string Cylindroid::GetRelatedModelName(int index) {
	switch(index) {
		case 0:
			return "Conservered Eccentricity Cylindroid";
		case 1:
			return "Non-conservered Eccentricity Cylindroid";
		default:
			return Model::GetRelatedModelName(index);
	}
}

Model *Cylindroid::CreateRelatedModel(int index) {
	switch(index) {
		case 0:
			return new Cylindroid();
		case 1:
			return new CylindroidVaryingEcc(); // TODO: Nonconserved Eccentricity
		default:
			return Model::CreateRelatedModel(index);
	}
}

void Cylindroid::DrawOpenGLPreview(const paramStruct &p) {
	VectorXd rad = VectorXd::Zero(p.layers);
	VectorXd ed = VectorXd::Zero(p.layers);
	
	for(int i = 0; i < p.layers; i++) {
		rad[i] = p.params[0][i].value;
		ed[i]  = p.params[1][i].value;
	}

	DrawGLNLayeredCylindroid(rad.data(), 1.0, ed.data(), p.layers, 1.8);
}

void Cylindroid::DrawPreviewScene() {
	DrawGLCylindroid(0.9f, 0.5f, 3.0f);
}

double Cylindroid::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
	double intensity = 0.0;

	if(_finite(h)) {	// Finite model
		int subSteps = int(2.0 * sqrt((double)steps));
		double root, rootX, sinValue, qc, res = 0.0;
		std::complex<double> im(0.0, 1.0);
		
#pragma omp parallel for reduction(+ : intensity)
		for(int dTheta = 0; dTheta < thetaSteps; dTheta++) {
			std::complex<double> innerRes(0.0, 0.0);
			rootX = sqrt(1.0 - sq(xThetaQ[dTheta]));	// Can move to PreCalculate
			sinValue = sq(sin(q * xThetaQ[dTheta] * h) / xThetaQ[dTheta]) / rootX;
			for(int phiQc = 0; phiQc < steps; phiQc++) {
				std::complex<double> tmp(0.0, 0.0);

				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for(int inner = 0; inner < steps; inner++) {
					qc = q * cos(xIn[inner] - xOut[phiQc]);
					for(int i = 1; i < nLayers - 1; i++) {
						root = sqrt(1.0 - sq(eps[i] * cosInner[inner]));	// Can move to PreCalculate
						tmp += (ed[i] - ed[i + 1]) * ( (b[i] / ( -im * qc * rootX * root)) + (1.0 / sq(qc * rootX)) )
									* exp(-im * b[i] * qc * rootX / root);
					}
					tmp += ed[nLayers - 1] * ( (b[nLayers - 1] / (-im * qc * rootX * root) ) + (1.0 / sq(qc * rootX)) )
								* exp(-im * b[nLayers - 1] * qc * rootX / root);
					tmp -= ed[nonzero] / sq(qc * rootX);
					innerRes += tmp * wIn[inner];
				}	//inner
				res += norm(innerRes * wOut[phiQc]);
			}	//phiQc
			intensity += res * sinValue * wThetaQ[dTheta];
		}	//dTheta

	} else {	// Infinite model
		std::complex<double> temp (0.0, 0.0), im (0.0, 1.0);
			double  perp = q; // q
			
			int subSteps = int(2.0 * sqrt((double)steps));

#pragma omp parallel for reduction(+ : intensity)
			for(int outest = 0; outest <= steps / subSteps; outest++) {
				double subIntensity = 0.0;
				for(int tester = outest * subSteps; tester < subSteps * (outest + 1); tester++) { // test
					double tempIm = 0.0, tempRe = 0.0;
					if(!(tester < steps1))
						continue;

					if(pStop && *pStop)		// Place these line strategically in
						continue;			// slow models.

					for(int inner = 0; inner < steps; inner++) {
						std::complex <double> res(0.0,0.0);
						double root = 0.0;
						double qc = perp * cos(xIn[inner] - xOut[tester]);	// q*cos(phi_r)
						for(int i = nonzero; i < nLayers - 1; i++) {
							root = sqrt(1.0 - sq(eps[i] * cosInner[inner]/*cos(xIn[inner])*/));	// Can move to PreCalculate
							res += (ed[i] - ed[i + 1]) * ( (b[i] / ( -im * qc * root)) + (1.0 / sq(qc)) )
									* exp(-im * b[i] * qc / root);
						}

						root = sqrt(1.0 - sq(eps[nLayers - 1] * cosInner[inner]/*cos(xIn[inner])*/));	// Can move to PreCalculate
						res += ed[nLayers - 1] * ( (b[nLayers - 1] / (-im * qc * root) ) + (1.0 / sq(qc)) )
								* exp(-im * b[nLayers - 1] * qc / root);
						res -= ed[nonzero] / sq(qc);
						tempRe += res.real() * wIn[inner];
						tempIm += res.imag() * wIn[inner];

					}
					subIntensity += wOut[tester] * (sq(4.0 * PI) / q * (sq(tempRe)+ sq(tempIm)));
				} // test
				intensity += subIntensity;
			} // outest
	}

	intensity *= (*extraParams)(0);   // Multiply by scale
	intensity += (*extraParams)(1); // Add background

	return intensity;

}

bool Cylindroid::IsSlow() {
	return true;
}

std::complex<double> Cylindroid::CalculateFF(Eigen::Vector3d qvec, int nLayers, double w, double precision, Eigen::VectorXd &p) {
	return std::complex<double>(0.0, 1.0);
}

#pragma endregion

#pragma region Non-conserved Eccentricity Cylindroid

CylindroidVaryingEcc::CylindroidVaryingEcc(int integralSteps, std::string str, ProfileShape eds, int nlp) : Cylindroid(integralSteps, str, eds, nlp) {
}

void CylindroidVaryingEcc::PreCalculate(Eigen::VectorXd &p, int nLayers) {
	Cylindroid::PreCalculate(p, nLayers);

	for(int i = nLayers - 1; i >= 0; i--) {
		if (!bSwitch)
			eps[i] = sqrt(1.0 - sq((r[i] + b1 - r[nonzero]) / r[i]));
		else
			eps[i] = sqrt(1.0 - sq(r[i] / (r[i] + b1 - r[nonzero])));
	}


}

//double CylindroidVaryingEcc::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
//}

#pragma endregion




