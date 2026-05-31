#include <cmath>
#include <cstdlib>
#include <complex>
#include <omp.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "Quadrature.h"
#include "models.h"
#include "fitting.h"


#include <limits>
#include "mathfuncs.h"
using std::complex;

bool bPrecalculate = true;
bool bLastQ = false;
int *pStop = NULL;


//for debug
std::string debugMatrixPrintM(MatrixXd a) {
	std::stringstream s;
	s<< "\t";
	for (int j = 0; j<a.cols(); j++) 
		s<<"["<<j<<"]\t";
	s<<"\n";
	for (int i = 0; i< a.rows(); i++){
		s<<"["<<i<<"]\t";
		for (int j = 0; j<a.cols(); j++) 
			s<<a(i,j)<<"\t";
		s<<"\n";
	}
	return s.str().c_str();
}

void SetSignal(int *pSignal) {
    pStop = pSignal;
}

void ClearSignal() {
    pStop = NULL;
}

void LastQ() {
	bLastQ = true;
}

std::complex <double> erf( std::complex <double> z) {
	double a = z.real();
	double b = z.imag();
	std::complex <double> im(0.0, 1.0), result (0.0, 0.0);
	if(fabs (a) < 1e-11) {
		int resolution = 1000 * int(fabs(b) + 1);
		double dx =  b / double(resolution); 
		for (int i = 0; i < resolution; i++) {
			double x =  double(i) * dx;
			result += dx * exp( x * x ); 
		}

		return  result * im;
	}
	
	int resolution = 1000 * int(fabs(a) + 1);
	double dx =  a / double(resolution); 
	for (int i = 0; i < resolution; i++) {
		double x =  double(i) * dx;
		result += dx * exp( - x * x * (1.0 + im * b /a) * (1.0 + im * b /a)); 
	}

	return  result * (1.0 + im * b/a);
}


void GetRandED(VectorXd& a, VectorXd& r, VectorXd& ed, int nd) {
    for(int i = 0; i < nd; i++) {
        if(i == 0)
          r[i] = a[i];
        else
          r[i] = r[i - 1] + a[i];
         
          
        ed[i] = a[i + nd];   
    }       
}

void GetR_ZandED_Symmetric(VectorXd& a, VectorXd& r, VectorXd& ed, int nd, VectorXd& z) {
   	ed[0] = a[nd];
	z[0] = 0.0;
	r[0] = 0.0;
	for(int i = 1; i < nd; i++) {
		if(i == 1) {
			r[i] = 2.0 * a[i];
			z[i] = 0.0;
		}
		else {
			r[i] = a[i];
			z[i] = a[i + nd + nd];
		}
         
          
        ed[i] = a[i + nd];
	}
}

void GetR_ZandED(VectorXd& a, VectorXd& r, VectorXd& ed, int nd, VectorXd& z) {
   	ed[0] = a[nd];
	z[0] = 0.0;
	r[0] = 0.0;
	for(int i = 1; i < nd; i++) {
		r[i] = a[i];
		z[i] = a[i + nd + nd]; 
		ed[i] = a[i + nd];
	}
}

