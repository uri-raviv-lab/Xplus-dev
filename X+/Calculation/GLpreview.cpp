#include "GLPreview.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <gl/gl.h>
#include <gl/glu.h>

#include <vector>
#include <cmath>

using std::vector;

static vector<GLfloat> mtsins, mtcoses;
static vector<GLfloat> sins, coses, thetaa;

GLUquadricObj *quad = NULL;

/************************************************************************/
/* Draws an N-Layered Hollow Sphere                                     */
/************************************************************************/
void DrawGLNLHollowSphere(GLfloat *rad, GLfloat *ed, int n, GLfloat oRad) {
	if(!quad) {
		quad = gluNewQuadric();
		gluQuadricNormals(quad, GLU_SMOOTH);
	}
	

	GLfloat totalRad = 0.0, cumRad;
	GLfloat matGreen[] = { 0.2f, 0.8f, 0.0f, 1.0f };
	GLfloat matRed[] = { 0.8f, 0.2f, 0.0f, 1.0f };


	glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
	{
		GLfloat mat[] = { 0.7f, 0.0f, 0.1f, 1.0f };
		GLdouble clip[] = { 0.6, -1.0, -0.8, 0.5 };

		glClipPlane(GL_CLIP_PLANE1, clip);
		glEnable(GL_CLIP_PLANE1);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					mat);
	}

	gluSphere(quad, oRad, 32, 32);

	glDisable(GL_CLIP_PLANE1);

	glTranslatef(-0.15f, 0.27f, 0.2f);
	glRotatef(55.0f, -1.8f, -1.1f, 0.0f);

	glDisable(GL_LIGHT0); glDisable(GL_LIGHTING);

	for(int i = 0; i < n; i++)
		totalRad += rad[i];

	cumRad = oRad * (rad[0] / totalRad);

	for(int i = 1; i < n; i++) {
		// Front face
		GLfloat matDensity[] = { (ed[i] * matRed[0] + 
								 (1000.0f - ed[i]) * matGreen[0]) / 1000.0f,
								 (ed[i] * matRed[1] + 
								 (1000.0f - ed[i]) * matGreen[1]) / 1000.0f,
								  0.0f	};
		glColor3fv(matDensity);
		gluDisk(quad, cumRad, cumRad + (oRad * (rad[i] / totalRad)), 32, 32);

		cumRad += oRad * (rad[i] / totalRad);
	}
}

/************************************************************************/
/* Draws a Sphere														*/
/************************************************************************/
void DrawGLSphere(float ed) {
	glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
	{
		GLfloat mat[] = { 0.7f, 0.0f, 0.1f, 1.0f };

		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					 mat);
	}
	gluSphere(quad, 1.5f, 32, 32);
	
	glDisable(GL_LIGHT0); glDisable(GL_LIGHTING);
}

/************************************************************************/
/* Draws a Hollow Cylindroid                                            */
/************************************************************************/
void DrawGLCylindroid(float innerRadius, float outerRadius, 
			   float height) {
   double rad[2] = { innerRadius, outerRadius };
   double ed[2] = { 0.0, 0.0 };
   glScaled(1.3, 0.7, 1.0);
   DrawGLNLayeredHC(rad, height, ed, 2, outerRadius);
}

/************************************************************************/
/* Draws an N-Layered Cylindroid                                        */
/************************************************************************/
void DrawGLNLayeredCylindroid(double *rad, double height, 
							  double *ed, int n, double oRad) {
	glScaled(1.0, 0.7, 1.0);
	DrawGLNLayeredHC(rad, height, ed, n, oRad);
}

