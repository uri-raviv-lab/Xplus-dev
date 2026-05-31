//#define NOMINMAX
//#include "Windows.h"	//For messagebox debugging

#include "Model.h"
#include "gpgpu.h"
#include "Quadrature.h" // For Quadrature

#include "Eigen/LU" // For matrix inverse
#include "mathfuncs.h" // For gaussianSig

Model::Model(std::string name, int extras, int nlp, 
			 int minlayers, int maxlayers, EDProfile edp, int disp) : 
			GPUKernel(NULL), nLayerParams(nlp), pStop(NULL),
			minLayers(minlayers), maxLayers(maxlayers),
			modelName(name), nExtraParams(extras), profile(edp),
			displayParams(disp), parameters(NULL), extraParams(NULL) {
				bParallelizeVector = true;
}


Model::~Model() {
	// Delete a stray arbitrary ED profile, if exists
	if(profile.func) {
		delete profile.func;
		profile.func = NULL;
	}
}


///// Get/Set Methods

int Model::GetNumLayerParams() {
	return nLayerParams;
}

int Model::GetNumExtraParams() {
	return nExtraParams;
}

std::string Model::GetName() {
	return modelName;
}

int Model::GetNumRelatedModels() {
	return 0;
}

int Model::GetNumBaseModels() {
	return 0;
}

std::string Model::GetLayerParamName(int index) {
	switch(index) {
		default:
			if(profile.func) {
				int edpparams = (nLayerParams - profile.func->GetNumEDParams());

				if(index >= edpparams && index < nLayerParams)
					return profile.func->GetEDParamName(index - edpparams);
			}

			return "N/A";
		case 0:
			return "Radius";
		case 1:
			return "E.D.";
	}
}

std::string Model::GetRelatedModelName(int index) {
	return "N/A";
}

Model *Model::GetBaseModel(int index) {
	return NULL;
}

ExtraParam Model::GetExtraParameter(int index) {
	if(index < 0 || index >= nExtraParams)
		return ExtraParam("N/A");

	switch(index) {
		case 0:
			return ExtraParam("Scale", 1.0, false, false, false, 0.0, 0.0,
							  false, 12);

		case 1:
			return ExtraParam("Background", 5.0);

		default:
			return ExtraParam("Unimplemented");
	}
}

bool Model::IsParamApplicable(int layer, int lpindex) {
	if(layer < 0 || lpindex < 0 || lpindex >= nLayerParams)
		return false;

	return true;
}

bool Model::IsLayerBased() {
	return true;
}

std::string Model::GetLayerName(int layer) {
	if(layer < 0)
		return "N/A";

	if(layer == 0)
		return "Solvent";

	std::stringstream ss;

	ss << "Layer " << layer;
	return ss.str();
}

int Model::GetMinLayers() { 
	return minLayers; 
}

int Model::GetMaxLayers() { 
	return maxLayers; 
}

void Model::SetStop(int *stop) { 
	pStop = stop; 
}

EDProfile Model::GetEDProfile() {
	return profile;
}

int Model::GetNumDisplayParams() {
	return displayParams;
}

std::string Model::GetDisplayParamName(int index) {
	// Override this function in subclasses
	return "";
}

bool Model::ParallelizeVector() {
	return bParallelizeVector;
}

double Model::GetDisplayParamValue(int index, const paramStruct *p) {
	// Override this function in subclasses
	return -1.0;
}

double Model::GetDefaultParamValue(int paramIndex, int layer) {
	switch(paramIndex) {
		default:
			if(profile.func) {
				int edpparams = (nLayerParams - profile.func->GetNumEDParams());

				if(paramIndex >= edpparams && paramIndex < nLayerParams)
					return profile.func->GetEDParamDefaultValue(paramIndex - edpparams, layer);
			}
			// FALLBACK

		case 0:
			// Radius
			if(layer == 0)
				return 0.0;
			
			return 1.0;

		case 1:
			// Electron Density
			if(layer == 0)
				return 333.0;

			return 400.0;
	}
}

Model *Model::CreateRelatedModel(int index) {
	return NULL;
}

///// Calculation Methods

void Model::PreCalculate(VectorXd& p, int nLayers) {
}

double Model::LiveCalculate(double q, int nLayers, Eigen::VectorXd &p) {
	return Calculate(q, nLayers, p);
}

