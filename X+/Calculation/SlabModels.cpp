#include "SlabModels.h"
#include "Quadrature.h" // For SetupIntegral

#include "GLPreview.h" // For Draw functions

#include "Windows.h"	//For MessageBox
#include "mathfuncs.h" // For bessel functions and square

#pragma region Abstract Slab
	SlabModel::SlabModel(std::string st, EDProfile edp, int nlp, int maxlayers) : FFModel(st, 
															2, nlp, 2, maxlayers, edp)
	{}

	bool SlabModel::IsParamApplicable(int layer, int lpindex)
	{
		if(layer == 0)	//Solvent
			if(lpindex == 0)	// Width
				return false;
		return true;
	}

	std::string SlabModel::GetLayerParamName(int index) {
		switch(index) {
			default:
				return Model::GetLayerParamName(index);
			case 0:
				return "Width";
			case 1:
				return "E.D.";
		}
	}

	std::string SlabModel::GetLayerName(int layer) {
		if(layer < 0)
			return "N/A";

		if(layer == 0)
			return "Solvent";

		std::stringstream ss;

		ss << "Layer " << layer;
		return ss.str();
	}


	double SlabModel::GetDefaultParamValue(int paramIndex, int layer) {
		switch(paramIndex) {
			default:
			case 0:
				// Width
				if(layer < 1)
					return 0.0;

				return 1.0;

			case 1:			
				// Electron Density
				if(layer == 0)
					return 333.0;
				if(layer == 1)
					return 280.0;
				
				return 400.0;
		}
	}

	int SlabModel::GetNumRelatedModels() {
		return 5;
	}

	std::string SlabModel::GetRelatedModelName(int index) {
		switch(index) {
		default:
			return Model::GetRelatedModelName(index);
		case 0:
			return "Symmetric Uniform Slabs";
		case 1:
			return "Asymmetric Uniform Slabs";
		case 2:
			return "Symmetric Gaussian Slabs";
		case 3:
			return "Asymmetric Gaussian Slabs";
		case 4:
			return "Membrane";
		}
	}

	Model *SlabModel::CreateRelatedModel(int index) {
		switch(index) {
		default:
			return Model::CreateRelatedModel(index);
		case 0:
			return new UniformSlabModel();
		case 1:
			return new UniformSlabModel("Asymmetric Uniform Slabs", ASYMMETRIC);
		case 2:
			return new GaussianSlabModel();
		case 3:
			return new GaussianSlabModel("Asymmetric Gaussian Slabs", -1, ASYMMETRIC);
		case 4:
			return new MembraneModel();
		}
	}

	void SlabModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers)
	{
		Model::OrganizeParameters(p, nLayers);
		width		= (*parameters).col(0);
		ED			= (*parameters).col(1);
		edSolvent	= (*parameters)(0,1);
		
		// DEBUG LINES
		//std::string str = debugMatrixPrintM(*parameters);
		//MessageBoxA(NULL, str.c_str(), "Parameters Matrix", NULL);
		// END DEBUG LINES
	}

	void SlabModel::DrawOpenGLPreview(const paramStruct& p){
		float *rad = new float[p.layers]();
		float *ed  = new float[p.layers]();
		
		for(int i = 0; i < p.layers; i++) {
			rad[i] = (float)p.params[0][i].value;
			ed[i]  = (float)p.params[1][i].value;
		}
		if(this->profile.type == SYMMETRIC)
			DrawGLNLayeredSlabs(rad, ed, 2.0f, p.layers);
		if(this->profile.type == ASYMMETRIC)
			DrawGLNLayeredAsymSlabs(rad, ed, 2.0f, p.layers);

		delete[] rad;
		delete[] ed;
	}

	void SlabModel::DrawPreviewScene() // So far, they're all the same
	{
		DrawGLMembrane(0.2f, 6, 10, 1.0f);
	}

	Model *SlabModel::GetSpecializedSF() {
		return new CailleModel();
	}


#pragma endregion