/************************************************************************/
/* Draws an N-Layered Hollow Cylinder                                   */
/************************************************************************/
void DrawGLNLayeredHC(GLdouble *rad, GLdouble height, 
					  GLdouble *ed, int n, GLdouble oRad) {
	if(!quad) {
	  quad = gluNewQuadric();
	  gluQuadricNormals(quad, GLU_SMOOTH);
	}

	GLdouble totalRad = 0.0, cumRad;
	GLdouble matGreen[] = { 0.2f, 0.8f, 0.0f, 1.0f };
	GLdouble matRed[] = { 0.8f, 0.2f, 0.0f, 1.0f };	

	glTranslated(0.0, 0.0, -1 * height / 2); // Center

	glColor3d(0.0, 0.1, 0.8);
	gluCylinder(quad, oRad, oRad, height, 32, 32);

	for(int i = 0; i < n; i++)
		totalRad += rad[i];

	cumRad = oRad * (rad[0] / totalRad);

	for(int i = 1; i < n; i++) {
		// Front face
		GLdouble matDensity[] = { (ed[i] * matRed[0] + 
								 (1000.0f - ed[i]) * matGreen[0]) / 1000.0f,
								 (ed[i] * matRed[1] + 
								 (1000.0f - ed[i]) * matGreen[1]) / 1000.0f,
								  0.0f	};
		glColor3dv(matDensity);
		gluDisk(quad, cumRad, cumRad + (oRad * (rad[i] / totalRad)), 32, 32);

		glTranslated(0.0, 0.0, height); // Back face

		glColor3dv(matDensity);
		gluDisk(quad, cumRad, cumRad + (oRad * (rad[i] / totalRad)), 32, 32);

		glTranslated(0.0, 0.0, -height); // Back face

		cumRad += oRad * (rad[i] / totalRad);
	}
}

/************************************************************************/
/* Draws an Discrete Helix			                                    */
/************************************************************************/
void DrawGLMicrotubule(GLfloat r, int totalsize) {
{
	if(!quad) {
		quad = gluNewQuadric();
		gluQuadricNormals(quad, GLU_SMOOTH);
	}

	GLfloat x, y;

	if(mtsins.size() == 0) {
		float vv;
		for(int i = 0; i <= 360; i += 20) {
			vv=(i/180.0f*3.142f);
			mtsins.push_back(sin(vv));
			mtcoses.push_back(cos(vv));
		}
	}	

	glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);

	GLfloat matBlue[] = { 0.0f, 0.1f, 0.8f, 1.0f };
	GLfloat matGreen[] = { 0.0f, 0.8f, 0.1f, 1.0f };

	glTranslatef(0.0f, -r * (float)totalsize / 5.0f, (float)totalsize * (-1) / 10.0f);

	bool bGreen = true;
	for(int rad=0; rad < totalsize; rad++)			
	{
		if(!bGreen) {
			glMaterialfv(GL_FRONT_AND_BACK, 
				GL_AMBIENT_AND_DIFFUSE,
				matGreen); bGreen = true;
		} else {
			glMaterialfv(GL_FRONT_AND_BACK, 
				GL_AMBIENT_AND_DIFFUSE,
				matBlue); bGreen = false;
		}
		for(int theta=0; theta < 360 / 20; theta++)
		{
			x = float(mtcoses.at(theta) * 2) * r;					
			y = float(mtsins.at(theta) * 2) * r;					

			glTranslatef(x, y, r / 10.0f);

			gluSphere(quad, r, 12, 12);
		}
	}
	glDisable(GL_LIGHT0); glDisable(GL_LIGHTING);
	}
}

void DrawGLMembrane(GLfloat r, int height, int size, GLfloat headED) {
	if(!quad) {
		quad = gluNewQuadric();
		gluQuadricNormals(quad, GLU_SMOOTH);
	}

	GLfloat matRed[] = { 0.9f, 0.1f, 0.0f, 1.0f };
	GLfloat matInside[] = { 2 * headED, 1.0f, 0.0f, 1.0f };

	glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,	matRed);

	glFogi(GL_FOG_MODE, GL_LINEAR);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_DENSITY, 0.5f);
	glFogf(GL_FOG_START, 3.0f);
	glFogf(GL_FOG_END, 6.0f);
	glEnable(GL_FOG);

	GLfloat h = float(height) / 10.0f;
	
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

	glTranslatef(-2*r * size / 2.0f, -2*r * size / 2.0f, 0.0f);

	for(int x = 0; x < size; x++) {
		glTranslatef(2*r, 0, 0);
		for(int y = 0; y < size; y++) {
			glTranslatef(0, 2*r, h);
			gluSphere(quad, r, 12, 12);

			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,	matInside);
			glBegin(GL_LINES);

			glVertex3f(0, 0, 0);
			glVertex3f(r/2.0f, 0, -h);

			glVertex3f(r/2.0f, 0, -h);
			glVertex3f(0, 0, -2*h);

			glVertex3f(0, 0, 0);
			glVertex3f(-r/2.0f, 0, -h);

			glVertex3f(-r/2.0f, 0, -h);
			glVertex3f(0, 0, -2*h);
			glEnd();
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,	matRed);

			glTranslatef(0, 0.0f, -2*h);
			gluSphere(quad, r, 12, 12);
			glTranslatef(0, 0, h);
		}
		glTranslatef(0, -2*size*r, 0.0f);
	}

	glDisable(GL_LIGHT0); glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
}