VectorXd Model::GPUCalculate(const std::vector<double>& q,int nLayers, VectorXd& p) {

	// If there is no GPU backend, return nothing
	if(!isGPUBackend())
		return VectorXd();

	PreCalculate(p, nLayers);

	VectorXf eigenX, eigenParams, eigenY;
	
	eigenX = VectorXf::Zero(q.size());
	eigenParams = p.cast<float>();
	eigenY = VectorXf::Zero(q.size());
	
	for(int i = 0; i < (int)q.size(); i++)
		eigenX(i) = (float)q[i];

	if(!GenerateGPUModel(GPUKernel, eigenX, eigenParams, eigenY, p.size(), nExtraParams))
		return VectorXd();

	return eigenY.cast<double>();
}

VectorXd Model::CalculateVector(const std::vector<double>& q, int nLayers, VectorXd& p) {
	VectorXd res (q.size());
	PreCalculate(p, nLayers);
    
    int size = q.size();
	bool error = false;
	int progress = 0;

	// When CailleModel is extended, use this code piece before calling Model::CalculateVector
	/*if(GetPeakType() == SHAPE_CAILLE) 
		SetX(x);*/
    
    // 1st tier of parallelization
#pragma omp parallel for shared(progress) if(bParallelizeVector)//if(GetPeakType() != SHAPE_CAILLE)
    for (int i = 0; i < size; i++) {
		progress++;
		// TODO: Now if we could only pass ProgressReport here...

        double cury;
		if(error)
			continue;
        
        if(pStop && *pStop) {
			error = true;
			continue;
		}
		Eigen::VectorXd dummy;
		// we shouldn't have to do this; however, using "cury = Calculate(q[i],  nLayers);"
		// causes an assertion failure (in Eigen because of the VectorXd() and there is 
		// a line in Eigen stating "ei_assert(dim > 0);" we therefore shouldn't have a 0
		// dimension VectorXd. Any other solutions?)
		cury = Calculate(q[i],  nLayers, dummy);
		if(cury != cury) {
	        error = true;
			continue;
		}

		res(i) = cury;
    }
	
	return res;
}

inline VectorXd Model::derF(const std::vector<double>& x, VectorXd& p, 
							int nLayers, int ai, double h, double m) {  
	VectorXd pDummy, res = VectorXd::Zero(x.size());
	int size = x.size();

	p(ai) += h;

	// Create copies for the parameter vector and the number of layers for
	// this iteration
	VectorXd guess = p;
	int guessLayers = nLayers;

	// The new layer model is created based on the Electron Density Profile
	EDPFunction *edp = GetEDProfile().func;
	if(IsLayerBased() && edp && x.size() > 1)
		guess = edp->ComputeParamVector(this, p, x, nLayers, guessLayers);
	// END of profile reshaping

	PreCalculate(guess, guessLayers);
	
	//  if(GetPeakType() != SHAPE_CAILLE) -- Extend CailleModel to not use omp

	VectorXd tmp = CalculateVector(x, guessLayers, guess);
	for(int i = 0; i < size; i++)
		res(i) = m * tmp(i);

	p(ai) -= h;

	return res;
}

VectorXd Model::Derivative(const std::vector<double>& x, VectorXd param,
						   int nLayers, int ai) {
	double h = 1.0e-9;

	// Special cases
	// Partial Scale Derivative
	if(ai == (param.size() - nExtraParams)) { 

		// Create copies for the parameter vector and the number of layers for
		// this iteration
		VectorXd guess = param;
		int guessLayers = nLayers;

		// The new layer model is created based on the Electron Density Profile
		EDPFunction *edp = GetEDProfile().func;
		if(IsLayerBased() && edp && x.size() > 1)
			guess = edp->ComputeParamVector(this, param, x, nLayers, guessLayers);
		// END of profile reshaping

		// Tal: I don't like the use of OrganizeParameters. Why not use PreCalculate instead?
		OrganizeParameters(guess, guessLayers);

		VectorXd der = CalculateVector(x, guessLayers, guess);
		der -= VectorXd::Constant(x.size(), (*extraParams)[1]);
		/*************
Coefficient wise operations

In Eigen2, coefficient wise operations which have no proper mathematical definition (as a coefficient wise product) were achieved using the .cwise() prefix, e.g.:

 a.cwise() * b 

In Eigen3 this .cwise() prefix has been superseded by a new kind of matrix type called Array for which all operations are performed coefficient wise. You can easily view a matrix as an array and vice versa using the MatrixBase::array() and ArrayBase::matrix() functions respectively. Here is an example:

Vector4f a, b, c;
c = a.array() * b.array();

Note that the .array() function is not at all a synonym of the deprecated .cwise() prefix. While the .cwise() prefix changed the behavior of the following operator, the array() function performs a permanent conversion to the array world. Therefore, for binary operations such as the coefficient wise product, both sides must be converted to an array as in the above example. On the other hand, when you concatenate multiple coefficient wise operations you only have to do the conversion once, e.g.:

Vector4f a, b, c;
c = a.array().abs().pow(3) * b.array().abs().sin();

With Eigen2 you would have written:

c = (a.cwise().abs().cwise().pow(3)).cwise() * (b.cwise().abs().cwise().sin());

		*************/
		der.array() /= (*extraParams)(0);
		return der;
	}
	//Partial Background Derivative
	else if(ai == (param.size() - nExtraParams + 1)) {
		return VectorXd::Ones(x.size());
	}

	// f'(x) ~ [f(x-2h) - f(x+2h)  + 8f(x+h) - 8f(x-h)] / 12h
	VectorXd av, bv, cv, dv;

	av = derF(x, param, nLayers, ai, -2.0 * h, 1.0 / (12.0 * h));
	bv = derF(x, param, nLayers, ai, h, 8.0 / (12.0 * h));
	cv = derF(x, param, nLayers, ai, -h, -8.0 / (12.0 * h));
	dv = derF(x, param, nLayers, ai, 2.0 * h, -1.0 / (12.0 * h));
	

	return (av + bv + cv + dv);
}


