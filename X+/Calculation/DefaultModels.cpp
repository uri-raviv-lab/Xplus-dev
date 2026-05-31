#define EXPORTER
#include "ModelContainer.h"

// Specific model headers
#include "CylindricalModels.h"
#include "HelicalModels.h"
#include "SlabModels.h"
#include "SphericalModels.h"
#include "OtherModels.h"

// The model container (contained within the backend DLL) that
// contains the default models included with the program

int GetNumModels() {
	return 14;
}

std::string GetModelName(int index) {
	switch(index) {
		default:
			return "N/A";

		case 0:
			return "Uniform Hollow Cylinder";
		case 1:
			return "Cylindroid";		
		case 2:
			return "Sphere";
		case 3:
			return "Cuboid";
		case 4:
			return "Symmetric Layered Slabs";
		case 5:
			return "Asymmetric Layered Slabs";		
		case 6:
			return "Helix";
		case 7:
			return "Discrete Helix";
		// From here on, the models are not reachable from within the OpeningWindow
		case 8:
			return "Gaussian Slabs";
		case 9:
			return "Membrane";
		case 10:
			return "Gaussian Sphere";
		case 11:
			return "Gaussian Hollow Cylinder";
		case 12:
			return "Smooth Sphere";
		case 13:
			return "Microemulsion";
	}
}

Model *GetModel(int index, ProfileShape shape) {
	// This serves as an abstract factory of sorts
	switch(index) {
		default:
			return NULL;

		case 0:
			if(shape == DISCRETE)
				return new UniformHCModel();
		case 1:
			return new Cylindroid();
		case 2:
			return new UniformSphereModel();
		case 3:
			return new CuboidModel();
		case 4:
			return new UniformSlabModel();
		case 5:
			return new UniformSlabModel("Asymmetric Uniform Slabs", ASYMMETRIC);
		case 6:
			return new HelixModel();
		case 7:
			return new DelixModel();
		case 8:
			return new GaussianSlabModel();
		case 9:
			return new MembraneModel();
		case 10:
			return new GaussianSphereModel();
		case 11:
			return new GaussianHCModel();
		case 12:
			return new SmoothSphereModel();
		case 13:
			return new MicroemulsionModel();
	}

	return NULL;
}