/*

/**
 * Fit only the scale and/or background (frozen FF)
 * The variable q should be the FF at point q_old
 **//*
double FitScaleBG(double q, VectorXd& a, int ma, int nd) {
	double scale = a[ma-EXTRA_PARAMS];   // Multiply by scale
	double bg = a[ma-EXTRA_PARAMS+1]; // Add background

	return q * scale + bg;
}

/**
 * An N-Layered Shell Model
 **//*
double NLayeredShell(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	VectorXd r = VectorXd::Zero(nd), ed = VectorXd::Zero(nd);

	GetRandED(a, r, ed, nd);
	
	for(int i = 0; i < nd - 1; i++)
		intensity += (ed[i] - ed[i + 1]) * 
				( (sin(q * r[i])) - (q * r[i] * cos(q * r[i])));

	intensity += (ed[nd - 1] - ed[SOLVENT]) * 
			( (sin(q * r[nd - 1]) )- (q * r[nd - 1] * cos(q * r[nd - 1])));

	intensity = sq(4.0 * PI * intensity / (q * sq(q)));

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;
}
double NLayeredShell_Gaussian(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	std::complex <double> im (0.0,1.0);
	VectorXd r = VectorXd::Zero(nd), ed = VectorXd::Zero(nd), t = VectorXd::Zero(nd);

	/* Old Model *//*
	//std::complex <double> rex(0.0,0.0);
	//GetR_ZandED(a, t, ed,nd, r);


	//for (int i = 1; i < nd; i++) {
	//	if (fabs(t[i]) < 1.0e-9) continue; 
	//	std::complex <double> resin(0.0,0.0);
	//	ed[i] -= ed[SOLVENT];
	//	double c = 2.0 * sqrt(log (2.0)) / t[i];
	//	resin  = sqrt(PI) * ed[i] / (8.0 * c * c * c) * exp (- 1.0/4.0 * q *( q/(c * c) + 4.0 * im * r[i]) );
	//	resin *= exp( 2.0 * im * q * r[i]) * (q - 2.0 * im * c * c * r[i]) *(1.0 + erf(im * q / (2.0 * c) + c * r[i]) )+ 
	//										  (q + 2.0 * im * c * c * r[i]) *(1.0 - erf(im * q / (2.0 * c) - c * r[i]) );
	//	rex += resin;  		
	//}
	//intensity = norm(rex / q);

	/////////////////////////////////////////////////
	/* Testing Model *//*
	GetR_ZandED(a, t, ed, nd, r);

	// Subtracting solvent
	ed -= VectorXd::Constant(ed.size(), ed[SOLVENT]);
	VectorXd edtexp = VectorXd::Zero(nd);

	for(int i = 1; i < nd; i++) {
		edtexp[i] = ed[i] * t[i] * exp(-sq(t[i] * q) / (16.0 * ln2));
		intensity += edtexp[i]
			* ( ((-2.0 * r[i] * sin(q * r[i])) / sqrt(ln2)) - ((sq(t[i]) * q * cos(q * r[i])) / (4.0 * pow(ln2, 1.5))) );
	}

	// += integral
	static VectorXd xx, ww;
	int steps = 500;
	SetupIntegral(xx, ww, 0.0f, 1.0f, steps);

	#pragma omp parallel for reduction(+ : intensity)
	for(int i = 0; i < steps; i++) {
		double inner = 0.0;
		for(int j = 1; j < nd; j++) {
			double rqy2 = r[j] * q * (sq(xx[i])- 1);

			inner += edtexp[j] * exp(sq(xx[i]) * ((sq(t[j] * q) / (16.0 * ln2)) - ((4.0 * ln2 * sq(r[j]) / sq(t[j])))))
				* (( (sq(2.0 * r[j]) / t[j]) - (sq(t[j] * q / (4.0 * ln2) ) * t[j])) * sin(rqy2) - (( r[j] * t[j] * q / ln2 ) * cos(rqy2)));
		}

		intensity += inner * ww[i];
	}

	intensity *= pow(PI, 1.5) / (2.0 * q);

	intensity *= intensity;

	/////////////////////////////////////////////////

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;

}

double RoG(double q, VectorXd& a) {
	// there are only three vars - a[0]: RoG, a[1]: scale, a[2]:background
	
	double intensity = exp(-pow(q*a[0],2)/3);
	intensity *= a[1];   // Multiply by scale
	intensity += a[2]; // Add background

	return intensity;
}


double _w, _h, _d, _ed, _q;
double _ma;
double _p; //Global Pitch for helix

double rectt(double phi, double theta){
	//return 1.0;
	double width = _w, depth = _d, height = _h;
	double res = 0.0;
	
	double qx = _q * sin(theta) * cos(phi),
		   qy = _q * sin(theta) * sin(phi),
		   qz = _q * cos(theta);

	res = _ed;

	res *= ((2 * sin(qx * (width / 2))) / qx);
	res *= ((2 * sin(qy * (depth / 2))) / qy);
	res *= ((2 * sin(qz * (height / 2))) / qz);




	return res * res * sin(theta);
}

double OA(double q, VectorXd& a, int ma, int nd) {
	_w = a[1]; _d = a[2]; _h = a[3];
	_q = q; _ed = a[nd];

	
	//Pablo
	int innerres = 2;
	int osc  = int((max(max(_w,_d),_h))*q*innerres);

	return Quadrature(rectt, max(defaultQuadRes,osc), 0.0 + EPS, 2.0 * PI + EPS, 0.0 + EPS, 
		              PI + EPS);
}

/**
 * A Rectangular Model
 **//*
double RectangularIntensity(double q, VectorXd& a, int ma, int nd) {
	//return OrientAvgIntensity(RectangularFF, a, q, ma, nd);
	double intensity = OA(q, a, ma, nd) ; // - (2 * PI * PI);

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;
}

double NLayeredPabloSlabs(double q, VectorXd& a, int ma, int nd) {
	// Symmetric to Asymmetric
	static VectorXd newa;
	static int newma, newnd;
	
	if(bPrecalculate) {
		int extras = ma - 2*nd;
		newnd = (nd > 1) ? (nd - 1) * 2 : nd;
		newma = (newnd * 2) + extras;
		
		newa = VectorXd::Zero(newma);
		
		// Extras
		for(int i = newma - extras; i < newma; i++) 
			newa[i] = a[ma - extras + (i - (newma - extras))];

		double *newr = &newa[0], *newed = &newa[newnd], *oldr = &a[0], *olded = &a[nd];
		newr[0] = a[0];
		newed[0] = a[nd];

		for(int i = 1; i < nd - 1; i++) {
			newr[nd - 1 + i] = oldr[i + 1];
			newr[nd - 1 - i] = oldr[i + 1];
			newed[nd - 1 + i] = olded[i + 1];
			newed[nd - 1 - i] = olded[i + 1];
		}

		if(nd > 1) {
			newr[nd - 1] = 2.0 * oldr[1];
			newed[nd - 1] = olded[1];
		}

		//std::string ss = debugMatrixPrintM(a);
		//std::string newss = debugMatrixPrintM(newa);
		//MessageBoxA(NULL, ss.c_str(), "Symmetric a", NULL);
		//MessageBoxA(NULL, newss.c_str(), "Asymmetric a", NULL);

		bPrecalculate = false;
	}

	// Call asymmetric version
	return NLayeredPabloAsymSlabs(q, newa, newma, newnd);
}

double NLayeredPabloAsymSlabs(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	VectorXd t = VectorXd::Zero(nd), ed = VectorXd::Zero(nd);

	GetRandED(a, t, ed, nd);

	for(int i = SOLVENT + 1; i < nd; i++)
		ed[i] = ed[i] - ed[SOLVENT];

	int IntRes = 10000;
	double end = 1.0 - std::numeric_limits<double>::epsilon();
	double start = 0.0 +  std::numeric_limits<double>::epsilon();
	static VectorXd xx  , ww ;

	SetupIntegral(xx, ww, start, end, IntRes);

	//#pragma omp parallel for reduction(+ : intensity)
	//Cylinders model
	/*for (int j = 0; j < nd; j++) {
		for (int k = 0; k < nd; k++) {
			double T1 = (k == 0)? t[k] : t[k] - t[k-1] ,
				   T2 = (j == 0)? t[j] : t[j] - t[j-1] ,
				   T3 =  t[j] - t[k] ;
			if (j != 0 ) T3 += t[j-1]; 
			if (k != 0 ) T3 -= t[k-1];
			double ab = 1.0;
			double result = 0;
			for (int i = 0; i < IntRes; i++) {
				double res = 0.0;
				res =  sq(bessel_j1(q * ab * sqrt(1.0 - xx[i] * xx[i]))/(xx[i]*sqrt(1.0 - xx[i] * xx[i])));
				res*= sin(q * T1 * xx[i] / 2.0 ) * sin(q * T2 * xx[i] / 2.0 ) * cos(q * T3 * xx[i] / 2.0 );
				res*= ww[i];
				
				result += res;
			}
			result *= ed[j]*ed[k];
			intensity += result * 16.0 * PI * sq(PI) * sq (ab) / sq(sq(q));
			
			}
	}*//*
						
	//Cuboid model with equal phi approx
	//for (int j = 0; j < nd; j++) {
	//	for (int k = 0; k < nd; k++) {
	//		double T1 = (k == 0)? t[k] : t[k] - t[k-1] ,
	//			   T2 = (j == 0)? t[j] : t[j] - t[j-1] ,
	//			   T3 =  t[j] - t[k] ;
	//		if (j != 0 ) T3 += t[j-1]; 
	//		if (k != 0 ) T3 -= t[k-1];

	//		double ab = 1000.0;
	//		double result = 0;
	////		double dx = (end - start) / double(IntRes);
	//		for (int i = 0; i < IntRes; i++) {
	////			double x = start + dx * double (i);
	//			double res = 0.0;
	//			res =  sin (q * ab * sqrt(2.0 - 2.0* xx[i] * xx[i])/2.0 );
	//			res *= res * res * res;
	//			res/=(xx[i] * xx[i]) * (1.0 - xx[i] * xx[i]) * (1.0 - xx[i] * xx[i]);
	//			res*= sin(q * T1 * xx[i] / 2.0 ) * sin(q * T2 * xx[i] / 2.0 ) * cos(q * T3 * xx[i] / 2.0 );
	//			res*= ww[i];
	//			
	//			result += res;
	//		}
	//		result *= ed[j]*ed[k] / pow(q, 6.0);
	//		intensity += result;
	//	}
	//}
	
	//Cuboid model no approximations
	static VectorXd xxin  , wwin ;
	SetupIntegral(xxin, wwin, 0.001, 2.0 * PI + 0.001, IntRes);

	#pragma omp parallel for reduction(+ : intensity)
	for (int j = 0; j < nd; j++) {
		for (int k = 0; k < nd; k++) {
			double T1 = (k == 0)? t[k] : t[k] - t[k-1] ,
				   T2 = (j == 0)? t[j] : t[j] - t[j-1] ,
				   T3 =  t[j] - t[k] ;
			if (j != 0 ) T3 += t[j-1]; 
			if (k != 0 ) T3 -= t[k-1];

			double ab = 1000.0;
			double result2 = 0.0;
			
			for (int l = 0; l <IntRes; l++) {
				double result = 0.0;
				double coxx = cos(xxin[l]);
				double sixx = sin(xxin[l]);

				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for (int i = 0; i < IntRes; i++) {
					double res = 0.0;
					double sqrtx = sqrt(1.0 -  xx[i] * xx[i]);
					res =  1.0 - cos(q * ab * sqrtx * coxx  );
					res *= 1.0 - cos(q * ab * sqrtx * sixx  );
					res /= sq(xx[i] * (1.0 - xx[i] * xx[i]) * coxx * sixx);
					res *= sin(q * T1 * xx[i] / 2.0 ) * sin(q * T2 * xx[i] / 2.0 ) * cos(q * T3 * xx[i] / 2.0 );
					res *= ww[i];
					
					result += res;
				}
				result2 += result * wwin[l];
			}
			result2 *= 4.0 * ed[j]*ed[k] / sq(q * sq(q));
			intensity += result2;
		}
	}

	intensity *= a[ma-EXTRA_PARAMS] / 1000.0;   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;
}

/**
 * N-Layered slabs (infinite at x and y axes)
 **//*
double NLayeredSlabs(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	VectorXd t = VectorXd::Zero(nd), ed = VectorXd::Zero(nd);

	GetRandED(a, t, ed, nd);

	for (int i = 1; i < nd; i++)
		t[i] *= 2.0;



	for(int i = SOLVENT + 1; i < nd; i++)
		ed[i] = ed[i] - ed[SOLVENT];

	
	for(int i = 1; i < nd; i++)
		intensity += 2 * ed[i] * (sin(q * t[i] / 2) - sin(q * t[i - 1] / 2));

	intensity *= intensity * (2 / q / q / q / q);

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;
}

double NLayeredAsymSlabs(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	VectorXd t = VectorXd::Zero(nd), ed = VectorXd::Zero(nd);

	GetRandED(a, t, ed, nd);

	for(int i = SOLVENT + 1; i < nd; i++)
		ed[i] = ed[i] - ed[SOLVENT];
	ed[SOLVENT] = 0.0;

	//Existing Model
	/*complex<double> c (0.0, 0.0);
	for(int i = 1; i < nd; i++) {
		complex<double> im(0, 1);
		
		c += double(ed[i]) * (exp(im * q * double(t[i])) - exp(im * q * double(t[i-1])));
	}

	intensity = norm(c);

	c = complex<double>(0.0, 0.0);
	for(int i = 1; i < nd; i++) {
		complex<double> im(0, 1);
		c += double(ed[i]) * (exp(-1.0 * im * q * double(t[i])) - exp(-1.0 * im * q * double(t[i-1])));
	}

	intensity += norm(c);

	intensity /= q * q * q * q;*//*
	// another Model
	for (int i = 1; i<nd; i++){
			for(int j = 1; j<nd;j++) {
			intensity += ed[i]*ed[j]*(cos(q*(t[j]-t[i]))-2.0*cos(q*(t[j-1]-t[i]))+cos(q*(t[j-1]-t[i-1])));
		}
	}
	intensity *= 2.0 /sq(sq(q));

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background

	return intensity;
}


/**
 * N-Layered slabs (infinite at x and y axes) with a gaussian ED distribution
 * along the z axis (where the width of the gaussian is 2*sqrt(ln(2))*sigma (FWHM))
 **//*
double NLayeredSlabs_GaussED(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	
	//std::string ss = debugMatrixPrintM(a);
	//MessageBoxA(NULL, ss.c_str(), "title", NULL);

	if(nd == 1)
		return a[ma-EXTRA_PARAMS+1];
	VectorXd z = VectorXd::Zero(nd), ed = VectorXd::Zero(nd), width = VectorXd::Zero(nd);
	double sum = 0.0;

	//z[n] is the distance between the center of the slab and the center of the "n"th layer
	GetR_ZandED_Symmetric(a, width, ed, nd, z);

	//if(bPrecalculate){
	//		std::string ss = debugMatrixPrintM(width) + "\n" + debugMatrixPrintM(ed)+ "\n" + debugMatrixPrintM(z);
	//		MessageBoxA(NULL, ss.c_str(), "T ED Z", NULL);
	//		bPrecalculate = false;
	//	}

	//change from ED to delta ED
	for(int cnt = SOLVENT + 1; cnt < nd; cnt++)
		ed[cnt] = ed[cnt] - ed[SOLVENT];
	
	width[0] = 0.0; //No solvent in the middle of a membrane
	
	sum = ed[SOLVENT+1] * width[SOLVENT+1] * exp(-q * q * width[SOLVENT+1] * width[SOLVENT+1]/ (16.0 * ln2));
	//sum = 0.0;

	for(int cnt = SOLVENT + 2; cnt < nd; cnt++) {
		sum += 2.0 * ed[cnt] * width[cnt] * exp( -(q * q * width[cnt] * width[cnt]) / (16.0 * ln2))
			* cos(z[cnt] * q);
	}
	
	intensity = (sum * sum) * (PI / (2.0 * ln2)) / (q * q);
	
	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background
	
	return intensity;
}

//
//N-Layered asymmetric slabs (infinite at x and y axes) with a gaussian ED distribution
//along the z axis (where the width of the gaussian is 2*sqrt(ln(2))*sigma (FWHM))
double NLayeredAsymSlabs_GaussED(double q, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;	
	VectorXd t = VectorXd::Zero(nd), ed = VectorXd::Zero(nd), z = VectorXd::Zero(nd);


	//z[n] is the distance between the center of the slab and the center of the "n"th layer
	GetR_ZandED(a, t, ed, nd, z);

	//if(bPrecalculate){
	//		std::string ss = debugMatrixPrintM(t) + "\n" + debugMatrixPrintM(ed)+ "\n" + debugMatrixPrintM(z);
	//		MessageBoxA(NULL, ss.c_str(), "T ED z", NULL);
	//		bPrecalculate = false;
	//	}

	for(int i = SOLVENT + 1; i < nd; i++)
		ed[i] = ed[i] - ed[SOLVENT];
	ed[SOLVENT] = 0.0;

	//current model

	//double gaussfactor = 16.0 * ln2; // 16 ln 2


	//complex<double> c (0.0, 0.0);
	//complex<double> im(0.0, 1.0);
	//for(int i = 1; i < nd; i++)	
	//	c += ed[i] * exp(((im * z[i] * q * gaussfactor) - (q * q * t[i] * t[i])) / gaussfactor);

	//intensity = norm(c);

	//c = complex<double>(0.0, 0.0);
	//for(int i = 1; i < nd; i++)	
	//	c += ed[i] * exp(((-im * z[i] * q * gaussfactor) - (q * q * t[i] * t[i])) / gaussfactor);

	//intensity += norm(c);

	//intensity /= q * q;

	//new model
	double gaussfactor = 16.0 * ln2;
	for (int i = 0; i<nd ; i++) {
		for(int j = 0; j<nd; j++) {
			intensity += 
				ed[i] * ed[j] * t[i] * t[j]
				* exp(-(sq(q) * (sq(t[i]) + sq(t[j]))) / gaussfactor)
				* cos(q * (z[i]-z[j]));

			//if(bPrecalculate) {
			//	std::stringstream ss;
			//	ss << "T["<< i <<"] = " << t[i] << "\t\tT["<< j <<"] = " << t[j] <<"\n";
			//	ss << "ED["<< i <<"] = " << ed[i] << "\t\tED["<< j <<"] = " << ed[j] <<"\n";
			//	ss << "Z["<< i <<"] = " << z[i] << "\t\tZ["<< j <<"] = " << z[j] <<"\n";
			//	MessageBoxA(NULL, ss.str().c_str(), "T ED z", NULL);
			//	bPrecalculate = false;
			//}
		}
	}
	intensity *= PI / (2.0 * ln2 *sq(q)); 

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background
	
	return intensity;
}

double HCgaussianIntensity (double q, VectorXd& a, int ma, int nd) {
	double H = -1.0; 
    double scale, background;
	VectorXd r = VectorXd::Zero(nd), ed = VectorXd::Zero(nd), t = VectorXd::Zero(nd);
    double intensity = 0.0;
	static int nonzero = 0;
	static MatrixXd xx, ww;
	static VectorXd xt,wt;
	int steps = 500;
	
	GetR_ZandED(a,t,ed,nd,r);
	ed -= VectorXd::Constant(nd, ed[0]);

	if(bPrecalculate) {
		
		for(nonzero = 0; (nonzero < nd) && (t[nonzero] <= 0.0); nonzero++);
		if(nonzero == nd)
			return 0.0;	//No layer thickness
		
		xx = MatrixXd::Zero(nd-nonzero,steps);
		ww = MatrixXd::Zero(nd-nonzero,steps);

		#pragma omp parallel for
		for(int i = nonzero; i < nd; i++) {
			VectorXd x, w;
			SetupIntegral(x, w, min(0.0f,r[i]- 3.0 *t[i]), r[i] + 3.0 *t[i], steps);
			xx.row(i - nonzero) = x;
			ww.row(i - nonzero) = w;
		}

		bPrecalculate = false;
	}

    scale = a[ma - EXTRA_PARAMS];
    background = a[ma - EXTRA_PARAMS + 1];
	H = a[ma - EXTRA_PARAMS + 2] < 0 ? -1.0 : a[ma - EXTRA_PARAMS + 2]/2.0;
	
	if(H > 0.0) {	// Finite Model
		int steps1 = 500;
		double resouter = 0.0;
		SetupIntegral(xt , wt , 0.0, 1.0, steps1); //double integral
#pragma omp parallel for reduction(+ : resouter)
		for (int outer = 0; outer < steps1; outer++) {
			double ressum = 0.0;
			for (int sum = nonzero; sum < nd; sum++) {
				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				int es = sum - nonzero;
				double resinner = 0.0;
				for (int inner = 0; inner < steps; inner ++ ) {
					resinner += exp(-4.0 * ln2 * sq(xx(es,inner)-r[sum])/sq(t[sum])) *
						xx(es,inner)* bessel_j0(q * sqrt(1-sq(xt[outer]))* xx(es,inner)) * ww(es,inner); 
				}
				ressum += resinner*ed[sum];
			}
			resouter += sq(ressum * sin(q * xt[outer] * H)/ xt[outer] ) * wt[outer];
		}
		intensity = 2.5 * resouter * 32.0 * sq(PI) * PI / sq(q) ; 
		//the 2.5 is a factor to normalize the ed area between this and the discrete model.
	} else {		// Infinite Model
		double ressum = 0.0;
#pragma omp parallel for reduction(+ : ressum)
		for (int sum = nonzero; sum < nd; sum++) {
			int es = sum - nonzero;
			double resinner = 0.0;

			if(pStop && *pStop)		// Place these line strategically in
				continue;			// slow models.

#pragma omp parallel for if(nd - nonzero < 2) reduction(+ : resinner)
			for (int inner = 0; inner < steps; inner ++ ) {
				resinner += exp(-4.0 * ln2 * sq(xx(es,inner)-r[sum])/sq(t[sum])) *
					xx(es,inner)* bessel_j0(q * xx(es,inner)) * ww(es,inner); 
			}
			ressum += resinner*ed[sum];
		}

		intensity = 2.0 * sq(ressum * sq(PI)) * 64.0 / q; // single integral
	//the 2.0 is a factor to normalize the ed area between this and the discrete model.
	}


	return scale * intensity + background;


}

//Returns scattering amplitude (w/o E.D.) of a solid cylindroid

complex<double> Fcylindroid (double q, double a, double h, double eps, double phiQ, double teta) {
	complex<double> res (0.0,0.0);
	
	static VectorXd xx, ww;
	int steps = 200;
	
	SetupIntegral(xx, ww, 0.0f, 2.0*PI, steps);

	double eps2 = eps * eps, qsint = q * sin(teta), eta;
	
	for(int j = 0; j < steps; j++) {
		complex <double> R (0.0,0.0), eiaeta (0.0, 0.0);
		double phiR = xx[j], cpr = cos(xx[j]);
		cpr *= cpr;

		eta = qsint * sqrt((1.0 - eps2)/(1.0 - eps2 * cpr)) * cos(phiQ - phiR);
		eiaeta = complex<double>(cos(-a * eta), sin(-a * eta));
		R = eiaeta * a / (complex<double>(0.0, -eta)) + eiaeta / (eta * eta) - 1.0 / (eta * eta);

		res += R*ww[j];
	}

	// Can be "taken out" of FCylindroid for inf. height?
	res *= (h < 0) ? 2.0  / (q * cos(teta)) : (2.0 * sin(q * cos(teta) * h))
													/ (q * cos(teta));
	return res;
}

double _eps;
VectorXd _epsilons, cyl_r, cyl_ed;
int _notZero, _nd;
VectorXd _a;


//This returns the numerical integration of a cylindroid
//for a specific orientation, subtracting a smaller solid cylidroid
//from a larger one and multiplying by ED

double cylindroid(double phi, double theta) {
	complex<double> res (0.0, 0.0);
	double q = _q, eps = _eps, h = _h;
	int notZero, nd = _nd;
	VectorXd& a = _a;
	VectorXd r = VectorXd::Zero(nd), ed = VectorXd::Zero(nd);
	
	GetRandED(a, r, ed, nd);
	
	// Find the first layer with non-zero thickness
	for(notZero = 0; (notZero < nd) && (r[notZero] <= 0.0); notZero++);
	if(notZero == nd)
		return 0.0;	//No layer thickness
 
	if (_eps < r[notZero])
		eps = sqrt(1 - (_eps * _eps) / (r[notZero] * r[notZero]));
	else
		eps = sqrt(1 - (r[notZero] * r[notZero]) / (_eps * _eps));

	complex<double> layer, previousLayer;
	
	previousLayer = Fcylindroid(q, r[notZero], h, eps, phi, theta);
	for (int i = notZero + 1; i < nd; i++) {
		if(!isConsEcc()) {
			if (_eps < r[notZero])
				eps = sqrt(1 - ((r[i] - r[notZero] + _eps) * (r[i] - r[notZero] + _eps)) / (r[i] * r[i]));
			else
				eps = sqrt(1 - (r[i] * r[i]) / ((r[i] - r[notZero] + _eps) * (r[i] - r[notZero] + _eps)));
		}
		layer = Fcylindroid(q, r[i], h, eps, phi, theta);
		res += (ed[i] - ed[SOLVENT]) * (layer - previousLayer);

		previousLayer = layer;
	}
	
	return norm(res) * sin(theta);
}

double NlayeredCylindroid (double q, VectorXd& a, int ma, int nd) {
	//
	// 1) Collect parameters and extra parameters
	// 2) GetRandED...
	// 3) Numeric integration over phiR
	// 4) Subtract innner cylinder
	// 4.5) Quadrature (Orientaional Average(?))
	// 5) Scale & BG
	//
	double intensity = 0.0;
// Testing Model
	VectorXd r = VectorXd::Zero(nd), ed = VectorXd::Zero(nd), b = VectorXd::Zero(nd), eps = VectorXd::Zero(nd);
	static int notZero = 0;
	static int steps = 0, steps1 = 0;
	int thetaSteps = 0;
	double h = a[ma - EXTRA_PARAMS + 2] / 2.0, // Height
			b1 = a[ma - EXTRA_PARAMS + 3];	//Short inner radius -> Eccentricity
	static 	VectorXd xIn, wIn, xOut, wOut, testX, testW, thetaQ, thetaQW;
	
	GetRandED(a, r, ed, nd);
		
	if(bPrecalculate) {
		// Find the first layer with non-zero thickness
	 	for(notZero = 0; (notZero < nd) && (r[notZero] <= 0.0); notZero++);
		if(notZero == nd)
			return 0.0;	//No layer thickness
	
		bool bSwitch = b1 > r[notZero];
		
		for(int i = nd - 1; i >= 0; i--) {
			ed[i] -= ed[0];
			if(isConsEcc()) {
				if (!bSwitch)
					eps[i] = sqrt(1.0 - (b1 * b1) / (r[notZero] * r[notZero]));
				else
					eps[i] = sqrt(1.0 - (r[notZero] * r[notZero]) / (b1 * b1));
				}
			else {
				if (!bSwitch)
					eps[i] = sqrt(1.0 - sq((r[i] + b1 - r[notZero]) / r[i]));
				else
					eps[i] = sqrt(1.0 - sq(r[i] / (r[i] + b1 - r[notZero])));
				}
		}
		
		for(int i = 0; i < nd; i++) {
			if(!bSwitch)
				b[i]   = r[i] * sqrt(1.0 - eps[i] * eps[i]);
			else {
				b[i] = r[i] / sqrt(1.0 - eps[i] * eps[i]);
				std::swap(b[i], r[i]);
			}
		}

		steps = (h > 0.0) ? 200 : 100;// + int(sqrt(1.0 - sq(b1 / r[notZero])) * 1000.0);// steps = 150 --> Good to q ~= 7.2
		steps1  = 1 + int(sqrt(1.0 - sq(b1 / r[notZero])) * 1000.0);		
		
		#pragma omp parallel sections 
		{
		#pragma omp section
			{
				SetupIntegral(xIn, wIn, 0.0000001, 2.0 * PI + 0.0000001, steps);	// x = theta_r
			}
		#pragma omp section
			{
				if(h > 0.0) {
					thetaSteps = 10 * int(h);
					SetupIntegral(xOut, wOut, 0.0, 2.0 * PI, steps); // Orientational Average: phiQ
					SetupIntegral(thetaQ, thetaQW, std::numeric_limits<double>::epsilon(), 1.0 - std::numeric_limits<double>::epsilon(), thetaSteps); // Orientational Average: x = cos(theta_q)
				}
				else
					SetupIntegral(testX, testW, 0.0, 2.0 * PI, steps1); // test
			}
		}
		bPrecalculate = false;
	}

	static VectorXd cosInner;
	if(cosInner.size() != steps) {
		cosInner = VectorXd::Zero(steps);
		#pragma omp parallel for default(none) shared(cosInner, xIn, steps)
		for(int p = 0; p < steps; p++)
			cosInner[p] = cos(xIn[p]);

	}

	if(h > 0.0) {	// Finite model
		int subSteps = int(2.0 * sqrt((double)steps));
		double root, rootX, sinValue, qc, res = 0.0;
		std::complex<double> im(0.0, 1.0);
		
		#pragma omp parallel for reduction(+ : intensity)
		for(int dTheta = 0; dTheta < thetaSteps; dTheta++) {
			std::complex<double> innerRes(0.0, 0.0);
			rootX = sqrt(1.0 - sq(thetaQ[dTheta]));
			sinValue = sq(sin(q * thetaQ[dTheta] * h) / thetaQ[dTheta]) / rootX;
			for(int phiQc = 0; phiQc < steps; phiQc++) {
				std::complex<double> tmp(0.0, 0.0);

				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for(int inner = 0; inner < steps; inner++) {
					qc = q * cos(xIn[inner] - xOut[phiQc]);
					for(int i = 1; i < nd - 1; i++) {
						root = sqrt(1.0 - sq(eps[i] * cosInner[inner]));
						tmp += (ed[i] - ed[i + 1]) * ( (b[i] / ( -im * qc * rootX * root)) + (1.0 / sq(qc * rootX)) )
									* exp(-im * b[i] * qc * rootX / root);
					}
					tmp += ed[nd - 1] * ( (b[nd - 1] / (-im * qc * rootX * root) ) + (1.0 / sq(qc * rootX)) )
								* exp(-im * b[nd - 1] * qc * rootX / root);
					tmp -= ed[notZero] / sq(qc * rootX);
					innerRes += tmp * wIn[inner];
				}	//inner
				res += norm(innerRes * wOut[phiQc]);
			}	//phiQc
			intensity += res * sinValue * thetaQW[dTheta];
		}	//dTheta

	} else {	// Infinite model
			complex<double> temp (0.0, 0.0), im (0.0, 1.0);
			double  perp = q; // q
			
			int subSteps = int(2.0 * sqrt((double)steps));

			#pragma omp parallel for reduction(+ : intensity)
			for(int outest = 0; outest <= steps / subSteps; outest++) {
				double subIntensity = 0.0;
				for(int tester = outest * subSteps; tester < subSteps * (outest + 1); tester++) { // test
					double tempIm = 0.0, tempRe = 0.0;
					if(!(tester < steps1))
						continue;

					if(pStop && *pStop)		// Place these line strategically in
						continue;			// slow models.

					for(int inner = 0; inner < steps; inner++) {
						complex <double> res(0.0,0.0);
						double root = 0.0;
						double qc = perp * cos(xIn[inner] - testX[tester]);	// q*cos(phi_r)
						for(int i = notZero; i < nd - 1; i++) {
							root = sqrt(1.0 - sq(eps[i] * cosInner[inner]/*cos(xIn[inner])*//*));
							res += (ed[i] - ed[i + 1]) * ( (b[i] / ( -im * qc * root)) + (1.0 / sq(qc)) )
									* exp(-im * b[i] * qc / root);
						}

						root = sqrt(1.0 - sq(eps[nd - 1] * cosInner[inner]/*cos(xIn[inner])*//*));
						res += ed[nd - 1] * ( (b[nd - 1] / (-im * qc * root) ) + (1.0 / sq(qc)) )
								* exp(-im * b[nd - 1] * qc / root);
						res -= ed[notZero] / sq(qc);
						tempRe += res.real() * wIn[inner];
						tempIm += res.imag() * wIn[inner];

					}
					subIntensity += testW[tester] * (sq(4.0 * PI) / q * (sq(tempRe)+ sq(tempIm)));
				} // test
				intensity += subIntensity;
			} // outest
	}


// Existing Model
	//_q = q;
	//_h = a[ma - EXTRA_PARAMS + 2] / 2.0; // Height
	//_eps = a[ma - EXTRA_PARAMS + 3];	//Short inner radius -> Eccentricity
	//_a = a;
	//_nd = nd;

	//double maxa = 0.0;

	////if(bPrecalculate) {
	//	for (int j = 0; j < nd; j++) 
	//		maxa += a[j];


	////	bPrecalculate = false;
	////}
	//int inerres = 3;

	//intensity = Quadrature(cylindroid, max(defaultQuadRes, inerres * int(q * maxa)),
	//						0.0 + EPS, 2.0 * PI, 0.0 + EPS, PI);

	intensity *= a[ma - EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma - EXTRA_PARAMS + 1]; // Add background

	return intensity;

}

double HelixIntensity(double q, VectorXd& a, int ma, int nd) {
	//ma - size of a
	//nd - number of helices
	//a[1]...a[nd - 1] - Delta from Helix 1
	//a[nd] - solvent ED
	//a[nd + 1]...a[2*nd - 1] - Helix ED
	//a[2*nd + 1]...a[3*nd - 1] - Helix Cross section
	// Extra params (in correct order): (0)scale, (1)background, (2)height, (3)R, (4)P
	
	double height = a[ma-EXTRA_PARAMS+2];
	//if (!(height < 0.0)) // Not an infinite helix
	//	return ShortHelixIntensity(q, a, ma, nd);

//	complex<double> i (0.0, 1.0);
	double rHelix, P, edSolvent; 
	double *delta = &a[1];
	double *deltaED = &a[nd+1];
	double *rCs = &a[nd+nd+1];
	VectorXd bess1 = VectorXd::Zero(nd);
	
	int hMax;
	if(delta[0] < 0.0) 
		delta[0] = 0.0;

	edSolvent = a[nd];
	static VectorXd xIn, wIn, xOut, wOut;
	static int subSteps;
	int steps, steps1;
	rHelix = a[ma-EXTRA_PARAMS+3];
	P =  a[ma-EXTRA_PARAMS+4];
	int N = int(height / P);
		
	hMax = int(floor(q * P / (2.0 * PI)));
	
	double intensity = 0.0;
	//trying to use model without assumptions about qz
//	if(bPrecalculate) {
//		SetupIntegral(xIn, wIn, 0.0000001, 2.0 * PI + 0.0000001, steps);	// x = theta_r
//		SetupIntegral(xOut, wOut, -1.0 + 0.0000001, 1.0 + 0.0000001, steps); // Orientational Average: x = cos(theta_q)
//
//		bPrecalculate = false;
//	}
//	std::complex <double>  I(0.0,1.0);
//#pragma omp parallel for reduction(+ : intensity)
//	for (int i = 0; i < steps; i++) {
//		double  qperp = q * sqrt(1.0 - sq(xOut[i])), qz = q * xOut[i];
//		std::complex <double> tot(0.0,0.0);  
//		for (int j = 0; j<steps; j++)  
//			tot += exp(-I * qperp * rHelix * cos(xIn[j]) - (I * qz * P * xIn[j] / (2.0 * PI)) ) * wIn[j];
//		complex <double> a(0.0);
//		for (int j = 0; j< nd -1; j++) {
//			complex <double> b(0.0);
//			b = (deltaED[j] - edSolvent) * P * exp( I * qz *  delta[j]); //delta is in nm so we took out the P and 2PI 
//			b*= (sin(qz * P *(double(N) + 0.5)) / sin (qz * P / 2.0)) * bessel_j1(qperp * rCs[j]) * rCs[j] / qperp;
//			a+=b;
//		}
//		tot *= a;
//		intensity += norm(tot)*wOut[i];
//	}

	
	
	//Our model using delta
	
	//for (int m = 0; m <= hMax; m++) {
	//	double root = sqrt(1.0-sq(2.0 * PI * double(m)/(P * q)));
	//	for (int k = 0; k < nd - 1 ; k++) 
	//		bess1[k] = bessel_j1(q * root * rCs[k]);
	//	std::complex <double> total(0.0,0.0), I(0.0,1.0);
	//	
	//	for(int i = 0; i< nd - 1; i++) {
	//		for (int j = 0; j< nd - 1; j++) {
	//			total += 
	//				(deltaED[i] - edSolvent) * (deltaED[j] - edSolvent) * rCs[i] * rCs[j] 
	//				* exp(I * double(m) * (delta[i]-delta[j])* 2.0 * PI / P ) * bess1[i] * bess1[j];
	//		}
	//	}
	//	if (total.imag() / total.real() > 0.001 ) 
 //			return -1.0;

	//	intensity += total.real() * sq(bessel_jn(m, q * root * rHelix)) /
	//		((P * q / (2.0 * PI)) * sq(root));
	//}
	//intensity *=  (2.0 * PI)* sq(2.0 * PI) * sq(P)*sq(2.0 * N + 1.0) / sq(q) ;

//Our model using ball
	
	
	//std::complex <double> total(0.0,0.0), I(0.0,1.0);
	//for(int i = 0; i< nd - 1; i++) {
	//	for (int j = 0; j< nd - 1; j++) {
	//		std::complex <double> tot(0.0,0.0);
	//		for (int m = 0; m <= hMax; m++) {
	//			tot += sq(bessel_jn(m, q * sqrt(1.0-sq(2.0 * PI * double(m)/(P * q))) * rHelix)) * 
	//				exp(I * double(m) * (delta[i]-delta[j])* 2.0 * PI / P );
	//		}
	//		total+=tot * (deltaED[i] - edSolvent) * (deltaED[j] - edSolvent) *
	//			(sin(rCs[i])*sin(rCs[j])+ sq(q)* (rCs[i] * rCs[j]) * cos(rCs[i])* cos(rCs[j]) - 
	//			 q * rCs[i] * cos(rCs[i]) * sin(rCs[j]) - q * rCs[j] * cos(rCs[j]) * sin(rCs[i]) );
	//	}
	//}
	//intensity = total.real() * 64.0 * sq(sq(PI)) / P * sq(2.0 * N + 1.0) / (sq(sq(q))* q * sq(q));
			



	//Current model
	//complex<double> tmpLayer ((rCs[0]*(deltaED[0] - edSolvent)*bessel_j1(q * rCs[0])), 0.0);
	//for (int h = 0; h <= hMax; h++) {
	//	complex<double> layer = tmpLayer;

	//	// Number of helices
	//	for (int j = 1; j < nd-1; j++) { 
	//		layer += rCs[j]*(deltaED[j] - edSolvent)*bessel_j1(q * rCs[j])*exp(i * (double)h * 2.0 * PI * delta[j] / P);
	//	}
	//
	//	layer *= bessel_jn( h, (q * rHelix * sqrt(1.0-pow((2.0*PI*h/(q*P)), 2.0))));
	//	intensity += norm(layer);
	//}

	//intensity /= (q * q *q*q);	//Fits better, need to justfiy the model
	//							// also fits the units...

	//Model from 02/02/2010 - Convolution of a thin helix and a disc
	if(height < 0.0) {	// Infinite model
#pragma omp parallel for reduction(+ : intensity)
		for(int m = 0; m <= hMax; m++) {
			double  root = sqrt(1.0 - sq(2.0 * PI * double(m) / (q * P)));
			std::complex <double> sum(0.0,0.0), i(0.0,1.0);
			for(int j = 0; j < nd - 1; j++) {
 					sum += rCs[j] * (bessel_j1(q * rCs[j] * root) / root) * (deltaED[j] - edSolvent) 
						* exp(i * 2.0 * PI * double(m) * delta[j] / P);
			}
			sum *= bessel_jn(m, q * rHelix * root) ;
			intensity += norm(sum);
		}
		intensity *= 8.0 * PI * sq(PI)/ (sq(q) * q);
	}
	else {	// Finite model
		if(height < 1.0e-12)
			return a[ma-EXTRA_PARAMS+1];
		steps = int(8.0 * (rHelix + P));	//inner
		steps1 = int(15.0 * height);		//outer
		if(bPrecalculate) {
			#pragma omp parallel sections
			{
				#pragma omp section
				{
					SetupIntegral(xIn, wIn, 0.00001, 2.0 * PI + 0.00001, steps);	// x = theta_r
					subSteps = int(sqrt((double)steps1));
				}
				#pragma omp section
				{
					SetupIntegral(xOut, wOut, -1.0, 1.0 , steps1); // Orientational Average: x = cos(theta_q)
				}
			}
			bPrecalculate = false;
		}
		std::complex <double>  i(0.0,1.0);
		
#pragma omp parallel for reduction(+ : intensity)
		for(int outest = 0; outest <= steps1 / subSteps ; outest++) {
			double subIntensity = 0.0;
			for (int ou = outest * subSteps; ou < (subSteps * outest) + subSteps; ou++) {
				if(!(ou < steps1)) continue;
				std::complex <double> sst(0.0,0.0),st(0.0,0.0);
				double outroot = sqrt(1.0 - sq(xOut[ou]));
				for (int s = 0; s<nd; s++) 
					st += (deltaED[s] - edSolvent)* exp(i * q * xOut[ou] * delta[s]) 
						* bessel_j1(q * rCs[s] * outroot) * rCs[s] / outroot ;	
				for (int in = 0; in < steps; in++) 
					sst += wIn[in] * exp(i * q * rHelix * outroot * cos(xIn[in]) 
						+ i * q * xOut[ou] * P * xIn[in] / (2.0 * PI)  );
				subIntensity += wOut[ou] * norm((fabs(xOut[ou]) < 1.0e-20) ? (N) : (sin(N * q * xOut[ou] * P / 2.0) / sin( q * xOut[ou] * P / 2.0)) * sst * st);
			}
			intensity += subIntensity;
		}
		intensity *= 2.0 * PI * sq(P) / sq (q);
	
	}

	/* End of 02/02/2010*//*

	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background
	
	return intensity;
}

double DiscreteHelixIntensity(double qd, VectorXd& a, int ma, int nd) {
	double intensity = 0.0;
	//ma - size of a
	//nd - number of helices
	//a[1]...a[nd - 1] - Delta from Helix 1
	//a[nd] - solvent ED
	//a[nd + 1]...a[2*nd - 1] - Helix ED
	//a[2*nd + 1]...a[3*nd - 1] - Helix Cross section
	// Extra params (in correct order): (0)scale, (1)background, (2)Number of spheres,
	//									(3)R, (4)P, (5) deltaw (6) Debye Waller factor
	
	int Nb = int(a[ma-EXTRA_PARAMS+2]);
	double debyeWaller = a[ma - EXTRA_PARAMS + 6];

	double q = qd;
	//complex<double> im (0.0, 1.0);
	double rHelix, P, edSolvent, deltaw; 
	double *delta = &a[1];
	double *deltaED = &a[nd+1];
	double *rCs = &a[nd+nd+1];
	//VectorXf delta = VectorXf::Zero(nd), deltaED = VectorXf::Zero(nd), rCs = VectorXf::Zero(nd);
	//for(int i = 0; i < nd; i++) {
	//	delta[i] = (float)Ddelta[i];
	//	deltaED[i] = (float)DdeltaED[i];
	//	rCs[i] = (float)DrCs[i];
	//}

	int steps = 400;
	VectorXd bess1 = VectorXd::Zero(nd);//, bess = VectorXd::Zero(steps);
	rHelix = a[ma-EXTRA_PARAMS+3];
	P =  a[ma-EXTRA_PARAMS+4];
	deltaw = a[ma-EXTRA_PARAMS+5];
	edSolvent = a[nd];
	VectorXd deltaz = VectorXd::Zero(nd), ball = VectorXd::Zero(nd);
	for (int i = 0; i < nd; i++) {
		if(isGaussED())
			deltaz[i] =  (P * (rCs[i] + deltaw) / ( 2.0 * PI * rHelix));
		else
			deltaz[i] =  (P * (2.0 * rCs[i] + deltaw) / ( 2.0 * PI * rHelix));
	}

	bool bImag = false;
	static VectorXd x, w;
	SetupIntegral(x, w, 0.0, 1.0, steps);
	for (int i = 0; i< nd; i++) {
		ball[i] = (deltaED[i] - edSolvent) *
			(isGaussED())? 
			(sq(rCs[i]) * rCs[i] * exp(-sq(rCs[i] * q / 4.0 ) / log (2.0) )) :
			(sin (rCs[i] * q) - rCs[i] * q * cos(rCs[i] * q )  );
	}
	//for(int g = 0; g < steps; g++)
	//	bess[g] = bessel_j0(2.0 * q * sqrt (1.0 - sq(x[g])));

//#pragma omp parallel for reduction(+ : intensity)
	for (int i = 0; i < nd; i++) {
		for (int j = 0; j < nd; j++) {
			double sum1 = 0.0;//, sumIm = 0.0;
#pragma omp parallel for reduction(+ : sum1)
			for(int n = 0; n < Nb; n++) {
				
				if(pStop && *pStop)		// Place these line strategically in
					continue;			// slow models.

				for (int m = 0; m < Nb; m++) {
					//std::complex <float> integ(0.0,0.0);
					double sinG = sin (PI * (double(n) * deltaz[i] - double(m) * deltaz[j]) / P + (delta[i] - delta[j]) * PI / P);
					for (int s = 0; s < steps; s++) {
						sum1 += w[s] * bessel_j0( 2.0 * q * sqrt (1.0 - sq(x[s])) * rHelix * sinG) *
							cos(q * x[s] * (double(n) * deltaz[i] - double(m) * deltaz[j]));

					//std::complex <double> integ(0.0,0.0);
					//for (int s = 0; s<steps; s++) {
					//	integ += w[s] * bessel_j0( 2.0 * q * sqrt (1.0 - sq(x[s])) * rHelix * 
					//		sin (PI * (double(n) * deltaz[i] - double(m) * deltaz[j]) / P + (delta[i] - delta[j])/2.0 )) *
					//		exp(im * q * x[s] * (double(n) * deltaz[i] - double(m) * deltaz[j]));

					}
				//	sum1 += integ.real();
				//	sumIm += integ.imag();
				}
			}
			//if (sumIm > 1e-6) 
			//	bImag = true;
			intensity += sum1 * ball[i] * ball[j] ; 
		}
	}
	//if(bImag)
	//	return -1000000023.0;
	intensity *= (isGaussED())? (sqrt(PI/ln2)*(PI/ln2) / 8.0) 
		: (32.0 * sq(PI) / sq(q * sq(q))); 

	intensity *= exp(-sq(q * debyeWaller) / 2.0);   // Debye Waller factor
	
	intensity *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	intensity += a[ma-EXTRA_PARAMS+1]; // Add background
	return intensity;
}

double ShortHelixIntensity (double q, VectorXd& a, int ma, int nd) {
	//Needed parameters: the entire "a" vector
	//					nd
	//					q
	//					(integration steps?)
	double res = 0.0;
	complex <double> i (0.0,1.0);

	double height = a[ma - EXTRA_PARAMS + 2];
	double hRadius = a[ma - EXTRA_PARAMS + 3];
	double pitch = min(a[ma-EXTRA_PARAMS+4], hRadius*2*PI);

	double *delta = &a[1];
	double *deltaED = &a[nd+1];
	double *rCs = &a[nd+nd+1];
	double edSolvent = a[nd];

	int N = int(height / pitch);

    int helices = nd - 1;

	static VectorXd intrnl, ww_in, extrnl, ww_ex;
	int steps = max(200, int(floor(N * q * pitch)));
	double halfPitch = pitch / 2.0;
	
	SetupIntegral(intrnl, ww_in, 0.0, 2.0*PI, steps);
	SetupIntegral(extrnl, ww_ex, -1.0, 1.0, steps);

	// Precompute some values for the 2D integrals
	static MatrixXd preCalc1, preCalc2;
	static int _steps = 0;
	
	bool newSteps = steps != _steps;

	if(preCalc1.size() == 0 || newSteps) {
		_steps = steps;
		preCalc1 = MatrixXd::Zero(steps, steps);

		for(int i = 0; i < steps; i++)
			for(int j = 0; j < steps; j++)
				preCalc1(i, j) = sqrt(1 - (extrnl[i] * extrnl[i])) * cos(intrnl[j]);
	}

	if(preCalc2.size() == 0 || newSteps) {
		preCalc2 = MatrixXd::Zero(steps, steps);

		for(int i = 0; i < steps; i++)
			for(int j = 0; j < steps; j++)
				preCalc2(i, j) = extrnl[i] * intrnl[j];
	}

	// The 2D integral
	#pragma omp parallel for default(shared) schedule(static) reduction(+ : res)
	for(int ex = 0; ex < steps; ex++) {	// The external integral from -1 to 1 over d(x)
		complex <double> R (0.0,0.0);


		if(pStop && *pStop)		// Place these line strategically in
			continue;			// slow models.

		for(int in = 0; in < steps; in++) {	// The internal integral from 0 to 2PI over d(phi)
			double tempVal = q * (preCalc1(ex, in) * hRadius + halfPitch * preCalc2(ex, in) / PI);
			
			// e^i*tempVal
			R += complex<double>(cos(tempVal), sin(tempVal)) * ww_in[in];
		}

		if (extrnl[ex] != 0.0)
			R *= sin(N * q * extrnl[ex] * halfPitch) / sin(q * extrnl[ex] * halfPitch);
		else
			R *= N;

		res += norm(R) * ww_ex[ex];
	} //End of external integral

	complex <double> sum (0.0, 0.0);
	for (int j = 0; j < helices; j++) {	// the sum including the delta j 
		sum += exp(i * 2.0 * PI / pitch * delta[j]) * (deltaED[j] - edSolvent) * rCs[j] * bessel_j1(q * rCs[j]);
	}
	res *= norm(sum) / (4.0 *PI);

	res *= a[ma-EXTRA_PARAMS];   // Multiply by scale
	res += a[ma-EXTRA_PARAMS+1]; // Add background

	return res * pitch * pitch / (4.0 * PI);
}



//roi's microemulsion model
double emulsionIntensity(double q, VectorXd& a, int ma, int nd) {
	//ma - size of a
	//a[0] and a[1] are the solvent parameters and are not used in this model
	//a[2] - Izero - Adjustable parameter, sharpens or broadens the peak (Theoretically the
	//intensity at q=0.
	//a[3] - Imax - Intensity at the top of experimental SAXS graph peak
	//a[4] - qmax - q of maximum intensity
		
	double Izero, Imax, qmax, scale, bg, res = 0.0; 
	
	Izero = a[ma-EXTRA_PARAMS+2];
	Imax = a[ma-EXTRA_PARAMS+3];
	qmax = a[ma-EXTRA_PARAMS+4];

	scale = a[ma-EXTRA_PARAMS];
	bg = a[ma-EXTRA_PARAMS+1];
	
	res = Izero/((1-Izero/Imax)*(q*q/(qmax*qmax)-1)*(q*q/(qmax*qmax)-1)+Izero/Imax);

	res *= scale;   // Multiply by scale
	res += bg; // Add background

	return res;
}*/

