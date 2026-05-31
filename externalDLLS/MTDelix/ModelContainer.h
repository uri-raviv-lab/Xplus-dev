#ifndef __MODEL_CONTAINER_H
#define __MODEL_CONTAINER_H

/////////////////////////////////////////////////////////////////////
// Prelude:                                                        //
// This .h file has to be implemented by every DLL model container //
/////////////////////////////////////////////////////////////////////

#include <string> // For std::string
#include "..\X+\Calculation\Model.h" // For Model, ProfileShape

#undef EXPORTED
#ifdef _WIN32
#ifdef EXPORTER
#define EXPORTED __declspec(dllexport)
#else
#define EXPORTED __declspec(dllimport)
#endif
#else
#define EXPORTED extern "C"
#endif

#ifdef _WIN32
	// Since std::string is a C++ type and we are exporting C-type declarations,
	// we disable the "C++ type in C declaration" warning (so, so hacky)
	#pragma warning(push)
	#pragma warning(disable: 4190)

	#ifdef __cplusplus    // If used by C++ code, 
	extern "C" {          // we need to export the C interface
	#endif
#endif

// Returns the number of models in this container
EXPORTED int GetNumModels();

// Returns the model's display name from the index. Supposed to return "N/A"
// for indices that are out of bounds
EXPORTED std::string GetModelName(int index);

// Returns the model object that matches the index. Supposed to return NULL
// for indices that are out of bounds
EXPORTED Model *GetModel(int index, ProfileShape shape = DISCRETE);

#ifdef _WIN32
	#ifdef __cplusplus
	}
	#endif

	#pragma warning(pop)
#endif

#endif
