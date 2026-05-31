#ifndef __HELICAL_MODELS_H
#define __HELICAL_MODELS_H

#include "Model.h"

class HelicalModel : public FFModel {
protected:
	double rHelix;	
	double P, edSolvent; 
	VectorXd delta;
	VectorXd deltaED;
	VectorXd rCs;

public:
	HelicalModel(std::string st = "Helical model - do not use", int extraParams = 5);

	virtual int GetNumRelatedModels();
	
	virtual std::string GetRelatedModelName(int index);

	virtual Model *CreateRelatedModel(int index);

	virtual ExtraParam GetExtraParameter(int index);

	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual bool IsLayerBased();

	virtual std::string GetLayerParamName(int index);

	virtual std::string GetLayerName(int layer);

	virtual double GetDefaultParamValue(int paramIndex, int layer);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Vector3d qvec,
		int nLayers, double w, double precision, VectorXd& p = VectorXd()) = 0;
											 
};



class HelixModel : public HelicalModel {
protected:
	int steps, steps1, subSteps;
	
	double height;	

	VectorXd xIn, wIn, xOut, wOut;
public:

	HelixModel(int integralStepsIn = 1000, int integralStepsOut = 1000 );

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p = VectorXd());

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);
	
	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void DrawPreviewScene();

	virtual bool IsSlow();
	
	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual void PreCalculateFF(VectorXd& p, int nLayers);

	virtual VectorXd Derivative(const std::vector<double>& x, VectorXd param, 
								int nLayers, int ai);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );

};

class DelixModel : public HelicalModel {
protected:
	VectorXd x, w, root, deltaz;
	int steps;
	double rHelix, P, edSolvent, deltaw, debyeWaller; 
	int Nb;

public:
	DelixModel(std::string st = "Discrete Helix", int step = 400);

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p = VectorXd());

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual ExtraParam GetExtraParameter(int index);
	
	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void DrawPreviewScene();

	virtual bool IsSlow();
	
	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual void PreCalculateFF(VectorXd& p, int nLayers);

	virtual VectorXd Derivative(const std::vector<double>& x, VectorXd param, 
								int nLayers, int ai);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );

};

class GaussianDelixModel : public DelixModel {
public:
	GaussianDelixModel(std::string st = "Gaussian Discrete Helix", int step = 400);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );
};

#endif