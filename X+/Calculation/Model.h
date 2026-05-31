#ifndef __MODEL_H
#define __MODEL_H

#undef EXPORTED
#ifdef _WIN32
#ifdef CALCULATION
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED __declspec(dllimport)
#endif
#else
#define EXPORTED extern "C"
#endif

// Avoid "need dll-interface" warnings (the easy way)
#pragma warning (disable: 4251)
/*
extern template class EXPORTED std::vector<Parameter>;
extern template class EXPORTED std::vector< std::vector<Parameter> >;
*/

#include <Eigen/Core> // For VectorXd
#include <vector> // For std::vector
#include <complex> // For std::complex
#include <limits> // For inf/-inf

#include "EDProfile.h" // For EDProfile

// Positive/Negative infinity definitions
#define POSINF std::numeric_limits<double>::infinity()
#define NEGINF -std::numeric_limits<double>::infinity()

using Eigen::Vector3d;
using Eigen::VectorXd;
using Eigen::Matrix3d;
using Eigen::MatrixXd;


//////////////////////////////////////////////////////////////
///////////////////// Specifications /////////////////////////
//////////////////////////////////////////////////////////////

// Forward declaration
class Model;

// The data structure representing a Model parameter
struct Parameter {
	// The parameter's initial value
	double value; 

	// Determines whether this parameter may change during fitting
	bool isMutable;

	// True iff the value is, during fitting, constrained
	// to be between consMin and consMax
	bool isConstrained;
	double consMin, consMax;

	// If this parameter is dynamically constrained to
	// other parameters, each of the values are the constraint
	// parameter indices. Otherwise, the value is -1.
	int consMinIndex, consMaxIndex;

	// NOTE: consMin and consMax may be -inf 
	// and inf respectively	

	// If this parameter is linked to another, this value is its
	// index. Otherwise, it is -1.
	int linkIndex;

	// If this parameter is poly-dispersed, this value will be larger than
	// 0.0 and will mean the std. deviation of the parameter.
	double sigma;

	// Constructor
	Parameter(double val = 0.0, bool bMutable = false, bool bCons = false, 
			  double consmin = NEGINF, double consmax = POSINF,
			  int minInd = -1, int maxInd = -1, int linkInd = -1, 
			  double stddev = 0.0) :
			value(val), isMutable(bMutable), isConstrained(bCons),
			consMin(consmin), consMax(consmax), consMinIndex(minInd),
			consMaxIndex(maxInd), linkIndex(linkInd), sigma(stddev) {}

};


// The data structure passed from the UI to the Calculation
// backend on fitting. Specifies all the parameters of a model
// and the model itself.
struct paramStruct {
	// A bit tricky. The outer vector represents the layer parameters,
	// while the inner represents the layer itself. i.e., params[i][j]
	// is the ith parameter of the jth layer.
	std::vector< std::vector<Parameter> > params;
	std::vector<Parameter> extraParams;
	int layers;
	Model *model;
	bool bConstrain;

	paramStruct(Model *m) : model(m) {}
};

// The data structure representing an extra parameter specification
struct ExtraParam {
	bool isIntegral;    // True iff accepts only integer values
	int decimalPoints;  // Number of decimal points to show/set	
	
	bool isRanged;      // True iff the value has to be between 
	                    // rangeMin and rangeMax
	double rangeMin, rangeMax; 
						// NOTE: rangeMin and rangeMax may be -inf 
						// and inf respectively

	bool isAbsolute;    // Convenience setting so that negative values
						// will be automatically turned to positive

	bool canBeInfinite; // True iff value can be infinite

	std::string name;   // The display name of the parameter

	double defaultVal;  // The default value of the parameter

	// Constructor
	ExtraParam(std::string pName, double defval = 0.0, bool bInf = false, 
			   bool bAbs = false, bool bRange = false, double minval = 0.0, 
			   double maxval = 0.0, bool bInt = false, int decPoints = 6) : 

		isIntegral(bInt), decimalPoints(decPoints), isRanged(bRange),
		rangeMin(minval), rangeMax(maxval), isAbsolute(bAbs), 
		canBeInfinite(bInf), name(pName), defaultVal(defval) {
			if(bInt)
				decimalPoints = 0;
		}
};


