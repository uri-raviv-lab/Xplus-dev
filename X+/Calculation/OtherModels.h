#ifndef __OTHER_MODELS_H
#define __OTHER_MODELS_H

#include "Model.h"

class MicroemulsionModel : public FFModel {
protected:
	double Izero, Imax, qmax; 

public:
	MicroemulsionModel(std::string st = "Micromulsion");

	virtual std::string GetDisplayParamName(int index);

	virtual double GetDisplayParamValue(int index, const paramStruct *p);

	virtual ExtraParam GetExtraParameter(int index);

	virtual bool IsLayerBased();

	virtual void OrganizeParameters(const Eigen::VectorXd &p, int nLayers);

	virtual void PreCalculate(VectorXd& p, int nLayers);

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w = 1.0, double precision = 1E-5, VectorXd& p = VectorXd()) {
			return std::complex<double>(0.0, 1.0);
	}

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p = VectorXd() );


};

#endif
