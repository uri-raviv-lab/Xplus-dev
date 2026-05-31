#include "PopModel.h"


PopModel::PopModel(const ArrayXXd &dataX, const ArrayXXd &dataY, string express, ArrayXd &coefficients): 
		Model(express, 0, 1, 1, 26)
{
	expr = express;
	xx = dataX;
	curves = dataY;
	coeffs = &coefficients;
	ps = new ParserSolver(errs, expr, xx, curves, *coeffs);
	y = ps->EvaluateExpression(x);
}

PopModel::~PopModel() {
	if(ps)
		delete ps;
}

void PopModel::OrganizeParameters(const VectorXd& p, int nLayers) {
}

void PopModel::PreCalculate(VectorXd &p, int nLayers) {
//	coeffs = &(p);
}

void PopModel::PreCalculate(ArrayXd &p, int nLayers) {
	coeffs = &(p);
}

double PopModel::Calculate(double q, int nLayers, VectorXd& p) {
	return 0.0;
}

VectorXd PopModel::CalculateVector(const std::vector<double> &q, int nLayers, VectorXd &p) {
	ArrayXd dummy = p.array();
	return CalculateExp(dummy).matrix();
}

void PopModel::GetModXY(ArrayXd& xC, ArrayXd& yC) {
	yC = ps->InterpolateCurve(xC, yC);
	xC = x;
}

ArrayXd PopModel::CalculateExp() {
	return ps->CalculateExpression();
}

ArrayXd PopModel::CalculateExp(ArrayXd& coef) {
	ChangeCoefficients(coef);
	return CalculateExp();
}

void PopModel::ChangeCoefficients(ArrayXd& coef) {
	this->coeffs = &coef;
	this->ps->ReplaceCoefficients(coef);
}

VectorXd PopModel::Derivative(const std::vector<double>& x, VectorXd param, int nLayers, int ai) {
	if(ps->CanExactDerive()) {
		return ps->Derive('a' + ai).matrix();
	}
	double h = 1.0e-9;

	// f'(x) ~ [f(x-2h) - f(x+2h)  + 8f(x+h) - 8f(x-h)] / 12h
	VectorXd av, bv, cv, dv;

	av = derF(x, param, nLayers, ai, -2.0 * h, 1.0 / (12.0 * h));
	bv = derF(x, param, nLayers, ai, h, 8.0 / (12.0 * h));
	cv = derF(x, param, nLayers, ai, -h, -8.0 / (12.0 * h));
	dv = derF(x, param, nLayers, ai, 2.0 * h, -1.0 / (12.0 * h));

	return (av + bv + cv + dv);

}
