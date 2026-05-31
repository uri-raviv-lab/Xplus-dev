#include "SphericalModels.h"

#include "Quadrature.h" // For SetupIntegral

#include "GLPreview.h" // For Draw functions

#include "mathfuncs.h" // For ln2 and square

//#include "Windows.h"	//For MessageBox


#pragma region Abstract Sphere

	SphericalModel::SphericalModel(std::string st, int nlp, ProfileShape edp, int exParams) : 
		FFModel(st, exParams, nlp, 2, -1, EDProfile(SYMMETRIC, edp)) {}
	

	bool SphericalModel::IsParamApplicable(int layer, int lpindex) {
		return Model::IsParamApplicable(layer, lpindex);
	}

	std::string SphericalModel::GetLayerParamName(int index) {
		switch(index) {
			default:
				return Model::GetLayerParamName(index);
			case 0:
				return "Radius";
			case 1:
				return "E.D.";
		}
	}

	void SphericalModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		Model::OrganizeParameters(p, nLayers);
	
		r			= (*parameters).col(0);
		ED			= (*parameters).col(1);
		edSolvent	= (*parameters)(0,1);
	}

	int SphericalModel::GetNumRelatedModels() {
		return 3;	// Uniform, Gaussian and Tanh  
	}

	std::string SphericalModel::GetRelatedModelName(int index) {
		switch(index) {
			case 0:
				return "Uniform Sphere";
			case 1:
				return "Gaussian Sphere";
			case 2: 
				return "Smooth Sphere";
			default:
				return Model::GetRelatedModelName(index);
		}
	}

	Model *SphericalModel::CreateRelatedModel(int index) {
		switch(index) {
			case 0:
				return new UniformSphereModel();
			case 1:
				return new GaussianSphereModel();
			case 2: 
				return new SmoothSphereModel();
			default:
				return Model::CreateRelatedModel(index);
		}
	}

	void SphericalModel::DrawPreviewScene() {
		DrawGLSphere(6.28f);	
	}
	
	void SphericalModel::DrawOpenGLPreview(const paramStruct &p) {
		Eigen::VectorXf rad = Eigen::VectorXf::Zero(p.layers);
		Eigen::VectorXf ed = Eigen::VectorXf::Zero(p.layers);
		
		for(int i = 0; i < p.layers; i++) {
			rad[i] = float(p.params[0][i].value);
			ed[i]  = float(p.params[1][i].value);
		}

		DrawGLNLHollowSphere(rad.data(), ed.data(), p.layers, 1.6f);
	}

#pragma endregion

#pragma region Uniform Sphere

	UniformSphereModel::UniformSphereModel(std::string st) : SphericalModel(st) {
	}

	void UniformSphereModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		SphericalModel::OrganizeParameters(p, nLayers);

		for(int i = 1; i < nLayers; i++)
			r[i] += r[i - 1];
	}

	void UniformSphereModel::PreCalculate(VectorXd& p, int nLayers) {
		OrganizeParameters(p, nLayers);
	}

	double UniformSphereModel::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
		double intensity = 0.0;

		if(p.size() > 0)
			OrganizeParameters(p, nLayers);

		for(int i = 0; i < nLayers - 1; i++)
			intensity += (ED[i] - ED[i + 1]) * 
			( (sin(q * r[i])) - (q * r[i] * cos(q * r[i])));

		intensity += (ED[nLayers - 1] - edSolvent) * 
			( (sin(q * r[nLayers - 1]) )- (q * r[nLayers - 1] * cos(q * r[nLayers - 1])));

		intensity = sq(4.0 * PI * intensity / (q * sq(q)));

		intensity *= (*extraParams)(0);	// Multiply by scale
		intensity += (*extraParams)(1);	// Add background

		return intensity;
	}

	std::complex<double> UniformSphereModel::CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p) {
			return std::complex<double>(0.0, -1.0); //TODO FIXME
	}	
#pragma endregion

#pragma region Gaussian Sphere

	GaussianSphereModel::GaussianSphereModel(std::string st, ProfileShape edp) : SphericalModel(st, 3, edp) {
		steps = 500;
		SetupIntegral(xx, ww, 0.0f, 1.0f, steps);
	}
	
	bool GaussianSphereModel::IsParamApplicable(int layer, int lpindex) {
		if(layer == 0 && (lpindex == 2 || lpindex == 0))
			return false;
		return Model::IsParamApplicable(layer, lpindex);
	}

	std::string GaussianSphereModel::GetLayerParamName(int index) {
		if(index == 2)
			return "R 0";
		return SphericalModel::GetLayerParamName(index);
	}
	double GaussianSphereModel::GetDefaultParamValue(int paramIndex, int layer) {
		if(paramIndex == 2) // Z0
			return (double)(layer - 1);
		return Model::GetDefaultParamValue(paramIndex, layer);
	}

	void GaussianSphereModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		SphericalModel::OrganizeParameters(p, nLayers);
		z0 = (*parameters).col(2);
	}

	void GaussianSphereModel::PreCalculate(VectorXd& p, int nLayers) {
		OrganizeParameters(p, nLayers);
	}

	double GaussianSphereModel::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
		double intensity = 0.0;

		if(p.size() > 0)
			OrganizeParameters(p, nLayers);

		VectorXd edtexp = VectorXd::Zero(nLayers);

		for(int i = 1; i < nLayers; i++) {
			edtexp[i] = (ED[i] - edSolvent) * r[i] * exp(-sq(r[i] * q) / (16.0 * ln2));
			intensity += edtexp[i]
				* ( ((-2.0 * z0[i] * sin(q * z0[i])) / sqrt(ln2)) - ((sq(r[i]) * q * cos(q * z0[i])) / (4.0 * pow(ln2, 1.5))) );
		}

		// += integral

		#pragma omp parallel for reduction(+ : intensity)
		for(int i = 0; i < steps; i++) {
			double inner = 0.0;
			for(int j = 1; j < nLayers; j++) {
				double rqy2 = z0[j] * q * (sq(xx[i])- 1);

				inner += edtexp[j] * exp(sq(xx[i]) * ((sq(r[j] * q) / (16.0 * ln2)) - ((4.0 * ln2 * sq(z0[j] / r[j]) ))))
					* (( (sq(2.0 * z0[j]) / r[j]) - (sq(r[j] * q / (4.0 * ln2) ) * r[j])) * sin(rqy2) - (( z0[j] * r[j] * q / ln2 ) * cos(rqy2)));
			}

			intensity += inner * ww[i];
		}

		intensity *= pow(PI, 1.5) / (2.0 * q);

		intensity *= intensity;	// Square, no need for an orientation average

		intensity *= (*extraParams)(0);	// Multiply by scale
		intensity += (*extraParams)(1);	// Add background

		return intensity;

	}

	std::complex<double> GaussianSphereModel::CalculateFF(Vector3d qvec, 
									 int nLayers, double w, double precision, VectorXd& p) {
		return std::complex<double>(0.0, -1.0); //TODO FIXME
	}

