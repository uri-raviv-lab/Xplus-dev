#ifndef __GLPREVIEW_H
#define __GLPREVIEW_H

void DrawGLNLHollowSphere(float *rad, float *ed, int n, float oRad);

void DrawGLSphere(float ed);

void DrawGLCylindroid(float innerRadius, float outerRadius, float height);

void DrawGLNLayeredCylindroid(double *rad, double height, double *ed, int n, double oRad);

void DrawGLNLayeredHC(double *rad, double height, double *ed, int n, double oRad);

void DrawGLMicrotubule(float r, int totalsize);

void DrawGLMembrane(float r, int height, int size, float headED);

void DrawGLHelix(double ed);

void DrawGLRectangular(float ed);

void DrawGLNLayeredAsymSlabs(float *rad, float *ed, float height, 
							 int n);

void DrawGLNLayeredSlabs(float *rad, float *ed, float height, int n);

#endif