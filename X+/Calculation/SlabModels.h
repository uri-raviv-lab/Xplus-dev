#ifndef __SLAB_MODELS_H
#define __SLAB_MODELS_H

#include "Model.h"

class SlabModel : public FFModel {
protected:
	double edSolvent; 
	VectorXd ED;
	VectorXd width;

public:
	SlabModel(std::string st = "Slab model - do not use? WIP", EDProfile edp = EDProfile(SYMMETRIC, DISCRETE),
		int nlp = 2, int maxlayers = -1);

	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::string GetLayerParamName(int index);

	virtual std::string GetLayerName(int layer);

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual int GetNumRelatedModels();	

	virtual std::string GetRelatedModelName(int index);

	virtual Model *CreateRelatedModel(int index);

	virtual bool IsSlow() { return false; }

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Vector3d qvec,
		int nLayers, double w, double precision, VectorXd& p = VectorXd()) = 0;

	virtual void DrawOpenGLPreview(const paramStruct &p);	// For FormFactor

	virtual void DrawPreviewScene();

	virtual bool HasSpecializedSF() { return true; }

	virtual Model *GetSpecializedSF();

};

class UniformSlabModel : public SlabModel {
public:
	UniformSlabModel(std::string st = "Symmetric Uniform Slabs", ProfileType t = SYMMETRIC);


	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p = VectorXd());

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual void PreCalculateFF(VectorXd& p, int nLayers);

	//virtual VectorXd Derivative(const std::vector<double>& x, VectorXd param, 
	//							int nLayers, int ai);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );
};

class GaussianSlabModel : public SlabModel {
protected:
	VectorXd z;

public:
	GaussianSlabModel(std::string st = "Symmetric Gaussian Slabs", int maxlayers = -1, ProfileType t = SYMMETRIC);

	virtual int GetNumLayerParams();

	virtual std::string GetLayerParamName(int index);

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p = VectorXd());

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);
	
	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual void PreCalculateFF(VectorXd& p, int nLayers);

	virtual bool IsParamApplicable(int layer, int lpindex);

	//virtual VectorXd Derivative(const std::vector<double>& x, VectorXd param, 
	//							int nLayers, int ai);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );
};

class MembraneModel : public GaussianSlabModel {
public:
	MembraneModel();

	virtual std::string GetLayerName(int layer);

	virtual std::string GetDisplayParamName(int index);

	virtual double GetDisplayParamValue(int index, const paramStruct *p);

};

// This is here because it makes more sense than adding a new file
class CuboidModel : public FFModel {
protected:
	VectorXd xP, wP, xX, wX, sinP, cosP;
	int stepsP, stepsX;

	double ed;
	double w, h, d;

public:
	CuboidModel();

	virtual std::string GetLayerName(int layer);

	virtual bool IsSlow();

	virtual void DrawOpenGLPreview(const paramStruct &p);

	virtual void DrawPreviewScene();

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual std::string GetLayerParamName(int index);

	virtual void OrganizeParameters(const VectorXd &p, int nLayers);

	virtual void PreCalculate(VectorXd &p, int nLayers);

	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::complex<double> CalculateFF(Eigen::Vector3d qvec, int nLayers, double w = 1.0, double precision = 1E-5, Eigen::VectorXd& p = Eigen::VectorXd()) {
		return std::complex<double>(0.0, 1.0);
	}

protected:
	virtual double Calculate(double q, int nLayers, VectorXd &p = VectorXd());

};


class CailleModel : public Model {
protected:
	double eta, amp, N0, sig, NDiff, D, Q;
	VectorXd resF, qGlob;
	Eigen::Matrix2d ex1;

	//double cailleZhang(double q, double h, double N_diff);

public:
	CailleModel(std::string name = "Caille");// : Model(name, 6, 0, 0, 0, EDProfile(NONE), 0/*Change later if we want to add kappa*/) {}

	virtual Eigen::VectorXd CalculateVector(const std::vector<double>& q, int nLayers, VectorXd& p = Eigen::VectorXd());

	// The actual calculation of the model should be done here (the call to "cailleZhang")
	virtual void PreCalculate(Eigen::VectorXd &p, int nLayers);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual ExtraParam GetExtraParameter(int index);

protected:
	virtual double Calculate(double q, int nLayers, Eigen::VectorXd &p = Eigen::VectorXd());
};

#endif