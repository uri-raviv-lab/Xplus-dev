#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#define sleep(x) Sleep((x) * 1000)
#else
#include <unistd.h>
#endif

#include "calculation_external.h"

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

int const PBAR_SIZE = 40;

void printProgress(int progress) {
    if(progress < 0)   progress = 0;
    if(progress > 100) progress = 100;
        
    printf("\r[");
    for(int i = 0; i < PBAR_SIZE; i++) {
        if(i < ((progress * PBAR_SIZE) / 100))
            printf("#");
        else
            printf(" ");
    }
    printf("] %d%%", progress);
    fflush(stdout);
}


/**
 * Prints the available model types and lets you choose one
 **/
Model *InputModelType(bool *bGauss) {
    char answer[16] = {0};
    printf("Available model types:\n");
	
	for(int i = 0; i < GetNumModels(); i++)
		printf("\t%d. %s\n", i + 1, GetModelName(i).c_str());

	Model *model = NULL;
	while(!model) {
		printf("Which model would you prefer? ");
		scanf("%s", answer);

		model = GetModel(atoi(answer) - 1);
	}

	return model;
}


/**
 * If there is a gaussian ED model, allows the user to choose whether
 * or not the ywould like to use it.
 **/
bool InputEDType(ModelType modelT) {
	//if we don't have a gaussian ED model
	if(!(modelT == MODEL_SLAB)) 
		return false;

	char answer[16] = {0};
    printf("Two types of models exist:\n");
    printf("\t1. Discrete electron density profile\n"
           "\t2. Gaussian electron density profile\n"
           "Which model would you prefer? "
        );
    scanf("%s", answer);

    return (atoi(answer) == 2);
}
/**
 * Prints the available peak types and lets you choose one
 **/
PeakType InputPeakType() {
    char answer[16] = {0};
    printf("Available peak types:\n");
    printf("\t1. Gaussians\n"
           "\t2. Lorentzians\n"
           "\t3. Lorentzians Squared\n"
           //"\t4. Caille\n"
           "Which model would you prefer? "
        );
    scanf("%s", answer);

    switch(atoi(answer)) {
    default:
    case 1:
        return SHAPE_GAUSSIAN;
    case 2:
        return SHAPE_LORENTZIAN;
    case 3:
		return SHAPE_LORENTZIAN_SQUARED;
    //case 4:
    //    return SHAPE_CAILLE;
    }
}

/**
 * Simple helper function which empties the fit range vectors so that the fitter
 * won't crash
 **/
void setEmptyVectors(paramStruct *p, int layers) {
}

void requestData(const char *question, Parameter& param) {
    char answer[100] = {0};
    
    printf("Input %s ", question);
    scanf("%s", answer);
	param.value = strtod(answer, NULL);

    printf("Mutable? [y/n] ");
    scanf("%s", answer);
	param.isMutable = (tolower(answer[0]) == 'y');
}

paramStruct InputInitialGuess(Model *type) {
    paramStruct result (type);
    int layers = 0;
    char question[100] = {0}, curLayer[100] = {0};
    
	do {
		printf("Input number of layers (%d - %d): ", type->GetMinLayers(),
			   type->GetMaxLayers());
		scanf("%s", question);
		layers = atoi(question);		
	} while(layers < type->GetMinLayers() && layers > type->GetMaxLayers());

	result.layers = layers;

	// Clear layer parameter vectors
	result.params.resize(type->GetNumLayerParams());
	for(int i = 0; i < type->GetNumLayerParams(); i++)
		result.params[i].resize(layers);
	
	// Get layer parameters
    for(int i = 0; i < layers; i++) {		
		for(int j = 0; j < type->GetNumLayerParams(); j++) {
			strcpy(curLayer, type->GetLayerName(i).c_str());
		
			if(type->IsParamApplicable(i, j)) {
				sprintf(question, "%s %s:", curLayer, type->GetLayerParamName(j).c_str());
				requestData(question, result.params[j][i]);
			} else {
				// When the layer parameter is N/A
				result.params[j][i].value = -1;
				result.params[j][i].isMutable = false;
			}
		}
	}
    


	// Get extra parameters
	result.extraParams.resize(type->GetNumExtraParams());
	for(int i = 0; i < type->GetNumExtraParams(); i++) {
		char expstr[256] = {0};
		sprintf(expstr, "%s:", type->GetExtraParameter(i).name.c_str());
		requestData(expstr, result.extraParams[i]);
	}

    return result;
}

