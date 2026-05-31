#include <windows.h> // For LoadLibrary etc.

#include "ExternalModelDialog.h"

namespace GUICLR {

	void ExternalModelDialog::FreeModelContainer() {
		if(hModule)
			FreeLibrary(hModule);

		hModule = NULL;

		// Trust the other windows to delete selectedmodel
		_selectedModel = NULL;
	}

	bool ExternalModelDialog::LoadModelContainer() {
		NumModels = NULL;
		ModelName = NULL;
		GetModelF = NULL;

		// Free previous model container
		FreeModelContainer();

		hModule = LoadLibrary(clrToWstring(filename).c_str());
		if(hModule == NULL) {
			MessageBox::Show("Invalid library");
			return false;
		}

		NumModels = (numModelsFunc)GetProcAddress(hModule, "GetNumModels");
		ModelName = (modelNameFunc)GetProcAddress(hModule, "GetModelName");
		GetModelF = (getModelFunc) GetProcAddress(hModule, "GetModel");
		if(NumModels && ModelName && GetModelF)
			return true;
		
		// Handle failures
		if(!NumModels)
			MessageBox::Show("Invalid model container");
		else if(!ModelName)
			MessageBox::Show("Invalid model container (2)");
		else if(!GetModelF)
			MessageBox::Show("Invalid model container (3)");
		
		NumModels = NULL;
		ModelName = NULL;
		GetModelF = NULL;
		FreeModelContainer();
		
		return false;
	}

	void ExternalModelDialog::LoadDefaultModels() {
		models->Items->Clear();
		models->Enabled = false;

		// Populate the combobox
		int nModels = GetNumModels();
		for(int i = 0; i < nModels; i++)
			models->Items->Add(stringToClr(GetModelName(i)));

		models->Enabled = (models->Items->Count > 0);
		if(models->Enabled)
			models->SelectedIndex = 0;

	}
	
	void ExternalModelDialog::GetAllModels() {
		models->Items->Clear();
		models->Enabled = false;

		// Try to load the model container
		if(!LoadModelContainer())
			return;

		// Populate the combobox
		int nModels = NumModels();
		for(int i = 0; i < nModels; i++)
			models->Items->Add(stringToClr(ModelName(i)));

		models->Enabled = (models->Items->Count > 0);
		if(models->Enabled)
			models->SelectedIndex = 0;
	}
	
	void ExternalModelDialog::models_SelectedIndexChanged(System::Object^  sender, 
														  System::EventArgs^  e) {
		if(_selectedModel)
			delete _selectedModel;
		_selectedModel = NULL;

		// If it's a default model, get it from the default functions
		if(!hModule) {
			_selectedModel = dynamic_cast<FFModel *>(GetModel(models->SelectedIndex, DISCRETE));
			ok->Enabled = (_selectedModel != NULL);
			return;
		}

		// Try to get the model and downcast it to form factor
		_selectedModel = dynamic_cast<FFModel *>(GetModelF(models->SelectedIndex, DISCRETE));

		ok->Enabled = (_selectedModel != NULL);
	}

};