#pragma region Uniform Slab

	UniformSlabModel::UniformSlabModel(std::string st, ProfileType t) : SlabModel(st, EDProfile(t))
		{}

	double UniformSlabModel::Calculate(double q, int nLayers, VectorXd& p)
	{
		if(p.size() > 0)	// For PD model
			OrganizeParameters(p, nLayers);
		
		double intensity = 0.0;
		
		if(this->profile.type == SYMMETRIC) {
			for(int i = 1; i < nLayers; i++)
				intensity += 2 * (ED[i] - edSolvent) * (sin(q * width[i]) - sin(q * width[i - 1]));
			
			intensity *= intensity;
		}
		if(this->profile.type == ASYMMETRIC) {
			for(int i = 1; i < nLayers; i++) {
				for(int j = 1; j < nLayers; j++) {
					intensity += (ED[i] - edSolvent) * (ED[j] - edSolvent) *
					(cos(q * (width[j] - width[i])) - 2.0 * cos(q * (width[j - 1] - width[i]))
					  + cos(q * (width[j - 1] - width[i - 1])));
				}
			}
		}
		
		intensity *= (2.0 / sq(sq(q)));
		
		intensity *= (*extraParams)(0);	// Multiply by scale
		intensity += (*extraParams)(1); // Add background

		return intensity;
	}

	std::complex<double> UniformSlabModel::CalculateFF(Vector3d qvec, 
									 int nLayers, double w, double precision, VectorXd& p)
	{
		return std::complex<double>(0.0, -1.0); //TODO FIXME
	}

	void UniformSlabModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers)
	{
		SlabModel::OrganizeParameters(p, nLayers);

		for(int i = 2; i < nLayers; i++)
			width[i] += width[i - 1];
	}

	void UniformSlabModel::PreCalculate(VectorXd& p, int nLayers)
	{
		OrganizeParameters(p, nLayers);
	}

	void UniformSlabModel::PreCalculateFF(VectorXd& p, int nLayers){}

#pragma endregion

#pragma region Gaussian Slab

	GaussianSlabModel::GaussianSlabModel(std::string st, int maxlayers, ProfileType t) : SlabModel(st,
															EDProfile(t, GAUSSIAN), 3, maxlayers)
	{minLayers = 2;}

	int GaussianSlabModel::GetNumLayerParams() {
		return 3;
	}

	bool GaussianSlabModel::IsParamApplicable(int layer, int lpindex) {
		if(layer < 2 && lpindex == 2)
			return false;
		return SlabModel::IsParamApplicable(layer, lpindex);
	}

	std::string GaussianSlabModel::GetLayerParamName(int index) {
		switch(index) {
			default:
				return SlabModel::GetLayerParamName(index);
			case 0:
				return "Width";
			case 1:
				return "E.D.";
			case 2:
				return "Position";
		}
	}

	double GaussianSlabModel::GetDefaultParamValue(int paramIndex, int layer) {
		switch(paramIndex) {
			default:
			case 0:
				// Width
				if(layer < 1)
					return 0.0;

				return 1.0;

			case 1:			
				// Electron Density
				if(layer == 0)
					return 333.0;
				if(layer == 1)
					return 280.0;

				return 400.0;

			case 2:
				// Position
				if(layer < 2) // Solvent and first layer
					return 0.0;
				else
					return (double)(layer - 1);
		}
	}

	double GaussianSlabModel::Calculate(double q, int nLayers, VectorXd& p) {
		if(p.size() > 0)
			OrganizeParameters(p, nLayers);

		double intensity = 0.0, sum = 0.0;

		if(this->profile.type == SYMMETRIC) {
			sum = (ED[1] - edSolvent) * width[1] * exp(-q * q * width[1] * width[1]/ (16.0 * ln2));

			for(int cnt = 2; cnt < nLayers; cnt++) {
				sum += 2.0 * (ED[cnt] - edSolvent) * width[cnt] * exp( -(q * q * width[cnt] * width[cnt]) / (16.0 * ln2))
					* cos(z[cnt] * q);
			}
		
			intensity = sq(sum / q) * (PI / (2.0 * ln2));
		}
		if(this->profile.type == ASYMMETRIC) {
			for (int i = 0; i < nLayers ; i++) {
				for(int j = 0; j < nLayers; j++) {
					intensity += 
						(ED[i] - edSolvent) * (ED[j] - edSolvent) * width[i] * width[j]
						* exp(-(sq(q) * (sq(width[i]) + sq(width[j]))) / (16.0 * ln2))
						* cos(q * (z[i] - z[j]));
				}
			}
			intensity *= PI / (2.0 * ln2 * sq(q)); 
		}

		intensity *= (*extraParams)(0);	// Multiply by scale
		intensity += (*extraParams)(1);	// Add background
	
	return intensity;
	}
	std::complex<double> GaussianSlabModel::CalculateFF(Vector3d qvec, 
									 int nLayers, double w, double precision, VectorXd& p) {
		return std::complex<double>(0.0, -1.0); //TODO FIXME
	}

	void GaussianSlabModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		SlabModel::OrganizeParameters(p, nLayers);
		z = (*parameters).col(2);
		if(this->profile.type == SYMMETRIC)
			width[1] *= 2.0;
	}

	void GaussianSlabModel::DrawOpenGLPreview(const paramStruct& p) {
		// TODO: Draw a better (Gaussian) profile
		SlabModel::DrawOpenGLPreview(p);
	}

	void GaussianSlabModel::PreCalculate(VectorXd& p, int nLayers) {
		OrganizeParameters(p, nLayers);
	}

	void GaussianSlabModel::PreCalculateFF(VectorXd& p, int nLayers){}

