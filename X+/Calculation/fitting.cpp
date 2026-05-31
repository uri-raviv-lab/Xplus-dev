#include <cstdlib>
#include <limits>
#include <sstream>
#include <iostream>	// cout
#include <stdio.h>	// fprintf

#include "fitting.h"

#include <Eigen/LU>
#include <Eigen/SVD>

using namespace Eigen;

#include "Model.h"
#include "models.h" // For pStop
#include "globalsettings.h"
#include "mathfuncs.h"

// To fix the std::numeric_limits<double>::min/max
#undef min
#undef max

/*#define NOMINMAX
#include <windows.h>*/ //USE FOR MESSAGEBOX TESTING

void LMFitter::calcErrors() {
	
	//VectorXd tmp = alpha.inverse().diagonal();
	lambda = 0.0;

	FitIteration();

	MatrixXd bl, bltmp(J.rows(), mutables);
	VectorXd blah;
	VectorXd tmp = alpha.diagonal();

	err->clear();

	for(int i = 0; i < tmp.size(); i++) 
		err->push_back(sqrt(fabs(tmp(i))));

	if(errY) {
		int j = 0;
		for (int i = 0; i < J.cols(); i++) {
			if(!J.col(i).isZero())
				bltmp.col(j++) = J.col(i).eval();
		}

		errY->clear();

		bl = bltmp * alpha * bltmp.transpose();
		blah = bl.diagonal();

		for(int i = 0; i < blah.size(); i++)
			errY->push_back(sqrt(fabs(blah(i))));
	}

	
}

double LMFitter::FitIteration() {
	VectorXd curParams, curBeta;
	MatrixXd curAlpha;

	if(mutables == 0 || GetError())
		return 0.0;

	// We need to solve the linear equation system curAlpha*delta = curBeta, where:
	// alpha = J'J                       where J is the Jacobian Matrix and J' is its transpose
	// curAlpha = J'J + lambda*diag(J'J)
	// curBeta  = J'(y-f(b))             where b is the initial guess vector, and y is the data

    curAlpha = alpha;
	curAlpha.diagonal() = alpha.diagonal() * (1.0 + lambda);
       
	curBeta = beta;


	// Solving the equations using SVD and Moore-Penrose Pseudoinverse
	if(isAccurateFitting()) {
		try {
			// I don't know if using ComputeFullU/V would be better 
			JacobiSVD<MatrixXd> svd(curAlpha, ComputeThinU  | ComputeThinV );

			MatrixXd sigma = MatrixXd::Zero(mutables, mutables);
			sigma.diagonal() = svd.singularValues();
			
			for(int i = 0; i < mutables; i++)
				if(sigma(i, i) != 0.0)
					sigma(i, i) = 1.0 / sigma(i, i);

			curAlpha = svd.matrixV() * sigma * svd.matrixU().transpose();
			curBeta = curAlpha * curBeta;
		} catch(...) {
			// Invalid matrix values
			error = true;
			return -1.0;
		}

	} else { // Solving the equations using LU decomposition
		// This currently requires using Eigen 2.0 (precompile define).
		// See http://eigen.tuxfamily.org/dox/Eigen2ToEigen3.html for ideas on how to upgrade
		FullPivLU<MatrixXd> lu(curAlpha);

		if (lu.isInvertible()) {
			VectorXd sol = lu.solve(curBeta);
			curAlpha = lu.inverse();
			curBeta = sol;
		}
		else {
			lambda *= 10.0;
			return curWssr;
		}
	}

	// Testing the results of the equation solution
	curParams = params;
	
	int j = 0;
    for (int i = 0; i < nParams; i++)
        if (paramMut[i]) 
			curParams(i) = params(i) + curBeta(j++);

	if(fabs(lambda) < 1e-14)
		alpha = curAlpha;

	double wssr = CalculateCoefficients(curParams, curAlpha, curBeta);

	// If better, keep them, otherwise, don't, and increase damping factor
    if (wssr < curWssr && wssr >= 0.0) {
        lambda *= 0.1;
        curWssr = wssr;

		alpha = curAlpha;
        beta = curBeta;
        params = curParams;
    } 
    else
		lambda *= 10.0;

	return curWssr;
}

