#pragma once
#include "OpeningWindow.h"
#include "About.h"
#include "SmoothWindow.h"
#include "ResultsWindow.h"
#include "ExtractBackground.h"
#include "FormFactor.h"
#include "DataManip.h"
#include "SignalSeries.h"

#include "UIsettings.h"
#include "calculation_external.h"
#include "clrfunctionality.h"

#include "edprofile.h"
#include <time.h>

#include "svnrev.h" // Current SVN revision

#include <fstream>
using std::wofstream;


#ifndef NOMINMAX

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#endif  /* NOMINMAX */

namespace GUICLR {

	void OpeningWindow::OpeningWindow_Load(System::Object^  sender, System::EventArgs^  e) {
		helpToolStripMenuItem->Visible = false;
		if(hasGPUBackend())
			gPUToolStripMenuItem->Enabled = true;

		//Find all dlls in a given (sub)directory
		array<System::String^>^ files = (System::IO::Directory::GetFiles(Application::StartupPath /*+ L"\\dlls" */, L"*.dll"));
		
		// Uncomment the following line if we release this version
		#define RELEASE_THIS_VERSION


		// Current revision/release
#ifndef RELEASE_THIS_VERSION
		this->Text += " (" + 			
#ifdef _DEBUG
			"DEBUG, " +
#endif
			"SVN R" + SVN_REV_STR + ")";	
#endif

		/* Fun and games */
		time_t tym;
		tm *ptm;
		tym = time(nullptr);
		ptm = localtime(&tym);
		
		if((ptm->tm_mday == 1 && ptm->tm_mon == 3))
			this->Text += "   April Fools!! Don't believe anything you hear today...";
		if((ptm->tm_mday == 8 && ptm->tm_mon == 0))
			this->Text += "   Uri's Birthday!!";
		if((ptm->tm_mday == 19 && ptm->tm_mon == 7))
			this->Text += "   Pablo's Birthday!!";
		if((ptm->tm_mday == 30 && ptm->tm_mon == 0))
			this->Text += "   Avi's Birthday!!";
		if((ptm->tm_mday == 14 && ptm->tm_mon == 10))
			this->Text += "   Tal's Birthday!!";
		if((ptm->tm_hour < 5))
			this->Text += " It's late. Wouldn't you rather be sleeping?";
		if((ptm->tm_hour >= 17 && ptm->tm_hour <= 19))
			this->Text += " It's late. Time to go home.";

		radioButton_CheckedChanged(rHC, gcnew EventArgs());
	}

	void OpeningWindow::button1_Click(System::Object^  sender, System::EventArgs^  e) {
		About about;
		_bRedrawGL = false;
		this->Visible = false;
		about.ShowDialog();
		_bRedrawGL = true;
		this->Visible = true;
	}

	void OpeningWindow::RenderOpeningGLScene() {
		glTranslatef(0.0f,0.0f,-6.0f);
		glRotatef(crx,0.0f,1.0f,0.0f);
		
		if(!bMousing)
			crx += 1.0f;

		if(crx > 360.0f) crx -= 360.0f;

		// Kept as reference for when we implement the DrawGL functions in Models
		/*
		switch(GetModelType()) {
		case MODEL_SPHERE: // Sphere
			glEnable(GL_LIGHTING); glEnable(GL_LIGHT0);
			{
				GLfloat mat[] = { 0.7f, 0.0f, 0.1f, 1.0f };

				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
							 mat);
			}
			gluSphere(quadratic, 1.5f, 32, 32);
			
			glDisable(GL_LIGHT0); glDisable(GL_LIGHTING);
			
			break;
	
		case MODEL_SLAB: // Membrane/Slab
		case MODEL_ASLAB:
			DrawGLSlab(quadratic, 0.2f, 6, 10, 1.0f);
			break;

		case MODEL_RECT:
			DrawGLRectangular(-1.0);
			break;

		case MODEL_DELIX:	//Tal, I meant to use that here... :)
			DrawGLMicrotubule(quadratic, 0.1f, 15);
			break;
		case MODEL_CYLINDROID: 
			glScaled(1.3, 0.7, 1.0);
			DrawGLRod(quadratic, 0.9f, 0.5f, 3.0f);
			break;
		}*/
		if(_currentModel)
			_currentModel->DrawPreviewScene();
	}


