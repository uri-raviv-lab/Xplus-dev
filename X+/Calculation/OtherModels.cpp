#include "OtherModels.h"

#include "mathfuncs.h" // For bessel functions and square

MicroemulsionModel::MicroemulsionModel(std::string st) : FFModel(st, 5, 0, 0, 0, EDProfile(NONE)) {
	displayParams = 3;
}

bool MicroemulsionModel::IsLayerBased() {
	return false;
}

double MicroemulsionModel::Calculate(double q, int nLayers, Eigen::VectorXd &p) {
	//roi's microemulsion model
	//ma - size of a
	//a[0] and a[1] are the solvent parameters and are not used in this model
	//a[2] - Izero - Adjustable parameter, sharpens or broadens the peak (Theoretically the
	//intensity at q=0.
	//a[3] - Imax - Intensity at the top of experimental SAXS graph peak
	//a[4] - qmax - q of maximum intensity
		
	double res = Izero /((1.0 - Izero / Imax) * (q * q / (qmax * qmax) - 1) * 
					(q * q / (qmax * qmax) - 1) + Izero / Imax);

	res *= (*extraParams)(0);	// Multiply by scale
	res += (*extraParams)(1);	// Add background

	return res;
}

ExtraParam MicroemulsionModel::GetExtraParameter(int index) {
	switch(index) {
		case 2:
			return ExtraParam("I(0)", 10.0, false, true);
		case 3:
			return ExtraParam("I(max)", 20.0, false, true);
		case 4:
			return ExtraParam("q(max)", 1.0, false, true);
		default:
			return FFModel::GetExtraParameter(index);
	}
}

std::string MicroemulsionModel::GetDisplayParamName(int index) {
		switch(index) {
			case 0:
				return "Domain Size";
			case 1:
				return "Correlation Length";
			case 2:
				return "Gradient Coefficient";
			default:
				return "N/A";
		}
}

double MicroemulsionModel::GetDisplayParamValue(int index, const paramStruct *p) {
	double c2a2, c1a2, Izero, Imax, qmax, e, d;
	qmax	= p->extraParams[4].value;
	Izero	= p->extraParams[2].value;
	Imax	= p->extraParams[3].value;

	c2a2 = 1.0 /(qmax * qmax * qmax * qmax) * (1.0 - Izero / Imax);
	c1a2 = -2.0 / (qmax * qmax) * (1 - Izero / Imax);
	e    = 1 / sqrt(0.5 * sqrt(1 / c2a2) - 0.5 * qmax * qmax);
	d    = 2 * PI / sqrt(0.5 * sqrt(1 / c2a2) + 0.5 * qmax * qmax);

	switch (index) {
		case 0:	//Domain Size
			return d;
		case 1:	//Correlation Length
			return e;
		case 2:	//Gradient Coefficient
			return c1a2;
		default:
			return -1.0;
	}
}

void MicroemulsionModel::OrganizeParameters(const Eigen::VectorXd &p, int nLayers) {
	Model::OrganizeParameters(p, nLayers);

	Izero	= (*extraParams)(2);
	Imax	= (*extraParams)(3);
	qmax	= (*extraParams)(4);
}

void MicroemulsionModel::PreCalculate(Eigen::VectorXd &p, int nLayers) {
	OrganizeParameters(p, nLayers);
}