#pragma endregion

#pragma region Membrane Model
	MembraneModel::MembraneModel() : GaussianSlabModel("Membrane", 3) {
		displayParams = 4;
		minLayers = 3;
	}

	std::string MembraneModel::GetLayerName(int layer) {
		switch(layer) {
			case 0:
				return "Solvent";
			case 1:
				return "Tail";
			case 2:
				return "Head";
			default:
				return "N/A";
		}
	}

	std::string MembraneModel::GetDisplayParamName(int index) {
		switch(index) {
			case 0:
				return "Head-to-Head";
			case 1:
				return "Chain Length";
			case 2:
				return "Bilayer Thickness";
			case 3:
				return "Max-to-Max";
			default:
				return "N/A";
		}
	}

	double EDatR(double r, double denorm1, double denorm2, const paramStruct *p) {
		//if(p->params.size() < 3)
		//	return -2.0;
		double solventED = p->params[0][0].value;
		std::vector<Parameter>	t	= p->params[0],
								ed	= p->params[1],
								z	= p->params[2];

		return denorm1 * gaussianFW(t[1].value * 2.0, z[1].value, ed[1].value - ed[0].value, 0.0, r)
					+ denorm2 * gaussianFW(t[2].value, z[2].value, ed[2].value - ed[0].value, 0.0, r);
	}

	double maxEDDistance(const paramStruct *p) {
		double resolution = 1.0e-3, r = 0.0, ed = 0.0, prevED = 0.0;
		double	denorm1 = sqrt(2.0 * PI) * 2.0 * p->params[1][1].value / (2.0 * sqrt(2.0 * ln2)),
				denorm2 = sqrt(2.0 * PI) * p->params[1][2].value / (2.0 * sqrt(2.0 * ln2));

		r = max(p->params[2][2].value - 0.5 * p->params[0][2].value, resolution);
		ed = EDatR(r - resolution, denorm1, denorm2, p);
		while(r < p->params[2][2].value + 0.5 * p->params[0][2].value) {
				prevED = ed;
				ed = EDatR(r, denorm1, denorm2, p);
				if(prevED > ed)
					return r - resolution;
				r += resolution;
		}
		return -1.0;
	}

	double MembraneModel::GetDisplayParamValue(int index, const paramStruct *p) {
		switch(index) {
			case 0:	// Head-to-Head = 2 * (Z0 of the headgroup)
				return p->params[2][2].value * 2.0;
			case 1:	// Chain Length = (Z0 of the headgroup) - 0.5 * (thickness of the headgroup)
				return p->params[2][2].value - 0.5 * p->params[0][2].value;
			case 2:	// Bilayer Thickness = Head-to-Head + (thickness of the headgroup)
				return p->params[2][2].value * 2.0 + p->params[0][2].value;
			case 3:	// Max-to-Max distance
				return 2.0 * maxEDDistance(p);
			default:
				return 0.0;
		}
	}