void DrawGLHelix(double ed) {
	GLfloat matDensity[] = { (GLfloat)(ed * 0.0008 + (1000.0f - ed) * 0.0),
							 (GLfloat)(ed * 0.0 + (1000.0f - ed) * 0.0008),
							 0.0f	};
	if(ed < 0)
		glColor3f(0.0f, 0.3f, 1.0f);
	else
		glColor3f(matDensity[0], matDensity[1], matDensity[2]);

	GLfloat x;													// Helix x Coordinate
	GLfloat y;													// Helix y Coordinate
	GLfloat z;													// Helix z Coordinate
	int phi;													// Angle
	int theta;													// Angle
	float		vertexes[4][3];	
																// Angles
	GLfloat r;													// Radius Of Twist
	int twists = 5;		// 2r Twists

	if(thetaa.size() == 0) {
		float vv;
		for(int i = 0; i <= 360 * twists; i += 20) {
			vv=(i/180.0f*3.142f);
			sins.push_back(sin(vv));
			coses.push_back(cos(vv));
			thetaa.push_back(vv);
		}
	}

	glScaled(0.5, 0.5, 0.5);
	glTranslatef(0, 2.5, 0);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

	r=0.25f;														// Radius


	glBegin(GL_QUADS);											// Begin Drawing Quads
	for(phi=0; phi < 18; phi++)							// 360 Degrees In Steps Of 20
	{
		for(theta=0; theta < 18*twists; theta++)			// 360 Degrees * Number Of Twists In Steps Of 20
		{
			x=float(coses[theta]*(2.0f+coses[phi] ))*r;					// Calculate x Position (1st Point)
			y=float(sins[theta]*(2.0f+coses[phi] ))*r;					// Calculate y Position (1st Point)
			z=float((( thetaa[theta]-(2.0f*3.142f)) + sins[phi] ) * r);		// Calculate z Position (1st Point)

			vertexes[0][0]=x;									// Set x Value Of First Vertex
			vertexes[0][1]=y;									// Set y Value Of First Vertex
			vertexes[0][2]=z;									// Set z Value Of First Vertex

			x=float(coses[theta+1]*(2.0f+coses[phi] ))*r;					// Calculate x Position (1st Point)
			y=float(sins[theta+1]*(2.0f+coses[phi] ))*r;					// Calculate y Position (1st Point)
			z=float((( thetaa[theta+1]-(2.0f*3.142f)) + sins[phi] ) * r);		// Calculate z Position (1st Point)

			vertexes[1][0]=x;									// Set x Value Of Second Vertex
			vertexes[1][1]=y;									// Set y Value Of Second Vertex
			vertexes[1][2]=z;									// Set z Value Of Second Vertex

			x=float(coses[theta+1]*(2.0f+coses[phi+1] ))*r;					// Calculate x Position (1st Point)
			y=float(sins[theta+1]*(2.0f+coses[phi+1] ))*r;					// Calculate y Position (1st Point)
			z=float((( thetaa[theta+1]-(2.0f*3.142f)) + sins[phi+1] ) * r);		// Calculate z Position (1st Point)

			vertexes[2][0]=x;									// Set x Value Of Third Vertex
			vertexes[2][1]=y;									// Set y Value Of Third Vertex
			vertexes[2][2]=z;									// Set z Value Of Third Vertex

			x=float(coses[theta]*(2.0f+coses[phi+1] ))*r;					// Calculate x Position (1st Point)
			y=float(sins[theta]*(2.0f+coses[phi+1] ))*r;					// Calculate y Position (1st Point)
			z=float((( thetaa[theta]-(2.0f*3.142f)) + sins[phi+1] ) * r);		// Calculate z Position (1st Point)

			vertexes[3][0]=x;									// Set x Value Of Fourth Vertex
			vertexes[3][1]=y;									// Set y Value Of Fourth Vertex
			vertexes[3][2]=z;									// Set z Value Of Fourth Vertex

			// Render The Quad
			glVertex3f(vertexes[0][0],vertexes[0][1],vertexes[0][2]);
			glVertex3f(vertexes[1][0],vertexes[1][1],vertexes[1][2]);
			glVertex3f(vertexes[2][0],vertexes[2][1],vertexes[2][2]);
			glVertex3f(vertexes[3][0],vertexes[3][1],vertexes[3][2]);
		}
	}
	glEnd();													// Done Rendering Quads

	glTranslatef(0,0,50);
}

