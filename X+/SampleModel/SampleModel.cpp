#include "SampleModel.h"

// OpenGL includes
#ifdef _WIN32
#include <windows.h>
#endif

#include <gl/gl.h>
#include <gl/glu.h>
// END of OpenGL includes

// Returns the number of models in this container
int GetNumModels() {
	return 2;	
}

// Returns the model's display name from the index. Supposed to return "N/A"
// for indices that are out of bounds
std::string GetModelName(int index) {
	switch(index) {
		default:
			return "N/A";
		case 0:
			return "Sample 1";
		case 1:
			return "Nonexistent model";
	}
}

// Returns the model object that matches the index. Supposed to return NULL
// for indices that are out of bounds
Model *GetModel(int index, ProfileShape shape) {
	switch(index) {
		default:
			return NULL;
		case 0:
			return new SampleModel();
	}
}



// SampleModel functions

SampleModel::SampleModel() : FFModel("Sample 1", 4, 2, 1, 5) {}

// A simple sine wave
double SampleModel::Calculate(double q, int nLayers, VectorXd& p) {
	return sin(q * 10.0);
}

std::complex<double> SampleModel::CalculateFF(Vector3d qvec, int nLayers, double w, 
											  double precision, VectorXd& p) {
	return std::complex<double> (0.0, 1.0);
}

// Sample extra parameters
ExtraParam SampleModel::GetExtraParameter(int index) {
	switch(index) {
	default:
		return ExtraParam("N/A");
	case 0:
		return ExtraParam("Infinity", 0.0, true);
	case 1:
		return ExtraParam("Ranged", 3.0, false, false, true, 1.0, 5.0, false);
	case 2:
		return ExtraParam("AbsIntegral", 5, true, true, false, 0.0, 0.0, true);
	case 3:
		return ExtraParam("Precise", 1.0, false, false, false, 0.0, 0.0, false, 8);
	}
}

int SampleModel::GetNumDisplayParams() {
	return 1;
}

std::string SampleModel::GetDisplayParamName(int index) {
	if(index == 0)
		return "Solvent Mult";
	return "N/A";
}

double SampleModel::GetDisplayParamValue(int index, const paramStruct *p) {
	if(index == 0) {
		// Returns solvent radius * ED
		return p->params[0][0].value * p->params[1][0].value;
	}
	return 0.0;
}

void SampleModel::DrawOpenGLPreview(const paramStruct& p) {
	DrawPreviewScene();
}

void SampleModel::DrawPreviewScene() {
	int numc = 32, numt = 32;
	int i, j, k;
	double s, t, x, y, z, twopi;

	double scale = 0.4;

	glEnable(GL_LIGHTING);

	GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 0.9f, 0.9f, 0.9f, 1.0f };
	GLfloat LightPosition[] = { 0.0f, 0.0f, 2.0f, 1.0f };
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);		// Setup The Ambient Light
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);		// Setup The Diffuse Light
	glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);	// Position The Light
	glEnable(GL_LIGHT1);								// Enable Light One

	
	GLfloat mat[] = { 0.7f, 0.5f, 0.0f, 1.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
		mat);

	twopi = 2 * 3.1415926;
	for (i = 0; i < numc; i++) {
		glBegin(GL_QUAD_STRIP);
		for (j = 0; j <= numt; j++) {
			for (k = 1; k >= 0; k--) {
				s = (i + k) % numc + 0.5;
				t = j % numt;

				x = (1+scale*cos(s*twopi/numc))*cos(t*twopi/numt);
				y = (1+scale*cos(s*twopi/numc))*sin(t*twopi/numt);
				z = scale * sin(s * twopi / numc);
				glVertex3d(x, y, z);
			}
		}
		glEnd();
	}

	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);
}