#pragma endregion


#pragma region Cuboid
	CuboidModel::CuboidModel() : FFModel("Cuboid", 2, 2, 4, 4, EDProfile(NONE)) {}

	bool CuboidModel::IsSlow() {
		return true;
	}

	std::string CuboidModel::GetLayerName(int layer) {
		switch(layer) {
			default: 
				return FFModel::GetLayerName(layer);
			case 0: 
				return "Contrast";
			case 1: 
				return "Width";
			case 2: 
				return "Height";
			case 3: 
				return "Depth";
		}
	}
	void CuboidModel::DrawOpenGLPreview(const paramStruct &p) {
		DrawGLRectangular((float)p.params[1][0].value);
	}

	void CuboidModel::DrawPreviewScene() {
		DrawGLRectangular(-1.0);
	}

	bool CuboidModel::IsParamApplicable(int layer, int lpindex) {
		if(layer == 0 && lpindex == 1)
			return true;

		if(layer > 0 && layer < 4 && lpindex == 0)
			return true;

		return false;
	}

	double CuboidModel::GetDefaultParamValue(int paramIndex, int layer) {
		switch(paramIndex) {
			case 0: 
				return 1.0;
			case 1:
				return 333.0;
			default:
				return FFModel::GetDefaultParamValue(paramIndex, layer);
		}
	}

	std::string CuboidModel::GetLayerParamName(int index) {
		switch(index) {
			case 0: 
				return "Thickness";
			case 1:
				return "E.D.";
			default:
				return FFModel::GetLayerParamName(index);
		}
	}
	
	void CuboidModel::OrganizeParameters(const VectorXd &p, int nLayers) {
		Model::OrganizeParameters(p, nLayers);
		
		ed	= (*parameters)(0,1);
		w	= (*parameters)(1,0);
		h	= (*parameters)(2,0);
		d	= (*parameters)(3,0);
	}

	void CuboidModel::PreCalculate(VectorXd &p, int nLayers) {
		OrganizeParameters(p, nLayers);

		// 20 is an arbitrary number used instead of q_max
		stepsX = int(20.0 * 2.0 * max(w, max(h, d)));
		stepsP = int(20.0 * 2.0 * max(w, d));

#pragma omp parallel sections
		{
#pragma omp section 
			{
 			SetupIntegral(xX, wX, 0.0, 1.0, stepsX);
			}
#pragma omp section 
			{
				SetupIntegral(xP, wP, 0.0, 2.0 * PI, stepsP);
			}
		}
		sinP = VectorXd::Zero(stepsX);
		cosP = VectorXd::Zero(stepsX);

#pragma omp parallel for
		for(int i = 0; i < stepsX; i++) {
			sinP[i] = sin(xX[i]);
			cosP[i] = cos(xX[i]);
		}
	}

	double CuboidModel::Calculate(double q, int nLayers, VectorXd &p) {
		double intensity = 0.0;

#pragma omp parallel for reduction(+ : intensity)
		for(int i = 0; i < stepsX; i++) {
			double ressum = 0.0;
			for(int j = 0; j < stepsP; j++) {
				ressum += (1 - cos(w * q * sqrt(1 - sq(xX[i])) * cosP(j))) * 
						  (1 - cos(d * q * sqrt(1 - sq(xX[i])) * sinP(j))) * 
						  wP[j];
			}
			intensity += ressum * sq(sin(q * xX[i] * h / 2.0)) * wX[i];
		}

		intensity *= sq(2.0 * ed / (q * sq(q)));

		intensity *= (*extraParams)(0);   // Multiply by scale
		intensity += (*extraParams)(1); // Add background

		return intensity;

	}


	//std::complex<double> CuboidModel::CalculateFF(Eigen::Vector3d qvec, int nLayers, double w, double precision, Eigen::Vector3d& p) {
	//	return std::complex<double>(0.0, 1.0);
	//}

