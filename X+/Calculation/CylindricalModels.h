#ifndef __CYLINDRICAL_MODELS_H
#define __CYLINDRICAL_MODELS_H

#include "Model.h"

class CylindricalModel : public FFModel {
protected:
	int steps;

	double edSolvent; 
	VectorXd ed;	// The electron density
	VectorXd t;		// The width of the layers

	VectorXd xx, ww;
public:
	CylindricalModel(int integralSteps = 1000, std::string str = "Abstract Cylindrical Model", ProfileShape eds = DISCRETE, int nlp = 2);

	virtual ExtraParam GetExtraParameter(int index);

	virtual int GetNumRelatedModels();
	
	virtual std::string GetRelatedModelName(int index);

	virtual Model *CreateRelatedModel(int index);

	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void DrawPreviewScene();

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd()) = 0;

};


class UniformHCModel : public CylindricalModel {
public:
	UniformHCModel(int integralSteps = 1000);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
											 int nLayers, double w, double precision, VectorXd& p = VectorXd()  );
protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd());

};

class GaussianHCModel : public CylindricalModel {
protected:
	VectorXd r;	// The center of the Gaussian relative to the center of the cylinder
	int steps1, nonzero;
	double H;
	MatrixXd xxR, wwR;

public:
	GaussianHCModel(int heightSteps = 1000, int radiiSteps = 500);

	virtual bool IsSlow();

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual std::string GetLayerParamName(int index);
	
	virtual bool IsParamApplicable(int layer, int lpindex);

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
											 int nLayers, double w, double precision, VectorXd& p = VectorXd()  );

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd());
};

/////////////////////////
// Cylindroid Model(s) //
/////////////////////////
class CylindroidVaryingEcc;
class Cylindroid : public FFModel {
protected:
	int steps, steps1, thetaSteps, nonzero;

	bool bSwitch;
	double edSolvent, b1, h; 
	VectorXd ed;	// The electron density
	VectorXd r;		// The radii of the layers
	VectorXd b;		// The short radii of the layers
	VectorXd eps;	// The eccentricities of the layers

	VectorXd xIn, xOut, wIn, wOut, xThetaQ, wThetaQ, cosInner;
public:
	Cylindroid(int integralSteps = 1000, std::string str = "Conservered Eccentricity Cylindroid", ProfileShape eds = DISCRETE, int nlp = 2);

	virtual ExtraParam GetExtraParameter(int index);

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual int GetNumRelatedModels();
	
	virtual std::string GetRelatedModelName(int index);

	virtual Model *CreateRelatedModel(int index);

	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void DrawPreviewScene();

	virtual bool IsSlow();

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
											 int nLayers, double w, double precision, VectorXd& p = VectorXd()  );

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd());
};

class CylindroidVaryingEcc : public Cylindroid {
public:
	CylindroidVaryingEcc(int integralSteps = 1000, std::string str = "Non-conservered Eccentricity Cylindroid", ProfileShape eds = DISCRETE, int nlp = 2);

	virtual void PreCalculate(VectorXd& p, int nLayers);
};


#endif