	void OpeningWindow::radioButton_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		int index = -1;

		if(sender == rHC)
			index = 0;
		else if(sender == rCylindroid)
			index = 1;
		else if(sender == rSphere)
			index = 2;
		else if(sender == rRect)
			index = 3;
		else if(sender == rMembrane)
			index = 4;
		else if(sender == rAsymSlab)
			index = 5;
		else if(sender == rHelix)
			index = 6;
		else if(sender == rDiscreteHelix)
			index = 7;

		selectedRadio = (RadioButton ^)sender;

		// Deleting previous model, if exists
		if(_currentModel)
			delete _currentModel;

		// Downcasting Model to FFModel since we only fit Form Factors
		_currentModel = dynamic_cast<FFModel *>(GetModel(index));
		button4->Enabled = (_currentModel != NULL);
	}

	void OpeningWindow::rExternalModel_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		// If we're leaving this radio button
		if(!rExternalModel->Checked) {
			emd->FreeModelContainer();
			rExternalModel->Text = "External Model...";
			return;
		}

		if(emd->ChooseModelContainer()) {			
			emd->ShowDialog();

			// Deleting previous model, if exists
			if(_currentModel) {
				delete _currentModel;
				_currentModel = NULL;
			}

			_currentModel = emd->GetSelectedModel();
			button4->Enabled = (_currentModel != NULL);
			if(_currentModel)
				rExternalModel->Text = "External Model: " + emd->GetSelectedModelName();

			selectedRadio = rExternalModel;
		} else
			radioButton_CheckedChanged(sender, e);
	}

	inline double vmax(vector<double>& v) {
		if(v.size() == 0)
			return 0.0;
		double val = v.at(0);
		for(unsigned int i = 0; i < v.size(); i++) {
			if(v[i] > val) {
				val = v[i];
			}
		}
		return val;
	}

	void OpeningWindow::button4_Click(System::Object^  sender, System::EventArgs^  e) {
		FormFactor ^ff;
		//_bRedrawGL = false;
		//this->Visible = false;
	
		pin_ptr<FFModel *> currentmodelptr = &_currentModel;

		if(generateModelToolStripMenuItem->Checked) {
			_bRedrawGL = false;
			this->Visible = false;
			ff = gcnew FormFactor(NULL, true, this);
			
			ff->ShowDialog();
		}
		else {
			std::wstring file;
			if(openDataFile(openFileDialog1, "Choose Data File", file, false)) {
				
				std::vector <double> x,y;

				ReadDataFile(file.c_str(), x, y);
			
				if (vmax(y)<1000.0) {
					System::Windows::Forms::DialogResult d=MessageBox::Show("The Intensity of your data is too low, would you like to scale it up?","Problem", System::Windows::Forms::MessageBoxButtons::YesNo);
					if(d==System::Windows::Forms::DialogResult::Yes) {
						std::wstring filename, dir;
						wchar_t temp[260] = {0};

						GetDirectory(file.c_str(), temp);
						dir = temp;
						wcsnset(temp, 0, 260);
						GetBasename(file.c_str(), temp);
						filename = temp;
						
						for (unsigned int i=0; i<y.size(); i++)   {
							y[i]*=1000; 
						}
						WriteDataFile((dir +filename + L"x1000.dat").c_str(),x,y);

						_bRedrawGL = false;
						this->Visible = false;
						ff = gcnew FormFactor((dir +filename + L"x1000.dat").c_str(), false, this);
						ff->ShowDialog();
					} else {
						_bRedrawGL = false;
						this->Visible = false;
						ff = gcnew FormFactor(file.c_str(), false, this);
						ff->ShowDialog();
					}
				} else {
					_bRedrawGL = false;
					this->Visible = false;
					ff = gcnew FormFactor(file.c_str(), false, this);
					ff->ShowDialog();
				}
				this->SetTopLevel(true);
			}
		}

		// Start anew
		if(selectedRadio == rExternalModel)
			rExternalModel->Text = "External Model: " + stringToClr(_currentModel->GetName());
		else
			radioButton_CheckedChanged(selectedRadio, gcnew EventArgs());

		_bRedrawGL = true;
		this->Visible = true;
		SetForegroundWindow(HWND(this->Handle.ToPointer()));

		OpenGL->MakeCurrent();
		if(ff)
			delete ff;
		Console::WriteLine( "Total Memory: {0}", GC::GetTotalMemory( false ) );

		System::GC::Collect();
		Console::WriteLine( "GGGTotal Memory: {0}", GC::GetTotalMemory( false ) );

	}

	void OpeningWindow::dragToFitToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		g_bDragToZoom = dragToFitToolStripMenuItem->Checked;
	}

	void OpeningWindow::dataAgainstBackgroundToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring data, background;
		if(openDataFile(openFileDialog1, "Choose Data File", 
						data, false)
			&&
		   openDataFile(openFileDialog1, "Choose Background Data", 
						background, true)) {
			struct graphLine graphs[2];
			graphs[0].color = RGB(255, 0, 0);
			graphs[1].color = RGB(0, 255, 0);

			graphs[0].legendKey = "Data";
			graphs[1].legendKey = "Background";

			ReadDataFile(data.c_str(), graphs[0].x, graphs[0].y);
			ReadDataFile(background.c_str(), graphs[1].x, graphs[1].y);
			ResultsWindow rw(graphs, 2);

			rw.ShowDialog();
		}
	}

	void OpeningWindow::formFactorAgainstModelToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring formfactor, model;
		if(openDataFile(openFileDialog1, "Choose Signal", 
						formfactor, true)
			&&
		   openDataFile(openFileDialog1, "Choose Model Fit", 
						model, true)) {
			struct graphLine graphs[2];
			graphs[0].color = RGB(255, 0, 0);
			graphs[1].color = RGB(0, 0, 255);

			graphs[0].legendKey = "Signal";
			graphs[1].legendKey = "Model";

			ReadDataFile(formfactor.c_str(), graphs[0].x, graphs[0].y);
			ReadDataFile(model.c_str(), graphs[1].x, graphs[1].y);
			ResultsWindow rw(graphs, 2);

			rw.ShowDialog();
		}
	}

	void OpeningWindow::fitExistingModelToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(fitExistingModelToolStripMenuItem->Checked)
			generateModelToolStripMenuItem->Checked = false;
		else
			fitExistingModelToolStripMenuItem->Checked = true;

		this->button4->Text = L"&Fit to Data >";
	}
	
	void OpeningWindow::generateModelToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(generateModelToolStripMenuItem->Checked)
			fitExistingModelToolStripMenuItem->Checked = false;
		else
			generateModelToolStripMenuItem->Checked = true;

		this->button4->Text = L"&Generate >";
	}

	void OpeningWindow::smoothDataToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring data;
		if(!openDataFile(openFileDialog1, "Choose Data File", 
						 data, false))
			return;
				
		SmoothWindow sw(data.c_str(), false);
		sw.ShowDialog();
	}

	void OpeningWindow::extractBackgroundieAirDataToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring data, background;
		if(!openDataFile(openFileDialog1, "Choose Data File", 
						 data, false))
			return;	
		if(!openDataFile(openFileDialog1, "Choose a Background Data File", 
						 background, true))
			return;	

		ExtractBackground eb(data.c_str(), background.c_str());

		eb.ShowDialog();
	}

	void OpeningWindow::plotElectronDensityProfileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		// A huge TODO comes here!!!
		////// TODO/ /////
		/*std::wstring params;
		openFileDialog1->Title = "Choose a Parameter File";
		openFileDialog1->Filter = "Parameter Files (*.ini)|*.ini|All files|*.*";
	
		if(openFileDialog1->ShowDialog() == 
			System::Windows::Forms::DialogResult::Cancel)
			return;

		clrToString(openFileDialog1->FileName, params);

		struct graphLine graphs[2];
		paramStruct par;
		ReadParameters(params.c_str(), typeToString(), &par);

		if(par.pl.r.size() == 0 || par.pl.ed.size() == 0) {
			MessageBox::Show("No model of this sort in this parameter file",
							 "No Such Model",
							 MessageBoxButtons::OK,
							 MessageBoxIcon::Error);
			return;
		}

		//NOTE: This cannot draw Gaussian ED profiles!!
		generateEDProfile(par.pl.r, par.pl.ed, par.pl.cs, graphs);
		std::pair<double, double> in = calcEDIntegral(par.pl.r, par.pl.ed);
		
		ResultsWindow rw(graphs, 2, "Positive one-sided area: " + Double(in.first).ToString("#.######") + 
			", Negative area: " + Double(in.second).ToString("#.######"));

		rw.ShowDialog();*/
	}
	void OpeningWindow::fileManipulationA1Nm1ToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		//		1) Select multiple files
		//		2) Multiply the q by 10 (A-1 --> nm-1)
		//		3) (optional) multiply I(q) by 1e-9
		//		4) Save files to subdirectory (/nm/)

		DataManip DM;
		DM.ShowDialog();
		}

	void OpeningWindow::cPUToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		gPUToolStripMenuItem->Checked = false;
		cPUToolStripMenuItem->Checked = true;
		SetGPUBackend(false);
	}

	void OpeningWindow::gPUToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(hasGPUBackend()) {
			gPUToolStripMenuItem->Checked = true;
			cPUToolStripMenuItem->Checked = false;
			SetGPUBackend(true);
		}
	}

	void OpeningWindow::OpeningWindow_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		switch (e->KeyCode) {
			//case Keys::F3:
			//	fitExistingModelToolStripMenuItem->Checked = true;
			//	generateModelToolStripMenuItem->Checked = false;
			//	fitExistingModelToolStripMenuItem_CheckedChanged(nullptr, e);
			//	e->Handled = true;
			//	break;
			//case Keys::F4:
			//	generateModelToolStripMenuItem->Checked = true;
			//	fitExistingModelToolStripMenuItem->Checked = false;
			//	generateModelToolStripMenuItem_CheckedChanged(nullptr, e);
			//	e->Handled = true;
			//	break;
			case Keys::F5:
				if(generateModelToolStripMenuItem->Checked) {
					fitExistingModelToolStripMenuItem->Checked = true;
					generateModelToolStripMenuItem->Checked = false;
					fitExistingModelToolStripMenuItem_CheckedChanged(nullptr, e);
				} else {
					generateModelToolStripMenuItem->Checked = true;
					fitExistingModelToolStripMenuItem->Checked = false;
					generateModelToolStripMenuItem_CheckedChanged(nullptr, e);
				}
				e->Handled = true;
				break;
			case Keys::F1:
				button1_Click(nullptr, e);
				e->Handled = true;
				break;
			default:
				//if(e->Shift && e->Control)
				//	consolidateAnalysesToolStripMenuItem->Visible = true;
				break;
		}
	}
	void OpeningWindow::OpeningWindow_KeyUp(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		//consolidateAnalysesToolStripMenuItem->Visible = false;
	}

	// This is a hidden option. To enable, hold down Shift and Ctrl
	void OpeningWindow::consolidateAnalysesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring file, laT, FFTitle, rTitle, edTitle, csTitle, exStr;
		void *iniFile;
		std::vector<std::vector<double>> params, final;
		std::vector<double> layer;
		graphTable			*dummy = new graphTable();
		int phaseTyp, maxLayers = 0, maxPeaks = 0, maxBG = 0, exPar = 0;

		OpenFileDialog ^consOFD = gcnew System::Windows::Forms::OpenFileDialog();
		consOFD->Multiselect = true;
		consOFD->Title = "Choose parameter files to consolidate...";
		consOFD->Filter = "Parameter files|*.ini";
		
		SaveFileDialog ^consSFD = gcnew System::Windows::Forms::SaveFileDialog();
		consSFD->AddExtension = true;
		consSFD->Filter = "Tab Separated Values|*.tsv";
		consSFD->Title = "Choose a filename to save parameters...";

		// Don't run without a selected model
		if(!_currentModel)
			return;

		if(consOFD->ShowDialog() == System::Windows::Forms::DialogResult::Cancel)
			return;

		// Ask the user to choose a type of model (rod, slab, etc.)
		// Use typeToString() for now

		for(int i = 0; i < consOFD->FileNames->Length; i++) {
			paramStruct			prl (_currentModel);
			peakStruct			pkl;
			cailleParamStruct	cll;
			bgStruct			bgl;
			phaseStruct			phl;

			layer.clear();

			iniFile = NewIniFile();

			// Read all parameter of modelT from file into prl, pkl, bgl and phl
			ReadParameters	(clrToWstring(consOFD->FileNames[i]), _currentModel->GetName(), &prl, iniFile);
			ReadPeaks		(clrToWstring(consOFD->FileNames[i]), _currentModel->GetName(), &pkl, iniFile);
			ReadBG			(clrToWstring(consOFD->FileNames[i]), _currentModel->GetName(), &bgl, iniFile);
			ReadPhases		(clrToWstring(consOFD->FileNames[i]), _currentModel->GetName(), &phl, &phaseTyp, iniFile);
			ReadCaille		(clrToWstring(consOFD->FileNames[i]), _currentModel->GetName(), dummy , &cll, iniFile);
			
			// Convert all parameters into a vector (layer)
			//FF
			for(int j = 0; j < prl.layers; j++)
				for(int k = 0; k < _currentModel->GetNumLayerParams(); k++)
					layer.push_back(prl.params[k][j].value);

			maxLayers = max(prl.layers, maxLayers);

			layer.push_back(-5.0);

			for(int j = 0; (unsigned)j < prl.extraParams.size(); j++)
				layer.push_back(prl.extraParams[j].value);

			layer.push_back(-5.0);

			exPar = max(exPar, (int)prl.extraParams.size());

			// SF
			for(int j = 0; (unsigned)j < pkl.amp.size(); j++) {
				layer.push_back(pkl.amp.at(j).value);
				layer.push_back(pkl.fwhm.at(j).value);
				layer.push_back(pkl.center.at(j).value);
			}
			maxPeaks = max(pkl.amp.size(), (unsigned)maxPeaks);

			layer.push_back(-5.0);

			// Caille, if applicable
			/*if(GetModelType() == MODEL_ASLAB || GetModelType() == MODEL_SLAB) {
				layer.push_back(cll.amp.at(0).value);
				layer.push_back(cll.eta.at(0).value);
				layer.push_back(cll.N.at(0).value);
				layer.push_back(cll.N_diffused.at(0).value);
				layer.push_back(cll.sig.at(0).value);

				layer.push_back(-5.0);
			}*/
			// TODO: Figure out a solution for Caille
			
			//Background
			for(int j = 0; (unsigned)j < bgl.base.size(); j++) {
				layer.push_back((double)int(bgl.type.at(j)));
				layer.push_back(bgl.base.at(j));
				layer.push_back(bgl.decay.at(j));
				layer.push_back(bgl.center.at(j));
			}
			maxBG = max(bgl.base.size(), (unsigned)maxBG);

			layer.push_back(-5.0);

			//Phases
			layer.push_back(phl.a);
			layer.push_back(phl.b);
			layer.push_back(phl.alpha);
			layer.push_back(phl.c);
			layer.push_back(phl.beta);
			layer.push_back(phl.gamma);
			
			layer.push_back(-5.0);
			
			//		insert a value of -5.0 between sets of parameters
			// Add layer to params (pushback)
			params.push_back(layer);

			CloseIniFile(iniFile);
		}

		// Ask user to save to a chosen filename
		if(consSFD->ShowDialog() == System::Windows::Forms::DialogResult::Cancel)
			return;

		// Write params to TSV file
		clrToString(consSFD->FileName, file);
		std::wofstream fstr(file.c_str());
		if(!fstr.good()) {
			fprintf(stderr, "Error opening file %s for writing\n",
							file);
			
			MessageBox::Show("Please make sure that the file is not open.", "Error opening file for writing", MessageBoxButtons::OK,
									 MessageBoxIcon::Error);
			return;
		}

		//fwprintf(fp, L"Filename\t");
		fstr << L"Filename\t";

		//FF Layers
		for(int i = 0; i < maxLayers; i++) {
			FFTitle = clrToWstring(gcnew System::String(_currentModel->GetLayerName(i).c_str()));
			for(int j = 0; j < _currentModel->GetNumLayerParams(); j++)
				fstr	<< FFTitle
						<< L" "
						<< clrToWstring(gcnew System::String(_currentModel->GetLayerParamName(j).c_str()))
						<< L"\t";
		}
		//ExParams
		for(int i = 0; i < _currentModel->GetNumExtraParams(); i++)
			fstr << clrToWstring(gcnew System::String(
						_currentModel->GetExtraParameter(i).name.c_str())) 
				 << L"\t";
		//fwprintf(fp, L"%s", exStr);
		fstr << exStr;

		// SF
		for(int i = 0; i < maxPeaks; i++) {
			laT = L"Peak " + clrToWstring((i + 1).ToString());
			fstr << laT << L" Amplitude\t";
			fstr << laT << L" Width\t";
			fstr << laT << L" Center\t";
		}

		// Caille Titles
		/*if(GetModelType() == MODEL_SLAB || GetModelType() == MODEL_ASLAB) {
			fstr << L"Caille Amplitude\t";
			fstr << L"Caille eta\t";
			fstr << L"Caille N\t";
			fstr << L"Caille N_diffused\t";
			fstr << L"Caille sigma\t";
		}*/
		// TODO: Find a way to deal with Caille existence

		// BG
		for(int i = 0; i < maxBG; i++) {
			laT = L"BG " + clrToWstring((i + 1).ToString());
			fstr << laT << L" Func. Type\t";
			fstr << laT << L" BG Amplitude\t";
			fstr << laT << L" BG Decay\t";
			fstr << laT << L" BG X_Center\t";
		}

		// Phase
		fstr << L"Phase a\t";
		fstr << L"Phase b\t";
		fstr << L"Phase alpha\t";
		fstr << L"Phase c\t";
		fstr << L"Phase beta\t";
		fstr << L"Phase gamma\t";

		fstr << L"\n";

		// Write Values
		for(int i = 0; i < consOFD->FileNames->Length; i++) {
			int empty = 0, need = 0, j;

			fstr << clrToWstring(consOFD->FileNames[i]) << '\t';

			// FF
			for(j = 0; ((params.at(i).at(j) + 5.0) > 0.0); j++) {
				if(params.at(i).at(j) == -5.0)
					break;
				if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
					fstr <<"-\t";
				else
					fstr << params.at(i).at(j) << '\t';
			}
			for(need = j; need < empty + maxLayers * _currentModel->GetNumLayerParams(); need++)
				fstr << L"--\t";
			
			empty = ++j;

			//Extra Params
			for(; j < empty + exPar; j++) {
				if(params.at(i).at(j) == -5.0)
					break;
				if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
					fstr << L"-\t";
				else
					fstr << params.at(i).at(j) << '\t';
			}
			empty = ++j;

			//SF
			for(; (fabs(params.at(i).at(j) + 5.0) > 1.0e-7)/*(unsigned)j < empty + pkl.amp.size()*/; j++) {
				if(params.at(i).at(j) == -5.0)
					break;
				if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
					fstr << L"-\t";
				else
					fstr << params.at(i).at(j) <<'\t';
			}
			for(need = j; need < empty + maxPeaks * 3; need++)
				fstr << L"--\t";
			empty = ++j;

			// Caille
			/*if(GetModelType() == MODEL_SLAB || GetModelType() == MODEL_ASLAB) {
				for(; j < empty + 5; j++) {
					if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
						fstr <<"-\t";
					else
						fstr << params.at(i).at(j) << '\t';
				}
				empty = ++j;
			}*/
			// TODO: Find a way to deal with Caille existence

			// BG
			for(; (fabs(params.at(i).at(j) + 5.0) > 1.0e-7)/*(unsigned)j < empty + bgl.base.size()*/; j++) {
				if(params.at(i).at(j) == -5.0)
					break;
				if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
					fstr << L"-\t";
				else
					fstr << params.at(i).at(j) << '\t';
			}
			for(need = j; need < empty + maxBG * 4; need++)
				fstr << L"--\t";
			empty = ++j;

			// Phase
			for(; j < empty + 6; j++) {
				if(fabs(params.at(i).at(j) + 1.0) < 1.0e-6)
					fstr << L"-\t";
				else
					fstr << params.at(i).at(j) << '\t';
			}

			fstr << L"\n";
		}

		fstr.close();

		if(dummy) {
			delete dummy;
			dummy = NULL;
		}
	}
	
	void OpeningWindow::signalSeriesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		SignalSeries ^s = gcnew SignalSeries();
		this->Visible = false;
		s->ShowDialog();
		this->Visible = true;
	}
};