//////////////////////////////////////////////////////////////
/////////////////// Model Abstract Class /////////////////////
//////////////////////////////////////////////////////////////

// An abstract class presenting a computable model
class EXPORTED Model {
////////////////////////////////////////////////////
//for debug
//#ifdef _DEBUG
public:
std::string debugMatrixPrintM(MatrixXd a) {
	std::stringstream s;
	s << "\t";
	for(int j = 0; j < a.cols(); j++) 
		s << "[" << j << "]\t";
	s << "\n";
	for(int i = 0; i < a.rows(); i++){
		s << "[" << i << "]\t";
		for (int j = 0; j < a.cols(); j++) 
			s << a(i,j) << "\t";
		s << "\n";
	}
	return s.str().c_str();
}

std::string debugParamStruct(paramStruct p) {
	std::stringstream s;
	
	if(p.params.size() < 1)
		s << "There are no parameters in the params vector.\n";
	else {
		s << "\t";
		for(int i = 0; i < (int)p.params[0].size(); i++)
			s << "[" << i << "]\t";
		s << "\n";
		for(int i = 0; i < (int)p.params[0].size(); i++) {
			s << "[" << i << "]\t";
			for(int j = 0; j < p.layers; j++)
				s << p.params[i][j].value << "\t";
			s << "\n";
		}
		s << "\n";
	}

	// Extra parameters
	if(p.extraParams.size() < 1)
		s << "There are no extra parameters paramStruct.\n";
	else {
		for(int i = 0; i < p.layers; i++)
			s << p.extraParams[i].value << "\n";
	}

	return s.str().c_str();
}

std::string debugModelParams() {
	std::stringstream s;
	if(parameters)
		s << "Main parameters\n" << this->debugMatrixPrintM(*parameters);
	if(extraParams)
		s << "\nExtra Parameters\n" << this->debugMatrixPrintM(*extraParams);
	return s.str().c_str();
}

//#endif
//end for debug
////////////////////////////////////////////////////

protected:
	// A pointer to the name of GPU kernel that calculates
	// the model, if applicable
	const char *GPUKernel;

	// The number of parameters per layer
	int nLayerParams;

	// Minimal and maximal amount of layers (if maxLayers is -1, layers can be
	// infinite)
	int minLayers, maxLayers;

	// The number of extra parameters
	int nExtraParams;

	// The electron density profile specifier
	EDProfile profile;

	// The number of displayed parameters
	int displayParams;

	// The global "should we stop" variable
	int *pStop;

	// The display name of this model
	std::string modelName;

	//a Matrix and a vector that contain the parameter structure in a logical way.
	MatrixXd *parameters;
	VectorXd *extraParams;

	// A flag indicating whether or not a coarse parallelization is possible
	// Override CalculateVector to set as false
	bool bParallelizeVector;
	
	Model(std::string name = "Abstract Model - DO NOT USE", 
		  int extras = 2, int nlp = 2, int minlayers = 2, 
		  int maxlayers = -1, EDProfile edp = EDProfile(),
		  int disp = 0);	
	
	// Numerical derivation helper function
	VectorXd derF(const std::vector<double>& x, VectorXd& p, int nLayers,
				  int ai, double h, double m);
public:

	// Destructor
	virtual ~Model();

	///// Get/Set Methods

	// Returns this model's display name
	virtual std::string GetName();

	// Returns the minimal amount of layers for this model
	virtual int GetMinLayers();

	// Returns the maximal amount of layers for this model
	virtual int GetMaxLayers();
	
	// Returns the number of layer parameters
	virtual int GetNumLayerParams();

	// Returns the number of extra parameters
	virtual int GetNumExtraParams();

	// Returns the number of related model types (i.e., with different
	// ED profiles)
	virtual int GetNumRelatedModels();

	// Returns the number of base models included in this model (for
	// composite models, polydisperse models, etc.)
	virtual int GetNumBaseModels();

	// If index is out of bounds (bounds: [0,nLayerParams)), returns N/A.
	virtual std::string GetLayerParamName(int index);

	// If index is out of bounds (bounds: [0,infinity)), returns N/A.
	// Usually returns "Solvent" or "Layer #"
	virtual std::string GetLayerName(int layer);