#pragma endregion

#pragma region Caille
		
	CailleModel::CailleModel(std::string name) : 
		Model(name, 6, 0, 0, 0, EDProfile(NONE), 0/*Change later if we want to add kappa*/) {}

	void CailleModel::PreCalculate(Eigen::VectorXd &p, int nLayers) {
		OrganizeParameters(p, nLayers);
		double mini = std::numeric_limits<double>::infinity();
		Q = 2.0 * PI / D;

	#pragma omp parallel for
		for(int i = 0; i < qGlob.size(); i++) {
			resF[i] = Calculate(qGlob[i], 0);
			mini = min (resF[i], mini);
		}

		if (mini - amp * NDiff < 0.0) {
			NDiff = (mini) / amp;
		}
	}

	void CailleModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
		Model::OrganizeParameters(p, nLayers);

		D		= (*extraParams)(0);
		amp		= (*extraParams)(1);
		eta		= (*extraParams)(2);
		N0		= (*extraParams)(3);
		sig		= (*extraParams)(4);
		NDiff	= (*extraParams)(5);
	}

	VectorXd CailleModel::CalculateVector(const std::vector<double> &q, int nLayers, Eigen::VectorXd &p) {
		int size = q.size();
		VectorXd res (size);
		bool error = false;

		qGlob = Eigen::VectorXd(size);
		resF = Eigen::VectorXd(size);
		for(int i = 0; i < size; i++)
			qGlob[i] = q[i];
		PreCalculate(p, nLayers);
	    
		for (int i = 0; i < size; i++) {
			double cury;
			if(error)
				continue;
	        
			Eigen::VectorXd dummy;
			// we shouldn't have to do this; however, using "cury = Calculate(q[i],  nLayers);"
			// causes an assertion failure (in Eigen because of the VectorXd() and there is 
			// a line in Eigen stating "ei_assert(dim > 0);" we therefore shouldn't have a 0
			// dimension VectorXd. Any other solutions?)
			cury = resF[i] - amp * NDiff;
			if(cury != cury) {
				error = true;
				continue;
			}

			res[i] = cury;
		}
		
		return res;
	}

	//double CailleModel::cailleZhang(double q, double h, double N_Diff) {
	double CailleModel::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
		double tot = 0.0, wtTot = 0.0;
		double h = q / Q;
		//Parameters:
		//double D, eta, amp, q;
		//int h, N;
		

	#pragma omp parallel for reduction( + : tot, wtTot)
		for(int N = int(max(0.0, N0 - 3.0 * sig)); N <= int(N0 + 3.0 * sig); N++) {
			double tmp = 0.0;

			if(pStop && *pStop)
				continue;

			for(int n1 = 1; n1 < N; n1++) {
				double ex = 0.0;
				for(int n2 = 1; n2 < N; n2++)
					// Should think of a way to move this to precalculate ~8 * 100^2 * 20 / 1024 [kB] = 1.5 MB
					ex += (1.0 - cos((double)n2 * PI * (double)n1 / double(N))) / (double)n2;
				ex = exp(-eta * double(h) * double(h) * ex);
				tmp += (1.0 - double(n1)/double(N)) * cos(q * double(n1) * D) * ex;
			}
			double wt = (sig > 0.0) ? gaussianSig(sig, N0, 1.0, 0.0, N) : 1.0;
			wtTot += wt;

			tot += amp * (double(N) * wt * (2.0 * tmp + 1.0) / (q * q));
		}

		return tot / wtTot;
	}

	ExtraParam CailleModel::GetExtraParameter(int index) {
		switch(index) {
			default:
				return ExtraParam("N/A");
			case 0:
				return ExtraParam("d", 1.0, false, true);
			case 1:
				return ExtraParam("Amplitude", 1.0, false, true);
			case 2:
				return ExtraParam("Eta", 1.0, false, true);
			case 3:
				return ExtraParam("N", 10, false, true, false, 0.0, 0.0, true);
			case 4:
				return ExtraParam("Sigma", 0.0, false, true);
			case 5:
				return ExtraParam("N_diff", 1.0, false, true);

		}
	}

#pragma endregion