#pragma endregion

#pragma region Smooth Sphere
	
	SmoothSphereModel::SmoothSphereModel(std::string st, ProfileShape edp) : SphericalModel(st, 3, edp/*FOR DEBUG, 3*/) {
	}

	bool SmoothSphereModel::IsParamApplicable(int layer, int lpindex) {
		return SphericalModel::IsParamApplicable(layer, lpindex);
	}

	std::string SmoothSphereModel::GetLayerParamName(int index) {
		switch(index) {
			default:
				return "N/A";
			case 0:
				return "Width";
			case 1:
				return "E.D.";
			case 2:
				return "Slope";
		}
	}

	double SmoothSphereModel::GetDefaultParamValue(int paramIndex, int layer) {
		switch(paramIndex) {
		default:
		case 0:
			// Width
			if(layer == 0)
				return 0.0;
			
			return (double)(layer);

		case 1:
			// Electron Density
			if(layer == 0)
				return 333.0;

			return 400.0;

		case 2:
			// Slope
			return 1.0;
		}
	}

	ExtraParam SmoothSphereModel::GetExtraParameter(int index) {
		//if(index == 2)
		//	return ExtraParam("Factor", 1.2, false, true);
		return SphericalModel::GetExtraParameter(index);
	}
	
	void SmoothSphereModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		Model::OrganizeParameters(p, nLayers);
		
		width		= (*parameters).col(0);
		ED			= (*parameters).col(1);
		slope		= (*parameters).col(2);
		edSolvent	= (*parameters)(0,1);
		r			= VectorXd::Zero(width.size());
		r(0)		= width(0);
		for(int i = 1; i < width.size(); i++)
			r(i) = r(i - 1) + width(i);
	}

	void SmoothSphereModel::PreCalculate(VectorXd& p, int nLayers) {
		OrganizeParameters(p, nLayers);

		// To find the upper limit for integration
		rMax = 0.0;
		double eps = 0.000001;
		double tmp = 0.5 * log( (2 - eps) / eps ), rad = 0.0;
		for (int i = 0; i < nLayers; i++){
			if(slope[i] < 1.0e-6)
				continue;
			rad += r[i];
			double curR = (tmp / slope[i]) + rad;
			if (curR > rMax)
				rMax = curR;
		}
		rMax += 2.0;
		// Have the maximal step size be an angstrom
		steps = max(int(rMax * 10.0), 200);
		
		LayersSum = VectorXd::Zero(steps);
		
		// Calculate the interval for each step
		SetupIntegral(xx, ww, 0.0, rMax, steps);
		
		// Calculation of the sum that does not depend on q, for each integration step.
		#pragma omp parallel for
		for(int i = 0; i < steps; i++) {
			double profamp = 0.0;
			double arg = 0.0;
			for (int j = 0; j < nLayers - 1; j++) {
				arg = slope[j] * (xx[i] - r[j]);
				profamp += (ED[j+1] - ED[j]) * tanh(arg);
			}	

			arg = slope[nLayers - 1] * (xx[i] - r[nLayers - 1]);
			profamp += (ED[0] - ED[nLayers - 1]) * tanh(arg);
			
			LayersSum[i] = profamp;
		}
	}

	double SmoothSphereModel::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
		double intensity = 0.0;

		if(p.size() > 0)
			OrganizeParameters(p, nLayers);

		// += integral
		#pragma omp parallel for reduction(+ : intensity)
		for(int i = 0; i < steps; i++) 
			intensity += (LayersSum[i] * xx[i] * sin(q * xx[i])) * ww[i];	
		

		intensity *= ((4.0 * PI) / q) / 2.0;

		intensity *= intensity;	// Square, no need for an orientation average

		intensity *= (*extraParams)(0);	// Multiply by scale
		intensity += (*extraParams)(1);	// Add background

		return intensity;

	}

	std::complex<double> SmoothSphereModel::CalculateFF(Vector3d qvec, 
									 int nLayers, double w, double precision, VectorXd& p) {
		return std::complex<double>(0.0, -1.0); //TODO FIXME
	}
#pragma endregion