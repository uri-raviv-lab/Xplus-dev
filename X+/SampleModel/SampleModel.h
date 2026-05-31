#ifndef __SAMPLE_MODEL_H
#define __SAMPLE_MODEL_H

#include <complex>
#include <string>

#define EXPORTER
#include "Z:\Homes\noam\NOAM-X-plus-real\r3\X+\Calculation\ModelContainer.h"

class SampleModel : public FFModel {
public:
	SampleModel();

	virtual std::complex<double> CalculateFF(Vector3d qvec, 
		int nLayers, double w, double precision, VectorXd& p = VectorXd()  );

	virtual ExtraParam GetExtraParameter(int index);

	virtual int GetNumDisplayParams();
	virtual std::string GetDisplayParamName(int index);
	virtual double GetDisplayParamValue(int index, const paramStruct *p);

	virtual void DrawOpenGLPreview(const paramStruct& p);

	virtual void DrawPreviewScene();

protected:
	virtual double Calculate(double q, int nLayers, VectorXd& p);
};



#endif