void Model::OrganizeParameters(const VectorXd& p, int nLayers) {
	//std::string str = debugMatrixPrintM(p);
	//MessageBoxA(NULL, str.c_str(), "Parameters Vector", NULL);

	if(parameters)
		delete parameters;
	if(extraParams)
		delete extraParams;

	parameters = new MatrixXd(MatrixXd::Zero(nLayers, nLayerParams));
	extraParams = new VectorXd(VectorXd::Zero(nExtraParams));
	int c = 0;
	for (int j = 0; j < parameters->cols(); j++)
		for (int i = 0; i < parameters->rows(); i++)
			(*parameters)(i,j) = p[c++];
	for (int i = 0; i < extraParams->size(); i++)
		(*extraParams)(i) = p(c++);
}

void Model::SetEDProfile(EDProfile edp) {
	if(profile.func) {
		nLayerParams -= profile.func->GetNumEDParams();
		delete profile.func;
	}

	profile.shape = edp.shape;
	profile.func = ProfileFromShape(profile.shape, MatrixXd::Zero(1, 1));
	if(!profile.func)
		return;

	nLayerParams += profile.func->GetNumEDParams();
}

///// Miscellaneous Methods

// Orientation Average
double OrientationAverage(double q, FFModel *model, int nLayers, VectorXd& p) {
	/*_w = a[1]; _d = a[2]; _h = a[3];
	_q = q; _ed = a[nd];
	//Pablo
	int innerres = 2;
	int osc  = int((max(max(_w,_d),_h))*q*innerres);*/

	static VectorXd phix, thetax, phiw, thetaw;
	double result = 0.0;

	SetupIntegral(phix, phiw, 0.0 + EPS, 2.0 * PI + EPS, defaultQuadRes);
	SetupIntegral(thetax, thetaw, 0.0 + EPS, PI + EPS, defaultQuadRes);

	if(defaultQuadRes <= 1)
		return result;
	
	#pragma omp parallel for default(shared) schedule(static) reduction(+ : result)
	for(int i = 0; i < defaultQuadRes; i++) {
		double inner = 0.0;
		
		for(int j = 0; j < defaultQuadRes; j++) {
			double precision = (j > 0) ? (thetax[j] - thetax[j -1]) / 2.0 : (thetax[j]) / 2.0   ;
			Vector3d qvector (q * sin(thetax[j]) * cos(phix[i]), 
					  q * sin(thetax[j]) * sin(phix[i]),
					  q * cos(thetax[j]));
			inner += std::norm(model->CalculateFF(qvector, 
							   nLayers,thetaw[j],precision)) * sin(thetax[j]) * thetaw[j];
		}
		result += inner * phiw[i];
	}

	return result;
}

// Polydisperse model
double PolydisperseModel::Calculate(double q, int nLayers, VectorXd& a) {
	// This should never be called
	return -5.3;
}
double PolydisperseModel::LiveCalculate(double q, int nLayers, VectorXd& a) {
	// This should never be called
	return -7.6;
}

int PolydisperseModel::GetNumBaseModels() {
	return 1;
}