double LMFitter::CalculateCoefficients(VectorXd& p, MatrixXd& alphaMat,
                                       VectorXd& betaVec) {
    double wssr = 0.0, mean = 0.0, sstotal = 0.0;
    
	VectorXd dy = VectorXd::Zero(x.size());
    MatrixXd dyda = MatrixXd::Zero(x.size(), nParams);

    betaVec.setZero();
    
    for (int i = 0; i < mutables; i++) 
        for (int j = 0; j <= i; j++) 
            alphaMat(i, j) = 0.0;

	// Link the Linked parameters 
	if (p_max) {
		for(int b = 0; b < p_max->link.size(); b++) {
			if(p_max->link[b] >= 0 && p_max->link[b] != b)
				p(b) = p(p_max->link[b]);
		}
	}
    
    // Constraints ("Penalty Method")
    for (int k = 0; k < nParams; k++) {
		if (p_min && p_max) {
			if (((p(k) < 0.0 && p_min->num[k] >= 0) ||
				(p_min->num[k] > p(k)) ||
				(p_max->num[k] < p(k)) ||
				(p_min->index[k] >= 0 && p(p_min->index[k]) > p(k)) ||
				(p_max->index[k] >= 0 && p(p_max->index[k]) < p(k))) && paramMut[k]) {
				return -1.0;
			}
		}
            
    }

	std::vector <double> ResY = MachineResolution(x, y, GetResolution());

    // Calculating the R-Squared coefficients
    if(!isWSSRFitting()) {
		mean = Mean(ResY);	

		for(int i = 0; i < (int)y.size(); i++)
			sstotal += (ResY[i] - mean) * (ResY[i] - mean);
	
    }

	// Create copies for the parameter vector and the number of layers for
	// this iteration
	VectorXd guess = p;
	int guessLayers = nLayers;

	// The new layer model is created based on the Electron Density Profile
	EDPFunction *edp = FitModel->GetEDProfile().func;
	if(FitModel->IsLayerBased() && edp && x.size() > 1)
		guess = edp->ComputeParamVector(FitModel, p, x, nLayers, guessLayers);
	// END of profile reshaping
    
    FitModel->PreCalculate(guess, guessLayers);

	/*if(edp)
		MessageBoxA(NULL, FitModel->debugModelParams().c_str(), "Hey!", NULL);*/
    
    int size = x.size();

	if(GetPeakType() == SHAPE_CAILLE) 
		SetX(x);
    
    // 1st tier of parallelization

	VectorXd pDummy = FitModel->CalculateVector(x, guessLayers, guess);

    for (int i = 0; i < size; i++) {
        double cury;
		if(error)
			continue;
        
        if(pStop && *pStop) {
			error = true;
			continue;
		}
		
		cury = pDummy(i) * mult[i] + add[i];
		if(cury != cury) {
	        //MessageBox::Show("Error in model calculation", "Calculation Error");
		    error = true;
			continue;
		}

		if(isLogFitting())
			cury = log10(cury);
        
		dy(i) = ResY[i] - cury;
    }

    // Calculate trial alpha and beta coefficients according to the
    // gradient
    int j = 0;
    for (int l = 0; l < nParams; l++) {
        if (paramMut[l]) {
			// Partial derivative calculation
			dyda.col(l) = FitModel->Derivative(x, p, nLayers, l);

            for(int i = 0; i < size; i++) {
                
				double wt = dyda(i, l) / sqWeights[i];
                
                int k = 0;
                for (int m = 0; m <= l; m++) {
                    if(paramMut[m]) { 
                        alphaMat(j, k) += wt * dyda(i, m); 
                        k++;
                    }
                }
				betaVec(j) += dy(i) * wt;
            }
            j++;
        }
    }
    
	J = dyda;

    // Makes matrix symmetric
    for (int i = 1; i < mutables; i++)
        for (int k = 0; k < i; k++) 
            alphaMat(k, i) = alphaMat(i, k);

    // Calculate WSSR
	mse = 0.0;
	for(int i = 0; i < size; i++) {
		mse += (dy(i) * dy(i));
		if(isWSSRFitting())
			wssr += (dy(i) * dy(i)) / sqWeights[i];
        else
			wssr += (dy(i) * dy(i))  / sstotal;
	}
    mse /= double(size);

	interimResY = VectorXd::Zero(ResY.size());
	for(int w = 0; w < int(ResY.size()); w++)
		interimResY(w) = ResY[w] - dy(w);
    return wssr;
}

