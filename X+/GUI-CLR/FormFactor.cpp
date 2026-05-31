#include "FormFactor.h"


#include "SmoothWindow.h"
#include "ResultsWindow.h"
#include "FitRange.h"
#include "ExtractBaseline.h"
#include "ExternalModelDialog.h"

#include "edprofile.h"

#include "ErrorTableWindow.h"

#include <limits>


using namespace System::Windows::Forms;

template <typename T> inline T sq(T x) { return x * x; }

namespace GUICLR {

	
	public ref class GlobalFitter {
	public:
		static FormFactor ^ff;
		static Graph ^graph;
		static std::vector<GraphSource> *graphType;
	};


	// Big MA TODO list: (order doesn't matter)
	// - Reimplement polydispersity with n-pd support
	
	// - Fix all TODOs
	
	// - Delete comments from before MA
	////////////////////////////////////////////////////////////////////////////
   
	/*
		ListviewFF indices for the nth layer parameter (out of nlp):
		Value                - 2 * n + 1
		Mutability           - 2 * n + 2
		Constraint minimum   - 2 * nlp + 7 * n + 1
		Constraint maximum   - 2 * nlp + 7 * n + 2
		Constraint min index - 2 * nlp + 7 * n + 3
		Constraint max index - 2 * nlp + 7 * n + 4
		Constraint link      - 2 * nlp + 7 * n + 5
		Use constraint field - 2 * nlp + 7 * n + 6
		Standard deviation   - 2 * nlp + 7 * n + 7
	*/
#ifndef LV_POSITIONS
#define LV_POSITIONS

#define LV_PAR_SUBITEMS       (7)
#define LV_NAME               (0)
#define LV_VALUE(n)           (2 * n + 1)
#define LV_MUTABLE(n)         (2 * n + 2)
#define LV_CONSMIN(n, nlp)    (2 * nlp + LV_PAR_SUBITEMS * n + 1)
#define LV_CONSMAX(n, nlp)    (2 * nlp + LV_PAR_SUBITEMS * n + 2)
#define LV_CONSIMIN(n, nlp)   (2 * nlp + LV_PAR_SUBITEMS * n + 3)
#define LV_CONSIMAX(n, nlp)   (2 * nlp + LV_PAR_SUBITEMS * n + 4)
#define LV_CONSLINK(n, nlp)   (2 * nlp + LV_PAR_SUBITEMS * n + 5)
#define LV_CONS(n, nlp)       (2 * nlp + LV_PAR_SUBITEMS * n + 6)
#define LV_SIGMA(n, nlp)      (2 * nlp + LV_PAR_SUBITEMS * n + 7)
#endif
	/*
		listView_Extraparams indices
		Name               - 0
		Value              - 1
		Mutable ("N"/"Y")  - 2
		Constraint minimum - 3
		Constraint maximum - 4
		Infinite ("0"/"1") - 5
		Use constraint     - 6
		Standard deviation - 7
	*/
#ifndef ELV_POSITIONS
#define ELV_POSITIONS
#define ELV_NAME     0
#define ELV_VALUE    1
#define ELV_MUTABLE  2
#define ELV_CONSMIN  3
#define ELV_CONSMAX  4
#define ELV_INFINITE 5
#define ELV_CONS     6
#define ELV_SIGMA    7
#endif
	// Debug function that displays a listView in a message box
	void DisplayLVFF(System::Windows::Forms::ListView^ LV) {
		/** DEBUG LINES **/
		std::stringstream s;
		s << " \t";
		for(int j = 0; j < LV->Items[0]->SubItems->Count; j++)
			s << "j=[" << j << "]\t";
		s << "\n";
		for(int i = 0; i < LV->Items->Count; i++) {
			s << "i=[" << i << "] ";
			for(int j = 0; j < LV->Items[i]->SubItems->Count; j++) {
				s << clrToString(LV->Items[i]->SubItems[j]->Text).substr(0, 7);
				s << '\t';
			}
			s << '\n';
		}
		MessageBoxA(NULL, s.str().c_str(), clrToString(LV->Name).c_str(), NULL);
		/** END DEBUG LINES **/
	}

	void ReportProgressDummy(int progress) {
		System::ComponentModel::ProgressChangedEventArgs^  pcea = gcnew System::ComponentModel::ProgressChangedEventArgs (progress, nullptr);
		if(GlobalFitter::ff) {
			// Safe
			DReportProgress ^del = gcnew DReportProgress(GlobalFitter::ff, &FormFactor::modelFitter_ProgressChanged);
			array<Object^> ^delParams = {nullptr, pcea};
	        GlobalFitter::ff->Invoke( del, delParams );
		}
	}

	void ReportDone(bool cancelled) {		
		System::ComponentModel::RunWorkerCompletedEventArgs^  rwcea = gcnew System::ComponentModel::RunWorkerCompletedEventArgs (nullptr, nullptr, cancelled);
		if(GlobalFitter::ff) {
			// Safe
			DReportDone ^del = gcnew DReportDone(GlobalFitter::ff, &FormFactor::modelFitter_RunWorkerCompleted);
			array<Object^> ^delParams = {nullptr, rwcea};
			int spacer = 9;
			spacer--;
	        GlobalFitter::ff->Invoke( del, delParams );
		}
	}

	void UpdateGeneratedGraph(const std::vector<double>& x, const std::vector<double>& y) {
		if(GlobalFitter::graph) {
			int i = 0;	
			// Locate the correct graph to update
			if(GlobalFitter::graphType) {
				for( ; i < (int)GlobalFitter::graphType->size(); i++) {
					if(GlobalFitter::graphType->at(i) == GRAPH_MODEL)
						break;
				}
				// If we're in Generate mode
				if(i == (int)GlobalFitter::graphType->size())
					i = 0;
			}
			GlobalFitter::graph->Modify(i, x, MachineResolution(x, y, GetResolution()));
		}
	}

	FormFactor::FormFactor(const wchar_t *filename, bool bGenerate, OpeningWindow^ parent) {
		_bSaved			= false;
		_bFrozenFF		= false;
		_bUseFF			= false;
		_peakPicker		= false;
		_bChanging		= false;
		_bFromFitter	= false;
		_curWssr = -1.0;
		_curPar = NULL;
		oldIndex = -1;

		// We have to allocate another int because of CLR and managed types
		_pShouldStop = new int; *_pShouldStop = 0;

		_bGenerateModel = bGenerate;
		if(!_bGenerateModel)
			_dataFile = gcnew System::String(filename);

		InitializeComponent();
		if(!_bGenerateModel)
			this->Text += " -  [" + CLRBasename(_dataFile) + "]";

		_data = new graphTable;
		_ff = new graphTable;
		_sf = new graphTable;
		_bg = new graphTable;
		_baseline = new graphTable;
		_storage = new graphTable;

		// For some reason, if the visibility is set to false in the designer then the tool strip is not
		// visible in the designer.  So, I set it to visible and will change it here.
		this->maskToolStrip->Visible = false;
		this->wssr->Text = UnicodeChars::chisqr + this->wssr->Text;
		this->rsquared->Text = UnicodeChars::rsqr + this->rsquared->Text;
		SetMinimumSig(5.0);

		_mask = new std::vector<bool>;

		_loadedFF = new std::wstring;

		graphType = new std::vector<GraphSource>;
		_ph = new std::vector<double>;
		_generatedPhaseLocs = new std::vector<double>;
		phaseSelected = new PhaseType;
		indicesLoc = new std::vector<std::string>;
		FFparamErrors = new std::vector<double>;
		SFparamErrors = new std::vector<double>;
		BGparamErrors = new std::vector<double>;
		PhaseparamErrors = new std::vector<double>;
		FFmodelErrors = new std::vector<double>;
		SFmodelErrors = new std::vector<double>;
		BGmodelErrors = new std::vector<double>;
		_copiedIndicesFF = new std::vector<int>;
		Globalization::CultureInfo ^American;
		American = gcnew Globalization::CultureInfo(L"en-US");
		Thread::CurrentThread->CurrentUICulture = American;//System::Globalization::CultureInfo::NumberFormat InvariantCulture; //CultureInfo("en") ;
		Thread::CurrentThread->CurrentCulture = American;//System::Globalization::CultureInfo::NumberFormat InvariantCulture; //CultureInfo("en") ;

		_parent = parent;
		_model = _parent->_currentModel;
	}


	FormFactor::~FormFactor() {
		delete _pShouldStop;

		delete _data;
		delete _ff;
		delete _sf;
		delete _bg;
		delete _baseline;
		delete _storage;
		delete _mask;
		delete FFparamErrors;
		delete SFparamErrors;
		delete BGparamErrors;
		delete PhaseparamErrors;
		delete FFmodelErrors;
		delete SFmodelErrors;
		delete BGmodelErrors;

		delete _loadedFF;
		delete _copiedIndicesFF;

		if(graphType)
			delete graphType;
		graphType = NULL;

		if(_curPar)
			delete _curPar;
		_curPar = NULL;

		if(_curPeaks)
			delete _curPeaks;
		_curPeaks = NULL;

		if(_curPeaksCaille)
			delete _curPeaksCaille;
		_curPeaksCaille = NULL;

		if(_curBG)
			delete _curBG;
		_curBG = NULL;
		
		if(_curPhases)
			delete _curPhases;
		_curPhases = NULL;

		if(_curCaille)
			delete _curCaille;
		_curCaille = NULL;

		if(_ph)
			delete _ph;
		_ph = NULL;

		if(_generatedPhaseLocs)
			delete _generatedPhaseLocs;
		_generatedPhaseLocs = NULL;


		if(phaseSelected)
			delete phaseSelected;
		phaseSelected = NULL;

		if(indicesLoc)
			delete indicesLoc;
		indicesLoc = NULL;
	
		if(iniFile)
			delete iniFile;
		iniFile = NULL;

		if (components)
		{
			delete components;
		}
	}

	void FormFactor::InitializeEDProfile() {
		this->wgtPreview = (gcnew GUICLR::WGTControl());
		this->edpBox->Controls->Add(this->wgtPreview);
		// 
		// wgtPreview
		// 
		this->wgtPreview->Cursor = System::Windows::Forms::Cursors::Cross;
		this->wgtPreview->Dock = System::Windows::Forms::DockStyle::Fill;
		this->wgtPreview->Location = System::Drawing::Point(0, 0);
		this->wgtPreview->Name = L"wgtPreview";
		this->wgtPreview->Size = System::Drawing::Size(343, 349);
		this->wgtPreview->TabIndex = 0;
		
		struct graphLine graphs[3];
		std::vector<std::vector<Parameter> > p;
		std::vector<Parameter> r, ed;

		r.push_back(Parameter(0.0));
		ed.push_back(Parameter(333.0));

		p.push_back(r);
		p.push_back(ed);

		
		generateEDProfile(p, graphs, _model->GetEDProfile());

		RECT area;
		area.top = 0;
		area.left = 0;
		area.right = wgtPreview->Size.Width;
		area.bottom = wgtPreview->Size.Height;
		wgtPreview->graph = gcnew Graph(
							area, 
							graphs[0].color, 
							DRAW_LINES, graphs[0].x, 
							graphs[0].y, 
							false,
							false);
		wgtPreview->graph->Add(graphs[1].color, 
							   DRAW_LINES, 
							   graphs[1].x, graphs[1].y);
		wgtPreview->graph->Add(graphs[2].color, 
							   DRAW_LINES, 
							   graphs[2].x, graphs[2].y);

		// No ticks
		wgtPreview->graph->ToggleXTicks();
		wgtPreview->graph->ToggleYTicks();
		wgtPreview->graph->Resize(area);
	}