void DrawGLRectangular(GLfloat ed) {
	GLfloat matDensity[] = { (GLfloat)(ed * 0.0008 + (1000.0f - ed) * 0.0),
							 (GLfloat)(ed * 0.0 + (1000.0f - ed) * 0.0008),
							 0.0f	};
	if(ed < 0)
		glColor3f(0.0f, 0.3f, 1.0f);
	else
		glColor3f(matDensity[0], matDensity[1], matDensity[2]);

	glTranslatef(0, -1.0, 0);

	glBegin(GL_QUADS);

	// Front
      glVertex3f( -1,  0,  1 );
      glVertex3f( -1,  2,  1 );
      glVertex3f(  1,  2,  1 );
      glVertex3f(  1,  0,  1 );

	// Back
	  glVertex3f( -1,  0, -1 );
	  glVertex3f(  1,  0, -1 );
	  glVertex3f(  1,  2, -1 );
	  glVertex3f( -1,  2, -1 );

	// Left side
	  glVertex3f( -1,  0,  1 );
	  glVertex3f( -1,  2,  1 );
	  glVertex3f( -1,  2, -1 );
	  glVertex3f( -1,  0, -1 );

	// Right side
	  glVertex3f(  1,  0,  1 );
	  glVertex3f(  1,  0, -1 );
	  glVertex3f(  1,  2, -1 );
	  glVertex3f(  1,  2,  1 );
	glEnd();

}

void DrawGLNLayeredAsymSlabs(GLfloat *rad, GLfloat *ed, GLfloat height, 
							 int n) {
	GLfloat totalRad = 0.0, cumRad = 0.0;
	glColor3f(0.0f, 0.1f, 0.8f);

	for(int i = 1; i < n; i++)
		totalRad += rad[i];

	glTranslatef(0.0f, -height / 2.0f, 0.0f);

	glBegin(GL_QUADS);

	for(int i = 1; i < n; i++) {
	  GLfloat matDensity[] = { (GLfloat)(ed[i] * 0.0008 + (1000.0f - ed[i]) * 0.0),
						  	   (GLfloat)(ed[i] * 0.0 + (1000.0f - ed[i]) * 0.0008),
							   0.0f	};
	  glColor3fv(matDensity);

	  glVertex3f( -1,  height * cumRad / totalRad,  1 );
	  glVertex3f(  1,  height * cumRad / totalRad,  1 );

	  cumRad += rad[i];

      glVertex3f(  1,  height * cumRad / totalRad,  1 );
	  glVertex3f( -1,  height * cumRad / totalRad,  1 );
      
	}

	glEnd();
}

void DrawGLNLayeredSlabs(GLfloat *rad, GLfloat *ed, GLfloat height, int n) {
	if(n <= 1)
		return;
	// 2n - 1 layers of asymmetric slabs
	GLfloat *newRad = new GLfloat[(2 * n) - 2];
	GLfloat *newED = new GLfloat[(2 * n) - 2];

	newRad[0] = rad[0];
	newED[0] = ed[0];

	for(int i = 1; i <= (n - 1); i++) {
		newRad[i] = rad[n - i];
		newED[i] = ed[n - i];
	}

	for(int i = n; i < ((2 * n) - 2); i++) {
		newRad[i] = rad[i - n + 2];
		newED[i] = ed[i - n + 2];
	}

	DrawGLNLayeredAsymSlabs(newRad, newED, height, (2 * n) - 2);

	delete[] newRad;
	delete[] newED;
}
