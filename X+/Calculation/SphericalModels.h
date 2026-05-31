#ifndef __SPHERICAL_MODELS_H
#define __SPHERICAL_MODELS_H

#include "Model.h"

// The abtract class that the Uniform and Gaussian electron densities hsould inherit from
class SphericalModel : public FFModel {
protected:
	double edSolvent; 
	VectorXd r;		// Radii
	VectorXd ED;	// Electron density

public:
	SphericalModel(std::string st = "Spherical model - do not use", int nlp = 2, ProfileShape edp = DISCRETE, int exParams = 2);

	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::string GetLayerParamName(int index);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual int GetNumRelatedModels();
	
	virtual std::string GetRelatedModelName(int index);

	virtual Model *CreateRelatedModel(int index);

	virtual void DrawPreviewScene();	// For OpeningWindow

	virtual void DrawOpenGLPreview(const paramStruct &p);	// For FormFactor

protected:
	virtual double Calculate(double q, int nLayers, Eigen::VectorXd &p = VectorXd()) = 0;	// Ensure that this class cannot be used

};

class UniformSphereModel : public SphericalModel{

public:
	UniformSphereModel(std::string st = "Uniform Sphere");

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Eigen::Vector3d qvec, int nLayers, double w, double precision, VectorXd& p = VectorXd());

protected:
	virtual double Calculate(double q, int nLayers, Eigen::VectorXd &p = VectorXd());
};

class GaussianSphereModel : public SphericalModel{
protected:
	VectorXd z0;		// Distance from center of the sphere
	VectorXd xx, ww;	// For the numerical integration
	int steps;

public:
	GaussianSphereModel(std::string st = "Gaussian Sphere", ProfileShape edp = GAUSSIAN);
	
	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::string GetLayerParamName(int index);

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Eigen::Vector3d qvec, int nLayers, double w, double precision, VectorXd& p = VectorXd());

protected:
	virtual double Calculate(double q, int nLayers, Eigen::VectorXd &p = VectorXd());

};

class SmoothSphereModel : public SphericalModel{
protected:
	VectorXd slope, width;		//The slope of the end of the layer	
	VectorXd xx, ww, LayersSum;	// For the numerical integration
	int steps;
	double rMax;

public:
	
	SmoothSphereModel(std::string st = "Smooth Sphere", ProfileShape edp = TANH);
	
	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::string GetLayerParamName(int index);

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Eigen::Vector3d qvec, int nLayers, double w, double precision, VectorXd& p = VectorXd());

	virtual ExtraParam GetExtraParameter(int index);

protected:
	virtual double Calculate(double q, int nLayers, Eigen::VectorXd &p = VectorXd());
};


#endif