peakStruct InputInitialPeaks(PeakType pType) {
    peakStruct result;
    int peaks = 0;
    char answer[100] = {0}, curLayer[100] = {0};
    
    printf("Input number of peaks: ");
    scanf("%s", answer);
    peaks = atoi(answer);
    
    for(int i = 0; i < peaks; i++) {
        sprintf(curLayer, "Peak %d", i + 1);

        printf("Input %s Amplitude: ", curLayer);
        scanf("%s", answer);
        result.amp.push_back(strtod(answer, NULL));
        
        printf("Mutable? [y/n] ");
        scanf("%s", answer);
		result.amp.back().mut = (tolower(answer[0]) == 'y');
        
        printf("Input %s Center: ", curLayer);
        scanf("%s", answer);
        result.center.push_back(strtod(answer, NULL));
        
        printf("Mutable? [y/n] ");
        scanf("%s", answer);
        result.center.back().mut = (tolower(answer[0]) == 'y');  

        printf("Input %s FWHM: ", curLayer);
        scanf("%s", answer);
        result.fwhm.push_back(strtod(answer, NULL));

        printf("Mutable? [y/n] ");
        scanf("%s", answer);
        result.fwhm.back().mut = (tolower(answer[0]) == 'y');
    }
    
    return result;
}

void printParameters(paramStruct *p) {
    printf("Parameters:\n\t");

	// Print table header
	for(int i = 0; i < p->model->GetNumLayerParams(); i++)
		printf("%s\t\t", p->model->GetLayerParamName(i).c_str());
	printf("\n");

	// Print table contents
    for(int i = 0; i < p->layers; i++) {		
		// Print layer number
		printf("%d\t", i);

		for(int j = 0; j < p->model->GetNumLayerParams(); j++) {
			if(!p->model->IsParamApplicable(i, j))
				printf("N/A\t\t");
			else
				printf("%.6f\t\t", p->params[j][i].value);
		}

		printf("\n");
    }

    printf("\n");
    
	// Print extra parameters
	for(int i = 0; i < (int)p->extraParams.size(); i++) {
		char printed[1024] = {0};

		// Print correct amount of decimal points
		sprintf(printed, "Extra[%%d]: %%.%df\n", 
				p->model->GetExtraParameter(i).decimalPoints);

        printf(printed, i, p->extraParams[i].value);
	}
    
}

int usage() {
    fprintf(stderr, "USAGE: xplus <DATA FILE> <OUTPUT MODEL> "
            "[-b/--baseline BASELINE FILE] [-i/--input INITIAL GUESS INI] [-o/--output RESULT INI]\n"
            "Note that baseline and INI file are not required\n");
    return 1;
}