	// Returns the requested extra parameter's specifications, when index
	// is out of bounds, returns a parameter with name N/A.
	virtual ExtraParam GetExtraParameter(int index);

	// Returns the related model type name
	virtual std::string GetRelatedModelName(int index);

	// Instantiates a new Model of a related type (see GetNumRelatedModels)
	virtual Model *CreateRelatedModel(int index);

	// Returns the base model included in this model (for
	// composite models, polydisperse models, etc.) or NULL if index is 
	// incorrect
	virtual Model *GetBaseModel(int index);

	// Returns the default value of a layer parameter by its index and layer
	// (spanning from 0 to NumParamLayers)
	virtual double GetDefaultParamValue(int paramIndex, int layer);

	// Returns false iff the layer and layer parameter index are not 
	// applicable
	virtual bool IsParamApplicable(int layer, int lpindex);
	
	// Returns true iff the model is layer-based, capable of having an electron
	// density profile
	virtual bool IsLayerBased();

	// Set the global pointer that is set when stop is requested
	virtual void SetStop(int *stop);

	// Returns the electron density profile specification
	virtual EDProfile GetEDProfile();

	// Sets a new electron density profile
	virtual void SetEDProfile(EDProfile edp);

	// Returns the number of displayed parameters
	virtual int GetNumDisplayParams();

	// Returns the title of the displayed parameter
	virtual std::string GetDisplayParamName(int index);

	// Returns true if the calculation of the vector is parallelizable
	virtual bool ParallelizeVector();

	// Returns the value of the displayed parameter, according to the
	// current parameters of the model
	virtual double GetDisplayParamValue(int index, const paramStruct *p);

	///// Calculation Methods

	// Organize parameters from the parameter vector into the matrix and vector defined earlier.
	virtual void OrganizeParameters(const VectorXd& p, int nLayers);

	// Called before each series of q-calculations
	virtual void PreCalculate(VectorXd& p, int nLayers);

protected:
	// Calculate the model's intensity for a given q
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd()) = 0;

	// Calculate an entire vector using a GPU, if applicable
	virtual VectorXd GPUCalculate(const std::vector<double>& q,int nLayers, VectorXd& p = VectorXd());

public:

	// Calculate the model's intensity for a given q; used for live generation
	virtual double LiveCalculate(double q, int nLayers, VectorXd& p = VectorXd());

	// Calculates an entire vector. Default is in parallel using OpenMP,
	// or a GPU if possible
	virtual VectorXd CalculateVector(const std::vector<double>& q, int nLayers, VectorXd& p = VectorXd());

	// Computes the derivative of the model on an entire vector. Default
	// is numerical derivation (may use analytic derivation)
	virtual VectorXd Derivative(const std::vector<double>& x, VectorXd param, 
								int nLayers, int ai);

};

//////////////////////////////////////////////////////////////
/////////////////// Special Model Types //////////////////////
//////////////////////////////////////////////////////////////

// A simple model represented only by a function
class EXPORTED FunctionModel : public Model {
protected:
	VectorXd parVec;	// BG funcs need to save the parameters to the obj
	typedef double (*modelFunction)(double q, VectorXd& p, int ma, int nd);
	modelFunction modelf;

public:
	FunctionModel(modelFunction f, int extras = 2, int nlp = 2) : 
				Model("Functional Model", extras, nlp), modelf(f) {
	}

	virtual void PreCalculate(VectorXd &p, int nLayers) {
		parVec = p;
	}

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p ) {
		return modelf(q, parVec, parVec.size(), nLayers);
	}
};

/*
Model *model = new SomeModel(...);
for(int i = 0; i < N; i++)
	model = new PolydisperseModel(model, pdIndex[i], sigma[i]);
*/

// A special model representing a model which is polydisperse in one 
// parameter. This class may be nested in itself for N polydisperse
// parameters.
class EXPORTED PolydisperseModel : public Model {
protected:
	Model *model;
	double polySigma;
	int polyInd;
	bool bDeleteInner;
	paramStruct p;
public:
	PolydisperseModel(Model *m, int index, double sigma, paramStruct& bigP,
					  bool deleteInnerModel = false) : 
	  Model("Polydisperse Model"), model(m), polyInd(index), polySigma(sigma),
	  p(bigP), bDeleteInner(deleteInnerModel) {
	  bParallelizeVector = false;
	  }