Model *PolydisperseModel::GetBaseModel(int index) {
	if(index == 0)
		return model;

	return NULL;
}

VectorXd PolydisperseModel::CalculateVector(const std::vector<double> &q, int nLayers, Eigen::VectorXd &a) {
	int points = GetPDResolution();
	VectorXd b = a, a1 = a, x = VectorXd::Zero(points), intensity = VectorXd::Zero(q.size());

	if(a1.size() == 0) {	// Outermost PD layer
		Model *innerModel = this;
		int eP = p.extraParams.size(), lay = p.layers;
		while((innerModel->GetName()).compare("Polydisperse Model") == 0)
			innerModel = ((PolydisperseModel*)innerModel)->model;

		int ma = (lay * innerModel->GetNumLayerParams()) + eP;
		a1 = VectorXd::Zero(ma); // Parameter Vector

		for(int i = 0; i < p.model->GetNumLayerParams(); i++)
			for(int j = 0; j < lay; j++)
				a1(i * lay + j) = p.params[i][j].value;

		for(int i = 0; i < eP; i++)
			a1(ma - eP + i) = p.extraParams[i].value;
		b = a1;
	}

	int param = polyInd;
	double sig = polySigma;
	double Z = 0.0;
	if(param < 0 || sig < 1.0e-7)
		return model->CalculateVector(q, nLayers, a);

	for(int i = 0; i < points; i++) {
		if(pStop && *pStop)
			return VectorXd::Zero(q.size());

		x(i) = a1(param) - 2.0 * sig + double(i) / double(points - 1) * 4.0 * sig; // taking 2 sigma on each side
		if (x(i) < 0.0)// don't use...
			continue;
		double ga = 0.0;

		// Later on, this will use a generic PDProfile class, so that each PD
		// can have its own arbitrary pattern
		switch(GetPDFunc()) {
			default:
				break;
			case SHAPE_GAUSSIAN:
				ga = gaussianSig(sig, a1(param), 1.0, 0.0, x(i));
				break;
			case SHAPE_LORENTZIAN:
				ga = lorentzian(sig, a1[param], 1.0, 0.0, x[i]);
				break;
			case SHAPE_LORENTZIAN_SQUARED: // Actually this is Uniform
				ga = 1.0 / (double)points;
				break;
		}

		Z += ga;
		b(param) = x(i);
		intensity += ga * model->CalculateVector(q, nLayers, b);
	}
	return intensity / Z;
}


// Composite model
CompositeModel::CompositeModel(FFModel **modelObjects, Vector3d *distances, 
			   Matrix3d *rotations, int numModels,
			   int *numModelsParam, int *numLayersParam ) : 
		FFModel("Composite Model"), models(NULL),
		dist(distances),rot(rotations), modelCount(numModels),
		modelsParamCount(numModelsParam),
		modelsLayerCount(numLayersParam){ 

	if(numModels <= 0)
		return;

	models = new FFModel*[numModels];
	memcpy(models, modelObjects, sizeof(FFModel*) * numModels);
}

CompositeModel::~CompositeModel() {
	if(models)
		delete[] models;
}

std::complex<double> CompositeModel::CalculateFF(Vector3d qvec, VectorXd& p, 
												 int nLayers, double w, double precision) {
	std::complex<double> res (0.0, 0.0);

	if(!models)
		return std::complex<double> (-1.0, 0.0);

	Vector3d *rotqvector = new Vector3d[modelCount];

	// Imaginary value "i"
	std::complex<double> im (0.0, 1.0);
	
	int paramNum = 0;
	// Moving and rotating a vector rho(A^(-1)r + a)-->exp(-im q a)FF(q A)
	for(int i = 0; i < modelCount; i++) {
		rotqvector[i] = rot[i] * qvec;
		//the inverse of a rotation matrix is equal to its transpose.
		paramNum += modelsParamCount[i];
		
		// Tal: find something better
		VectorXd params = VectorXd::Zero(modelsParamCount[i]);
		for (int j = 0; j <  params.size(); j++) 
			params[j] = p[paramNum + j] ;
		res += exp(-im * qvec.dot(rot[i].inverse() * dist[i]))*
			models[i]->CalculateFF(rotqvector[i] ,modelsLayerCount[i]); 
	}


	return res;
}


int CompositeModel::GetNumBaseModels() {
	return modelCount;
}

Model *CompositeModel::GetBaseModel(int index) {
	if(index < 0 || index >= modelCount)
		return NULL;

	return models[index];
}

bool FFModel::IsSlow()
{
	return false;
}