int main(int argc, wchar_t **argv) {
    // Filenames
    wchar_t data[MAX_PATH], baseline[MAX_PATH], params[MAX_PATH], outparams[MAX_PATH], modelFile[MAX_PATH];
    bool bBaseline = false, bIni = false, bOutIni = false, bSF = false, bFitSF = false, bGauss = false;

    // Argument handling
    if(argc < 3)
        return usage();

    argc--; argv++; // Shifting the arguments forward
	wcscpy(data, *argv);

    argc--; argv++; // Shifting the arguments forward
    wcscpy(modelFile, *argv);

    argc--; argv++; // Shifting the arguments forward    
    while(argc > 0) {
		if(!wcscmp(*argv, L"-b") || !wcscmp(*argv, L"--baseline")) {
			argc--; argv++; // Shifting the arguments forward

			if(argc < 1)
				fprintf(stderr, "ERROR: No baseline file\n");
			else {
				bBaseline = true;
				wcsncpy(baseline, *argv, MAX_PATH);
				argc--; argv++; // Shifting the arguments forward
			}
		} else if(!wcscmp(*argv, L"-i") || !wcscmp(*argv, L"--input")) {
			argc--; argv++; // Shifting the arguments forward

			if(argc < 1)
				fprintf(stderr, "ERROR: No input file\n");
			else {
				bIni = true;
				wcsncpy(params, *argv, MAX_PATH);
				argc--; argv++; // Shifting the arguments forward
			}
		} else if(!wcscmp(*argv, L"-o") || !wcscmp(*argv, L"--output")) {
			argc--; argv++; // Shifting the arguments forward

			if(argc < 1)
				fprintf(stderr, "ERROR: No output file\n");
			else {
				bOutIni = true;
				wcsncpy(outparams, *argv, MAX_PATH);
				argc--; argv++; // Shifting the arguments forward
			}
		} else {
		  printf("Error in argument %s\n", *argv);
		  argc--; argv++;
		}
    }

    printf("Input stage\n");

	

	// Get the model type
	Model *type = InputModelType(&bGauss);

	paramStruct param (type);
	PeakType pType;
    peakStruct peaks;

	void *iniFile = NewIniFile();

	if(bIni)
		ReadParameters(std::wstring(params), type->GetName(), &param, iniFile);
	else // Input
		param = InputInitialGuess(type);

	param.model = type;

    printf("Does the signal include a structure factor? [y/n] ");
    char ans[256] = {0};
    scanf("%s", ans);

    
    if(tolower(ans[0]) == 'y') {
        bSF = true;
        
        printf("Would you like to fit the structure factor? [y/n] ");
        fflush(stdout);        

        scanf("%s", ans);
        if(tolower(ans[0]) == 'y')
            bFitSF = true;
    }

    
    if(bSF) {
		pType = InputPeakType();
		SetPeakType(pType);

		if(bIni)
			ReadPeaks(params, type->GetName(), &peaks, iniFile);
		else			
			peaks = InputInitialPeaks(pType);
	
    }

	CloseIniFile(iniFile);
	iniFile = NULL;

    // Work
    int pStop = 0;
    std::vector<double> ffx, ffy, bgly, resy, bg, errors, merrors;
	std::vector<bool> mask;
    bool bSuccess = true;
    // Baseline subtraction
    if(bBaseline) {
		// I assume the baseline file name should include "-baseline.out"
		// like in all other places...
		wcscat(baseline, L"-baseline.out");

        GenerateBGLinesandFormFactor(data, baseline, ffx, bgly, ffy, false);
    } else // File reading
        ReadDataFile(data, ffx, ffy);

	bg.resize(ffx.size(), 0.0);

    printf("\n");
	if(!bOutIni)
		printParameters(&param);
    
    if(!bFitSF) {
		if(bSF) {
			printf("\nGenerating Structure Factor...\n");
			bSuccess = GenerateStructureFactorU(ffx, resy, bg, &peaks, NULL, 
												&pStop, printProgress);
		}
        
		printf("\nFitting...\n");
		printProgress(0);

        bSuccess &= CreateModelU(ffx, ffy, resy, bg, mask, &param, errors, merrors, NULL, &pStop, 
                                 printProgress);
	
	} else {
		printf("\nGenerating Form Factor...\n");
		bSuccess = GenerateModelU(ffx, resy, bg, &param, NULL, 
								  &pStop, printProgress);
        
		printf("\nFitting...\n");
		printProgress(0);

		bSuccess &= FitStructureFactorU(ffx, ffy, resy, bg, mask, &peaks, errors, merrors, NULL, &pStop, 
										printProgress);
    }

    // Finish up
    printProgress(100);
    printf("\n");

    if(!bSuccess) {
        printf("Failed to fit model, exiting.\n");
		delete type;
        return 2;
    }

    // Output
    WriteDataFile(modelFile, ffx, resy);

	iniFile = NewIniFile();

	if(bOutIni) {
		WriteParameters(outparams, type->GetName(), &param, iniFile);
	} else {
		printf("Final ");
		printParameters(&param);
	}

	CloseIniFile(iniFile);
	iniFile = NULL;

	delete type;

    printf("Done successfully! R-Squared: %.12f\n", RSquared(ffy, resy));

    return 0;
}