	virtual ~PolydisperseModel() { if(model && bDeleteInner) delete model; }

	virtual double LiveCalculate(double q, int nLayers, VectorXd& p);

	virtual int GetNumBaseModels();

	virtual Eigen::VectorXd CalculateVector(const std::vector<double>& q, int nLayers, VectorXd& p = VectorXd());
	
	virtual Model *GetBaseModel(int index);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p);
};


//////////////////////////////////////////////////////////////
/////////////////// Form Factor Models ///////////////////////
//////////////////////////////////////////////////////////////

// Performs orientation average on a given model for a given q
class FFModel;
double OrientationAverage(double q, FFModel *model, int nLayers, VectorXd& p);

// A model containing a form factor
class EXPORTED FFModel : public Model {
protected:
	FFModel(std::string name = "Abstract FF Model - DO NOT USE",
		int extras = 2, int nlp = 2, int minlayers = 2, int maxlayers = -1,
		EDProfile edp = EDProfile()) : 
			Model(name, extras, nlp, minlayers, maxlayers, edp) {}

	// Calculate the model's intensity for a given q. Default
	// is numerical orientation average of the |FF|^2.
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd()) {
		return OrientationAverage(q, this, nLayers, p);
	}

public:

	// Called before each series of q-calculations
	virtual void PreCalculateFF(VectorXd& p, int nLayers) {}

	

	// Calculate the model's form factor for a given q vector = 
	// (qx,qy,qz) in cartesian coordinates. 
	// Because many times the FFCalculate contains a Dirac Delta Function, we need to 
	// take it into account: after discretisation: w - is the wheight of the point at
	// the integral in which the function is used and precision is the percision of the delta function
	virtual std::complex<double> CalculateFF(Vector3d qvec, 
											 int nLayers, double w = 1.0, double precision = 1E-5, VectorXd& p = VectorXd()) = 0;

	// Returns true iff this form factor has a special structure
	// factor function
	virtual bool HasSpecializedSF() { return false; }

	// Returns a special structure factor function (Model),
	// such as Caille in membranes/slabs
	virtual Model *GetSpecializedSF() { return NULL; }

	// Returns true iff this model takes a lot of time to calculate
	virtual bool IsSlow();


	///// Miscellaneous Methods

	// Draws the OpenGL preview model
	virtual void DrawOpenGLPreview(const paramStruct& p) {}

	//draws the 3d model on the FF window
	virtual void DrawPreviewScene() {}


};

// A special model representing a composition of non-overlapping models
class EXPORTED CompositeModel : public FFModel {
protected:
	// The model objects
	FFModel **models;

	// Relative distance vectors from the center of mass of a model to that 
	// of the first model
	Vector3d *dist;
	// Rotation matrix of object relative to original orientation in which
	// it was calculated
	Matrix3d *rot;
	
	// The amount of models in the composition
	int modelCount;

	// the amount of parameter for each model. arranged in the order of the *models.
	int *modelsParamCount;

	//layers per model 
	int *modelsLayerCount;
		
		

public:
	// Think how to add distances to p (so they can be mutable)
	CompositeModel(FFModel **modelObjects, Vector3d *distances, 
		Matrix3d *rotations, int numModels,
		int *numModelsParam, int *numLayersParam );

	~CompositeModel();

	virtual int GetNumBaseModels();

	virtual Model *GetBaseModel(int index);

	virtual std::complex<double> CalculateFF(Vector3d qvec, VectorXd& p,
											 int nLayers, double w = 1.0 , 
											 double precision = 1E-5);
};
/*
// Avi:	This should have been a (relatively simple task. However, it should actually be
//		a simple version of a composite model, as there can be N different BG functions
//		contributing to the total BG.
class EXPORTED BGModel : public Model {
protected:
	BGModel(std::string name = "Abstract BG Model - DO NOT USE",
		int nlp = 4, int minlayers = 1, int maxlayers = 1,
		int extras = 0, EDProfile edp = EDProfile(NONE)) :
			Model(name, extras, nlp, minlayers, maxlayers, edp) {}
public:

	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd()) = 0;

	virtual bool IsSlow() { return false; }

	virtual Model::

};
*/

#endif