	void FormFactor::logScale_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
			 if(wgtFit && wgtFit->graph) {
				 wgtFit->graph->SetScale((sender == logScale ? 0 : 1), ((CheckBox ^)sender)->Checked ? SCALE_LOG : SCALE_LIN);
				 wgtFit->Invalidate();
			 }
		 }

	void FormFactor::UpdateEDPreview() {
		//std::vector<double> r, ed, z0;
		struct graphLine graphs[3];
		//std::string str;

		if(_bChanging)
			return;
	
		if(!this->Visible || !wgtPreview->Visible)
			return;
		paramStruct p = *_curPar;

		if(_model->GetEDProfile().type != NONE)
			generateEDProfile(p.params, graphs, _model->GetEDProfile());

		wgtPreview->graph->Modify(0, graphs[0].x, graphs[0].y);
		wgtPreview->graph->Modify(1, graphs[1].x, graphs[1].y);
		wgtPreview->graph->Modify(2, graphs[2].x, graphs[2].y);
		wgtPreview->graph->FitToAllGraphs();

		wgtPreview->Invalidate();
	}

	void FormFactor::FormFactor_Load(System::Object^  sender, System::EventArgs^  e) {
		GlobalFitter::ff = this;
		GlobalFitter::graphType = graphType;
		_bLoading = true;
		
		//reportButton->Visible = false;

		this->KeyPreview = true;

		gPUToolStripMenuItem->Enabled = hasGPUBackend();
		gPUToolStripMenuItem->Checked = isGPUBackend();
		cPUToolStripMenuItem->Checked = !isGPUBackend();

		edpResolution->Text = Int32(DEFAULT_EDRES).ToString();

		InitializeEDProfile();

		edpBox->Visible = _model->IsLayerBased();

		// The big "disable everything related to fitting" if
		if(_bGenerateModel) {
			generationToolStripMenuItem->Visible = true;
			genRangeBox->Visible = true;
			calculate->Text = "Generate";
			liveFittingToolStripMenuItem->Text = "Live Generation";
			label6->Text = "Generating...";
			calculate->Enabled = true;
			fitphase->Enabled = false;
			Caille_button->Visible = false;
			undo->Visible = false;
			manipBox->Visible = false;
			label1->Visible = false;
			minim->Enabled	= false;
			maxim->Enabled	= false;
			LocOnGraph->Visible = true;
			consGroupBox->Visible = false;
			wssr->Visible = false;
			rsquared->Visible = false;
			exportBackgroundToolStripMenuItem->Visible = false;
			fittingMethodToolStripMenuItem->Visible = false;
			maskButton->Visible = false;
			reportButton->Visible = false;

			thresholdBox1->Visible = false;
			thresholdBox2->Visible = false;
			Threshold_label1->Visible = false;
			Threshold_label2->Visible = false;
			automaticPeakFinderButton->Visible = false;
			PeakPicker->Visible = false;
			PeakFinderCailleButton->Visible = false;
			
			// Phases
			label22->Enabled	= false;
			label23->Enabled	= false;
			MinPhases->Enabled	= false;
			MaxPhases->Enabled	= false;
			fitphase->Text = L"Generate Phase";
			fitphase->Width = 92;
			undoPhases->Width = 50;
			clearPositionsButton->Width = 50;
			undoPhases->Location = System::Drawing::Point(110, 43);
			clearPositionsButton->Location = System::Drawing::Point(163, 43);
			listView_phases->Columns[1]->Width *= 2;
			listView_phases->Columns[2]->Width = 0;
			listView_phases->Columns[3]->Width = 0;
			listView_phases->Columns[4]->Width = 0;
			listView_phases->Columns[5]->Width *= 2;

			// Caille
			mutLabel->Visible					= false;
			cailleAmpCheckbox->Visible			= false;
			cailleNCheckbox->Visible			= false;
			cailleEtaCheckbox->Visible			= false;
			cailleMinGroupbox->Visible			= false;
			cailleMaxGroupbox->Visible			= false;
			cailleSigCheckbox->Visible			= false;
			cailleN_diffusedCheckBox->Visible	= false;
			cailleParamListView->Columns[2]->Width = 0;
			cailleParamListView->Columns[3]->Width = 0;

			exmin->Enabled = false;
			exmax->Enabled = false;

			exportSignalToolStripMenuItem->Visible = false;
			exportDecomposedToolStripMenuItem->Visible = false;
			exportSigModBLToolStripMenuItem->Visible = false;
			importBaselineToolStripMenuItem->Enabled = false;

			liveRefreshToolStripMenuItem->Checked = false;

			changeData->Visible = false;
			PeakPicker->Visible = false;
			PeakFinderCailleButton->Visible = false;

			accurateDerivativeToolStripMenuItem->Visible = false;
			accurateFittingToolStripMenuItem->Visible = false;
			chiSquaredBasedFittingToolStripMenuItem->Visible = false;
			logScaledFittingParamToolStripMenuItem->Visible = false;
			minimumSignalToolStripMenuItem->Visible = false;

			// Background tab
			baseMut->Visible	= false;
			baseMaxBox->Visible	= false;
			baseMinBox->Visible	= false;
			decayMut->Visible	= false;
			decMaxBox->Visible	= false;
			decMinBox->Visible	= false;
			xCenterMut->Visible	= false;
			xcMaxBox->Visible	= false;
			xcMinBox->Visible	= false;
			maxLabel->Visible	= false;
			minLabel->Visible	= false;
			BGListview->Columns[3]->Width = 0;
			BGListview->Columns[5]->Width = 0;
			BGListview->Columns[7]->Width = 0;
		}
		// End of the "big generation if"

		ExtractBaseline::bUsingOld = false;

		// Load the rest of the UIs
		StructureFactor_Load();
		Background_Load();


		// Read radii and EDs
		paramStruct par (_model);
		std::string type = _model->GetName();
		std::wstring filename;
	
		if(_bGenerateModel) {
			filename = L".\\XModelFitter.ini";
		} else {
			std::wstring res, dir;
			std::wstring dataFile;
			clrToString(_dataFile, dataFile);
			
			dir = clrToWstring(CLRDirectory(_dataFile));
			res = clrToWstring(CLRBasename(_dataFile));
			
			res = dir + res + L"-params.ini";

			filename = res;
		}

		iniFile = NewIniFile();

		if(CheckSizeOfFile(filename.c_str()) > 0 && IniHasModelType(filename.c_str(), type, iniFile)) {
			// Read ED Profile configuration (prior to parameters)
			{
				// Get the default electron density profile
				EDProfile defaultEDP = _model->GetEDProfile();
				// If not discrete, disable other options
				if(defaultEDP.shape != DISCRETE) {
					electronDensityProfileToolStripMenuItem->Visible = false;
				} else {
					electronDensityProfileToolStripMenuItem->Visible = true;

					// Read profile shape
					ProfileShape psh = (ProfileShape)GetIniInt(filename, type, 
															   "EDProfileShape", 
															   iniFile, DISCRETE);
					EDProfile op = _model->GetEDProfile();
					_model->SetEDProfile(EDProfile(op.type, psh));


					// Read profile resolution
					int res = GetIniInt(filename, type, "EDProfileResolution", 
										iniFile, DEFAULT_EDRES);
					if(_model->GetEDProfile().func)
						_model->GetEDProfile().func->SetResolution(res);

					int absres = (res < 0) ? -res : res;

					// Resolution GUI modification
					edpResolution->Enabled = true;
					edpResolution->Text = Int32(absres).ToString();
					adaptiveToolStripMenuItem->Checked = (res < 0) ? true : false;
				}
			}
				
			// Prepares the GUI for the chosen model (adding parameters, extra 
			// parameters and so on)
			PrepareModelUI();

			// Set ED profile menu items
			{
				ProfileShape psh = _model->GetEDProfile().shape;

				discreteStepsToolStripMenuItem->Checked = (psh == DISCRETE);
				gaussiansToolStripMenuItem->Checked = (psh == GAUSSIAN);
				hyperbolictangentSmoothStepsToolStripMenuItem->Checked = (psh == TANH);
			}
				
			ReadParameters(filename.c_str(), type, &par, iniFile);

			if(par.layers > 0) {
				ParametersToUI(&par);
				UItoParameters(_curPar);

				if(_bGenerateModel) {
					std::string tmpstr;
					GetIniString(filename, type, "GenRangeStart", tmpstr, iniFile);
					if(! (strtod(tmpstr.c_str(), NULL) > 0.0))
						startGen->Text = gcnew System::String("0.100000");
					else
						startGen->Text = gcnew System::String(tmpstr.c_str());
					GetIniString(filename, type, "GenRangeEnd", tmpstr, iniFile);
					if(! (strtod(tmpstr.c_str(), NULL) > 0.0))
						endGen->Text = gcnew System::String("5.000000");
					else
						endGen->Text = gcnew System::String(tmpstr.c_str());
					int tmpInt = GetIniInt(filename, type, "GenResolution", iniFile);
					toolStripTextBox2->Text = (tmpInt > 1) ? tmpInt.ToString() : gcnew System::String("500");
				}

				// The reason the following (quadrature, extra parameters) are
				// here is to make sure we have a saved model before applying settings
				// (the defaults are different)

				// Quadrature
				if(integrationToolStripMenuItem->Visible) {
					toolStripTextBox1->Text = GetIniInt(filename, type, "quadratureres", iniFile).ToString();

					switch(int(GetIniInt(filename, type, "quadraturemethod", iniFile))) {
						default:
							break;

						case QUAD_MONTECARLO:
							gaussLegendreToolStripMenuItem->Checked = false;
							monteCarloToolStripMenuItem->Checked = true;
							break;

						case QUAD_SIMPSON:
							gaussLegendreToolStripMenuItem->Checked = false;
							simpsonsRuleToolStripMenuItem->Checked = true;
							break;
					}
					ClassifyQuadratureMethod((QuadratureMethod)(int((GetIniInt(filename, type, "quadraturemethod", iniFile)))));
				}
			}
			
			// Loading peaks from INI
			peakStruct peaks;
			ReadPeaks(filename, type, &peaks, iniFile);
			//peakfit->SelectedIndex = GetPeakType();
			for(unsigned int i = 0; i < peaks.amp.size(); i++)
				AddPeak(peaks.amp[i].value, peaks.amp[i].mut, peaks.fwhm[i].value, peaks.fwhm[i].mut,
				peaks.center[i].value, peaks.center[i].mut);
			

			// Loading BG functions from INI
			bgStruct BGs;
			ReadBG(filename, type, &BGs, iniFile);
			SetBGtoGUI(&BGs);

			AddPhasesParam("a",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[0]
			AddPhasesParam("b",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[1]
			AddPhasesParam(UnicodeChars::gammaUnicode,MODE_ABSOLUTE,90.0,0.0, 180.0);	// Items[2]
			AddPhasesParam("c",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[3]
			AddPhasesParam(UnicodeChars::alphaUnicode,MODE_ABSOLUTE,90.0,0.0,180.0);	// Items[4]
			AddPhasesParam(UnicodeChars::betaUnicode,MODE_ABSOLUTE,90.0,0.0,180.0);	// Items[5]

			// Loading Phases from INI
			int pt;
			phaseStruct ps;
			ReadPhases(filename, type, &ps, &pt, iniFile);
			order->SelectedIndex = pt;
			SetPhases(&ps);

			// Calculate Reciprocal values
			calculateRecipVectors();

			if(_model->HasSpecializedSF()) {
				AddCailleParam("Amplitude", MODE_PRECISION, 1.0);
				AddCailleParam(UnicodeChars::etaUnicode, MODE_PRECISION, 0.5);
				AddCailleParam("N0", MODE_ABSOLUTE, 3.0);
				cailleParamListView->Items[cailleParamListView->Items->Count - 1]->SubItems[3]->Text = "70.000000";
				AddCailleParam(UnicodeChars::sigmaUnicode, MODE_ABSOLUTE, 1.0);
				AddCailleParam("N Diffused", MODE_ABSOLUTE, 0.0);

				// Loading Caille from INI
				graphTable caille;
				cailleParamStruct cailledrawing;
				ReadCaille(filename, type, &caille, &cailledrawing, iniFile);
				SetCailletoGUI(&caille, &cailledrawing);
				SetD(clrToDouble(listView_phases->Items[0]->SubItems[1]->Text));
			}

			// Polydispersity settings. TODO: Add to GUI, modify 
			int pdType, pdRes;
			pdType = GetIniInt(filename, type, "PDFunc", iniFile);
			pdRes = GetIniInt(filename, type, "PDResolution", iniFile);
			SetPDFunc((PeakType)pdType);
			switch((PeakType)pdType) {
				default:
				case SHAPE_GAUSSIAN:
					uniformPDToolStripMenuItem->Checked = false;
					gaussianPDToolStripMenuItem->Checked = true;
					lorentzianPDToolStripMenuItem->Checked = false;
					break;

				case SHAPE_LORENTZIAN:
					uniformPDToolStripMenuItem->Checked = false;
					gaussianPDToolStripMenuItem->Checked = false;
					lorentzianPDToolStripMenuItem->Checked = true;
					break;

				case SHAPE_LORENTZIAN_SQUARED:
					uniformPDToolStripMenuItem->Checked = true;
					gaussianPDToolStripMenuItem->Checked = false;
					lorentzianPDToolStripMenuItem->Checked = false;
					break;
			}
			SetPDResolution(pdRes);
			
			// General Settings
			logScale->Checked = (GetIniChar(filename, "Settings", "logscale", iniFile) == 'Y');
			liveRefreshToolStripMenuItem->Checked = (GetIniChar(filename, "Settings", "liverefresh", iniFile) == 'Y');
			liveFittingToolStripMenuItem->Checked = (GetIniChar(filename, "Settings", "livefit", iniFile) == 'Y');
			sigmaToolStripMenuItem->Checked = (GetIniChar(filename, "Settings", "Sigma", iniFile) == 'Y' ||GetIniChar(filename, "Settings", "Sigma", iniFile) == '-');
			a1nm1ToolStripMenuItem->Checked = (GetIniChar(filename, "Settings", "Angstrom", iniFile) == 'Y');
			fWHMToolStripMenuItem->Checked = !sigmaToolStripMenuItem->Checked;
			sigmaToolStripMenuItem_CheckedChanged(sender,e);
			sigmaFWHMToolStripMenuItem->Visible = GetPeakType() == SHAPE_GAUSSIAN;			

		} else {	// Enter default values for Caille and Phases

			// Prepares the GUI for the chosen model (adding parameters, extra 
			// parameters and so on)
			PrepareModelUI();

			if(_model->HasSpecializedSF()) {
				AddCailleParam("Amplitude", MODE_PRECISION, 1.0);
				AddCailleParam(UnicodeChars::etaUnicode, MODE_PRECISION, 0.5);
				AddCailleParam("N0", MODE_ABSOLUTE, 3.0);
				cailleParamListView->Items[cailleParamListView->Items->Count - 1]->SubItems[3]->Text = "70.000000";
				AddCailleParam(UnicodeChars::sigmaUnicode, MODE_ABSOLUTE, 1.0);
				AddCailleParam("N Diffused", MODE_ABSOLUTE, 0.0);
			}

			AddPhasesParam("a",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[0]
			AddPhasesParam("b",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[1]
			AddPhasesParam("gamma",MODE_ABSOLUTE,90.0,0.0, 180.0);	// Items[2]
			AddPhasesParam("c",MODE_ABSOLUTE,6.0,0.0,100.0);		// Items[3]
			AddPhasesParam("alpha",MODE_ABSOLUTE,90.0,0.0,180.0);	// Items[4]
			AddPhasesParam("beta",MODE_ABSOLUTE,90.0,0.0,180.0);	// Items[5]
		}

		CloseIniFile(iniFile);
		iniFile = NULL;
		
		// Load the preview window
		if(!timer1->Enabled) {
			oglPreview = gcnew OpenGLWidget(oglPanel, 
				gcnew renderFunc(this, &GUICLR::FormFactor::RenderPreviewScene));

			timer1->Enabled = true;
			oglPreview->Render();
			oglPreview->SwapOpenGLBuffers();
		}


		// Update E.D. Preview
		UpdateEDPreview();

		initPhasesParams();

		if(_model->HasSpecializedSF()) {
			showCailleButton->Visible = true;
			if(caillePeaksListView->Items->Count > 0) {
				showCailleButton->Enabled = true;
				if(GetPeakType() == SHAPE_CAILLE) {
					cailleGroupbox->Enabled = true;
					cailleGroupbox->Visible	= true;
					Peakfitter->Visible		= false;
				} else {
					cailleGroupbox->Enabled = false;
					cailleGroupbox->Visible	= false;
					Peakfitter->Visible		= true;
				}
			} else {
				showCailleButton->Enabled	= false;
				cailleGroupbox->Enabled		= false;
				cailleGroupbox->Visible		= false;
				Peakfitter->Visible			= true;
			}

		}

		if(caillePeaksListView->Items->Count > 0)
			showCailleButton->Enabled = true;

		// Start in BGTab so baseline can be removed
		if(!_bGenerateModel)
			tabControl1->SelectTab("BGTab");
		
		EDAreaGroup->Visible = true;

		slowModelGroupbox->Visible = _model->IsSlow();

		listView_Extraparams->SelectedItems;

		exParamGroupbox->check->Enabled = false;

		SFParameterUpdateHandler();
		BGParameterUpdateHandler();
		_bChanging = true;	// triggers the UpdateGraph(true) in ParameterUpdateHandler
		FFParameterUpdateHandler();
		_bLoading = false;
		_bChanging = false;
	
	}

	/*bool FormFactor::isSlowModel() {
		switch(GetModelType()) {
			case MODEL_CYLINDROID:
			case MODEL_DELIX:
				return true;

			case MODEL_HELIX:
			case MODEL_ROD:
				if(!(infExtraParam->Checked))
					return true;
				break;

			case MODEL_ASLAB:
			case MODEL_SLAB:
				if(isCuboidModel())
					return true;
			default:
				return false;
		}
		return false;
	}*/

	//Adds each corresponding value of a and b
	void FormFactor::AddVectors(vector<double> &result, const vector<double> &a, 
								const vector<double> &b) {
		int size = min(a.size(), b.size());

		result.resize(size);

		for(int i = 0; i < size; i++)
			result[i] = a[i] + b[i];
	}

	//Subtracts each corresponding value of b from a
	void FormFactor::SubtractVectors(vector<double> &result, const vector<double> &a, 
									 const vector<double> &b) {
		int size = min(a.size(), b.size());

		result.resize(size);
		
		for(int i = 0; i < size; i++)
			result[i] = a[i] - b[i];
	}

	//Multiplies each corresponding value of a and b
	void FormFactor::MultiplyVectors(vector<double> &result, const vector<double> &a, 
									 const vector<double> &b) {
		int size = min(a.size(), b.size());

		result.resize(size);
		for(int i = 0; i < size; i++)
			result[i] = a[i] * b[i];
	}
	//Divides each corresponding value of a by b
	void FormFactor::DivideVectors(vector<double> &result, const vector<double> &a, 
									 const vector<double> &b) {
		int size = min(a.size(), b.size());

		result.resize(size);
		for(int i = 0; i < size; i++)
			if(fabs(b[i]) > 0.0)
				result[i] = a[i] / b[i];
			else
				result[i] = a[i] / 1e-9;

	}

	void FormFactor::multiplyVectorByValue(std::vector<double> &vec, double val) {
		for(int i = 0; i < (int)vec.size(); i++)
			vec.at(i) *= val;
	}

	void FormFactor::InitializeFitGraph() {
		vector<double> x, y;
		InitializeFitGraph(x, y);
	}

	void FormFactor::InitializeFitGraph(vector<double>& x, vector<double>& y) {
		delete wgtFit;
		graphType->clear();
		this->wgtFit = (gcnew GUICLR::WGTControl());
		this->wgtFit->setDragToZoom(g_bDragToZoom);
		this->groupBox1->Controls->Add(this->wgtFit);

		// 
		// wgtFit
		// 
		this->wgtFit->Cursor = System::Windows::Forms::Cursors::Cross;
		this->wgtFit->Dock = System::Windows::Forms::DockStyle::Fill;
		this->wgtFit->Location = System::Drawing::Point(0, 0);
		this->wgtFit->Name = L"wgtFit";
		this->wgtFit->Size = System::Drawing::Size(343, 349);
		this->wgtFit->TabIndex = 0;
		this->wgtFit->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::wgtFit_MouseMove);
		this->wgtFit->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::wgtFit_MouseDown);
		this->wgtFit->MouseUp   += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::wgtFit_MouseUp);
		this->wgtFit->Visible = true;

		RECT area;
		area.top = 0;
		area.left = 0;
		area.right = wgtFit->Size.Width;
		area.bottom = wgtFit->Size.Height;
		wgtFit->graph = gcnew Graph(
							area, 
							RGB(250, 0, 0), 
							DRAW_LINES, x, y, 
							logXCheckBox->Checked,
							logScale->Checked);
		graphType->push_back(GRAPH_DATA);

		wgtFit->graph->SetXLabel("q [nm" + UnicodeChars::minusOne + "]");
		wgtFit->graph->SetYLabel("Intensity [a.u.]");

		wgtFit->graph->Resize(area);

		std::vector<std::string> legendNames;

		legendNames.push_back("Model");

		if(!_bGenerateModel) {
			std::vector<double> my;
			paramStruct par (_model);
			peakStruct peaks;
			phaseStruct phase;
			bgStruct background;
			par = *_curPar;
			/*par.b_polydisp = PolycheckBox->Checked;
			par.polydispValue = clrToDouble(PolytextBox->Text);
			par.polydispInd = PolycomboBox->SelectedIndex;*/
			GetPhasesFromListView(&phase); 
			GetBGFromGUI(&background);
			if(cailleGroupbox->Enabled)
				GetCaillePeaksFromGUI(&peaks); 
			else
				GetPeaksFromListView(&peaks);


			_ff->x = x;
			_sf->x = x;
			_bg->x = x;
			_ff->y.clear();
			_sf->y.clear();	
			_bg->y.clear();	

			//SetGaussED(_bGaussian);
			//SetConsEccentricity(_bConsEcc);

			// Filling our separate form factor, structure factor and background graphs
			GenerateStructureFactor(x, _sf->y, &peaks);
			GenerateBackground(x, _bg->y, &background);
			if(_bLoadedFF)
				loadFileAsFormFactor(*_loadedFF, x, _ff->y);
			else
				GenerateModel(x, _ff->y, &par, _pShouldStop);

			if(_ff->y.empty())	// Something not so good happened
				_ff->y.resize(_ff->x.size(), 0.0);

			//TODO:: create a condition to select between structure factor and phases.
			MultiplyVectors(my, _ff->y, _sf->y);
			AddVectors(my, my, _bg->y);
			my = MachineResolution(_ff->x, my, GetResolution());

			graphType->push_back(GRAPH_MODEL);
			wgtFit->graph->Add(RGB(54,13,187), DRAW_LINES, _ff->x, my);

			// Calculate Chi squared (WSSR)
			UpdateChisq(WSSR(y, my));
			// Calculate R squared
			UpdateRSquared(RSquared(y, my));

			legendNames.insert(legendNames.begin(), "Signal");

			wgtFit->graph->Legend(legendNames);

			PeakPicker->Enabled = true;
		} else {
			int res = int(clrToDouble(toolStripTextBox2->Text));
			std::vector<double> x (res, 0.0);

			double s = clrToDouble(startGen->Text), end = clrToDouble(endGen->Text);
			for(int i = 0; i < int(x.size()); i++)
				x[i] = s + (double(i + 1) * (end - s) / (double(res)));

			_ff->x = x;
			_sf->x = x;

			_ff->y.resize(x.size(), 1.0);
			_sf->y.resize(x.size(), 1.0);

			wgtFit->graph->Legend(legendNames);
		}

		wgtFit->Invalidate();
		GlobalFitter::graph = wgtFit->graph;
	}

	void FormFactor::UpdateExtraParamBox() {
		ListViewItem ^lvi = 
			listView_Extraparams->Items[paramBox->SelectedIndex];

		if(listView_Extraparams->SelectedItems->Count == 0)
			listView_Extraparams->SelectedIndices->Add(0);
		exParamGroupbox->Text = listView_Extraparams->SelectedItems[0]->SubItems[ELV_NAME]->Text;
		exParamGroupbox->text->Text = lvi->SubItems[exParamGroupbox->rStddev->Checked ? ELV_SIGMA : ELV_VALUE]->Text;
		exmin->Text   = lvi->SubItems[ELV_CONSMIN]->Text;
		exmax->Text   = lvi->SubItems[ELV_CONSMAX]->Text;
	
		// Extra parameter specification modifications
		ExtraParam ep = _model->GetExtraParameter(paramBox->SelectedIndex);
		
		infExtraParam->Visible = ep.canBeInfinite;
		infExtraParam->Checked = lvi->SubItems[ELV_INFINITE]->Text->Equals("1");

		if(infExtraParam->Checked) {
				 exParamGroupbox->Enabled = false;
				 exmin->Enabled   = false;
				 exmax->Enabled   = false;
		} else {
				exParamGroupbox->Enabled = true;
				exmin->Enabled   = !_bGenerateModel;
				exmax->Enabled   = !_bGenerateModel;
				exParamGroupbox->check->Enabled   = !_bGenerateModel;
		}

		if(lvi->SubItems[ELV_MUTABLE]->Text->Equals("-"))
			exParamGroupbox->check->Enabled = false;

		if(!_bGenerateModel)
			exParamGroupbox->check->Checked = (lvi->SubItems[ELV_MUTABLE]->Text->Equals("Y"));
	}

	void FormFactor::UItoParameters(paramStruct *p) {
		
		p->layers = listViewFF->Items->Count;
		int nlp = _model->GetNumLayerParams();
		p->params.resize(nlp);
		for(int jaja = 0; jaja < nlp; jaja++)
			p->params[jaja].resize(p->layers);
		
		for(int i = 0; i < listViewFF->Items->Count; i++) {
			std::string str;
			ListViewItem ^lvi = listViewFF->Items[i];
		
			for (int j = 0; j < nlp; j++) {
				Parameter parame (clrToDouble(lvi->SubItems[LV_VALUE(j)]->Text),
					lvi->SubItems[LV_MUTABLE(j)]->Text->Equals("Y") ? true : false,
					(lvi->SubItems[LV_CONS(j, nlp)]->Text->Equals("Y")),
					clrToDouble(lvi->SubItems[LV_CONSMIN(j, nlp)]->Text),
					clrToDouble(lvi->SubItems[LV_CONSMAX(j, nlp)]->Text),
					(int)clrToDouble(lvi->SubItems[LV_CONSIMIN(j, nlp)]->Text),
					(int)clrToDouble(lvi->SubItems[LV_CONSIMAX(j, nlp)]->Text),
					(int)clrToDouble(lvi->SubItems[LV_CONSLINK(j, nlp)]->Text),
					clrToDouble(lvi->SubItems[LV_SIGMA(j, nlp)]->Text));

				p->params[j][i] = parame;
			}
		}
		
		p->extraParams.resize(listView_Extraparams->Items->Count);
		for(int i = 0; i < listView_Extraparams->Items->Count; i++) {
			std::string str;
			ListViewItem ^lvi2 = listView_Extraparams->Items[i];

			Parameter para(clrToDouble(lvi2->SubItems[ELV_VALUE]->Text), 
					lvi2->SubItems[ELV_MUTABLE]->Text->Equals("Y")? true : false, 
					(constraints->Checked && 
					 lvi2->SubItems[ELV_CONS]->Text->Equals("Y")) || raindrop->Checked,
					clrToDouble(lvi2->SubItems[ELV_CONSMIN]->Text),
					clrToDouble(lvi2->SubItems[ELV_CONSMAX]->Text),
					-1, -1, -1,
					clrToDouble(lvi2->SubItems[ELV_SIGMA]->Text));
			p->extraParams[i] = para;

			// If this parameter is infinite
			if(lvi2->SubItems[ELV_INFINITE]->Text->Equals("1"))
				p->extraParams[i].value = std::numeric_limits<double>::infinity();
		}

		p->bConstrain = constraints->Checked;
	}

	void FormFactor::ParametersToUI(const paramStruct *p) {
	

		// Regular parameters
		int nlp = _model->GetNumLayerParams();

		while(listViewFF->Items->Count < p->layers)
			AddParamLayer();
		while(listViewFF->Items->Count > p->layers)
			listViewFF->Items->RemoveAt(listViewFF->Items->Count - 1);

		for(int i = 0; i < listViewFF->Items->Count; i++) {
			ListViewItem ^lvi = listViewFF->Items[i];

			for(int j = 0; j < nlp; j++) {
				Parameter param = p->params[j][i];

				if(_model->IsParamApplicable(i, j)) {
					// Value
					lvi->SubItems[LV_VALUE(j)]->Text = 
						param.value.ToString("0.000000");

					// Mutability
					lvi->SubItems[LV_MUTABLE(j)]->Text = param.isMutable ? "Y" : 
													 "N";

					// Absolute constraints
					lvi->SubItems[LV_CONSMIN(j, nlp)]->Text =
										param.consMin.ToString("0.000000");
					lvi->SubItems[LV_CONSMAX(j, nlp)]->Text =
										param.consMax.ToString("0.000000");
					lvi->SubItems[LV_CONS(j, nlp)]->Text =
										param.isConstrained ? "Y" : "N";

					// Relative constraints
					lvi->SubItems[LV_CONSIMIN(j, nlp)]->Text =
										param.consMinIndex.ToString();
					lvi->SubItems[LV_CONSIMAX(j, nlp)]->Text =
										param.consMaxIndex.ToString();
					
					// Parameter link constraint
					if(param.linkIndex >= 0) {
						lvi->SubItems[LV_MUTABLE(j)]->Text = "L";
						lvi->SubItems[LV_CONSLINK(j, nlp)]->Text = 
										param.linkIndex.ToString();
					}

					// Model modifiers
					lvi->SubItems[LV_SIGMA(j, nlp)]->Text =
										param.sigma.ToString();
				}
			}
		}
		
		for(int i = 0; i < listView_Extraparams->Items->Count; i++) {
			ListViewItem ^lvi2 = listView_Extraparams->Items[i];
			int decpoints = _model->GetExtraParameter(i).decimalPoints;

			Parameter param = p->extraParams[i];

			char a[64] = {0};
			sprintf(a, "%.*f", _model->GetExtraParameter(i).decimalPoints,
					param.value);

			// If this parameter is infinite
			if(_model->GetExtraParameter(i).canBeInfinite && 
				!_finite(param.value)) {
					if(!(lvi2->SubItems[ELV_INFINITE]->Text->Equals("1"))) {
						// If the parameter wasn't infinite, add an "(inf)"
						lvi2->Text += " (inf)";
						lvi2->SubItems[ELV_INFINITE]->Text = "1";
					}
					sprintf(a, "%.*f", decpoints, 0.0);
			} else if(_finite(param.value) && lvi2->SubItems[ELV_INFINITE]->Text->Equals("1")) {
				// Removing the "(inf)" if necessary
				lvi2->Text = lvi2->Text->Substring(0, lvi2->Text->Length - 6);
				lvi2->SubItems[ELV_INFINITE]->Text = "0";
			}

			// Value
			lvi2->SubItems[ELV_VALUE]->Text = gcnew String(a);
			
			// Mutability
			lvi2->SubItems[ELV_MUTABLE]->Text = param.isMutable ? "Y" : "N";

			// Constraints
			sprintf(a, "%.*f", decpoints, param.consMin);
			lvi2->SubItems[ELV_CONSMIN]->Text = gcnew String(a);
			sprintf(a, "%.*f", decpoints, param.consMax);
			lvi2->SubItems[ELV_CONSMAX]->Text = gcnew String(a);
			lvi2->SubItems[ELV_CONS]->Text = param.isConstrained ? "Y" : "N";

			// Modifiers
			sprintf(a, "%.*f", decpoints, param.sigma);
			lvi2->SubItems[ELV_SIGMA]->Text = gcnew String(a);
		}
		UpdateExtraParamBox();
			
		FFParameterUpdateHandler();
	}

	void FormFactor::InitializeGraph(bool bZero, std::vector<double>& bgx,std::vector<double>& bgy,
									 std::vector<double>& ffy) {

		std::vector <double> x,y;
		ReadCLRFile(_dataFile, x, y);

		_data->x = x;
		_data->y = y;

		_baseline->x = bgx;
		_baseline->y = bgy;

		exportBackgroundToolStripMenuItem->Enabled = true;
		calculate->Enabled = true;

		if(listView_PeakPosition->Items->Count > 0) {
			fitphase->Enabled = true;
			Caille_button->Enabled = true;
		} else {
			fitphase->Enabled = false;
			Caille_button->Enabled = false;
		}
		smooth->Enabled = true;

		InitializeFitGraph(bZero ? x : bgx, bZero ? y : ffy);
	}

	void FormFactor::ExtraParameter_Enter(System::Object^ sender, System::EventArgs^  e) {
		if(listView_Extraparams->SelectedItems->Count == 0)
			listView_Extraparams->SelectedIndices->Add(paramBox->SelectedIndex);
	}

	void FormFactor::ExtraParameter_TextChanged(System::Object^ sender, System::EventArgs^  e) {
		// This function transforms a written extra parameter in a textbox (sender)
		// to the correct form (using ExtraParameter description)
		// and updates the corresponding listview
		TextBox ^textbox = (TextBox ^)sender;

		if(listView_Extraparams->SelectedItems->Count == 0)
			return;

		int exindex = listView_Extraparams->SelectedIndices[0];
		// This will define the extra parameter
		ExtraParam def = _model->GetExtraParameter(exindex);

		double res;
		std::string str;
		char f[128] = {0};
		
		// Parse the value to a string
		clrToString(textbox->Text, str);
		res = strtod(str.c_str(), NULL);

		// Modify the value according to the definition

		// Range
		if(def.isRanged) {
			if(res < def.rangeMin)
				res = def.rangeMin;
			if(res > def.rangeMax)
				res = def.rangeMax;
		}

		// Absolute values
		if(def.isAbsolute && res < 0.0)
			res = -res;

		// Format the double as the modified value
		sprintf(f, "%.*f", def.decimalPoints, res);

					
		textbox->Text = gcnew String(f);
		
		if(sender == exParamGroupbox->text)
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[exParamGroupbox->rValue->Checked ? ELV_VALUE : ELV_SIGMA]->Text = exParamGroupbox->text->Text;
		if(sender == exmin)
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_CONSMIN]->Text = exmin->Text;
		if(sender == exmax)
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_CONSMAX]->Text = exmax->Text;

		FFParameterUpdateHandler();
		
		save->Enabled = true;
		undo->Enabled = false;
	}


	void FormFactor::Parameter_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		double res;
		std::string str;
		char f[128] = {0};

		if(_bChanging || _bLoading) return;

		if(listViewFF->SelectedIndices->Count == 0) return;

		clrToString(((TextBox ^)(sender))->Text, str);

		res = fabs(strtod(str.c_str(), NULL));
		
		sprintf(f, "%.6f", res);		
		((TextBox ^)(sender))->Text = gcnew String(f);

		for (int i = 0; i < _model->GetNumLayerParams(); i++)
			if(sender == GroupBoxList[i]->text)
				for(int j = 0; j < listViewFF->SelectedIndices->Count; j++)
					listViewFF->SelectedItems[j]->SubItems[
						GroupBoxList[i]->rValue->Checked
						? LV_VALUE(i)
						: LV_SIGMA(i, _model->GetNumLayerParams())
						]->Text = GroupBoxList[i]->text->Text;	
			

		linkedParameterChangedCheck(listViewFF->Items, listViewFF->SelectedIndices[0]);

		FFParameterUpdateHandler();

		save->Enabled = true;
		undo->Enabled = false;
	}
	
	void FormFactor::PDRadioChanged(System::Object ^ sender, System::EventArgs^ e) {
		if((Control^)(((System::Windows::Forms::RadioButton^)(sender))->Parent) == exParamGroupbox) {
			listView_Extraparams_SelectedIndexChanged(sender, e);
			exParamGroupbox->text->Text = listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[exParamGroupbox->rValue->Checked ? ELV_VALUE : ELV_SIGMA]->Text;
		} else
			listViewFF_SelectedIndexChanged(sender, e);
	}

	void FormFactor::double_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		double res;
		std::string str;
		char f[128] = {0};
		
		clrToString(((TextBox ^)(sender))->Text, str);

		res = strtod(str.c_str(), NULL);
		
		sprintf(f, "%.6f", res);		
		((TextBox ^)(sender))->Text = gcnew String(f);
		
		FFParameterUpdateHandler();

		save->Enabled = true;
		undo->Enabled = false;
	}

	void FormFactor::AddParamLayer(std::vector<Parameter> layer) {
		ListViewItem ^lvi;
		char f[128] = {0};
		int currentLayer = listViewFF->Items->Count;
		

		String ^param = gcnew String(_model->GetLayerName(
							currentLayer).c_str());

		lvi = gcnew ListViewItem(param);
	
		for(int i = 0; i < (int)layer.size(); i++) {
			if(!_model->IsParamApplicable(currentLayer, i)) {
				lvi->SubItems->Add("N/A");
				lvi->SubItems->Add("-");
			} else {		
				lvi->SubItems->Add(layer[i].value.ToString("0.000000"));
				lvi->SubItems->Add(layer[i].isMutable ? "Y" : "N");
			}
		}

		for(int i = 0; i < (int)layer.size(); i++) {
			lvi->SubItems->Add(layer[i].consMin.ToString("0.000000"));
			lvi->SubItems->Add(layer[i].consMax.ToString("0.000000"));
			lvi->SubItems->Add(layer[i].consMinIndex.ToString());
			lvi->SubItems->Add(layer[i].consMaxIndex.ToString());
			lvi->SubItems->Add(layer[i].linkIndex.ToString());
			lvi->SubItems->Add(layer[i].isConstrained ? "Y" : "N");
			lvi->SubItems->Add(layer[i].sigma.ToString("0.000000"));
		}
	
		listViewFF->Items->Add(lvi);
		

		/*String ^s = listViewFF->Columns[1]->Text + " " + (listViewFF->Items->Count - 1).ToString(); 
		PolycomboBox->Items->Add(s);
		if( isThreeParamModel()) {
			String ^s = listViewFF->Columns[5]->Text + " " + (listViewFF->Items->Count - 1).ToString(); 
			PolycomboBox->Items->Add(s);
		}*/
		
		// Making sure we don't pass maximal layer count
		if(listViewFF->Items->Count == _model->GetMaxLayers())
			addLayer->Enabled = false;

		FFParameterUpdateHandler();

		save->Enabled = true;
		undo->Enabled = false;
	}

	void FormFactor::listViewFF_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		int paramNum = _model->GetNumLayerParams();
		int minLayers = _model->GetMinLayers();

		if(listViewFF->SelectedIndices->Count == 0) {
			// No Items
			paramLabel->Text = "<None>";
			removeLayer->Enabled = false;
			
			for (int i = 0; i < paramNum; i++){
				GroupBoxList[i]->Enabled = false;
				GroupBoxList[i]->track->Value = int((GroupBoxList[i]->track->Minimum + GroupBoxList[i]->track->Maximum) / 2.0);
			}	
				
			fitRange->Enabled = false;
			
		} else if(listViewFF->SelectedIndices->Count > 1) {
			// Multiple Items
			paramLabel->Text = "<Multiple>";

			for (int i = 0; i < paramNum; i++){
				GroupBoxList[i]->Enabled = true;
				GroupBoxList[i]->text->Enabled = false;
				GroupBoxList[i]->track->Enabled = true;
				GroupBoxList[i]->check->Enabled = false;

				if(!_bGenerateModel)  
					GroupBoxList[i]->check->Enabled = true;				
			}		
				
			fitRange->Enabled = false;
			
			
			removeLayer->Enabled = true;
			for(int i = 0; i < listViewFF->SelectedIndices->Count; i++)
				if(listViewFF->SelectedIndices[i] < minLayers)
						removeLayer->Enabled = false;
				

			
			for(int i = 0; i < listViewFF->SelectedItems->Count; i++) 
				for (int j = 0; j < paramNum ; j++){
					if(listViewFF->SelectedItems[i]->SubItems[LV_MUTABLE(j)]->Text->Equals("-"))
						GroupBoxList[j]->check->Enabled = false;
					if(listViewFF->SelectedItems[i]->SubItems[LV_VALUE(j)]->Text->Equals("N/A"))
						GroupBoxList[j]->track->Enabled = false;
			}

		} else {
			// One Item
			ListViewItem ^lvi = listViewFF->SelectedItems[0];
		
			paramLabel->Text = lvi->Text;
			
			if(listViewFF->SelectedIndices[0] < minLayers)
				removeLayer->Enabled = false;
			else
				removeLayer->Enabled = true;
			
			for (int i = 0; i < paramNum; i++){
				GroupBoxList[i]->Enabled = true;
				GroupBoxList[i]->text->Enabled = true;
				GroupBoxList[i]->check->Enabled = false;
				GroupBoxList[i]->track->Enabled = true; 
				if(!_bGenerateModel)  
					GroupBoxList[i]->check->Enabled = true;
			}		

			if(constraints->Checked)
				fitRange->Enabled = true;


			for (int j = 0; j < paramNum ; j++){
				if(lvi->SubItems[LV_MUTABLE(j)]->Text->Equals("-"))
					GroupBoxList[j]->check->Enabled = false;
				if(lvi->SubItems[LV_VALUE(j)]->Text->Equals("N/A") || 
				   Int32::Parse(lvi->SubItems[LV_CONSLINK(j, paramNum)]->Text) > 0) {
					GroupBoxList[j]->track->Enabled = false;
					GroupBoxList[j]->text->Enabled = false;
				}
			}
			
			for (int i = 0; i < paramNum; i++){
				GroupBoxList[i]->text->Text = lvi->SubItems[
					GroupBoxList[i]->rValue->Checked
					? LV_VALUE(i)
					: LV_SIGMA(i, _model->GetNumLayerParams())
					]->Text;
				GroupBoxList[i]->check->Checked = (lvi->SubItems[LV_MUTABLE(i)]->Text->Equals("Y") ? true : false);
			}

			EDU();
			
		}
	}

	void FormFactor::listView_Extraparams_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		if(listView_Extraparams->SelectedIndices->Count > 0 &&
			paramBox->SelectedIndex != listView_Extraparams->SelectedIndices[0])
			paramBox->SelectedIndex = listView_Extraparams->SelectedIndices[0];
	}

	void FormFactor::UpdateChisq(double chisq) {
		if(chisq < 0.0) return;
		wssr->Text = wssr->Text->Substring(0, wssr->Text->LastIndexOf('=') + 1);
		wssr->Text += chisq.ToString();
		_curWssr = chisq;
	}

	void FormFactor::UpdateRSquared(double rsq) {
		if(rsq < 0.0)
			rsq = 0.0;
		if(rsq > 1.0)
			rsq = 1.0;
		rsquared->Text = rsquared->Text->Substring(0, rsquared->Text->LastIndexOf('=') + 1);
		rsquared->Text += rsq.ToString("0.000000");
		_curRSquared = rsq;
	}

	void FormFactor::calculate_Click(System::Object^  sender, System::EventArgs^  e) {
		static paramStruct oldPl (_model);
		static peakStruct oldPeaks, oldCaillePeaks;
		static bgStruct oldBG;
	//	static std::vector<double> oldEx;
		static bool IsHighIterOfMultifit;// = false;

		// For iterative fitting
		static paramStruct oldPl_iter (_model);
		static peakStruct oldPeaks_iter, oldCaillePeaks_iter;
		static bgStruct oldBG_iter;
	//	static std::vector<double> oldEx_iter;
		
		if(_peakPicker)
			PeakPicker_Click(sender, e);

		if(sender == calculate) {
			_bMultCalc = (CalcComboBox->SelectedIndex > 0);
			IsHighIterOfMultifit = false;
		}

		if(sender == undo) {
			if(!_bLastFitMult) {
				ParametersToUI(&oldPl);
				SetPeaks(&oldPeaks);
				SetBGtoGUI(&oldBG);
				SetCaillePeakstoGUI(&oldCaillePeaks);
			} else {
				ParametersToUI(&oldPl_iter);
				SetPeaks(&oldPeaks_iter);
				SetBGtoGUI(&oldBG_iter);
				SetCaillePeakstoGUI(&oldCaillePeaks_iter);
			}
			// TODO: Add calcAll parameter? (_bLastFitMult here)
			SFParameterUpdateHandler();
			BGParameterUpdateHandler();
			FFParameterUpdateHandler();
			return;
		}
		
		if (_bMultCalc && sender != calculate && sender != undo)
			IsHighIterOfMultifit = true;

		if(tabControl1->SelectedTab->Name == "FFTab" && ((!ffUseCheckBox->Checked) || _bFrozenFF))
			return;
		else if(tabControl1->SelectedTab->Name == "SFTab" && !sfUseCheckBox->Checked)
			return;
		else if(tabControl1->SelectedTab->Name == "BGTab" && !bgUseCheckBox->Checked)
			return;

		// FIXME: What is this?
		if(calculate->Text->Equals("Stop") && !IsHighIterOfMultifit/* && sender != modelFitter*/) {	
			_CounterCalc = (int)(clrToDouble(fittingIter->Text)/10.0) + 1;
			calculate->Enabled = false;
		
			// Cancelling fitting
			*_pShouldStop = 1;

			return;
		}

		this->Cursor = System::Windows::Forms::Cursors::AppStarting;
		for (int i = 0; _CounterCalc == 0 && i < this->Controls->Count; i++) { 
			this->Controls[i]->AllowDrop = this->Controls[i]->Enabled;
			this->Controls[i]->Enabled = false;
		}

		this->listViewFF->SelectedIndices->Clear();
		this->listView_PeakPosition->SelectedIndices->Clear();
		this->listView_peaks->SelectedIndices->Clear();
		this->BGListview->SelectedIndices->Clear();
		this->cailleParamListView->SelectedIndices->Clear();
		this->label6->Visible = true;
		this->progressBar1->Visible = true;
		this->iterProgressBar->Visible = _bMultCalc;
		progressBar1->Value = 0;
		if(sender == calculate)
			iterProgressBar->Value = iterProgressBar->Minimum;


		if(!wgtFit)
			InitializeFitGraph();
					
		if(_mask->size() != wgtFit->graph->x[0].size())
			_mask->resize(wgtFit->graph->x[0].size(), false);

		if(!_bGenerateModel && !wgtFit->Visible) {
			wgtFit->Visible = true;
			LocOnGraph->Visible = true;
			wssr->Visible = true;
			rsquared->Visible = true;
		}

		if(!_curPeaks)
			_curPeaks = new peakStruct;

		if(!_curPeaksCaille)
			_curPeaksCaille = new peakStruct;

		if(!_curBG)
			_curBG = new bgStruct;

		if(!_curCaille)
			_curCaille = new cailleParamStruct;

	//	_curPar->b_polydisp = PolycheckBox->Checked;
	//	_curPar->polydispValue = clrToDouble(PolytextBox->Text);
	//	_curPar->polydispInd = PolycomboBox->SelectedIndex;
		GetPeaksFromListView(_curPeaks);
		GetBGFromGUI(_curBG);
		GetCaillePeaksFromGUI(_curPeaksCaille);


		if(sender == calculate && CalcComboBox->SelectedIndex > 0) {
			oldPl_iter			= *_curPar;
			//oldEx_iter			= _curPar->exParams;
			oldPeaks_iter		= *_curPeaks;
			oldBG_iter			= *_curBG;
			oldCaillePeaks_iter	= *_curPeaksCaille;
		} else {
			oldPl = *_curPar;
			//oldEx = _curPar->exParams;
			oldPeaks = *_curPeaks;
			oldBG = *_curBG;
			oldCaillePeaks = *_curPeaksCaille;
			_bLastFitMult = false;
		}

		calculate->Text = "Stop";
		calculate->Enabled = true;		
		fitphase->Enabled = false;
		Caille_button->Enabled = false;
		tableLayoutPanel1->Enabled = true;
		panel3->Enabled = true;

		// Disabling what needs to be disabled
		pauseFittingToolStripMenuItem->Enabled = true;
		fittingEmergencyStopToolStripMenuItem->Enabled = true;
		save->AllowDrop = save->Enabled;
		save->Enabled = false;
		changeData->AllowDrop = changeData->Enabled;
		changeData->Enabled = false;
		changeModel->Enabled = false;
		panel2->Enabled = false;
		Caille_button->Enabled = false;
		undo->Enabled = false;
		maskButton->Enabled = false;
		
		double grrr = clrToDouble(listView_phases->Items[0]->SubItems[1]->Text);
		SetD(grrr);
		// FIXME: What is this?
		//if (sender != modelFitter) { 
		if (!IsHighIterOfMultifit) {
			_CounterCalc = 0;
			_LastTab2Calc = tabControl1->SelectedIndex;
			if(CalcComboBox->SelectedIndex > 0)
				_bMultCalc = true;
			else 
				_bMultCalc = false;
		}
		
		fitterThread = gcnew Threading::Thread(gcnew Threading::ParameterizedThreadStart(this, &FormFactor::modelFitter_threadFunc));
		bThreadSuspended = false;
		fitterThread->Start(gcnew Int32(_LastTab2Calc));
		//this->modelFitter->RunWorkerAsync();	
	}

	void FormFactor::pauseFittingToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(bThreadSuspended)
			fitterThread->Resume();
		else
			fitterThread->Suspend();

		bThreadSuspended = !bThreadSuspended;
	}

	void FormFactor::fittingEmergencyStopToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		fitterThread->Abort();
	}

	void FormFactor::UpdateGraph() {
		UpdateGraph(false);
	}

	void FormFactor::UpdateGraph(bool calcAll) {
		if(_bLoading || !wgtFit)
			return;
		if(!wgtFit->graph)
			return;
		if(_bFrozenFF && tabControl1->SelectedTab->Name == "FFTab") {
			ScaleandBGFF();
			return;
		}
		std::vector<double> mx, my;
		paramStruct par = *_curPar;
		peakStruct peaks;
		bgStruct background;

		
	//	par.b_polydisp = PolycheckBox->Checked;
	//	par.polydispValue = clrToDouble(PolytextBox->Text);
	//	par.polydispInd = PolycomboBox->SelectedIndex;

		if(GetPeakType() == SHAPE_CAILLE) {
			GetCaillePeaksFromGUI(&peaks);
			SetD(clrToDouble(listView_phases->Items[0]->SubItems[1]->Text));
		}
		else
			GetPeaksFromListView(&peaks);

		GetBGFromGUI(&background);

		mx = wgtFit->graph->x[0];

		if(tabControl1->SelectedTab->Name == "SFTab" || calcAll) { //sender is the SF tab
			if(sfUseCheckBox->Checked) {
				_sf->y.clear(); //use existing form factor
				GenerateStructureFactor(mx, _sf->y, &peaks);

				if(_peakPicker)  // We're looking only at the SF...
					my = _sf->y;
			}
			if(!_peakPicker) {
				MultiplyVectors(my, _ff->y, _sf->y);
				AddVectors(my, my, _bg->y);
			}
		}
		if(tabControl1->SelectedTab->Name == "FFTab" || calcAll) { // sender is in FF tab
			if(ffUseCheckBox->Checked) {
				_ff->y.clear();	//use existing structure factor
				if(!_bLoadedFF) {
					//SetGaussED(_bGaussian);
					//SetConsEccentricity(_bConsEcc);
					GenerateModel(mx, _ff->y, &par, _pShouldStop);
				} else {
					loadFileAsFormFactor(*_loadedFF, _ff->x, _ff->y);
				}

			}
			MultiplyVectors(my, _ff->y, _sf->y);
			AddVectors(my, my, _bg->y);
		}
		if(tabControl1->SelectedTab->Name == "BGTab" || calcAll) { //sender is in the BG tab
			if(bgUseCheckBox->Checked) {
				_bg->y.clear();
				GenerateBackground(mx, _bg->y, &background);

				if(_fitToBaseline)
					my = _bg->y;
			}
			
			if(!_fitToBaseline) {
				MultiplyVectors(my, _ff->y, _sf->y);
				AddVectors(my, my, _bg->y);
			}
		}

		my = MachineResolution(mx, my, GetResolution());

		if(!_bGenerateModel) {
			UpdateChisq(WSSR(wgtFit->graph->y[0], my));
			UpdateRSquared(RSquared(wgtFit->graph->y[0], my));
		}

		wgtFit->graph->ToggleYTicks();
		wgtFit->graph->ToggleYTicks();
		if(my.size() == mx.size())
			wgtFit->graph->Modify(_bGenerateModel ? 0 : 1, mx, my);
		wgtFit->Invalidate();
	}

	void FormFactor::zeroBG_Click(System::Object^  sender, System::EventArgs^  e) {
		if(_peakPicker) PeakPicker_Click(sender, e);
		if(_fitToBaseline) fitToBaseline_Click(sender, e);
		bool angstrom = a1nm1ToolStripMenuItem->Checked;
		if(angstrom)
			a1nm1ToolStripMenuItem->Checked = !a1nm1ToolStripMenuItem->Checked;
		label1->Visible = false;
		exportSignalToolStripMenuItem->Enabled = true;
		exportModelToolStripMenuItem1->Enabled = true;
		plotFittingResultsToolStripMenuItem->Enabled = true;
		exportGraphToolStripMenuItem->Enabled = true;
		exportFormFactorToolStripMenuItem->Enabled = true;
		exportSigModBLToolStripMenuItem->Enabled = true;
		exportDecomposedToolStripMenuItem->Enabled = true;
		exportStructureFactorToolStripMenuItem->Enabled = true;
		SetMinimumSig(0.0); // As per Pablo & Tal's request
		minimumSignalTextbox->Text = GetMinimumSig().ToString();
		
		fitToBaseline->Enabled = false;

		ffUseCheckBox->Checked = true;
		sfUseCheckBox->Checked = true;
		bgUseCheckBox->Checked = true;

		_ff->tmpY.clear();
		_sf->tmpY.clear();
		_bg->tmpY.clear();

		std::vector <double> x, y, ffy; 
		InitializeGraph(true, x, y, ffy);

		ffUseCheckBox->Enabled = true;
		sfUseCheckBox->Enabled = true;
		bgUseCheckBox->Enabled = true;

		if(angstrom)
			a1nm1ToolStripMenuItem->Checked = !a1nm1ToolStripMenuItem->Checked;

		tabControl1->SelectTab("FFTab");
	}


	void FormFactor::modelFitter_RunWorkerCompleted(System::Object^  sender, System::ComponentModel::RunWorkerCompletedEventArgs^  e) {
		if(*_pShouldStop == 2) {
			this->Close();
			return;
		}

		_bFromFitter = true;
		int qqqq = (int)(clrToDouble(fittingIter->Text)/10.0);
		
		*_pShouldStop = 0;

		if(_bMultCalc)
			_bLastFitMult = (_CounterCalc + 2 >= qqqq);

		if( _CounterCalc + 1  >=  qqqq || !_bMultCalc)	{
			calculate->Text = _bGenerateModel ? L"Generate" : L"(Re)Calculate";
			calculate->Enabled = true;

			ffUseCheckBox->Enabled = true;
			sfUseCheckBox->Enabled = true;
			bgUseCheckBox->Enabled = true;
			maskButton->Enabled = true;

			fitphase->Text = (_bGenerateModel) ? L"Generate Phase" : L"Fit Phase";
			fitphase->Enabled = true;
			Caille_button->Enabled = true;

			progressBar1->Value = 100;
			iterProgressBar->Value = iterProgressBar->Maximum;
			Sleep(100); // Showing the full progress for some time

			if(!_bGenerateModel) {
				// Undo changes (in case of failure)
				if(e->Cancelled) {
					if(_bFitPhase)
						fitphase_Click(undoPhases, gcnew System::EventArgs());
					else
						calculate_Click(undo, gcnew System::EventArgs());
				} else {
					if (!_bFitPhase) {
						ParametersToUI(_curPar);
						SetPeaks(_curPeaks);
						SetBGtoGUI(_curBG);
						if(_model->HasSpecializedSF())
							SetCaillePeakstoGUI(_curPeaksCaille);
						undo->Enabled = true;
					}
					else {
						if(_curPhases)
							SetPhases(_curPhases);
						undoPhases->Enabled = true;
						PhasesCompleted();
					}

					_curWssr = WSSR(wgtFit->graph->y[0], wgtFit->graph->y[1]);
					UpdateChisq(_curWssr);
					UpdateRSquared(RSquared(wgtFit->graph->y[0], wgtFit->graph->y[1]));
					exportSignalToolStripMenuItem->Enabled = true;
					exportModelToolStripMenuItem1->Enabled = true;
					exportFormFactorToolStripMenuItem->Enabled = true;
					exportSigModBLToolStripMenuItem->Enabled = true;
					exportDecomposedToolStripMenuItem->Enabled = true;
					exportStructureFactorToolStripMenuItem->Enabled = true;

					plotFittingResultsToolStripMenuItem->Enabled = true;
					save->AllowDrop = true;
				}
				wgtFit->Invalidate();
			} else if(_bFitPhase) { // _bFitPhase and _bGenerate
				calculateRecipVectors();
				PhasesCompleted();
			}
		} //if( _CounterCalc + 1  >=  qqqq || !_bMultCalc)	{
		else { // We just finished an iteration and need to move on to the next one
			// Need to set the just modified parameters to the GUI and obtain the next set of parameters.
			iterProgressBar->Value = min((int)(double(_CounterCalc) * double(iterProgressBar->Maximum - iterProgressBar->Minimum) 
								/ double(qqqq)) + iterProgressBar->Minimum,
							iterProgressBar->Maximum);

			switch (CalcComboBox->SelectedIndex) {
				default:
					break;
				case 1:
					if(_LastTab2Calc == 0 ) { // finished FF
						ParametersToUI(_curPar);
						GetPeaksFromListView(_curPeaks);
					} else //finished SF
						SetPeaks(_curPeaks);
					break;
				case 2:
					if(_LastTab2Calc == 0 ) { // Finished FF
						ParametersToUI(_curPar);
						GetBGFromGUI(_curBG);
					} else // Finished BG
						SetBGtoGUI(_curBG);
					break;
				case 3:
					if(_LastTab2Calc == 1 ) { // Finished SF
						SetPeaks(_curPeaks);
						GetBGFromGUI(_curBG);
					} else { // Finished BG
						SetBGtoGUI(_curBG);
						GetPeaksFromListView(_curPeaks);
					}
					break;
				case 4:
					if(_LastTab2Calc == 0 ) {// Finished FF
						ParametersToUI(_curPar);
						GetPeaksFromListView(_curPeaks);
					} else if(_LastTab2Calc == 1 ) { // Finished SF
						SetPeaks(_curPeaks);
						GetBGFromGUI(_curBG);
					} else // Finished BG
						SetBGtoGUI(_curBG);
					break;
			}

		}

		// Can we not delete these...?
		//if(_curPar)
		//	delete _curPar;
		//_curPar = NULL;

		if(_curPeaks)
			delete _curPeaks;
		_curPeaks = NULL;

		if(_curPeaksCaille)
			delete _curPeaksCaille;
		_curPeaksCaille = NULL;

		if(_curBG)
			delete _curBG;
		_curBG = NULL;
		
		if(_curPhases)
			delete _curPhases;
		_curPhases = NULL;

		if(_curCaille)
			delete _curCaille;
		_curCaille = NULL;

		switch (CalcComboBox->SelectedIndex) {
			case 0:
			default:
				break;
			case 1:
				if( ++_CounterCalc  <  (int)(clrToDouble(fittingIter->Text)/10.0)) {
					if(_LastTab2Calc == 0 ) _LastTab2Calc = 1;
					else  _LastTab2Calc = 0;
					calculate_Click(sender, e);
					
				}
				break;
			case 2:
				if( ++_CounterCalc  <  (int)(clrToDouble(fittingIter->Text)/10.0)) {
					if(_LastTab2Calc == 0 ) _LastTab2Calc = 2;
					else  _LastTab2Calc = 0;
					calculate_Click(sender, e);
					
				}
				break;
			case 3:
				if( ++_CounterCalc  <  (int)(clrToDouble(fittingIter->Text)/10.0)) {
					if(_LastTab2Calc == 1 ) _LastTab2Calc = 2;
					else  _LastTab2Calc = 1;
					calculate_Click(sender, e);
					
				}
				break;
			case 4:
				if( ++_CounterCalc  <  (int)(clrToDouble(fittingIter->Text)/10.0)) {
					if(_LastTab2Calc == 0 ) _LastTab2Calc = 1;
					else if  (_LastTab2Calc == 1 ) _LastTab2Calc = 2;
					else _LastTab2Calc = 0;
					calculate_Click(sender, e);
					
				}
				break;

		} // switch(CalcComboBox->SelectedIndex)

		if( _CounterCalc  >=  qqqq || !_bMultCalc)	{

			wgtFit->Visible = true;
			wgtFit->Invalidate();
			exportModelToolStripMenuItem1->Enabled = true;
			exportGraphToolStripMenuItem->Enabled = true;
			exportFormFactorToolStripMenuItem->Enabled = true;
			exportSigModBLToolStripMenuItem->Enabled = true;
			exportDecomposedToolStripMenuItem->Enabled = true;
			exportStructureFactorToolStripMenuItem->Enabled = true;
			for (int i = 0 ; i < this->Controls->Count ; i++) { 
				this->Controls[i]->Enabled = true;//this->Controls[i]->AllowDrop;
				// Avi: this was causing Pablo's iterating feature to be Disabled after 2 iterations
				//		I still don't know exactly what it is...
				//		Control[0] is: The entire pane except the menu bar
				//		Control[1] is: The menu bar
				this->Controls[i]->AllowDrop = false;
			}
			changeData->Enabled = true;
			changeModel->Enabled = true;
			panel2->Enabled = true;
			Caille_button->Enabled = true;
			save->Enabled = true;

			pauseFittingToolStripMenuItem->Enabled = false;
			fittingEmergencyStopToolStripMenuItem->Enabled = false;

			this->label6->Visible = false;
			this->progressBar1->Visible = false;
			this->iterProgressBar->Visible = false;
			this->Cursor = System::Windows::Forms::Cursors::Default;
			this->UseWaitCursor = false;

			

			_bFitPhase = false;

			System::GC::Collect();
			_CrtDumpMemoryLeaks();
		}
		
		//UItoParameters(_curPar);	// Crashes when _curPar is NULL
		// Avi: This is what's causing the fitter to crash. This method calls UItoParameters (step 0)
		// which takes _curPar as a parameter, but on line 2099 of this file we delete _curPar
		// and set it to NULL. This (I think) has to do with a different understanding of what _curPar
		// is and what it should be used for (pre-MA / post-MA). While I'm going on about this method,
		// ParameterUpdateHandler is a FF method, and should not be called for SF or BG issues. There
		// are items there (such as FFparamErrors->clear(); etc.) that are only for the FF pane. In fact,
		// pretty much every item there has to do with the FF pane. In fact, it definitely should not be
		// called here, as one of the lines is "undo->Enabled = false;" which would kinda defeat the
		// purpose of having the undo button. I'm adding UItoParameters to attempt to address this issue.
		// I think that I'll commit now.
		//ParameterUpdateHandler();
		_bFromFitter = false;

	}

	FitMethod FormFactor::GetFitMethod() {
		if(levmar->Checked)
			return FIT_LM;
		if(diffEvo->Checked)
			return FIT_DE;
		if(raindrop->Checked)
			return FIT_RAINDROP;

		return FIT_LM;
	}

	FitMethod FormFactor::GetPhaseFitMethod() {
		if(pLevmar->Checked)
			return FIT_LM;
		if(pDiffEvo->Checked)
			return FIT_DE;
		if(pRaindrop->Checked)
			return FIT_RAINDROP;

		return FIT_LM;
	}
	bool RequalsZero(String ^s) {
		//R² = ERROR
		wchar_t *w = new wchar_t;
		//w[0] = 'R'; ; w[1] ='²'; w[2]=' '; w[3]='=';
		//= ("R² =");
		w = L"R² =";
		for (int i=0; i < 4; i++)
			s = (s->TrimStart(w[i]));
		double a = clrToDouble(s);
		return abs(a)<1e-9;		
	}

	void FormFactor::modelFitter_threadFunc(System::Object ^args) {
		System::ComponentModel::DoWorkEventArgs ^evargs = gcnew System::ComponentModel::DoWorkEventArgs(args);
		modelFitter_DoWork(calculate, evargs);
	}

	void FormFactor::modelFitter_DoWork(System::Object^  sender, System::ComponentModel::DoWorkEventArgs^  e) {
		// Initializing the stop signal
		*_pShouldStop = 0;
		
		// Initializing fitting settings
		//SetGaussED(gaussianEDToolStripMenuItem->Checked);
		//SetConsEccentricity(conserveEccentricityToolStripMenuItem->Checked);
		setAccuracySettings(accurateFittingToolStripMenuItem->Checked, accurateDerivativeToolStripMenuItem->Checked, 
			chiSquaredBasedFittingToolStripMenuItem->Checked || (_curRSquared <= 1e-6),logScaledFittingParamToolStripMenuItem->Checked);
		SetQuadResolution(int(clrToDouble(toolStripTextBox1->Text)));
		SetFitIterations((_bMultCalc)? 10:int(clrToDouble(_bFitPhase ? phaseIterations->Text : fittingIter->Text)));
		SetFitMethod(_bFitPhase ? GetPhaseFitMethod() : GetFitMethod());
	
		bool withinConstraints = true;

		if(!_bFitPhase) {
			if(_bGenerateModel) {
				std::vector<double> y, exParams;
				bool success = false;

				int res = int(clrToDouble(toolStripTextBox2->Text));
				std::vector<double> x (res, 0.0);

				double s = clrToDouble(startGen->Text), end = clrToDouble(endGen->Text);
				for(int i = 0; i < int(x.size()); i++)
					x[i] = s + (double(i + 1) * (end - s) / (double(res)));

				_ff->x = x;
				_sf->x = x;
				_bg->x = x;

				if((int)_ff->y.size() != res)
					_ff->y.resize(res, 0.0);
				if((int)_sf->y.size() != res)
					_sf->y.resize(res, 1.0);
				if((int)_bg->y.size() != res)
					_bg->y.resize(res, 0.0);

				// Generating form factor
				if(((Int32 ^)e->Argument)->Equals(0)) {
					_ff->y = _sf->y;
					if(liveFittingToolStripMenuItem->Checked)
						success = GenerateModelU(_ff->x, _ff->y, _bg->y, _curPar, &UpdateGeneratedGraph, 
												_pShouldStop, &ReportProgressDummy);
					else
						success = GenerateModelU(_ff->x, _ff->y, _bg->y, _curPar, NULL, _pShouldStop,
												NULL);

				} else if(((Int32 ^)e->Argument)->Equals(1)) { // Generating structure factor
					_sf->y = _ff->y;
					if(liveFittingToolStripMenuItem->Checked)
						success = GenerateStructureFactorU(_sf->x, _sf->y, _bg->y, (GetPeakType() == SHAPE_CAILLE) ? _curPeaksCaille : _curPeaks, &UpdateGeneratedGraph, 
														  _pShouldStop, &ReportProgressDummy);
					else
						success = GenerateStructureFactorU(_sf->x, _sf->y, _bg->y, (GetPeakType() == SHAPE_CAILLE) ? _curPeaksCaille : _curPeaks, NULL, _pShouldStop,
														   NULL);

				} else if(((Int32 ^)e->Argument)->Equals(2)) { // Generating background
					MultiplyVectors(y, _ff->y, _sf->y);

					if(liveFittingToolStripMenuItem->Checked)
						success = GenerateBackgroundU(_bg->x, _bg->y, y, _curBG, &UpdateGeneratedGraph, 
													  _pShouldStop, &ReportProgressDummy);
					else
						success = GenerateBackgroundU(_bg->x, _bg->y, y, _curBG, NULL, _pShouldStop,
													  NULL);
				}
				MultiplyVectors(y, _ff->y, _sf->y);
				AddVectors(y, y, _bg->y);

				if(success) {
					ReportProgressDummy(100);

					if(y.size() > 0) {
						x.resize(y.size());
						_ff->x = x;
						_sf->x = x;
						_bg->x = x;
						wgtFit->graph->Modify(0, x,  MachineResolution(x, y, GetResolution()));
					}

				} else
					e->Cancel = true;
			} else {	// Fitting a model
				std::vector<double> my (_ff->x.size(), 0.0);

				double lastChisq = WSSR_Masked(wgtFit->graph->y[0], wgtFit->graph->y[1], *_mask), 
					lastRsq = RSquared_Masked(wgtFit->graph->y[0], wgtFit->graph->y[1], *_mask);
				std::vector<double> ox = wgtFit->graph->x[1], oy = wgtFit->graph->y[1];

				bool success = false;

				// Fitting form factor
				if(((Int32 ^)e->Argument)->Equals(0)) {
					_ff->y = _sf->y;

					// Make sure all mutable variables are within constraints
					_curPar->bConstrain = constraints->Checked;
					// Extra options: return a list of parameters that are out of the constraints...
					if(constraints->Checked || raindrop->Checked) {
						//Extra Parameters
						for(unsigned int c = 0; c < _curPar->extraParams.size(); c++) {
							if((_curPar->extraParams[c].isConstrained || raindrop->Checked) && _curPar->extraParams[c].isMutable){
								if( fabs(_curPar->extraParams[c].consMax - _curPar->extraParams[c].consMin)>1e-6 && (_curPar->extraParams[c].value > _curPar->extraParams[c].consMax) ||
									(_curPar->extraParams[c].value < _curPar->extraParams[c].consMin)) {
											
											withinConstraints = false;
											break;
								}
							}
						}
						//Parameters
						for(unsigned int c = 0; c < _curPar->params.size() && withinConstraints; c++) {
							for(unsigned int d = 0; d < _curPar->params[c].size() && withinConstraints; d++) {
								if(_curPar->params[c][d].isMutable && (raindrop->Checked || _curPar->params[c][d].isConstrained) &&
									(_curPar->params[c][d].value < _curPar->params[c][d].consMin
									|| _curPar->params[c][d].value > _curPar->params[c][d].consMax)) {
									withinConstraints = false;
									continue;
								}
							}
						}
					}

					if(withinConstraints) {
						if(liveFittingToolStripMenuItem->Checked)
							success = CreateModelU(wgtFit->graph->x[0], wgtFit->graph->y[0], 
							_ff->y, _bg->y, *_mask, _curPar, *FFparamErrors, *FFmodelErrors, &UpdateGeneratedGraph,
												  _pShouldStop, ReportProgressDummy);
						else
							success = CreateModel(wgtFit->graph->x[0], wgtFit->graph->y[0], 
							_ff->y, _bg->y, *_mask, _curPar, *FFparamErrors, *FFmodelErrors, _pShouldStop);
					}
				} 
				// Fitting structure factor
				else if(((Int32 ^)e->Argument)->Equals(1)) { 
					_sf->y = _ff->y;
					if (GetPeakType() != SHAPE_CAILLE)
						SetFitMethod(FIT_LM);	// Until we implement constraints for SF

					if(GetPeakType() == SHAPE_CAILLE){
						if(_curPeaksCaille->cailleSigma[0].mut == 'Y' &&  fabs(_curPeaksCaille->cailleSigma[0].max - _curPeaksCaille->cailleSigma[0].min )>1e-6 &&
							( (_curPeaksCaille->cailleSigma[0].value < _curPeaksCaille->cailleSigma[0].min ) || 
							(_curPeaksCaille->cailleSigma[0].value > _curPeaksCaille->cailleSigma[0].max) ) )
								withinConstraints = false;
						if(_curPeaksCaille->amp[0].mut == 'Y' && fabs(_curPeaksCaille->amp[0].max - _curPeaksCaille->amp[0].min )>1e-6 &&
							( (_curPeaksCaille->amp[0].value < _curPeaksCaille->amp[0].min ) || 
							(_curPeaksCaille->amp[0].value > _curPeaksCaille->amp[0].max) ) )
								withinConstraints = false;
						if(_curPeaksCaille->center[0].mut == 'Y' && fabs(_curPeaksCaille->center[0].max - _curPeaksCaille->center[0].min )>1e-6 &&
							( (_curPeaksCaille->center[0].value < _curPeaksCaille->center[0].min ) || 
							(_curPeaksCaille->center[0].value > _curPeaksCaille->center[0].max) ) )
								withinConstraints = false;
						if(_curPeaksCaille->fwhm[0].mut == 'Y' && fabs(_curPeaksCaille->fwhm[0].max - _curPeaksCaille->fwhm[0].min )>1e-6 &&
							( (_curPeaksCaille->fwhm[0].value < _curPeaksCaille->fwhm[0].min ) || 
							(_curPeaksCaille->fwhm[0].value > _curPeaksCaille->fwhm[0].max) ) )
								withinConstraints = false;
						if(_curPeaksCaille->cailleNDiffused[0].mut == 'Y' && fabs(_curPeaksCaille->cailleNDiffused[0].max - _curPeaksCaille->cailleNDiffused[0].min )>1e-6 &&
							( (_curPeaksCaille->cailleNDiffused[0].value < _curPeaksCaille->cailleNDiffused[0].min ) || 
							(_curPeaksCaille->cailleNDiffused[0].value > _curPeaksCaille->cailleNDiffused[0].max) ) )
								withinConstraints = false;
					}

					if(withinConstraints) {
						if(liveFittingToolStripMenuItem->Checked)
							success = FitStructureFactorU(wgtFit->graph->x[0], wgtFit->graph->y[0], _sf->y, 
														  _bg->y, *_mask, (GetPeakType() == SHAPE_CAILLE) ? _curPeaksCaille : _curPeaks, *SFparamErrors, *SFmodelErrors, &UpdateGeneratedGraph,
														  _pShouldStop, &ReportProgressDummy);
						else
							success = FitStructureFactor(wgtFit->graph->x[0], wgtFit->graph->y[0], _sf->y, 
														 _bg->y, *_mask, (GetPeakType() == SHAPE_CAILLE) ? _curPeaksCaille : _curPeaks, *SFparamErrors, *SFmodelErrors);
					}
				} 
				// Fitting background
				else if(((Int32 ^)e->Argument)->Equals(2)) {
					if(!_fitToBaseline)
						MultiplyVectors(my, _ff->y, _sf->y);

					for(unsigned int c = 0; c < _curBG->base.size() && withinConstraints ; c++) {
						if(_curBG->baseMutable[c] == 'Y'&& fabs( _curBG->basemax[c] - _curBG->basemin[c])>1e-6 &&
							(_curBG->base[c] < _curBG->basemin[c] || _curBG->base[c] > _curBG->basemax[c])) {
							withinConstraints = false;
							continue;
						}
						if(_curBG->type[c] != BG_LINEAR && _curBG->centerMutable[c] == 'Y'&& fabs( _curBG->centermax[c] - _curBG->centermin[c])>1e-6 &&
							(_curBG->center[c] < _curBG->centermin[c] || _curBG->center[c] > _curBG->centermax[c])) {
							withinConstraints = false;
							continue;
						}
						if(_curBG->decayMutable[c] == 'Y'&& fabs( _curBG->decmin[c] - _curBG->decmax[c])>1e-6 &&
							(_curBG->decay[c] < _curBG->decmin[c] || _curBG->decay[c] > _curBG->decmax[c])) {
							withinConstraints = false;
							continue;
						}
					}
				
					if(withinConstraints) {
						if(liveFittingToolStripMenuItem->Checked)
							success = FitBackgroundU(wgtFit->graph->x[0], wgtFit->graph->y[0], _bg->y, 
													 my, *_mask, _curBG, *BGparamErrors, *BGmodelErrors, &UpdateGeneratedGraph,
													 _pShouldStop, &ReportProgressDummy);
						else
							success = FitBackground(wgtFit->graph->x[0], wgtFit->graph->y[0], _bg->y, 
													my, *_mask, _curBG, *BGparamErrors, *BGmodelErrors);
					}
				}

				if(_fitToBaseline)
					my = _bg->y;
				else {
					MultiplyVectors(my, _ff->y, _sf->y);
					AddVectors(my, my, _bg->y);
				}
				my = MachineResolution(wgtFit->graph->x[0], my, GetResolution());
				
				double finalWssr = -1.0, finalRsq = 0.0;
				if(success) {
					finalWssr = WSSR_Masked(wgtFit->graph->y[0], my, *_mask);
					finalRsq = RSquared_Masked(wgtFit->graph->y[0], my, *_mask);
				}

				if(!withinConstraints) {
					MessageBox::Show("There are parameters that are not within their absolute constraints", "ERROR", MessageBoxButtons::OK,
										 MessageBoxIcon::Error);
					e->Cancel = true;
				} else if(!success) {
					if(!*_pShouldStop)
						MessageBox::Show("Error while fitting", "ERROR", MessageBoxButtons::OK,
										 MessageBoxIcon::Error);
					e->Cancel = true;
				} else if(!*_pShouldStop && 
						  lastChisq > -0.5 && finalWssr > lastChisq && finalRsq < lastRsq &&
						  !(_bMultCalc && !_bLastFitMult)) { // Don't keep on popping up during iterative fitting
					MessageBox::Show("No better fit has been found. Consider pressing undo.", "Best Fit", 
									 MessageBoxButtons::OK, MessageBoxIcon::Warning);

					// The following line undoes the fitting results. Sometimes we don't want that so it's commented
					//e->Cancel = true;
				}

				int others = 0;
				for(int z = 0; z < (int)graphType->size(); z++)
					if(graphType->at(z) != GRAPH_DATA && graphType->at(z) != GRAPH_MODEL) others++;

				// We already sent my through the machine resolution (~40 lines above)
				if(!e->Cancel && wgtFit && wgtFit->graph && *_pShouldStop != 2)
					wgtFit->graph->Modify(wgtFit->graph->numGraphs - others - 1, wgtFit->graph->x[0], my);
			} 
		} else if(!_bGenerateModel) { //Phases

				// Phases can only fit with WSSR and not logarithmic
				setAccuracySettings(accurateFittingToolStripMenuItem->Checked, 
									accurateDerivativeToolStripMenuItem->Checked, 
									true,false);

			
				std::vector <double> locs; 
				std::vector <std::string> indices_locs;
				locs = *_ph;
				std::vector<double> my(listView_PeakPosition->Items->Count, 1.0);
				
				//TODO:: create a new Chisq and Rsq
			
				
				// Calculate last chi squared
				std::vector <double> genPhase = GenPhases(*phaseSelected, _curPhases, indices_locs);
				
				double lastChisq = WSSR(locs, genPhase), lastRsq = RSquared(locs, genPhase);
				
				bool success;

				// Check constraints
				if(_curPhases->aM == 'Y' && (_curPhases->a > _curPhases->amax || _curPhases->a < _curPhases->amin))
					withinConstraints = false;
				if(_curPhases->alphaM == 'Y' && (_curPhases->alpha > _curPhases->alphamax || _curPhases->alpha < _curPhases->alphamin))
					withinConstraints = false;
				if(_curPhases->bM == 'Y' && (_curPhases->b > _curPhases->bmax || _curPhases->b < _curPhases->bmin))
					withinConstraints = false;
				if(_curPhases->betaM == 'Y' && (_curPhases->beta > _curPhases->betamax || _curPhases->beta < _curPhases->betamin))
					withinConstraints = false;
				if(_curPhases->cM == 'Y' && (_curPhases->c > _curPhases->cmax || _curPhases->c < _curPhases->cmin))
					withinConstraints = false;
				if(_curPhases->gammaM == 'Y' && (_curPhases->gamma > _curPhases->gammamax || _curPhases->gamma < _curPhases->gammamin))
					withinConstraints = false;


				// Fitting Phases

				if(withinConstraints) {
					if(liveFittingToolStripMenuItem->Checked)
						success = FitPhasesU(*phaseSelected, locs, _curPhases, *PhaseparamErrors, indices_locs,
													  _pShouldStop, &ReportProgressDummy);
					else
						success = FitPhases(*phaseSelected, locs, _curPhases, *PhaseparamErrors, indices_locs);
				}

				double finalWssr = -1.0, finalRsq = 0.0;
				std::vector <std::string> indicesGen;

				genPhase = GenPhases(*phaseSelected, _curPhases, indicesGen);
				
				*_generatedPhaseLocs = genPhase;
				*indicesLoc = indicesGen;

				if(success) {
					finalWssr = WSSR(locs, genPhase);
					finalRsq = RSquared(locs, genPhase);
				}

				if(!withinConstraints) {
					MessageBox::Show("There are parameters that are not within their absolute constraints", "ERROR", MessageBoxButtons::OK,
										 MessageBoxIcon::Error);
					e->Cancel = true;
				} else if(!success) {
					if(!*_pShouldStop)
						MessageBox::Show("Error while fitting", "ERROR", MessageBoxButtons::OK,
										 MessageBoxIcon::Error);
					e->Cancel = true;
				} else if(!*_pShouldStop && 
						  lastChisq > -0.5 && finalWssr > lastChisq && finalRsq < lastRsq &&
						  !(_bMultCalc && !_bLastFitMult)) { // Don't keep on popping up during iterative fitting
					MessageBox::Show("No better fit has been found. Consider pressing undo.", "Best Fit", 
									 MessageBoxButtons::OK, MessageBoxIcon::Warning);

				}
		} else { // _bGenerateModel && phases
			;
			//calculateRecipVectors();
			//PhasesCompleted();
		} // end if phases

		ReportDone(e->Cancel);
	}


	void FormFactor::save_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring filename;
		std::string type = _model->GetName();

		if(_bGenerateModel) {
			filename = L".\\XModelFitter.ini";
		} else {
			std::wstring res;

			res = clrToWstring(CLRDirectory(_dataFile)) + 
				clrToWstring(CLRBasename(_dataFile)) + L"-params.ini";

			filename = res;
		}

		if (sender == this->saveParametersAsToolStripMenuItem) {
			sfd->FileName = "";
			sfd->Title = "Save parameter file as... ";
			sfd->Filter = "Parameter Files (*.ini)|*.ini|All Files (*.*)|*.*";
			if(sfd->ShowDialog() == 
				System::Windows::Forms::DialogResult::Cancel)
				return;
			clrToString(sfd->FileName, filename);
		}
		
		if(sender == exportSigModBLToolStripMenuItem)
			filename = clrToWstring(CLRDirectory(_dataFile)) + clrToWstring(CLRBasename(_dataFile)) + L"-auto-params.ini";



		paramStruct p = *_curPar;

		iniFile = NewIniFile();

		// Write parameters		
		WriteParameters(filename, type, &p, iniFile);
		
		// Write ED Profile configuration
		SetIniInt(filename, type, "EDProfileShape", iniFile, _model->GetEDProfile().shape);
		if(adaptiveToolStripMenuItem->Checked)
			SetIniInt(filename, type, "EDProfileResolution", iniFile, -Int32::Parse(edpResolution->Text));
		else
			SetIniInt(filename, type, "EDProfileResolution", iniFile, Int32::Parse(edpResolution->Text));

		// Write Polydispersity configuration
		SetIniInt(filename, type, "PDFunc", iniFile, GetPDFunc());
		SetIniInt(filename, type, "PDResolution", iniFile, GetPDResolution());

		// Write quadrature configuration
		if(integrationToolStripMenuItem->Visible) {
			SetIniString(filename, type, "QuadratureRes", iniFile, clrToString(toolStripTextBox1->Text));
			if(monteCarloToolStripMenuItem->Checked)
				SetIniString(filename, type, "QuadratureMethod", iniFile, "1");
			else if(simpsonsRuleToolStripMenuItem->Checked)
				SetIniString(filename, type, "QuadratureMethod", iniFile, "2");
			else // if(gaussLegendreToolStripMenuItem->Checked)
				SetIniString(filename, type, "QuadratureMethod", iniFile, "0");
		}

		if(_bGenerateModel) {
			SetIniString(filename, type, "GenRangeStart", iniFile, clrToString(startGen->Text));
			SetIniString(filename, type, "GenRangeEnd", iniFile, clrToString(endGen->Text));
			SetIniString(filename, type, "GenResolution", iniFile, clrToString(toolStripTextBox2->Text));
		}

		// Saving Structure Factor
		peakStruct peaks;
		GetPeaksFromListView(&peaks);
		WritePeaks(filename, type, &peaks, iniFile);
		if(_model->HasSpecializedSF()) {
			graphTable caille;
			cailleParamStruct cailleP;
			if(cailleParamListView->Items->Count > 0) {
				GetCailleFromGUI(&caille, &cailleP);
				WriteCaille(filename, type, &caille, &cailleP, iniFile);
			}
		}
		// Save phases
		phaseStruct ps;
		GetPhasesFromListView(&ps);
		WritePhases(filename, type, &ps, order->SelectedIndex, iniFile);

		// Saving background
		bgStruct BGs;
		GetBGFromGUI(&BGs);
		WriteBG(filename, type, &BGs, iniFile);

		// Saving General Settings
		SetIniChar(filename, "Settings", "LogScale", iniFile, (logScale->Checked ? 'Y' : 'N'));
		SetIniChar(filename, "Settings", "LiveRefresh", iniFile, (liveRefreshToolStripMenuItem->Checked ? 'Y' : 'N'));
		SetIniChar(filename, "Settings", "LiveFit", iniFile, (liveFittingToolStripMenuItem->Checked ? 'Y' : 'N'));
		SetIniChar(filename, "Settings", "sigma", iniFile, (sigmaToolStripMenuItem->Checked ? 'Y' : 'N'));
		SetIniChar(filename, "Settings", "Angstrom", iniFile, (a1nm1ToolStripMenuItem->Checked ? 'Y' : 'N'));
		
		CloseIniFile(iniFile);
		iniFile = NULL;

		save->Enabled = false;
		_bSaved = true;
	}

	void FormFactor::plotElectronDensityProfileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		struct graphLine graphs[3];
		paramStruct p = *_curPar;

		generateEDProfile(p.params, graphs,_model->GetEDProfile());
		std::pair<double, double> in = calcEDIntegral(p.params[0], p.params[0]);
		
		ResultsWindow rw(graphs, 3, "Positive one-sided area: " + Double(in.first).ToString("#.######") + 
			", Negative area: " + Double(in.second).ToString("#.######"));

		rw.ShowDialog();
	}

	void FormFactor::importParametersToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		ofd->Title = "Choose a parameter file to import";
		ofd->Filter = "Parameter Files (*.ini)|*.ini|All Files (*.*)|*.*";
		if(ofd->ShowDialog() == System::Windows::Forms::DialogResult::Cancel)
			return;

		paramStruct par(_model);

		iniFile = NewIniFile();

		ReadParameters(clrToWstring(ofd->FileName), _model->GetName(), &par, iniFile);

		if(par.params.size() == 0 || par.params[0].size() == 0) {
			for(int i = 0; i < _model->GetNumRelatedModels(); i++) {
				ReadParameters(clrToWstring(ofd->FileName), _model->GetRelatedModelName(i), &par, iniFile);
				if(!(par.params.size() == 0 || par.params[0].size() == 0)) {
					// Change model to first available one
					FFModel *ffm = dynamic_cast<FFModel *>(_model->CreateRelatedModel(i));

					handleModelChange(*ffm);
					break;
				}
			}
		}

		if(par.params.size() == 0 || par.params[0].size() == 0) {
			MessageBox::Show("No model of this sort in this parameter file",
							 "No Such Model",
							 MessageBoxButtons::OK,
							 MessageBoxIcon::Error);
			CloseIniFile(iniFile);
			iniFile = NULL;
			return;
		}

		System::String ^filename;
		if(_bGenerateModel) {
			filename = ".\\XModelFitter.ini";
		} else {
			filename = CLRDirectory(_dataFile) + 
				CLRBasename(_dataFile) + "-params.ini";
	 	}

	//	CLRBasename(ofd->FileName)->Equals(CLRBasename(filename)
		
		if(!ofd->FileName->Equals(filename))
			if(!_bGenerateModel || !CLRBasename(ofd->FileName)->Equals(CLRBasename(filename)))
				System::IO::File::Copy(ofd->FileName, filename, true);

		// TODO: Think of ED profile type here
		
		// Remove Layers
		while(listViewFF->Items->Count > 1)
			listViewFF->Items->RemoveAt(1);

		//TEST TODO DEBUG ETC
		listViewFF->Items->Clear();

		// Remove Peaks
		listView_peaks->Items->Clear();

		// Remove BG Functions
		BGListview->Items->Clear();

		// Remove Extra Parameters
		listView_Extraparams->Items->Clear();
		paramBox->Items->Clear();

		// Remove Phase listView
		listView_phases->Items->Clear();

		// Remove cailleParam LV
		cailleParamListView->Items->Clear();

		delete wgtPreview;
		wgtPreview = nullptr;
		FormFactor_Load(sender, e);

		CloseIniFile(iniFile);
		iniFile = NULL;

		paramStruct p = *_curPar;

		UpdateGraph(true);	//Avi: Already called in FF_Load, but not "true"
	}

	void FormFactor::importBaselineToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring fname;
		if(!openDataFile(ofd, "Choose a baseline file to import", fname, true))
			return;

		std::vector <double> x, y, ffy;
		// Copy externalfile to <data>-baseline.out
		System::String ^filename, ^dir = CLRDirectory(_dataFile);

		filename = dir + CLRBasename(_dataFile) + "-baseline.out";

		if(!filename->Equals(ofd->FileName))
			System::IO::File::Copy(ofd->FileName, filename, true);

		GenerateBGLinesandFormFactor(clrToWstring(_dataFile).c_str(), clrToWstring(filename).c_str(), x, y, ffy,a1nm1ToolStripMenuItem->Checked);
		label1->Visible = false;
		exportSignalToolStripMenuItem->Enabled = true;
		exportModelToolStripMenuItem1->Enabled = true;
		plotFittingResultsToolStripMenuItem->Enabled = true;
		exportGraphToolStripMenuItem->Enabled = true;
		exportFormFactorToolStripMenuItem->Enabled = true;
		exportSigModBLToolStripMenuItem->Enabled = true;
		exportDecomposedToolStripMenuItem->Enabled = true;
		exportStructureFactorToolStripMenuItem->Enabled = true;

		// Don't ask if we want to use the existing baseline
		ExtractBaseline::DontAsk(); 

 	    InitializeGraph(false, x, y, ffy);
		
		ffUseCheckBox->Enabled = true;
		sfUseCheckBox->Enabled = true;
		bgUseCheckBox->Enabled = true;
	}


	void FormFactor::RenderPreviewScene() {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
		glEnable(GL_DEPTH_TEST);							

		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
		glLoadIdentity();

		glTranslatef(0.0f,0.0f,-5.0f);
		
		// Kept as reference for when we implement the DrawGL functions in Models
		/*
		switch(GetModelType()) {
			case MODEL_CYLINDROID:
				// TODO: if a > b scale somehow on x < 1.0, else scale otherwise (y < 1.0, x = 1.0)
				glScaled(1.0, 0.7, 1.0);
				DrawGLNLayeredHC(quadric, radii, 1.0f, edies, r.size(), 1.8f);			
				break;

			case MODEL_SPHERE: // Sphere/Hollow Sphere
				DrawGLNLHollowSphere(quadric, radii, edies, r.size(), 1.6f);
				break;
		
			case MODEL_SLAB: // Membrane/Slab
				DrawGLNLayeredSlabs(radii, edies, 2.0f, r.size());
				break;

			case MODEL_ASLAB:
				DrawGLNLayeredAsymSlabs(radii, edies, 2.0f, r.size());
				break;

			case MODEL_DELIX:	//Tal, you can add the microtubule ;)
				break;

			case MODEL_RECT:
				DrawGLRectangular(edies[0]);
				break;
		}
		Tal fix it I don't know what this does.
		I think it is enough
		*/

			_model->DrawOpenGLPreview(*_curPar);

		glDisable(GL_SMOOTH);

		glPopMatrix();
		
	}

	void FormFactor::exportDataFileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
			std::wstring file;
			// As per Moshe's request.  Maybe we should do something similar for all of the exports
			if(!_bGenerateModel) {
			sfd->FileName = CLRBasename(_dataFile);
			sfd->FileName = sfd->FileName->Substring(1, sfd->FileName->Length - 1);
			}
			sfd->Filter = L"Output Files (*.out)|*.out|Data Files (*.dat, *.chi)|*.dat;*.chi|All files|*.*";
			if(sender == this->exportSignalToolStripMenuItem || sender == this->baseline) sfd->FileName += "-sig";
			else if(sender == this->exportBackgroundToolStripMenuItem) sfd->FileName += "_BG";
			else if(sender == this->exportModelToolStripMenuItem1) sfd->FileName += "_model";
			else sfd->FileName = "";

			if(sender == this->exportSigModBLToolStripMenuItem) {
				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-sigA.dat", file);
				WriteDataFile(file.c_str(), _data->x, _data->y);

				clrToString(sfd->FileName + "_BGA.dat", file);
				if(fitToBaseline->Enabled && fitToBaseline->Text->StartsWith("Fit Back")) // Make sure there is a baseline
					WriteDataFile(file.c_str(), _baseline->x, _baseline->y);

				clrToString(sfd->FileName + "_modelA.dat", file);
				WriteDataFile(file.c_str(), wgtFit->graph->x[1], wgtFit->graph->y[1]);

				save_Click(sender, e);
				return;
			}

			if(sender == exportDecomposedToolStripMenuItem) {
				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-FF.dat", file);
				WriteDataFile(file.c_str(), _ff->x, ffUseCheckBox->Checked ? _ff->y : _ff->tmpY);

				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-SF.dat", file);
				WriteDataFile(file.c_str(), _sf->x, sfUseCheckBox->Checked ? _sf->y : _sf->tmpY);

				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-BG.dat", file);
				WriteDataFile(file.c_str(), _bg->x, bgUseCheckBox->Checked ? _bg->y : _bg->tmpY);

				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-model.dat", file);
				std::vector<double> model;
				MultiplyVectors(model, ffUseCheckBox->Checked ? _ff->y : _ff->tmpY, sfUseCheckBox->Checked ? _sf->y : _sf->tmpY);
				AddVectors(model, model, bgUseCheckBox->Checked ? _bg->y : _bg->tmpY);
				WriteDataFile(file.c_str(), _ff->x, MachineResolution(_ff->x, model, GetResolution()));

				sfd->FileName = CLRDirectory(_dataFile) + CLRBasename(_dataFile);
				clrToString(sfd->FileName + "-signal.dat", file);
				WriteDataFile(file.c_str(), wgtFit->graph->x[0], wgtFit->graph->y[0]);

				return;
			}

			sfd->Title = "Choose a signal output file";
			if(sfd->ShowDialog() == 
				System::Windows::Forms::DialogResult::Cancel)
				return;

			clrToString(sfd->FileName, file);

			if(sender == this->exportFormFactorToolStripMenuItem) {
				//addValueToVector(_ff->y, -Double::Parse(listView_Extraparams->Items[1]->SubItems[ELV_VALUE]->Text));
				WriteDataFile(file.c_str(), _ff->x, ffUseCheckBox->Checked ? _ff->y : _ff->tmpY);
				//addValueToVector(_ff->y, Double::Parse(listView_Extraparams->Items[1]->SubItems[ELV_VALUE]->Text));
				return;
			}

			if(sender == this->exportStructureFactorToolStripMenuItem) {
				WriteDataFile(file.c_str(), _sf->x, sfUseCheckBox->Checked ? _sf->y : _sf->tmpY);
				return;
			}

			if(sender == this->exportBackgroundToolStripMenuItem) {
				WriteDataFile(file.c_str(), _bg->x, (bgUseCheckBox->Checked) ? _bg->y : _bg->tmpY);
				return;
			}

			WriteDataFile(file.c_str(), wgtFit->graph->x[0], wgtFit->graph->y[0]);
		 }

	void FormFactor::exportModelToolStripMenuItem1_Click(System::Object^  sender, System::EventArgs^  e) {
			std::wstring file;
			std::vector<double> err(_ff->x.size());
			sfd->FileName = "";
			sfd->Title = "Choose a model output file";
			sfd->Filter = L"Output Files (*.out)|*.out|Data Files (*.dat, *.chi)|*.dat;*.chi|All files|*.*";
			if(sfd->ShowDialog() == 
				System::Windows::Forms::DialogResult::Cancel)
				return;

			clrToString(sfd->FileName, file);
			int pos = (_bGenerateModel ? 0 : 1);
			
			// Calculate Error Vector
			for(int i = 0; i < (int)err.size(); i++) {
				double dff = ((int)FFmodelErrors->size() > i) ? FFmodelErrors->at(i) : 0.0;
				double dsf = ((int)SFmodelErrors->size() > i) ? SFmodelErrors->at(i) : 0.0;
				double dbg = ((int)BGmodelErrors->size() > i) ? BGmodelErrors->at(i) : 0.0;

				err.at(i) = sqrt(sq(dff*_sf->y.at(i)) + sq(dsf*_ff->y.at(i))  + sq(dbg) );
			}

			if(_peakPicker){
				vector<double> res(_ff->y.size(), 0.0);
				MultiplyVectors(res, _ff->y, _sf->y);
				AddVectors(res, res, _bg->y);
				Write3ColDataFile(file.c_str(), wgtFit->graph->x[pos], MachineResolution(_ff->x, res, GetResolution()), err);
			} else
				Write3ColDataFile(file.c_str(), wgtFit->graph->x[pos], MachineResolution(_ff->x, wgtFit->graph->y[pos], GetResolution()), err);
		 }

	void FormFactor::changeData_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring fname;
		if(!openDataFile(ofd, "Choose a data file", fname, false))
			return;

		if(_fitToBaseline) fitToBaseline_Click(sender, e);
		if(a1nm1ToolStripMenuItem->Checked)
			a1nm1ToolStripMenuItem->Checked = false;
		_dataFile = ofd->FileName;

		if(wgtFit && wgtFit->graph)
			delete wgtFit->graph;
		delete wgtFit;
		wgtFit = nullptr;
		graphType->clear();

		this->Text = this->Text->Substring(0, this->Text->IndexOf('['));

		this->Text += "[" + CLRBasename(_dataFile) + "]";

		calculate->Enabled = false;
		fitphase->Enabled = false;
		Caille_button->Enabled = false;
		smooth->Enabled = false;
		exportBackgroundToolStripMenuItem->Enabled = false;
		label1->Visible = true;
		tabControl1->SelectTab("BGTab");

		SFParameterUpdateHandler();
		BGParameterUpdateHandler();
		FFParameterUpdateHandler();
	}

	void FormFactor::FormFactor_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		// Save parameters
		if((e->KeyCode == Keys::S) && e->Control && save->Enabled) {
			if(e->Shift)
				this->save_Click(saveParametersAsToolStripMenuItem, gcnew EventArgs());
			else
				this->save_Click(saveParametersToolStripMenuItem, gcnew EventArgs());
			
			e->Handled = true;
		}

	}

	void FormFactor::listViewFF_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		ListView ^lv = (ListView ^)sender;
		//If the user selected indices and pressed delete/backspace, remove indicies
		if(e->KeyCode == Keys::Delete || e->KeyCode == Keys::Back) {
			while(lv->SelectedItems->Count > 0) {
				// Check to make sure the layer is removable
				for(int i = 0; i < listViewFF->SelectedIndices->Count; i++)
					if(listViewFF->SelectedIndices[i] < _model->GetMinLayers())
						return;
				removeLayer_Click(this, e);
			}

			save->Enabled = true;
			undo->Enabled = false;
			e->Handled = true;
		}

		// Copy selected listViewItems
		if((e->KeyCode == Keys::C) && (System::Windows::Forms::Control::ModifierKeys == Keys::Control)) {
			if(lv->SelectedItems->Count > 0)
				_copiedIndicesFF->clear();

			for(int i = 0; i < lv->SelectedItems->Count; i++)
				_copiedIndicesFF->push_back(lv->SelectedIndices[i]);
			e->Handled = true;
		}

		// Paste selected items (default is linked to the original items
		if((e->KeyCode == Keys::V) && (System::Windows::Forms::Control::ModifierKeys == Keys::Control)) {
			int nlp = _model->GetNumLayerParams();
			for(int i = 0; (i < (int)_copiedIndicesFF->size()) && 
					(listViewFF->Items->Count < (_model->GetMaxLayers() > 0 ? _model->GetMaxLayers() : listViewFF->Items->Count + 50));
					i++) {
				// Add the item
				listViewFF->Items->Add((ListViewItem^)(listViewFF->Items[_copiedIndicesFF->at(i)]->Clone()));
				int cnt = listViewFF->Items->Count - 1;
				int ind = listViewFF->Items[cnt]->SubItems[LV_NAME]->Text->LastIndexOf(" ");
				System::String ^str = listViewFF->Items[cnt]->SubItems[LV_NAME]->Text->Remove(ind + 1);
				listViewFF->Items[cnt]->SubItems[LV_NAME]->Text = str->Insert(ind + 1, cnt.ToString());

				// Link all the parameters to the _copiedIndicesFF[i]th layer
				for(int n = 0; n < nlp; n++) {
					listViewFF->Items[cnt]->SubItems[LV_CONSLINK(n, nlp)]->Text = _copiedIndicesFF->at(i).ToString();
					listViewFF->Items[cnt]->SubItems[LV_MUTABLE(n)]->Text = "L";
				}
			}

			FFParameterUpdateHandler();
			save->Enabled = true;
			undo->Enabled = false;
			e->Handled = true;
		}
	}

	void FormFactor::EDU() {
		double solvent = clrToDouble(listViewFF->Items[0]->SubItems[LV_VALUE(1)]->Text);
		double area = 0.0;

		// Computing total ED profile area
		if(_model->GetEDProfile().func) {
			_model->GetEDProfile().func->MakeSteps(0.0, 0.0, 0.0, area);
			area -= (_model->GetEDProfile().func->GetUpperLimit() - 
			         _model->GetEDProfile().func->GetLowerLimit()) * solvent;
		} else { 
			// Discrete layers, compute manually			
			for(int i = 0; i < listViewFF->Items->Count; i++) {
				double layerWidth, layerHeight;
				layerWidth  = clrToDouble(listViewFF->Items[i]->SubItems[LV_VALUE(0)]->Text);
				layerHeight = clrToDouble(listViewFF->Items[i]->SubItems[LV_VALUE(1)]->Text);

				area += layerWidth * (layerHeight - solvent);
			}				
		}
		if(_model->GetEDProfile().type == SYMMETRIC)
			area *= 2.0;

		AreaText->Text = area.ToString("0.000");		
	}

	void FormFactor::plotGenerateResults() {
		if(!wgtFit || !wgtFit->graph || wgtFit->graph->numGraphs == 0)
			return;

		struct graphLine graphs[1];
		graphs[0].color = RGB(255, 0, 0);

		graphs[0].legendKey = "Model";

		graphs[0].x = wgtFit->graph->x[0];
		graphs[0].y = wgtFit->graph->y[0];

		ResultsWindow rw(graphs, 1);

		rw.ShowDialog();
	}

	void FormFactor::plotFittingResults() {
		if(!wgtFit || !wgtFit->graph || wgtFit->graph->numGraphs == 0)
			return;

		struct graphLine graphs[2];
		graphs[0].color = RGB(255, 0, 0);
		graphs[1].color = RGB(0, 0, 255);

		graphs[0].legendKey = "Signal";
		graphs[1].legendKey = "Model";

		graphs[0].x = wgtFit->graph->x[0];
		graphs[0].y = wgtFit->graph->y[0];

		graphs[1].x = wgtFit->graph->x[1];
		graphs[1].y = wgtFit->graph->y[1];
		
		ResultsWindow rw(graphs, 2);

		rw.ShowDialog();
	}

	void FormFactor::automaticPeakFinderButton_Click(System::Object^  sender, System::EventArgs^  e) {
			 /**
			  * Collect the two thresholds from the fields
			  *	Send the signal (data), FormFactor and StructureFactor vectors along with
			  *	the thresholds to an automatic FindPeaks function.
			 **/
			 if(!wgtFit || !wgtFit->graph) return;
			 threshold1 = clrToDouble(thresholdBox1->Text);
			 threshold2 = clrToDouble(thresholdBox2->Text);
			 
			 graphTable *signal;
			 signal = new graphTable;
			 signal->x = wgtFit->graph->x[0];
			 signal->y = wgtFit->graph->y[0];
			 AutoFindPeaks();
			 delete signal;
		 }

	void FormFactor::exportGraphToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(_bGenerateModel)
			plotGenerateResults();
		else
			plotFittingResults();
	}

	void FormFactor::minimumSignalTextbox_Leave(System::Object^  sender, System::EventArgs^  e) {
		double prevMin = GetMinimumSig(), newMin;
		newMin = clrToDouble(minimumSignalTextbox->Text);
		
		if((newMin == 0.0 && !minimumSignalTextbox->Text->StartsWith("0"))	//Starts with text
			|| fabs(newMin - prevMin) < 1e-12) {	// The number hasn't changed
			minimumSignalTextbox->Text = prevMin.ToString("0.000000");
			return;
		}
		
		if(!wgtFit || _fitToBaseline) {
			SetMinimumSig(newMin);
			minimumSignalTextbox->Text = newMin.ToString();
			return;
		}

		// As we don't change graph->x/y directly (it's computed inside 
		// graphtoolkit), we have to call the Modify method
		std::vector<double> newy, newx = wgtFit->graph->x[0];
		for (int i = 0; i < (int)wgtFit->graph->y[0].size(); i++)
			newy.push_back(wgtFit->graph->y[0].at(i) - prevMin + newMin);

		wgtFit->graph->Modify(0, newx, newy);

		SetMinimumSig(newMin);
		minimumSignalTextbox->Text = newMin.ToString("0.000000");
		
		if(liveRefreshToolStripMenuItem->Checked) {
			RedrawGraph();
			wgtFit->graph->FitToAllGraphs();
		}
	}

	void FormFactor::minimumSignalTextbox_Enter(System::Object^  sender, System::EventArgs^  e) {
		minimumSignalTextbox->Text = GetMinimumSig().ToString("0.000000");
	}

	void FormFactor::addValueToVector(std::vector<double> &vec, double val) {
		for(int i = 0; i < (int)vec.size(); i++)
			vec.at(i) += val;
	}

	void FormFactor::accurateDerivativeToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		// TODO: WTF?!
		//_bConsEcc = this->accurateDerivativeToolStripMenuItem->Checked;
	}

	// Helper function for a1nm1ToolStripMenuItem_CheckedChanged
	void MultiplyGraphX(Graph ^graph, double factor) {
		for(int gr = 0; gr < graph->numGraphs; gr++) {
			std::vector<double> newx = graph->x[gr], newy = graph->y[gr];
			for(int i = 0; i < (int)graph->x[gr].size(); i++)
				newx[i] *= factor;

			graph->Modify(gr, newx, newy);
		}
	}

	/**
	 * Converts the q units from inverse angstroms to inverse nanometers and vice versa
	**/
	void FormFactor::a1nm1ToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(sender != a1nm1ToolStripMenuItem) return;

		// TODO: Need to add something to deal with the case where a FF is loaded from a file

		if(a1nm1ToolStripMenuItem->Checked) {
			multiplyVectorByValue(_ff->x, 10.0);
			multiplyVectorByValue(_sf->x, 10.0);
			multiplyVectorByValue(_bg->x, 10.0);
			SetResolution(GetResolution() * 10.0);

			if(wgtFit && wgtFit->graph)
				MultiplyGraphX(wgtFit->graph, 10.0);
		} else {
			multiplyVectorByValue(_ff->x, 0.1);
			multiplyVectorByValue(_sf->x, 0.1);
			multiplyVectorByValue(_bg->x, 0.1);
			SetResolution(GetResolution() * 0.1);

			if(wgtFit && wgtFit->graph)
				MultiplyGraphX(wgtFit->graph, 0.1);
		}

		if(liveRefreshToolStripMenuItem->Checked)
			UpdateGraph(true);
	
		if(wgtFit && wgtFit->graph) {
			//Force a redraw of the background
			wgtFit->graph->ToggleGrid();
			wgtFit->graph->ToggleGrid();
			wgtFit->Invalidate();
		}
	}

	void FormFactor::AddParamLayer() {
		std::vector<Parameter> layer (_model->GetNumLayerParams());
		int nlp = _model->GetNumLayerParams();
		for(int i = 0; i < _model->GetNumLayerParams(); i++)
			layer[i].value = _model->GetDefaultParamValue(i, 
									listViewFF->Items->Count);
		
		
		AddParamLayer(layer);
	}

	void FormFactor::smooth_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!wgtFit || !wgtFit->graph)
			return;

		// Write temporary file with current signal
		String ^tempFile = System::IO::Path::GetTempFileName();
		std::wstring tempStr = clrToWstring(tempFile);

		WriteDataFile(tempStr.c_str(), wgtFit->graph->x[0],
			wgtFit->graph->y[0]);

		// Show smoothing dialog
		SmoothWindow sw (tempStr.c_str(), true);
		sw.ShowDialog();

		// Read back the results
		std::vector<double> resx, resy;
		ReadDataFile(tempStr.c_str(), resx, resy);

		// Modify the graphs
		wgtFit->graph->Modify(0, resx, resy);

		wgtFit->Invalidate();

		// Delete the temporary file
		System::IO::File::Delete(tempFile);
	}

	void FormFactor::WriteCSVParamsFile() {
		// Get the name/dir of the file (+ dialog)
		// Open a file for writing
		// Write each tab with its parameters and titles (+chisqr/Rsqr)
		// Close file

		// Get the name/dir of the file (+ dialog)
		std::wstring file;
		if(!_bGenerateModel) {
			sfd->FileName = CLRBasename(_dataFile)->Remove(0, 1);
			sfd->FileName += "_parameters";
		}

		sfd->Filter = "TSV Files (*.tsv)|*.tsv|All Files (*.*)|*.*";
		sfd->Title = "Choose a filename";
		if(sfd->ShowDialog() == 
			System::Windows::Forms::DialogResult::Cancel)
			return;
		
		clrToString(sfd->FileName, file);


		// Open a file for writing
		FILE *fp;

		if ((fp = _wfopen(file.c_str(), L"w, ccs=UTF-8")) == NULL) {
			fprintf(stderr, "Error opening file %s for writing\n",
							file);
			
			MessageBox::Show("Please make sure that the file is not open.", "Error opening file for writing", MessageBoxButtons::OK,
									 MessageBoxIcon::Error);
			return;
		}

		// Write each tab with its parameters and titles (+chisqr/Rsqr)
		// Collect titles (header field names)
		System::Collections::Generic::List<ListView^>^ LV = gcnew System::Collections::Generic::List<ListView^>();
		LV->Add(listViewFF);
		LV->Add(listView_Extraparams);
		LV->Add(listView_peaks);
		LV->Add(listView_PeakPosition);
		LV->Add(listView_phases);
		LV->Add(BGListview);
		LV->Add(caillePeaksListView);
		LV->Add(cailleParamListView);

		for(int cnt = 0; cnt < LV->Count; cnt++) {
			if(LV[cnt]->Items->Count > 0) {
				for(int i = 0; i < LV[cnt]->Columns->Count; i++) {
					if(!(LV[cnt]->Columns[i]->Text->Equals("M") || LV[cnt]->Columns[i]->Text->Length == 0)) {
						fwprintf(fp, L"%s\t ", clrToWstring(LV[cnt]->Columns[i]->Text).c_str());
					}
				}
				fwprintf(fp, L"\t ");
			}
		}

		fwprintf(fp, L"\n");
		
		// Fill in the data for the tables
		int maxRows = 0;
		maxRows = max(listViewFF->Items->Count, max(listView_Extraparams->Items->Count, 
			max(listView_peaks->Items->Count, max(listView_PeakPosition->Items->Count,
			max(listView_phases->Items->Count, listView_phases->Items->Count)))));

		for(int row = 0; row < maxRows; row++) {
			for(int cnt = 0; cnt < LV->Count; cnt++) {
				if(LV[cnt]->Items->Count > 0) {
					for(int itm = 0; itm < LV[cnt]->Columns->Count; itm++) {
						if(row < LV[cnt]->Items->Count) {
							if(!(LV[cnt]->Columns[itm]->Text->Equals("M") || LV[cnt]->Columns[itm]->Text->Length == 0)) {
							//make sure not to write muts, and stuff with no title... (mn, mx etc)
								fwprintf(fp, L"%s", clrToWstring(LV[cnt]->Items[row]->SubItems[itm]->Text).c_str());
							}
						}
						if(!(LV[cnt]->Columns[itm]->Text->Equals("M") || LV[cnt]->Columns[itm]->Text->Length == 0))
							fwprintf(fp, L"\t ");
					}
					if(!(cnt == LV->Count - 1))
						fwprintf(fp, L"\t ");
				}
			}
			fwprintf(fp, L"\n");
		}


		fwprintf(fp, L"\n\n\n%s \t %s", clrToWstring(UnicodeChars::chisqr).c_str(), clrToWstring(wssr->Text).c_str());
		if(phaseErrorTextBox->Text != "-")
			fwprintf(fp, L"\t\t\tPhase Error \t %s", clrToWstring(phaseErrorTextBox->Text).c_str());
		fwprintf(fp, L"\n%s \t %s", clrToWstring(UnicodeChars::rsqr).c_str(), clrToWstring(rsquared->Text).c_str());
		if(Volume->Text != "-")
			fwprintf(fp, L"\t\t\tUnit cell volume \t %s\n", clrToWstring(Volume->Text).c_str());

		// Close file
		fclose(fp);

	}

	void FormFactor::exportAllParametersAsCSVFileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		WriteCSVParamsFile();
	}

	void FormFactor::General_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
		if(e->KeyCode == Keys::Enter || e->KeyCode == Keys::Return)
			//Change the focus so that any field that has a new value will be updated
			this->tabControl1->Focus();
		// The program makes a bell sound when this is used, but I can't figure out why...
	}

	void FormFactor::fitRange_Click(System::Object^  sender, System::EventArgs^  e) {
			//SetGaussED(_bGaussian);
			FitRange fr (listViewFF->Items, listViewFF->SelectedIndices[0], 
						 _model->GetNumLayerParams(), _model);

			fr.ShowDialog();

			linkedParameterCheck(listViewFF, listViewFF->SelectedIndices[0]);

			// Enable/Disable appropriate Textboxes
			listViewFF_SelectedIndexChanged(sender, e);

			UItoParameters(_curPar);

	}

	void FormFactor::linkedParameterCheck(System::Windows::Forms::ListView ^lv, int layer) {
		ListViewItem ^lvi = lv->Items[layer];
		int curLayer, finalLayer;
		int paramNum = _model->GetNumLayerParams();

		for(int i = 0; i < paramNum; i++) {
			bool bLink;
			curLayer = layer;
			int col = LV_CONSLINK(i, paramNum);
			bLink = Int32::Parse(lv->Items[curLayer]->SubItems[col]->Text) > -1;
			while(curLayer >= 0) {
				finalLayer = curLayer;
				curLayer = Int32::Parse(lv->Items[curLayer]->SubItems[col]->Text);
				if(curLayer == layer) { // Circular
					// Remove link
					char aa[255] = {0};
					sprintf(aa, "You have made a circular set of linked %s parameters",  
							lv->Columns[2 * i + 1]->Text);
					System::String ^text = gcnew System::String(aa);
					MessageBox::Show(text, "ERROR", MessageBoxButtons::OK,
									 MessageBoxIcon::Error);

					lvi->SubItems[col]->Text = "-1";
					curLayer = -2;
					bLink = false;
				}
			}
			if(bLink && finalLayer > -1 /*&& Int32::Parse(lvi->SubItems[col]->Text) != -1*/) {
				// change mutability to 'L'
				// change value to match linked item
				// change the constraints to match the linked item
				
				lvi->SubItems[LV_VALUE(i)]->Text = lv->Items[finalLayer]->SubItems[LV_VALUE(i)]->Text;
				lvi->SubItems[LV_SIGMA(i,_model->GetNumLayerParams())]->Text = lv->Items[finalLayer]->SubItems[LV_SIGMA(i,_model->GetNumLayerParams())]->Text;
				GroupBoxList[i]->check->Checked = false;

				//Causes a bug: all parameters are linked to another, even if not told to be linked.
				//lvi->SubItems[LV_CONSLINK(i, nlp)]->Text = finalLayer.ToString();

				lvi->SubItems[LV_MUTABLE(i)]->Text = "L";
				if(Int32::Parse(lvi->SubItems[col]->Text) > 0) {
					for( int j = col - 4; j < col; j++)
						lvi->SubItems[j]->Text = lv->Items[/*Int32::Parse(lvi->SubItems[col]->Text)*/ finalLayer]->SubItems[j]->Text;
								
				}
			}
			// If the linking was removed, change the mutability of the item to mutable
			if((lvi->SubItems[LV_MUTABLE(i)]->Text == "L") && (Int32::Parse(lvi->SubItems[col]->Text) == -1))
				lvi->SubItems[LV_MUTABLE(i)]->Text = "Y";
		}	//end for(i < paramNum)
	}

	void FormFactor::linkedParameterChangedCheck(System::Windows::Forms::ListView::ListViewItemCollection ^lv, int layer) {
		ListViewItem ^lvi;
		if(_bGenerateModel) // No links
			return;
		
		int nlp = _model->GetNumLayerParams();

		// Go over other linked items.  If they are linked to the changed value, change their value
		for(int i = 0; i < lv->Count; i++) {
			lvi = lv[i];
			for (int j = 0; j < nlp; j++) {
				int col = LV_CONSLINK(j, nlp);

				System::String^ ter = lvi->SubItems[col]->Text;

				if(Int32::Parse(lvi->SubItems[col]->Text) == layer)	{
					lvi->SubItems[LV_VALUE(j)]->Text = lv[layer]->SubItems[LV_VALUE(j)]->Text;
					linkedParameterChangedCheck(lv, i);
				}
			}

		}
	}

	void FormFactor::addLayer_Click(System::Object^  sender, System::EventArgs^  e) {
			AddParamLayer();
	}

	void FormFactor::removeLayer_Click(System::Object^  sender, System::EventArgs^  e) {
			if(listViewFF->SelectedIndices->Count == 0) return;

			// This function works the following way:
			// 1. Relink all linked layers (or remove links to the removed layer/s)
			// 2. Remove the layer
			// 3. Rename the layers

			int nlp = _model->GetNumLayerParams();

			while(listViewFF->SelectedIndices->Count > 0) {
				int index = listViewFF->SelectedIndices[0];
								
				for(int i = 0; i < listViewFF->Items->Count; i++) {
					// Looping over all layer parameters for linked indices
					for(int j = 0; j < nlp; j++) {
						int linkInd = LV_CONSLINK(j, nlp);

						// If a linked parameter/index constraint point to the current item, 
						// remove that link/index
						if(Int32::Parse(listViewFF->Items[i]->SubItems[linkInd]->Text) == index) {
							listViewFF->Items[i]->SubItems[linkInd]->Text = "-1";
							listViewFF->Items[i]->SubItems[LV_MUTABLE(j)]->Text = "N";
						}

						// All linked/index constraints that point to subsequent items should 
						// be reduced by one
						if(Int32::Parse(listViewFF->Items[i]->SubItems[linkInd]->Text) > index)
							listViewFF->Items[i]->SubItems[linkInd]->Text = (Int32::Parse(listViewFF->Items[i]->SubItems[linkInd]->Text) - 1).ToString();
					}
				}

				// 2. Removing the item from the list
				listViewFF->Items->RemoveAt(index);

				// 3. Renaming all the layers
				for(int i = index; i < listViewFF->Items->Count; i++) {
					Windows::Forms::ListViewItem ^lvi = listViewFF->Items[i];

					// Changing layer name
					lvi->Text = stringToClr(_model->GetLayerName(i));

					// Checking for applicability of the layer params
					for(int j = 0; j < nlp; j++) {
						if(!_model->IsParamApplicable(i, j)) {
							lvi->SubItems[LV_VALUE(j)]->Text = "N/A";
							lvi->SubItems[LV_MUTABLE(j)]->Text = "-";
						}
					}
				}
				
			}

			// Maximal layer count check
			if(listViewFF->Items->Count < _model->GetMaxLayers())
				addLayer->Enabled = true;

			_copiedIndicesFF->clear();

			FFParameterUpdateHandler();
			save->Enabled = true;
			undo->Enabled = false;
	}

	void FormFactor::Mut_CheckedChanged(System::Object^  sender, System::EventArgs^  e){
		if( (Object^)(((CheckBox^)(sender))->Parent) == exParamGroupbox ) {
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_MUTABLE]->Text = exParamGroupbox->check->Checked ? "Y" : "N";
			_curPar->extraParams[paramBox->SelectedIndex].isMutable = exParamGroupbox->check->Checked;
		} else {
		int nlp = _model->GetNumLayerParams();
		for(int i  = 0; i < listViewFF->SelectedItems->Count; i++) {
			for ( int j = 0; j < nlp; j++) {
				if (sender == GroupBoxList[j]->check){
					listViewFF->SelectedItems[i]->SubItems[LV_MUTABLE(j)]->Text = GroupBoxList[j]->check->Checked ? "Y" : "N";
					listViewFF->SelectedItems[i]->SubItems[LV_CONSLINK(j, nlp)]->Text = "-1";
					_curPar->params[j][listViewFF->SelectedIndices[i]].isMutable
								= GroupBoxList[j]->check->Checked;
				}
			}
		}
		}
		save->Enabled = true;
	}

	void FormFactor::exmut_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_MUTABLE]->Text = exParamGroupbox->check->Checked ? "Y" : "N";
		_curPar->extraParams[paramBox->SelectedIndex].isMutable = exParamGroupbox->check->Checked;
		save->Enabled = true;
	}

	void FormFactor::useCheckBox_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(sender == ffUseCheckBox) {
			_bUseFF = ffUseCheckBox->Checked;
			FFGroupbox->Enabled = ffUseCheckBox->Checked;
			if(_ff->tmpY.size() < _ff->y.size())
				_ff->tmpY.resize(_ff->y.size(), 1.0);
			std::swap(_ff->tmpY, _ff->y);
			freezeFFCheckBox_CheckedChanged(sender, e);

		} else if(sender == sfUseCheckBox) {
			cailleGroupbox->Enabled = sfUseCheckBox->Checked;
			phasefitter->Enabled = sfUseCheckBox->Checked;
			Peakfitter->Enabled = sfUseCheckBox->Checked;
			if(_peakPicker)
				PeakPicker_Click(sender, e);
			if(wgtFit && wgtFit->graph) {
				for(int i = 0; i < wgtFit->graph->numGraphs; i++) {
					if (graphType->at(i) == GRAPH_PEAK || graphType->at(i) == GRAPH_PHASEPOS)
						wgtFit->graph->SetGraphVisibility(i, sfUseCheckBox->Checked);
				}
				if(!(sfUseCheckBox->Checked))
					listView_peaks->SelectedItems->Clear();
			}
			if(_sf->tmpY.size() < _sf->y.size())
				_sf->tmpY.resize(_sf->y.size(), 1.0);
			std::swap(_sf->tmpY, _sf->y);

		} else if(sender == bgUseCheckBox) {
			functionsGroupBox->Enabled = bgUseCheckBox->Checked;
			if(_bg->tmpY.size() < _bg->y.size())
				_bg->tmpY.resize(_bg->y.size(), 0.0);
			std::swap(_bg->tmpY, _bg->y);

		}

		if(wgtFit && wgtFit->graph && wgtFit->graph->x)
			if(liveRefreshToolStripMenuItem->Checked)
				UpdateGraph();

	}

	void FormFactor::paramBox_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
		if(_bLoading || _bChanging)
			return;
		// Saving old parameters
		if(listView_Extraparams->SelectedIndices->Count == 0 || 
		   paramBox->SelectedIndex != listView_Extraparams->SelectedIndices[0]) {
			   exParamGroupbox->track->Value = int((exParamGroupbox->track->Minimum + exParamGroupbox->track->Maximum) / 2.0);
			listView_Extraparams->SelectedIndices->Clear();
			listView_Extraparams->SelectedIndices->Add(paramBox->SelectedIndex);
		}

		if(_bFrozenFF && paramBox->SelectedIndex > 1) {
			paramBox->SelectedIndex = 0;
			listView_Extraparams->SelectedIndices->Clear();
			listView_Extraparams->SelectedIndices->Add(paramBox->SelectedIndex);
		}

		ListViewItem ^lvi;

		if(oldIndex > -1) {
			lvi = listView_Extraparams->Items[oldIndex];

			lvi->SubItems[exParamGroupbox->rValue->Checked ? ELV_VALUE : ELV_SIGMA]->Text = exParamGroupbox->text->Text;
			if(!_bGenerateModel)
				lvi->SubItems[ELV_MUTABLE]->Text = exParamGroupbox->check->Checked ? "Y" : "N";
			lvi->SubItems[ELV_CONSMIN]->Text = exmin->Text;
			lvi->SubItems[ELV_CONSMAX]->Text = exmax->Text;
		}
		
		// Loading new parameters
		UpdateExtraParamBox();
		

		oldIndex = paramBox->SelectedIndex;
	}

	void FormFactor::exportElectronDensityProfileToolStripMenuItem1_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring file;
		sfd->FileName = "";
		sfd->Title = "Choose an E.D. profile data file";
		sfd->Filter = L"Output Files (*.out)|*.out|Data Files (*.dat, *.chi)|*.dat;*.chi|All files|*.*";
		if(sfd->ShowDialog() == 
			System::Windows::Forms::DialogResult::Cancel)
			return;

		clrToString(sfd->FileName, file);
		WriteDataFile(file.c_str(), wgtPreview->graph->x[0], wgtPreview->graph->y[0]);
	}

	void FormFactor::FormFactor_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e) {
		this->Visible = false;

		// We need to delete all the vectors, peaks, layers, etc.!
		if(wgtFit != nullptr)
			wgtFit->graph = nullptr;

		if(fitterThread && fitterThread->IsAlive)
			fitterThread->Abort();

		//SetGaussED(false);
		//// setPolyDisp(false);
		SetMinimumSig(5.0);

		this->Visible = true;
	}

	void FormFactor::logScaledFittingParamToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(logScaledFittingParamToolStripMenuItem->Checked == false) {
			for (unsigned int i=0; i<wgtFit->graph->y[0].size(); i++) {
				if( wgtFit->graph->y[0][i] <= 0.0) {
					MessageBox::Show("Negative values found: log scaled fitting is not allowed","Negative values found:" );
					logScaledFittingParamToolStripMenuItem->Checked = false;
					break;
				}
			}
		}
		setAccuracySettings(accurateFittingToolStripMenuItem->Checked, accurateDerivativeToolStripMenuItem->Checked, 
			chiSquaredBasedFittingToolStripMenuItem->Checked || (_curRSquared <= 1e-6),logScaledFittingParamToolStripMenuItem->Checked);

		if(wgtFit && wgtFit->graph) {
			UpdateChisq(WSSR(wgtFit->graph->y[0], wgtFit->graph->y[1]));
			UpdateRSquared(RSquared(wgtFit->graph->y[0], wgtFit->graph->y[1]));
		}
		return;
	}

	void FormFactor::infExtraParam_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(listView_Extraparams->SelectedItems->Count == 0)
			return;

		ListViewItem ^lvi = listView_Extraparams->SelectedItems[0];
		
		// Modify parameter name so that the user knows it's infinite
		if(infExtraParam->Checked && lvi->SubItems[ELV_INFINITE]->Text->Equals("0")) {
			lvi->Text += " (inf)";
			lvi->SubItems[ELV_INFINITE]->Text = "1";
		} else if(!infExtraParam->Checked && lvi->SubItems[ELV_INFINITE]->Text->Equals("1")) {
			lvi->Text = lvi->Text->Substring(0, lvi->Text->Length - 6);
			lvi->SubItems[ELV_INFINITE]->Text = "0";
		}

		slowModelGroupbox->Visible = _model->IsSlow();

		// Enable/disable controls according to infinity status
		if(infExtraParam->Checked) {
			exParamGroupbox->Enabled = false;
			exmin->Enabled   = false;
			exmax->Enabled   = false;
		} else {
			exParamGroupbox->Enabled = true;
			exmin->Enabled   = !_bGenerateModel;
			exmax->Enabled   = !_bGenerateModel;
			exParamGroupbox->check->Enabled   = !_bGenerateModel;
			exParamGroupbox->track->Enabled = true;
		}

		FFParameterUpdateHandler();

		save->Enabled = true;
		undo->Enabled = false;
	}

	void FormFactor::freezeFFCheckBox_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		_bFrozenFF = freezeFFCheckBox->Checked;
		bool chk = _bFrozenFF ;
		_bUseFF = ffUseCheckBox->Checked;
		if(_bUseFF) {
			listViewFF->Enabled		= !chk;
			consGroupBox->Enabled	= !chk;
			infExtraParam->Enabled	= !chk;
			paramPanel->Enabled     = !chk;
			if(chk) {
				_frozenBG = Double::Parse(listView_Extraparams->Items[1]->SubItems[ELV_VALUE]->Text);
				_frozenScale = Double::Parse(listView_Extraparams->Items[0]->SubItems[ELV_VALUE]->Text);

				if(paramBox->SelectedIndex > 1) {
					exParamGroupbox->Enabled	= false;
					exmin->Enabled					= false;
					exmax->Enabled					= false;
				}
			}
		}
	}	// freezeFFCheckBox_CheckedChanged

	/**
	 * Makes the wgtFit graph be redrawn without recalculating any models.
	**/
	void FormFactor::RedrawGraph() {
		if(!wgtFit || !wgtFit->graph)
			return;
		// These are the only relevant parts of UpdateGraph
		if(!_bGenerateModel) {
			UpdateChisq(WSSR(wgtFit->graph->y[0], wgtFit->graph->y[1]));
			UpdateRSquared(RSquared(wgtFit->graph->y[0], wgtFit->graph->y[1]));
		}

		wgtFit->graph->ToggleYTicks();
		wgtFit->graph->ToggleYTicks();
		wgtFit->Invalidate();
	}

	void FormFactor::ScaleandBGFF() {
		double oldVal, newVal;
		std::vector<double> my;
		
		oldVal = paramBox->SelectedIndex == 1 ? _frozenBG : _frozenScale;
		newVal = Double::Parse(exParamGroupbox->text->Text);

		if(!(paramBox->Text->Equals("Scale") || paramBox->Text->Equals("Background"))) {
			// reset value
			exParamGroupbox->text->Text = oldVal.ToString("0.000000");
			return;
		}

		my.resize(_ff->x.size(), 0.0);
		
		if(paramBox->Text->Equals("Background")) {
			addValueToVector(_ff->y, newVal - oldVal);
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_VALUE]->Text = newVal.ToString("0.000000");
			_frozenBG = newVal;
		} else {
			double bg = Double::Parse(listView_Extraparams->Items[1]->SubItems[ELV_VALUE]->Text);
			addValueToVector(_ff->y, -bg);
			multiplyVectorByValue(_ff->y, newVal / oldVal);
			addValueToVector(_ff->y, bg);
			listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_VALUE]->Text = newVal.ToString("0.000000000000");
			_frozenScale = newVal;
		}
		
		MultiplyVectors(my, _ff->y, _sf->y);
		AddVectors(my, my, _bg->y);

		wgtFit->graph->Modify(_bGenerateModel ? 0 : 1, _ff->x, MachineResolution(_ff->x, my, GetResolution()));

		RedrawGraph();

	}
	
	void FormFactor::loadFileAsFormFactorToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		std::wstring fname;
		std::vector<double> tmpx, tmpy;

		if(!openDataFile(ofd, "Choose a data file to use as a form factor", fname, tmpx, tmpy, false))
			return;

		_loadedFF->assign(fname);

		if(_peakPicker)
			PeakPicker_Click(sender, e);

		loadFileAsFormFactor(fname, tmpx, tmpy);

		std::vector<double> my;

		MultiplyVectors(my, _ff->y, _sf->y);
		AddVectors(my, my, _bg->y);
		
		if(liveRefreshToolStripMenuItem->Checked)
			UpdateGraph();
	}

	void FormFactor::loadFileAsFormFactor(std::wstring fname, std::vector<double> tmpX, std::vector<double> tmpY) {
		if(tmpY.size() < 2) {
			tmpY.clear();
			tmpX.clear();
			ReadDataFile(fname.c_str(), tmpX, tmpY);
		}

		if(!_ff->x.empty() && (_ff->x[0] < tmpX[0] || _ff->x[_ff->x.size() - 1] > tmpX[tmpX.size() - 1])) {
			int j = 0, k = 0;
			for(; j < (int)_ff->x.size() && _ff->x[j] < tmpX[0]; j++);
			for(k = int(_ff->x.size()) - 1; k >= j && _ff->x[k] > tmpX[tmpX.size() - 1]; k--);

			if(k - j < 1) { // less than 2 points left
				MessageBox::Show("The file that contains the form factor is not in range of the selected area." + 
					"  Please recrop the data to include the q range: " + tmpX[0].ToString() + " - " + 
					tmpX[tmpX.size() - 1] + ". Have a nice day.", "ERROR", MessageBoxButtons::OK,
					MessageBoxIcon::Error);
				return;
			}

			// Crop all the global graph tables to the maximum range
			_ff->y.resize(k - j + 1, 0.0);

			_ff->x.erase(_ff->x.begin() + k + 1, _ff->x.end());
			if((int)_ff->tmpY.size() > k)
				_ff->tmpY.erase(_ff->tmpY.begin() + k + 1, _ff->tmpY.end());

			_ff->x.erase(_ff->x.begin(), _ff->x.begin() + j);
			if((int)_ff->tmpY.size() > j)
				_ff->tmpY.erase(_ff->tmpY.begin(), _ff->tmpY.begin() + j);
			
			
			_sf->x.erase(_sf->x.begin() + k + 1, _sf->x.end());
			if((int)_sf->tmpY.size() > k)
				_sf->tmpY.erase(_sf->tmpY.begin() + k + 1, _sf->tmpY.end());
			_sf->y.erase(_sf->y.begin() + k + 1, _sf->y.end());
			
			_sf->x.erase(_sf->x.begin(), _sf->x.begin() + j);
			if((int)_sf->tmpY.size() > j)
				_sf->tmpY.erase(_sf->tmpY.begin(), _sf->tmpY.begin() + j);
			_sf->y.erase(_sf->y.begin(), _sf->y.begin() + j);
			

			_bg->x.erase(_bg->x.begin() + k + 1, _bg->x.end());
			if((int)_bg->tmpY.size() > k)
				_bg->tmpY.erase(_bg->tmpY.begin() + k + 1, _bg->tmpY.end());
			_bg->y.erase(_bg->y.begin() + k + 1, _bg->y.end());
		
			_bg->x.erase(_bg->x.begin(), _bg->x.begin() + j);
			if((int)_bg->tmpY.size() > j)
				_bg->tmpY.erase(_bg->tmpY.begin(), _bg->tmpY.begin() + j);
			_bg->y.erase(_bg->y.begin(), _bg->y.begin() + j);

			//Crop the data graph to the correct size
			if(!_bGenerateModel) {
				std::vector<double> tx = wgtFit->graph->x[0];
				std::vector<double> ty = wgtFit->graph->y[0];

				for(j = 0; j < (int)tx.size() && tx[j] < _ff->x[0]; j++);
				for(k = int(tx.size()) - 1; k >= j && tx[k] > _ff->x[_ff->x.size() - 1]; k--);

				tx.erase(tx.begin() + k + 1, tx.end());
				tx.erase(tx.begin(), tx.begin() + j);
				ty.erase(ty.begin() + k + 1, ty.end());
				ty.erase(ty.begin(), ty.begin() + j);

				SetResolution(0.0);

				wgtFit->graph->Modify(0, tx, ty);
			}
		} else {
			_ff->y.resize(_ff->x.size(), 0.0);
		}

		for(int i = 0; i < (int)_ff->x.size(); i++)
			_ff->y[i] = GUICLR::ExtractBaseline::InterpolatePoint(_ff->x[i], tmpX, tmpY);
		
		// Store previously calculated FF if Freeze is checked
		if(_bFrozenFF && !_bLoadedFF)
			_storage->y = _ff->y;
		else if(!_bLoadedFF)
			_storage->y.clear();	//Clear

		std::vector<double> grr = _ff->x;
		useModelFFButton->Visible = true;
		freezeFFCheckBox->Checked = true;
		freezeFFCheckBox->Enabled = false;
		relatedModelsToolStripMenuItem->Enabled = false;
		_bLoadedFF = true;
		SetUseFrozenFF(_bLoadedFF);
		
		/* Problem points: TODO FIXME ETC.
			* a1nm1ToolStripMenuItem
		*/
	}

	void FormFactor::useModelFFButton_Click(System::Object^  sender, System::EventArgs^  e) {
		freezeFFCheckBox->Checked = (_storage->y.size() > 1);
		useModelFFButton->Visible = false;
		_bLoadedFF = false;
		freezeFFCheckBox->Enabled = true;
		relatedModelsToolStripMenuItem->Enabled = true;
		SetUseFrozenFF(_bLoadedFF);

		_loadedFF->clear();

		std::swap(_storage->y, _ff->y);
		if(liveRefreshToolStripMenuItem->Checked)
			UpdateGraph();
	}

	void FormFactor::maskButton_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!wgtFit || !wgtFit->graph)
			return;

		maskToolStrip->Visible = !maskToolStrip->Visible;
		_bMasking = maskToolStrip->Visible;
		_bAddMask = _bMasking;

		if(_mask->size() != wgtFit->graph->x[0].size())
			_mask->resize(wgtFit->graph->x[0].size(), false);
		
		if(!_bMasking)
			_pressX = _pressY = -1;
	}

	void FormFactor::maskPanel_Click(System::Object^  sender, System::EventArgs^  e) {
		if(sender == addMaskButton) {
			_bAddMask = true;
			return;
		}

		if(sender == removeMaskButton) {
			_bAddMask = false;
			return;
		}

		if(sender == invertMaskButton)
			for(int i = 0; i < (int)_mask->size(); i++)
				_mask->at(i) = !(_mask->at(i));

		if(sender == clearMaskButton) {
			_mask->clear();
			_mask->resize(_mask->size(), false);
		}

		wgtFit->graph->RemoveMask();
		if(wgtFit && wgtFit->graph) {
			int ind = 0;
			while (ind < (int)_mask->size()) {
				int srt = -1, nd = -1;

				for(; ind < (int)_mask->size() && !_mask->at(ind); ind++);

				srt = ind;
				for(nd = srt + 1; nd < (int)_mask->size(); nd++)
					if(!(_mask->at(nd)))
						break;

				ind = nd--;

				if(nd < (int)_mask->size())
					wgtFit->graph->Mask(0, srt, nd, RGB(50, 50, 50));
			//wgtFit->graph->Mask(0, left, right, RGB(50, 50, 50));
				wgtFit->Invalidate();
			}
			RedrawGraph();

		}
	} //maskPanel_Click
	void FormFactor::minimumSignalToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	}
	void FormFactor::expResToolStripTextBox_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		double prevRes = GetResolution(), newRes;
		newRes = clrToDouble(expResToolStripTextBox->Text);
		
		if((newRes == 0.0 && !expResToolStripTextBox->Text->StartsWith("0"))	//Starts with text
			|| fabs(newRes - prevRes) < 1e-12) {	// The number hasn't changed
			expResToolStripTextBox->Text = prevRes.ToString("0.000000");
			return;
		}
		
		if(!wgtFit) {
			SetResolution(newRes);
			expResToolStripTextBox->Text = newRes.ToString();
			return;
		}

		SetResolution(newRes);
		expResToolStripTextBox->Text = newRes.ToString("0.000000");
		
		if(liveRefreshToolStripMenuItem->Checked) {
			UpdateGraph();
		}
	}
	void FormFactor::expResToolStripTextBox_Enter(System::Object^  sender, System::EventArgs^  e) {
		expResToolStripTextBox->Text = GetResolution().ToString("0.000000");
	}

	void FormFactor::reportButton_Click(System::Object^  sender, System::EventArgs^  e) {
		std::vector <std::vector <double>> errorsVector;
		System::Collections::Generic::List<ListView^>^ LV = gcnew System::Collections::Generic::List<ListView^>();
		LV->Add(listViewFF);
		LV->Add(GetPeakType() == SHAPE_CAILLE ? cailleParamListView : listView_peaks);
		LV->Add(listView_phases);
		LV->Add(BGListview);
		LV->Add(listView_Extraparams);

		errorsVector.push_back(*FFparamErrors);
		errorsVector.push_back(*SFparamErrors);
		errorsVector.push_back(*PhaseparamErrors);
		errorsVector.push_back(*BGparamErrors);

		ErrorTableWindow errrr(LV, errorsVector, _dataFile);

		errrr.ShowDialog();
	}

	// Helper function to change the current model in a paramStruct
	void FormFactor::ChangeModel(paramStruct *p, Model *newModel) {
		// The real deal
		// Main parameters
		int first = p->params.size(),
			second = p->params[0].size();
		p->params.resize(newModel->GetNumLayerParams());
		for(int i = 0; i < newModel->GetNumLayerParams(); i++) {
			p->params[i].resize(max(p->layers, newModel->GetMinLayers()));
			for(int j = (i < first) ? second : 0; j < max(p->layers, newModel->GetMinLayers()); j++) {
				// Prepare Parameter using default values
				Parameter param(newModel->GetDefaultParamValue(i, j));
				// Insert into p
				p->params[i][j] = param;
			}
		}
		int maxLayers = newModel->GetMaxLayers();
		if(maxLayers > -1) {
			for(int i = 0; i < maxLayers; i++)
				p->params[i].resize(maxLayers);
			p->layers = maxLayers;
		}

		// Extra parameters
		first = p->extraParams.size();
		p->extraParams.resize(newModel->GetNumExtraParams());
		for(int i = first; i < newModel->GetNumExtraParams(); i++) {
			ExtraParam gr = newModel->GetExtraParameter(i);
			Parameter pr(gr.defaultVal);
			p->extraParams[i] = pr;
		}		
	}

	void FormFactor::changeModel_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!_model) {
			MessageBox::Show("Error changing model: no model");
			return;
		}

		// Modify model type
		ExternalModelDialog^ emd = _parent->emd;
		emd->ClearModelSelection();

		emd->LoadDefaultModels();
		emd->ShowDialog();
		
		FFModel *ffm = emd->GetSelectedModel();

		// No model was selected
		if(!ffm)
			return;

		handleModelChange(*ffm);
	}

	void FormFactor::relatedModelToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
		if(!_model) {
			MessageBox::Show("Error changing model: no model");
			return;
		}

		// Determine which model was selected
		int ind = relatedModelsToolStripMenuItem->DropDownItems->IndexOf((ToolStripItem^)(sender));

		// Create new instance of selected model
		FFModel *ffm = dynamic_cast<FFModel *>(_model->CreateRelatedModel(ind));

		handleModelChange(*ffm);
	}

	void FormFactor::handleModelChange(FFModel &ffm) {
		delete _model;		
		_model = &ffm;
		_parent->_currentModel = _model;
		
		ReloadModelUI();
	}

	void FormFactor::ReloadModelUI() {
		// Load current parameters
		paramStruct p = *_curPar;

		_bChanging = true;

		// Resize parameter struct
		ChangeModel(&p, _model);

		// Reload UI
		PrepareModelUI();

		// Reload parameters
		ParametersToUI(&p);

		_bChanging = false;

		FFParameterUpdateHandler();
	}

	void FormFactor::PrepareModelUI() {	
		// Add extra parameters to the interface
		listView_Extraparams->Items->Clear();
		paramBox->Items->Clear();
		for(int i = 0; i < _model->GetNumExtraParams(); i++) {
			ListViewItem ^lvi = gcnew ListViewItem();
			
			char valstr[128] = {0};
			ExtraParam ep = _model->GetExtraParameter(i);
			String ^nameCLRstr = gcnew String(ep.name.c_str());

			lvi->Text = nameCLRstr;
			paramBox->Items->Add(nameCLRstr);

			// Format the default value
			sprintf(valstr, "%.*f", ep.decimalPoints, 
				_finite(ep.defaultVal) ? ep.defaultVal : 0.0);
			
			lvi->SubItems->Add(gcnew String(valstr));
			
			lvi->SubItems->Add("N");           // Mutable
			
			// Constraints (default constraints are the range values)
			sprintf(valstr, "%.*f", ep.decimalPoints, ep.rangeMin);
			lvi->SubItems->Add(gcnew String(valstr));  // Min constraint
			sprintf(valstr, "%.*f", ep.decimalPoints, ep.rangeMax);
			lvi->SubItems->Add(gcnew String(valstr));  // Max constraint

			// Infinite
			lvi->SubItems->Add(_finite(ep.defaultVal) ? "0" : "1");
			if(!_finite(ep.defaultVal))
				lvi->SubItems[ELV_NAME]->Text += " (inf)";

			lvi->SubItems->Add("Y");  // isConstained -> Fix this if we ever want to add another checkbox

			lvi->SubItems->Add("0.000000"); // Standard deviation

			listView_Extraparams->Items->Add(lvi);
		}
		paramBox->SelectedIndex = 0;
		if(exParamGroupbox)
			delete exParamGroupbox;
		exParamGroupbox = gcnew ParamGroupBox(listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_NAME]->Text, ELV_VALUE, true);
		exParamGroupbox->text->Text = listView_Extraparams->Items[paramBox->SelectedIndex]->SubItems[ELV_VALUE]->Text;

		// Group box Events
		exParamGroupbox->text->Enter += gcnew System::EventHandler(this, &FormFactor::ExtraParameter_Enter);
		exParamGroupbox->text->Leave += gcnew System::EventHandler(this, &FormFactor::ExtraParameter_TextChanged);
		exParamGroupbox->rStddev->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::PDRadioChanged);
		exParamGroupbox->track->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
		exParamGroupbox->check->Click += gcnew System::EventHandler(this, &FormFactor::Mut_CheckedChanged);

		globalParamtersGroupBox->Controls->Add(exParamGroupbox);
		exParamGroupbox->Location = Point(6, 42);
		exParamGroupbox->Enabled = true;
		// END of extra parameters

		// Initialize param groupbox list
		while(GroupBoxList && GroupBoxList->Count > 0)
		{
			delete GroupBoxList[0];
			GroupBoxList->RemoveAt(0);
		}
		GroupBoxList = gcnew System::Collections::Generic::List<ParamGroupBox^>();

		// Initialize related models list
		while(relatedModelsList && relatedModelsList->Count > 0)
		{
			delete relatedModelsList[0];
			relatedModelsList->RemoveAt(0);
		}
		relatedModelsList = gcnew System::Collections::Generic::List<ToolStripMenuItem^>();
		
		// Layer parameters set up
		while(listViewFF->Columns->Count > 1)
			listViewFF->Columns->RemoveAt(1);
		listViewFF->Items->Clear();
		for (int i = 0; i < _model->GetNumLayerParams(); i++) {
			String ^lpName = stringToClr(_model->GetLayerParamName(i));
			int lvIndex;

			// Handle layer listview columns
			lvIndex = listViewFF->Columns->Count;			
			listViewFF->Columns->Add(lpName);			
			listViewFF->Columns->Add("M");

			// Default column widths
			listViewFF->Columns[lvIndex]->Width = 80;

			// Don't show mutability columns if we're generating
			listViewFF->Columns[lvIndex + 1]->Width = _bGenerateModel ? 0 : 25;
			listViewFF->Columns[lvIndex + 1]->TextAlign = HorizontalAlignment::Center;
			// END of listview columns

			// Handle parameter groupboxes
			GroupBoxList->Add(gcnew ParamGroupBox(lpName, lvIndex, true));

			// Group box Events
			GroupBoxList[i]->text->Leave += gcnew System::EventHandler(this, &FormFactor::Parameter_TextChanged);
			GroupBoxList[i]->rStddev->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::PDRadioChanged);
			GroupBoxList[i]->track->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
			GroupBoxList[i]->check->Click += gcnew System::EventHandler(this, &FormFactor::Mut_CheckedChanged);

			if(_bGenerateModel)
				GroupBoxList[i]->check->Enabled = false;

			paramPanel->Controls->Add(GroupBoxList[i]);
			// END of groupboxes
		}

		// Initial layers
		for(int i = 0; i < _model->GetMinLayers(); i++)
			AddParamLayer();

		// Model display values
		// If there are no display parameters, don't show the list
		if (_model->GetNumDisplayParams() == 0)
			listView_display->Visible = false;
		else
			listView_display->Visible = true;

		// Custom Electron Density profile functions
		if(_model->IsLayerBased()) {
			// Get the default electron density profile
			EDProfile defaultEDP = _model->GetEDProfile();
			// If not discrete, disable other options
			if(defaultEDP.shape != DISCRETE && !defaultEDP.func) {
				electronDensityProfileToolStripMenuItem->Visible = false;
			} else {
				electronDensityProfileToolStripMenuItem->Visible = true;

				discreteStepsToolStripMenuItem->Enabled = true;			
				gaussiansToolStripMenuItem->Enabled = true;
				hyperbolictangentSmoothStepsToolStripMenuItem->Enabled = true;
				stepResolutionToolStripMenuItem->Enabled = true;				
			}

			edpBox->Visible = true;			

			// Reset model ED profile type
			discreteStepsToolStripMenuItem->Checked = true;
			gaussiansToolStripMenuItem->Checked = false;
			hyperbolictangentSmoothStepsToolStripMenuItem->Checked = false;
		} else {
			discreteStepsToolStripMenuItem->Enabled = false;			
			gaussiansToolStripMenuItem->Enabled = false;
			hyperbolictangentSmoothStepsToolStripMenuItem->Enabled = false;
			stepResolutionToolStripMenuItem->Enabled = false;
			edpBox->Visible = false;
		}

		// Getting paramStruct from GUI
		paramStruct p (_model);
		UItoParameters(&p);
		
		if(!_curPar)
			_curPar = new paramStruct(_model);
		*_curPar = p;

		// Update ED profile
		UpdateEDPreview();

		// If there are display parameters, add the parameters names and 
		// values to the list
		listView_display->Items->Clear();
		for (int i = 0 ; i < _model->GetNumDisplayParams(); i++){
			ListViewItem ^lvi = gcnew ListViewItem();

			std::string nameStr = _model->GetDisplayParamName(i);
			String ^nameCLRstr = gcnew String(nameStr.c_str());

			lvi->Text = nameCLRstr;
			lvi->SubItems->Add(_model->GetDisplayParamValue(i, &p).
				ToString("0.000000"));
			listView_display->Items->Add(lvi);
		}
		// END of display values

		// This is silly. The only time this (PrepareModelUI) method is called is during loading or changing.
		// The first thing that ParameterUpdateHandler does is return if it is one of those...
		FFParameterUpdateHandler();

		// Fill the related models menu
		relatedModelsToolStripMenuItem->DropDownItems->Clear();
		for(int i = 0; i < _model->GetNumRelatedModels(); i++) {
			relatedModelsList->Add(gcnew System::Windows::Forms::ToolStripMenuItem());
			relatedModelsList[i]->Name = stringToClr(_model->GetRelatedModelName(i));
			relatedModelsList[i]->Text = stringToClr(_model->GetRelatedModelName(i));
			relatedModelsList[i]->Click += gcnew System::EventHandler(this, &FormFactor::relatedModelToolStripMenuItem_Click);
			relatedModelsToolStripMenuItem->DropDownItems->Add(relatedModelsList[i]);
			if(_model->GetRelatedModelName(i).compare(_model->GetName()) == 0)
				relatedModelsList[i]->Visible = false;
		}
		relatedModelsToolStripMenuItem->Visible = (relatedModelsToolStripMenuItem->DropDownItems->Count > 0);
	}

	void FormFactor::FFParameterUpdateHandler() {
		if(_bLoading ^ _bChanging)
			return;
		// This function should:
		// 0. Update _curPar
		UItoParameters(_curPar);

		// 1. If necessary, redraw the graph
		if(liveRefreshToolStripMenuItem->Checked)
			UpdateGraph(_bLoading || _bChanging);		

		// 2. Modify display values
		// 3. Redraw 3d preview and ED profile		

		// Modifying display values
		for( int i = 0; i < listView_display->Items->Count; i++) {
			ListViewItem ^lvi = listView_display->Items[i];
			lvi->SubItems[1]->Text = _model->GetDisplayParamValue(i, _curPar).
													ToString("0.000000");	
		}

		if(!_bFromFitter) {
			FFparamErrors->clear();
			FFmodelErrors->clear();

			undo->Enabled = false;
		}
		// Disable/enable + button according to number of layers
		if(_model->GetMaxLayers() < 0) // The infinite case
			addLayer->Enabled = true;
		else
			addLayer->Enabled = (listViewFF->Items->Count < _model->GetMaxLayers());

		// Updating ED profile-related stuff
		UpdateEDPreview();
		EDU();

	}

	void FormFactor::listViewFF_DoubleClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
		if(listViewFF->SelectedItems->Count < 1)
			return;
		fitRange_Click(sender, e);
	}

	void FormFactor::discreteStepsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		// Change ED Profile in model, if necessary
		if(discreteStepsToolStripMenuItem->Checked && _model) {
			_model->SetEDProfile(EDProfile(SYMMETRIC, DISCRETE));

			ReloadModelUI();
		}

		discreteStepsToolStripMenuItem->Checked = true;
		gaussiansToolStripMenuItem->Checked = false;
		hyperbolictangentSmoothStepsToolStripMenuItem->Checked = false;		
	}

	void FormFactor::gaussiansToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		// Change ED Profile in model, if necessary
		if(gaussiansToolStripMenuItem->Checked && _model) {
			_model->SetEDProfile(EDProfile(SYMMETRIC, GAUSSIAN));

			// Modify profile resolution
			int res = atoi(clrToString(edpResolution->Text).c_str());
			if(adaptiveToolStripMenuItem->Checked)
				_model->GetEDProfile().func->SetResolution(-res);
			else
				_model->GetEDProfile().func->SetResolution(res);

			// Reload UI
			ReloadModelUI();
		}

		discreteStepsToolStripMenuItem->Checked = false;
		gaussiansToolStripMenuItem->Checked = true;
		hyperbolictangentSmoothStepsToolStripMenuItem->Checked = false;
	}

	void FormFactor::hyperbolictangentSmoothStepsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		// Change ED Profile in model, if necessary
		if(hyperbolictangentSmoothStepsToolStripMenuItem->Checked && _model) {
			_model->SetEDProfile(EDProfile(SYMMETRIC, TANH));

			// Modify profile resolution
			int res = atoi(clrToString(edpResolution->Text).c_str());
			if(adaptiveToolStripMenuItem->Checked)
				_model->GetEDProfile().func->SetResolution(-res);
			else
				_model->GetEDProfile().func->SetResolution(res);

			// Reload UI
			ReloadModelUI();
		}

		discreteStepsToolStripMenuItem->Checked = false;
		gaussiansToolStripMenuItem->Checked = false;
		hyperbolictangentSmoothStepsToolStripMenuItem->Checked = true;
	}

	void FormFactor::edpResolution_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		int res = atoi(clrToString(edpResolution->Text).c_str());
		if(res < 0)
			res = -res;
		else if(res == 0)
			res = 1;
		edpResolution->Text = Int32(res).ToString();

		if(_model && _model->GetEDProfile().func)
			_model->GetEDProfile().func->SetResolution(
							adaptiveToolStripMenuItem->Checked ? -res : res);

		FFParameterUpdateHandler();
	}

	void FormFactor::adaptiveToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) {
		if(_model && _model->GetEDProfile().func) {
			int res = Int32::Parse(edpResolution->Text);

			if(adaptiveToolStripMenuItem->Checked) {
				res = DEFAULT_EDEPS;				
				_model->GetEDProfile().func->SetResolution(-res);
			} else {
				res = DEFAULT_EDRES;
				_model->GetEDProfile().func->SetResolution(res);
			}

			edpResolution->Text = Int32(res).ToString();

			FFParameterUpdateHandler();
		}
	}

}