double bessel_j0(double x)
{
    double ax,z;
    double xx,y,ans,ans1,ans2;
    if ((ax=fabs(x)) < 8.0) {
        y=x*x;
        ans1=57568490574.0+y*
            (-13362590354.0+y*(651619640.7
                               +y*(-11214424.18+y*
                                   (77392.33017+y*(-184.9052456)))));
        ans2=57568490411.0+y*
            (1029532985.0+y*(9494680.718
                             +y*(59272.64853+y*(267.8532712+y*1.0))));
        ans=ans1/ans2;
    }
    else {
        z=8.0/ax;
        y=z*z;
        xx=ax-0.785398164;
        ans1=1.0+y*(-0.1098628627e-2+y*
                    (0.2734510407e-4
                     +y*(-0.2073370639e-5+y*0.2093887211e-6)));
        ans2 = -0.1562499995e-1+y*
            (0.1430488765e-3
             +y*(-0.6911147651e-5+y*(0.7621095161e-6
                                     -y*0.934935152e-7)));
        ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
    }
    
    return ans;
}

double bessel_j1(double x)
{
	double ax,z;
	double xx,y,ans,ans1,ans2;

	if ((ax=fabs(x)) < 8.0)
	{ y=x*x;
	ans1=x*(72362614232.0+y*(-7895059235.0+y*(242396853.1
		+y*(-2972611.439+y*(15704.48260+y*(-30.16036606))))));
	ans2=144725228442.0+y*(2300535178.0+y*(18583304.74
		+y*(99447.43394+y*(376.9991397+y*1.0))));
	ans=ans1/ans2;
	}

	else {
		z=8.0/ax;
		y=z*z;
		xx=ax-2.356194491;
		ans1=1.0+y*(0.183105e-2+y*(-0.3516396496e-4
			+y*(0.2457520174e-5+y*(-0.240337019e-6))));
		ans2=0.04687499995+y*(-0.2002690873e-3
			+y*(0.8449199096e-5+y*(-0.88228987e-6
			+y*0.105787412e-6)));
		ans=sqrt(0.636619772/ax)*(cos(xx)*ans1-z*sin(xx)*ans2);
		if (x < 0.0)
			ans = -ans;
	}
	return ans;
}
double bessel_jn(int n, double x) {
	static const double ACC = 160.0;
	static const int IEXP = std::numeric_limits<double>::max_exponent/2;
	bool jsum;
	int j,k,m;
	double ax,bj,bjm,bjp,dum,sum,tox,ans;
	if (n==0) return bessel_j0(x);
	if (n==1) return bessel_j1(x);
	ax=fabs(x);
	if (ax*ax <= 8.0 * std::numeric_limits<double>::min()) return 0.0;
	else if (ax > double(n)) {
		tox=2.0/ax;
		bjm=bessel_j0(ax);
		bj=bessel_j1(ax);
		for (j=1;j<n;j++) {
			bjp=j*tox*bj-bjm;
			bjm=bj;
			bj=bjp;
		}
		ans=bj;
	} else {
		tox=2.0/ax;
		m=2*((n+int(sqrt(ACC*n)))/2);
		jsum=false;
		bjp=ans=sum=0.0;
		bj=1.0;
		for (j=m;j>0;j--) {
			bjm=j*tox*bj-bjp;
			bjp=bj;
			bj=bjm;
			dum=frexp(bj,&k);
			if (k > IEXP) {
				bj=ldexp(bj,-IEXP);
				bjp=ldexp(bjp,-IEXP);
				ans=ldexp(ans,-IEXP);
				sum=ldexp(sum,-IEXP);
			}
			if (jsum) sum += bj;
			jsum=!jsum;
			if (j == n) ans=bjp;
		}
		sum=2.0*sum-bj;
		ans /= sum;
	}
	return x < 0.0 && (n & 1) ? -ans : ans;
}
