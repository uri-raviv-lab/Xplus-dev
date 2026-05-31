#pragma once

#include "WGTControl.h"
#include "OpenGLControl.h"
#include "graphtoolkit.h"
#include "OpeningWindow.h"

#include "UnicodeChars.h"
#include "GUIHelperClasses.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;
using namespace System::Threading;

enum ParamMode {
	MODE_DEFAULT,
	MODE_ABSOLUTE,
	MODE_PRECISION,
	MODE_01,
	MODE_INTEGER,
};

enum GraphSource{
	GRAPH_DATA,
	GRAPH_MODEL,
	GRAPH_PEAK,
	GRAPH_PHASEPOS
};

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

namespace GUICLR {

	// CLR is playing games with us again, needed for C externalization
	void ReportProgressDummy(int progress);
	void ReportDone(bool cancelled);
	void UpdateGeneratedGraph(const std::vector<double>& x, const std::vector<double>& y);

	// Thread-safe delegate functions
	delegate void DReportProgress(System::Object^  sender, System::ComponentModel::ProgressChangedEventArgs^  e);
	delegate void DReportDone(System::Object^  sender, System::ComponentModel::RunWorkerCompletedEventArgs^  e);
	
	/// <summary>
	/// Summary for FormFactor
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class FormFactor : public System::Windows::Forms::Form
	{
#pragma region Local Variables
	public: 
		GUICLR::WGTControl ^wgtPreview; // The electron density profile graph
 		GUICLR::WGTControl ^wgtFit;   // The signal against model graph

	protected:
		bool _bMultCalc;		// Flag to indicate whether we are calculating for one or more tabs
		int  _LastTab2Calc;
		int  _CounterCalc;
		bool _bLastFitMult;		// Flag to indicate whether the last fit was iterative or not
		
		System::Collections::Generic::List<ParamGroupBox^>^ GroupBoxList;
		System::Collections::Generic::List<ToolStripMenuItem^>^ relatedModelsList;
		ParamGroupBox^ exParamGroupbox;	// Contains the value of the selected extra parameter

		bool _bGenerateModel;
		bool _bLoading;		// Flag to indicate whether or not to UpdateGraph
		bool _bLoadedFF;	// Flag to indicate whether or not a FF was loaded from a file
		//bool _bGaussian;	// Flag to indicate gaussian or discrete model
		//bool _bmembrane;	// Flag to indicate the different specific definitions for membranes.
		//bool _bConsEcc;		// Flag to indicate whether the eccentricity of a cylindroid is constant
		bool _bSaved;		// Flag to indicate if there were any changes since the last save
		bool _bFrozenFF;	// Flag for frozen form factor. Tell whether to en/dis able items
		bool _bUseFF;		// Flag to tell whether to en/dis able items
		bool _bsigma;		// Flag to indicate that in Gaussian peak we chose to work with sigma 
		bool _bMasking;		// Flag to indicate that we are currently cropping
		bool _bAddMask;		// Flag to indicate whether a mask should be added (true) or removed (false)
		bool _bChanging;	// Flag to indicate that the model is in the midst of being changed and that the graphics should not be updated
		bool _bFromFitter;	// Flag to indicate that the parameters were returned from the fitter and certain things should not be done
		double _curWssr, _curRSquared;
		double _frozenScale, _frozenBG;		// Values for calculating FF when frozen
		int oldIndex;
		int oldPhasesI;
	    System::String ^_dataFile;
		GUICLR::OpenGLWidget ^oglPreview;
		paramStruct *_curPar;
		peakStruct *_curPeaks, *_curPeaksCaille;
		bgStruct *_curBG;
		phaseStruct *_curPhases;
		double _pressX, _pressY;	 // Used for the user inputed SF peaks
		int _Xdown, _Ydown;			 // Used to draw the user inputed SF peak outlines
		std::vector<int> *_copiedIndicesFF;	// Indices of listView items to be copied for FF
		std::vector<bool> *_mask;
		graphTable *_data, *_ff, *_sf, *_bg, *_baseline, *_storage; // Vectors to contain the seperate form factor, structure factor and background
		int *_pShouldStop;	// != 0 when fitter should stop
		bool _peakPicker;	//Flag to tell if in peak finder mode
		bool _bFitPhase; // tells the computer whether the user clicked on FitPhase or not
		std::vector<GraphSource> *graphType; //List of the source of the wgtFit graphs
		double threshold1, threshold2;	//threshold values for automatic peak finding
		std::vector<double> *_ph, *_generatedPhaseLocs; //pointer to phases vector to pass to fitter
		PhaseType *phaseSelected;        //pointer to phase selected
		std::vector <std::string> *indicesLoc; //vector to indicate position of peaks in phases.
		bool _fitToBaseline;	// Flag to tell if in fit to baseline mode
		cailleParamStruct *_curCaille;
		std::wstring *_loadedFF;
		std::vector<double> *FFparamErrors;
		std::vector<double> *SFparamErrors;
		std::vector<double> *BGparamErrors;
		std::vector<double> *PhaseparamErrors;
		std::vector<double> *FFmodelErrors;
		std::vector<double> *SFmodelErrors;
		std::vector<double> *BGmodelErrors;
		OpeningWindow ^_parent;

		void *iniFile;

		FFModel *_model;         // The current model we are fitting (FF)
		bool bThreadSuspended;
#pragma endregion

#pragma region UI Objects and Variables
	private:
		System::Threading::Thread ^fitterThread;
		System::Windows::Forms::CheckBox^  logXCheckBox;
		System::Windows::Forms::ToolStripMenuItem^  ExperimentResToolStripMenuItem;
		System::Windows::Forms::ToolStripTextBox^  expResToolStripTextBox;
		System::Windows::Forms::Button^  reportButton;
		System::Windows::Forms::ListView^  listView_display;
		System::Windows::Forms::ColumnHeader^  columnHeader16;
		System::Windows::Forms::ColumnHeader^  columnHeader17;
		System::Windows::Forms::FlowLayoutPanel^  paramPanel;
		System::Windows::Forms::Button^  changeModel;
		System::Windows::Forms::Timer^  timer1;
		System::Windows::Forms::Label^  LocOnGraph;
		System::Windows::Forms::ToolStripMenuItem^  exportToolStripMenuItem1;
		System::Windows::Forms::ToolStripMenuItem^  exportSignalToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  exportModelToolStripMenuItem1;
		System::Windows::Forms::ToolStripMenuItem^  exportElectronDensityProfileToolStripMenuItem1;
		System::Windows::Forms::ProgressBar^  progressBar1;
		System::Windows::Forms::Label^  label6;
		System::Windows::Forms::SaveFileDialog^  sfd;
		System::Windows::Forms::ToolStripMenuItem^  importParametersToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  importBaselineToolStripMenuItem;
		System::Windows::Forms::OpenFileDialog^  ofd;
		System::Windows::Forms::TabControl^  tabControl1;
		System::Windows::Forms::TabPage^  FFTab;
		System::Windows::Forms::TabPage^  SFTab;
		System::Windows::Forms::GroupBox^  globalParamtersGroupBox;



		System::Windows::Forms::ComboBox^  paramBox;
		System::Windows::Forms::Label^  minim;
		System::Windows::Forms::Label^  maxim;
		System::Windows::Forms::TextBox^  exmax;
		System::Windows::Forms::TextBox^  exmin;
		System::Windows::Forms::CheckBox^  infExtraParam;
		System::Windows::Forms::ListView^  listView_Extraparams;
		System::Windows::Forms::ColumnHeader^  Param;
		System::Windows::Forms::ColumnHeader^  Value;
		System::Windows::Forms::GroupBox^  edpBox;
		System::Windows::Forms::GroupBox^  genRangeBox;
		System::Windows::Forms::Label^  label5;
		System::Windows::Forms::TextBox^  startGen;
		System::Windows::Forms::TextBox^  endGen;
		System::Windows::Forms::Button^  removeLayer;
		System::Windows::Forms::Button^  addLayer;
		System::Windows::Forms::Button^  fitRange;
		System::Windows::Forms::Label^  paramLabel;
		System::Windows::Forms::GroupBox^  manipBox;
		System::Windows::Forms::Button^  baseline;
		System::Windows::Forms::Button^  zeroBG;
		System::Windows::Forms::ListView^  listViewFF;
		System::Windows::Forms::ColumnHeader^  ParameterCol;
		System::Windows::Forms::ComboBox^  order;
		System::Windows::Forms::Button^  button1;
		System::Windows::Forms::Button^  button2;
		System::Windows::Forms::Label^  label10;
		System::Windows::Forms::ListView^  listView_peaks;
		System::Windows::Forms::ColumnHeader^  columnHeader1;
		System::Windows::Forms::ColumnHeader^  columnHeader2;
		System::Windows::Forms::ColumnHeader^  columnHeader3;
		System::Windows::Forms::ColumnHeader^  columnHeader4;
		System::Windows::Forms::ColumnHeader^  columnHeader5;
		System::Windows::Forms::ColumnHeader^  columnHeader6;
		System::Windows::Forms::ColumnHeader^  columnHeader7;
		System::Windows::Forms::CheckBox^  centerMut;
		System::Windows::Forms::TextBox^  centerBox;
		System::Windows::Forms::Label^  label11;
		System::Windows::Forms::CheckBox^  fwhmMut;
		System::Windows::Forms::CheckBox^  ampMut;
		System::Windows::Forms::TextBox^  fwhmBox;
		System::Windows::Forms::Label^  label12;
		System::Windows::Forms::TextBox^  ampBox;
		System::Windows::Forms::Label^  label13;
		System::Windows::Forms::Label^  label14;
		System::Windows::Forms::Label^  label15;
		System::Windows::Forms::ColumnHeader^  columnHeader8;
		System::Windows::Forms::ColumnHeader^  columnHeader9;
		System::Windows::Forms::TrackBar^  ampTrack;
		System::Windows::Forms::TrackBar^  fwhmTrack;
		System::Windows::Forms::TrackBar^  centerTrack;
		System::Windows::Forms::Button^  removePeak;
		System::Windows::Forms::Button^  addPeak;
		System::Windows::Forms::Timer^  scrollerTimer;
		System::Windows::Forms::Button^  PeakPicker;
		System::Windows::Forms::GroupBox^  phasefitter;
		System::Windows::Forms::GroupBox^  Peakfitter;
		System::Windows::Forms::Button^  Move2Phases;
		System::Windows::Forms::Button^  fitphase;
		System::Windows::Forms::ToolStripMenuItem^  optionsToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  relatedModelsToolStripMenuItem;
		System::Windows::Forms::Label^  rsquared;
		System::Windows::Forms::TextBox^  thresholdBox2;
		System::Windows::Forms::Label^  Threshold_label2;
		System::Windows::Forms::TextBox^  thresholdBox1;
		System::Windows::Forms::Label^  Threshold_label1;
		System::Windows::Forms::Button^  automaticPeakFinderButton;
		System::Windows::Forms::ToolStripMenuItem^  exportGraphToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  saveParametersAsToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  exportFormFactorToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  exportStructureFactorToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  exportBackgroundToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  accurateDerivativeToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  accurateFittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  computingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  cPUToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  gPUToolStripMenuItem;
		System::Windows::Forms::ToolStripSeparator^  toolStripSeparator1;
		System::Windows::Forms::ToolStripMenuItem^  chiSquaredBasedFittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  minimumSignalToolStripMenuItem;
		System::Windows::Forms::ToolStripTextBox^  minimumSignalTextbox;
		System::Windows::Forms::TabPage^  BGTab;
		System::Windows::Forms::GroupBox^  functionsGroupBox;
		System::Windows::Forms::ListView^  BGListview;
		System::Windows::Forms::ColumnHeader^  funcHeader;
		System::Windows::Forms::ColumnHeader^  BaseHeader;
		System::Windows::Forms::ColumnHeader^  BaseMutHeader;
		System::Windows::Forms::ColumnHeader^  DecHeader;
		System::Windows::Forms::ColumnHeader^  DecMutHeader;
		System::Windows::Forms::ColumnHeader^  xCenterHeader;
		System::Windows::Forms::ColumnHeader^  xCenterMutHeader;
		System::Windows::Forms::ColumnHeader^  funcNumHeader;
		System::Windows::Forms::Label^  xCenterLabel;
		System::Windows::Forms::TrackBar^  xCenterTrackBar;
		System::Windows::Forms::CheckBox^  xCenterMut;
		System::Windows::Forms::TextBox^  xCenterTextbox;
		System::Windows::Forms::CheckBox^  decayMut;
		System::Windows::Forms::TextBox^  decayTextbox;
		System::Windows::Forms::Label^  decayLabel;
		System::Windows::Forms::TrackBar^  decayTrackBar;
		System::Windows::Forms::Button^  removeFuncButton;
		System::Windows::Forms::Button^  addFuncButton;
		System::Windows::Forms::GroupBox^  funcTypeBox;
		System::Windows::Forms::ComboBox^  funcTypeList;
		System::Windows::Forms::Label^  DEBUG_label;
		System::Windows::Forms::Label^  baseLabel;
		System::Windows::Forms::TextBox^  baseTextbox;
		System::Windows::Forms::CheckBox^  baseMut;
		System::Windows::Forms::TrackBar^  baseTrackBar;
		System::Windows::Forms::ListView^  listView_phases;
		System::Windows::Forms::ColumnHeader^  columnHeader10;
		System::Windows::Forms::ColumnHeader^  columnHeader11;
		System::Windows::Forms::GroupBox^  phasesParamsGroupBox;
		System::Windows::Forms::CheckBox^  MutPhases;
		System::Windows::Forms::Label^  label21;
		System::Windows::Forms::TextBox^  ValPhases;
		System::Windows::Forms::ComboBox^  comboPhases;
		System::Windows::Forms::Label^  label22;
		System::Windows::Forms::Label^  label23;
		System::Windows::Forms::TextBox^  MaxPhases;
		System::Windows::Forms::TextBox^  MinPhases;
		System::Windows::Forms::Label^  maxLabel;
		System::Windows::Forms::Label^  minLabel;
		System::Windows::Forms::TextBox^  baseMaxBox;
		System::Windows::Forms::TextBox^  baseMinBox;
		System::Windows::Forms::TextBox^  xcMaxBox;
		System::Windows::Forms::TextBox^  xcMinBox;
		System::Windows::Forms::TextBox^  decMaxBox;
		System::Windows::Forms::TextBox^  decMinBox;
		System::Windows::Forms::ToolStripMenuItem^  a1nm1ToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  exportSigModBLToolStripMenuItem;
		System::Windows::Forms::Label^  label25;
		System::Windows::Forms::Label^  label24;
		System::Windows::Forms::Label^  label26;
		System::Windows::Forms::Button^  Caille_button;
		System::Windows::Forms::Button^  smooth;
		System::Windows::Forms::GroupBox^  cailleGroupbox;
		System::Windows::Forms::GroupBox^  PeakShapeGroupbox;
		System::Windows::Forms::ComboBox^  peakfit;

		System::Windows::Forms::ColumnHeader^  columnHeader13;
		System::Windows::Forms::ColumnHeader^  columnHeader12;
		System::Windows::Forms::GroupBox^  EDAreaGroup;
		System::Windows::Forms::TextBox^  AreaText;
		System::Windows::Forms::Button^  SortButton;
		System::Windows::Forms::ToolStripMenuItem^  exportAllParametersAsCSVFileToolStripMenuItem;
		System::Windows::Forms::GroupBox^  consGroupBox;
		System::Windows::Forms::CheckBox^  constraints;
		System::Windows::Forms::Button^  fitToBaseline;
		System::Windows::Forms::ColumnHeader^  columnHeader14;
		System::Windows::Forms::Button^  undoPhases;
		System::Windows::Forms::ListView^  listView_PeakPosition;
		System::Windows::Forms::ToolStripMenuItem^  fittingMethodToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  fittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  levmar;
		System::Windows::Forms::ToolStripMenuItem^  diffEvo;
		System::Windows::Forms::ToolStripMenuItem^  raindrop;
		System::Windows::Forms::ToolStripSeparator^  toolStripSeparator3;
		System::Windows::Forms::ToolStripMenuItem^  iterationsToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  phaseFittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  pLevmar;
		System::Windows::Forms::ToolStripMenuItem^  pDiffEvo;
		System::Windows::Forms::ToolStripMenuItem^  pRaindrop;
		System::Windows::Forms::ToolStripSeparator^  toolStripSeparator4;
		System::Windows::Forms::ToolStripMenuItem^  iterationsToolStripMenuItem1;
		System::Windows::Forms::ToolStripTextBox^  fittingIter;
		System::Windows::Forms::ToolStripTextBox^  phaseIterations;
		System::Windows::Forms::ListView^  caillePeaksListView;
		System::Windows::Forms::ColumnHeader^  hColumnHeader;
		System::Windows::Forms::ColumnHeader^  cailleCenterColumnHeader;
		System::Windows::Forms::GroupBox^  cailleValueGroupbox;
		System::Windows::Forms::TrackBar^  cailleValueTrackBar;
		System::Windows::Forms::TextBox^  cailleValueTextBox;
		System::Windows::Forms::ListView^  cailleParamListView;
		System::Windows::Forms::ColumnHeader^  cailleParamColumnHeader;
		System::Windows::Forms::ColumnHeader^  cailleValuecolumnHeader;
		System::Windows::Forms::GroupBox^  cailleMaxGroupbox;
		System::Windows::Forms::TextBox^  cailleMaxTextBox;
		System::Windows::Forms::GroupBox^  cailleMinGroupbox;
		System::Windows::Forms::TextBox^  cailleMinTextBox;
		System::Windows::Forms::ColumnHeader^  cailleMinColumnHeader;
		System::Windows::Forms::ColumnHeader^  cailleMaxColumnHeader;
		System::Windows::Forms::Button^  returnToLineshapeButton;
		System::Windows::Forms::CheckBox^  cailleSigCheckbox;
		System::Windows::Forms::CheckBox^  cailleNCheckbox;
		System::Windows::Forms::CheckBox^  cailleEtaCheckbox;
		System::Windows::Forms::CheckBox^  cailleAmpCheckbox;
		System::Windows::Forms::Label^  mutLabel;
		System::Windows::Forms::ColumnHeader^  columnHeader15;
		System::Windows::Forms::ColumnHeader^  recipColumnHeader;
		System::Windows::Forms::Button^  showCailleButton;
		System::Windows::Forms::TextBox^  phaseErrorTextBox;
		System::Windows::Forms::Label^  phaseErrorLabel;
		System::Windows::Forms::Button^  clearPositionsButton;
		System::Windows::Forms::CheckBox^  ffUseCheckBox;
		System::Windows::Forms::CheckBox^  sfUseCheckBox;
		System::Windows::Forms::CheckBox^  bgUseCheckBox;
		System::Windows::Forms::GroupBox^  FFGroupbox;
		System::Windows::Forms::Label^  label2;
		System::Windows::Forms::GroupBox^  previewBox;
		System::Windows::Forms::Panel^  oglPanel;
		System::Windows::Forms::ToolStripMenuItem^  sigmaFWHMToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  sigmaToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  fWHMToolStripMenuItem;
		System::Windows::Forms::GroupBox^  slowModelGroupbox;
		System::Windows::Forms::Label^  warningLabel;
		System::Windows::Forms::TextBox^  Volume;
		System::Windows::Forms::Label^  Vollabel;
		System::Windows::Forms::Button^  PeakFinderCailleButton;
		System::Windows::Forms::ToolStripMenuItem^  exportDecomposedToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  logScaledFittingParamToolStripMenuItem;






		System::Windows::Forms::CheckBox^  cailleN_diffusedCheckBox;
		System::Windows::Forms::CheckBox^  freezeFFCheckBox;
		System::Windows::Forms::ToolStripMenuItem^  loadFileAsFormFactorToolStripMenuItem;
		System::Windows::Forms::Button^  useModelFFButton;
		System::Windows::Forms::ComboBox^  CalcComboBox;
		System::Windows::Forms::ToolStrip^  maskToolStrip;
		System::Windows::Forms::ToolStripButton^  addMaskButton;
		System::Windows::Forms::ToolStripButton^  removeMaskButton;
		System::Windows::Forms::ToolStripButton^  invertMaskButton;
		System::Windows::Forms::ToolStripButton^  clearMaskButton;
		System::Windows::Forms::Button^  maskButton;
		System::Windows::Forms::ProgressBar^  iterProgressBar;
		System::Windows::Forms::ToolStripSeparator^  toolStripSeparator5;
		System::Windows::Forms::ToolStripMenuItem^  pauseFittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  fittingEmergencyStopToolStripMenuItem;
		System::Windows::Forms::MenuStrip^  menuStrip1;
		System::Windows::Forms::ToolStripMenuItem^  exportToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  settingsToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  logarToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  dragToZoomToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  liveRefreshToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  liveFittingToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  plotElectronDensityProfileToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  plotFittingResultsToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  integrationToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  integrationTypeToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  gaussLegendreToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  monteCarloToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  simpsonsRuleToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  saveParametersToolStripMenuItem;
		System::Windows::Forms::ToolStripSeparator^  toolStripSeparator2;
		System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
		System::Windows::Forms::Panel^  movetophases;
		System::Windows::Forms::GroupBox^  groupBox1;
		System::Windows::Forms::Label^  label1;
		System::Windows::Forms::Button^  calculate;
		System::Windows::Forms::Label^  wssr;
		System::Windows::Forms::Button^  save;
		System::Windows::Forms::Button^  changeData;
		System::Windows::Forms::CheckBox^  logScale;
		System::Windows::Forms::Panel^  panel3;
		System::Windows::Forms::Button^  undo;
		System::Windows::Forms::Panel^  panel2;
		System::Windows::Forms::ToolStripMenuItem^  quadratureResolutionToolStripMenuItem;
		System::Windows::Forms::ToolStripTextBox^  toolStripTextBox1;
		System::Windows::Forms::ToolStripMenuItem^  generationToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  gridResolutionToolStripMenuItem;
		System::Windows::Forms::ToolStripTextBox^  toolStripTextBox2;
private: System::Windows::Forms::ToolStripMenuItem^  electronDensityProfileToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  discreteStepsToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  gaussiansToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  hyperbolictangentSmoothStepsToolStripMenuItem;
private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator6;
private: System::Windows::Forms::ToolStripMenuItem^  stepResolutionToolStripMenuItem;
private: System::Windows::Forms::ToolStripTextBox^  edpResolution;
private: System::Windows::Forms::ToolStripMenuItem^  adaptiveToolStripMenuItem;
private: System::Windows::Forms::ToolStripSeparator^  toolStripSeparator7;
private: System::Windows::Forms::ToolStripMenuItem^  polydiToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  uniformPDToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  gaussianPDToolStripMenuItem;
private: System::Windows::Forms::ToolStripMenuItem^  lorentzianPDToolStripMenuItem;
		 System::ComponentModel::IContainer^  components;
#pragma endregion

	public:
		FormFactor(const wchar_t *filename, bool bGenerate, OpeningWindow^ parent);
		~FormFactor();



#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->components = (gcnew System::ComponentModel::Container());
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(FormFactor::typeid));
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->exportToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->importParametersToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->importBaselineToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->saveParametersToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->saveParametersAsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->loadFileAsFormFactorToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator2 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->plotElectronDensityProfileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->plotFittingResultsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator5 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->pauseFittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->fittingEmergencyStopToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->generationToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->gridResolutionToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripTextBox2 = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->exportToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportSignalToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportModelToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportBackgroundToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportGraphToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportElectronDensityProfileToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportFormFactorToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportStructureFactorToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportDecomposedToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportSigModBLToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->exportAllParametersAsCSVFileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->settingsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->dragToZoomToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->logarToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->liveRefreshToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->liveFittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->a1nm1ToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->optionsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->relatedModelsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->electronDensityProfileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->discreteStepsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->gaussiansToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->hyperbolictangentSmoothStepsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator6 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->stepResolutionToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->edpResolution = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->toolStripSeparator7 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->adaptiveToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator1 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->accurateDerivativeToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->accurateFittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->chiSquaredBasedFittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->logScaledFittingParamToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->fittingMethodToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->fittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->levmar = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->diffEvo = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->raindrop = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator3 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->iterationsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->fittingIter = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->phaseFittingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->pLevmar = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->pDiffEvo = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->pRaindrop = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripSeparator4 = (gcnew System::Windows::Forms::ToolStripSeparator());
			this->iterationsToolStripMenuItem1 = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->phaseIterations = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->computingToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->cPUToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->gPUToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->minimumSignalToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->minimumSignalTextbox = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->ExperimentResToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->expResToolStripTextBox = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->sigmaFWHMToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->sigmaToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->fWHMToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->integrationToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->integrationTypeToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->gaussLegendreToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->monteCarloToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->simpsonsRuleToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->quadratureResolutionToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->toolStripTextBox1 = (gcnew System::Windows::Forms::ToolStripTextBox());
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->panel2 = (gcnew System::Windows::Forms::Panel());
			this->tabControl1 = (gcnew System::Windows::Forms::TabControl());
			this->FFTab = (gcnew System::Windows::Forms::TabPage());
			this->ffUseCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->FFGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->paramPanel = (gcnew System::Windows::Forms::FlowLayoutPanel());
			this->listView_display = (gcnew System::Windows::Forms::ListView());
			this->columnHeader16 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader17 = (gcnew System::Windows::Forms::ColumnHeader());
			this->freezeFFCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->useModelFFButton = (gcnew System::Windows::Forms::Button());
			this->slowModelGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->warningLabel = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->genRangeBox = (gcnew System::Windows::Forms::GroupBox());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->startGen = (gcnew System::Windows::Forms::TextBox());
			this->endGen = (gcnew System::Windows::Forms::TextBox());
			this->consGroupBox = (gcnew System::Windows::Forms::GroupBox());
			this->constraints = (gcnew System::Windows::Forms::CheckBox());
			this->fitRange = (gcnew System::Windows::Forms::Button());
			this->EDAreaGroup = (gcnew System::Windows::Forms::GroupBox());
			this->AreaText = (gcnew System::Windows::Forms::TextBox());
			this->previewBox = (gcnew System::Windows::Forms::GroupBox());
			this->oglPanel = (gcnew System::Windows::Forms::Panel());
			this->listViewFF = (gcnew System::Windows::Forms::ListView());
			this->ParameterCol = (gcnew System::Windows::Forms::ColumnHeader());
			this->paramLabel = (gcnew System::Windows::Forms::Label());
			this->globalParamtersGroupBox = (gcnew System::Windows::Forms::GroupBox());
			this->paramBox = (gcnew System::Windows::Forms::ComboBox());
			this->minim = (gcnew System::Windows::Forms::Label());
			this->maxim = (gcnew System::Windows::Forms::Label());
			this->exmax = (gcnew System::Windows::Forms::TextBox());
			this->exmin = (gcnew System::Windows::Forms::TextBox());
			this->infExtraParam = (gcnew System::Windows::Forms::CheckBox());
			this->listView_Extraparams = (gcnew System::Windows::Forms::ListView());
			this->Param = (gcnew System::Windows::Forms::ColumnHeader());
			this->Value = (gcnew System::Windows::Forms::ColumnHeader());
			this->edpBox = (gcnew System::Windows::Forms::GroupBox());
			this->removeLayer = (gcnew System::Windows::Forms::Button());
			this->addLayer = (gcnew System::Windows::Forms::Button());
			this->SFTab = (gcnew System::Windows::Forms::TabPage());
			this->sfUseCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->phasefitter = (gcnew System::Windows::Forms::GroupBox());
			this->order = (gcnew System::Windows::Forms::ComboBox());
			this->Volume = (gcnew System::Windows::Forms::TextBox());
			this->phaseErrorTextBox = (gcnew System::Windows::Forms::TextBox());
			this->Vollabel = (gcnew System::Windows::Forms::Label());
			this->phaseErrorLabel = (gcnew System::Windows::Forms::Label());
			this->Caille_button = (gcnew System::Windows::Forms::Button());
			this->undoPhases = (gcnew System::Windows::Forms::Button());
			this->clearPositionsButton = (gcnew System::Windows::Forms::Button());
			this->listView_PeakPosition = (gcnew System::Windows::Forms::ListView());
			this->columnHeader9 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader8 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader14 = (gcnew System::Windows::Forms::ColumnHeader());
			this->phasesParamsGroupBox = (gcnew System::Windows::Forms::GroupBox());
			this->MutPhases = (gcnew System::Windows::Forms::CheckBox());
			this->label21 = (gcnew System::Windows::Forms::Label());
			this->ValPhases = (gcnew System::Windows::Forms::TextBox());
			this->comboPhases = (gcnew System::Windows::Forms::ComboBox());
			this->label22 = (gcnew System::Windows::Forms::Label());
			this->label23 = (gcnew System::Windows::Forms::Label());
			this->MaxPhases = (gcnew System::Windows::Forms::TextBox());
			this->MinPhases = (gcnew System::Windows::Forms::TextBox());
			this->fitphase = (gcnew System::Windows::Forms::Button());
			this->label10 = (gcnew System::Windows::Forms::Label());
			this->listView_phases = (gcnew System::Windows::Forms::ListView());
			this->columnHeader10 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader13 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader12 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader11 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader15 = (gcnew System::Windows::Forms::ColumnHeader());
			this->recipColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->Peakfitter = (gcnew System::Windows::Forms::GroupBox());
			this->showCailleButton = (gcnew System::Windows::Forms::Button());
			this->PeakShapeGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->peakfit = (gcnew System::Windows::Forms::ComboBox());
			this->SortButton = (gcnew System::Windows::Forms::Button());
			this->automaticPeakFinderButton = (gcnew System::Windows::Forms::Button());
			this->Move2Phases = (gcnew System::Windows::Forms::Button());
			this->thresholdBox2 = (gcnew System::Windows::Forms::TextBox());
			this->Threshold_label2 = (gcnew System::Windows::Forms::Label());
			this->thresholdBox1 = (gcnew System::Windows::Forms::TextBox());
			this->Threshold_label1 = (gcnew System::Windows::Forms::Label());
			this->PeakPicker = (gcnew System::Windows::Forms::Button());
			this->removePeak = (gcnew System::Windows::Forms::Button());
			this->addPeak = (gcnew System::Windows::Forms::Button());
			this->centerTrack = (gcnew System::Windows::Forms::TrackBar());
			this->centerMut = (gcnew System::Windows::Forms::CheckBox());
			this->centerBox = (gcnew System::Windows::Forms::TextBox());
			this->label11 = (gcnew System::Windows::Forms::Label());
			this->fwhmMut = (gcnew System::Windows::Forms::CheckBox());
			this->ampMut = (gcnew System::Windows::Forms::CheckBox());
			this->fwhmBox = (gcnew System::Windows::Forms::TextBox());
			this->label12 = (gcnew System::Windows::Forms::Label());
			this->ampBox = (gcnew System::Windows::Forms::TextBox());
			this->label13 = (gcnew System::Windows::Forms::Label());
			this->label14 = (gcnew System::Windows::Forms::Label());
			this->label15 = (gcnew System::Windows::Forms::Label());
			this->listView_peaks = (gcnew System::Windows::Forms::ListView());
			this->columnHeader1 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader2 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader3 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader4 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader5 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader6 = (gcnew System::Windows::Forms::ColumnHeader());
			this->columnHeader7 = (gcnew System::Windows::Forms::ColumnHeader());
			this->fwhmTrack = (gcnew System::Windows::Forms::TrackBar());
			this->ampTrack = (gcnew System::Windows::Forms::TrackBar());
			this->cailleGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->PeakFinderCailleButton = (gcnew System::Windows::Forms::Button());
			this->cailleN_diffusedCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->cailleSigCheckbox = (gcnew System::Windows::Forms::CheckBox());
			this->cailleNCheckbox = (gcnew System::Windows::Forms::CheckBox());
			this->cailleEtaCheckbox = (gcnew System::Windows::Forms::CheckBox());
			this->cailleAmpCheckbox = (gcnew System::Windows::Forms::CheckBox());
			this->mutLabel = (gcnew System::Windows::Forms::Label());
			this->returnToLineshapeButton = (gcnew System::Windows::Forms::Button());
			this->cailleParamListView = (gcnew System::Windows::Forms::ListView());
			this->cailleParamColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->cailleValuecolumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->cailleMinColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->cailleMaxColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->caillePeaksListView = (gcnew System::Windows::Forms::ListView());
			this->hColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->cailleCenterColumnHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->cailleMaxGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->cailleMaxTextBox = (gcnew System::Windows::Forms::TextBox());
			this->cailleMinGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->cailleMinTextBox = (gcnew System::Windows::Forms::TextBox());
			this->cailleValueGroupbox = (gcnew System::Windows::Forms::GroupBox());
			this->cailleValueTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->cailleValueTextBox = (gcnew System::Windows::Forms::TextBox());
			this->BGTab = (gcnew System::Windows::Forms::TabPage());
			this->bgUseCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->label26 = (gcnew System::Windows::Forms::Label());
			this->manipBox = (gcnew System::Windows::Forms::GroupBox());
			this->fitToBaseline = (gcnew System::Windows::Forms::Button());
			this->smooth = (gcnew System::Windows::Forms::Button());
			this->baseline = (gcnew System::Windows::Forms::Button());
			this->zeroBG = (gcnew System::Windows::Forms::Button());
			this->label25 = (gcnew System::Windows::Forms::Label());
			this->label24 = (gcnew System::Windows::Forms::Label());
			this->DEBUG_label = (gcnew System::Windows::Forms::Label());
			this->functionsGroupBox = (gcnew System::Windows::Forms::GroupBox());
			this->maxLabel = (gcnew System::Windows::Forms::Label());
			this->minLabel = (gcnew System::Windows::Forms::Label());
			this->baseLabel = (gcnew System::Windows::Forms::Label());
			this->baseMaxBox = (gcnew System::Windows::Forms::TextBox());
			this->baseMinBox = (gcnew System::Windows::Forms::TextBox());
			this->baseTextbox = (gcnew System::Windows::Forms::TextBox());
			this->funcTypeBox = (gcnew System::Windows::Forms::GroupBox());
			this->funcTypeList = (gcnew System::Windows::Forms::ComboBox());
			this->baseMut = (gcnew System::Windows::Forms::CheckBox());
			this->xCenterLabel = (gcnew System::Windows::Forms::Label());
			this->xCenterMut = (gcnew System::Windows::Forms::CheckBox());
			this->xcMaxBox = (gcnew System::Windows::Forms::TextBox());
			this->xcMinBox = (gcnew System::Windows::Forms::TextBox());
			this->decMaxBox = (gcnew System::Windows::Forms::TextBox());
			this->xCenterTextbox = (gcnew System::Windows::Forms::TextBox());
			this->decMinBox = (gcnew System::Windows::Forms::TextBox());
			this->decayMut = (gcnew System::Windows::Forms::CheckBox());
			this->decayTextbox = (gcnew System::Windows::Forms::TextBox());
			this->decayLabel = (gcnew System::Windows::Forms::Label());
			this->removeFuncButton = (gcnew System::Windows::Forms::Button());
			this->addFuncButton = (gcnew System::Windows::Forms::Button());
			this->BGListview = (gcnew System::Windows::Forms::ListView());
			this->funcNumHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->funcHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->BaseHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->BaseMutHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->DecHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->DecMutHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->xCenterHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->xCenterMutHeader = (gcnew System::Windows::Forms::ColumnHeader());
			this->baseTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->decayTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->xCenterTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->movetophases = (gcnew System::Windows::Forms::Panel());
			this->changeModel = (gcnew System::Windows::Forms::Button());
			this->maskButton = (gcnew System::Windows::Forms::Button());
			this->rsquared = (gcnew System::Windows::Forms::Label());
			this->LocOnGraph = (gcnew System::Windows::Forms::Label());
			this->logXCheckBox = (gcnew System::Windows::Forms::CheckBox());
			this->logScale = (gcnew System::Windows::Forms::CheckBox());
			this->changeData = (gcnew System::Windows::Forms::Button());
			this->save = (gcnew System::Windows::Forms::Button());
			this->wssr = (gcnew System::Windows::Forms::Label());
			this->groupBox1 = (gcnew System::Windows::Forms::GroupBox());
			this->maskToolStrip = (gcnew System::Windows::Forms::ToolStrip());
			this->addMaskButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->removeMaskButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->invertMaskButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->clearMaskButton = (gcnew System::Windows::Forms::ToolStripButton());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->panel3 = (gcnew System::Windows::Forms::Panel());
			this->CalcComboBox = (gcnew System::Windows::Forms::ComboBox());
			this->iterProgressBar = (gcnew System::Windows::Forms::ProgressBar());
			this->progressBar1 = (gcnew System::Windows::Forms::ProgressBar());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->reportButton = (gcnew System::Windows::Forms::Button());
			this->undo = (gcnew System::Windows::Forms::Button());
			this->calculate = (gcnew System::Windows::Forms::Button());
			this->timer1 = (gcnew System::Windows::Forms::Timer(this->components));
			this->sfd = (gcnew System::Windows::Forms::SaveFileDialog());
			this->ofd = (gcnew System::Windows::Forms::OpenFileDialog());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->button2 = (gcnew System::Windows::Forms::Button());
			this->scrollerTimer = (gcnew System::Windows::Forms::Timer(this->components));
			this->polydiToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->uniformPDToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->gaussianPDToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->lorentzianPDToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->menuStrip1->SuspendLayout();
			this->tableLayoutPanel1->SuspendLayout();
			this->panel2->SuspendLayout();
			this->tabControl1->SuspendLayout();
			this->FFTab->SuspendLayout();
			this->FFGroupbox->SuspendLayout();
			this->slowModelGroupbox->SuspendLayout();
			this->genRangeBox->SuspendLayout();
			this->consGroupBox->SuspendLayout();
			this->EDAreaGroup->SuspendLayout();
			this->previewBox->SuspendLayout();
			this->globalParamtersGroupBox->SuspendLayout();
			this->SFTab->SuspendLayout();
			this->phasefitter->SuspendLayout();
			this->phasesParamsGroupBox->SuspendLayout();
			this->Peakfitter->SuspendLayout();
			this->PeakShapeGroupbox->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->centerTrack))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->fwhmTrack))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->ampTrack))->BeginInit();
			this->cailleGroupbox->SuspendLayout();
			this->cailleMaxGroupbox->SuspendLayout();
			this->cailleMinGroupbox->SuspendLayout();
			this->cailleValueGroupbox->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->cailleValueTrackBar))->BeginInit();
			this->BGTab->SuspendLayout();
			this->manipBox->SuspendLayout();
			this->functionsGroupBox->SuspendLayout();
			this->funcTypeBox->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->baseTrackBar))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->decayTrackBar))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->xCenterTrackBar))->BeginInit();
			this->movetophases->SuspendLayout();
			this->groupBox1->SuspendLayout();
			this->maskToolStrip->SuspendLayout();
			this->panel3->SuspendLayout();
			this->SuspendLayout();
			// 
			// menuStrip1
			// 
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(6) {this->exportToolStripMenuItem, 
				this->generationToolStripMenuItem, this->exportToolStripMenuItem1, this->settingsToolStripMenuItem, this->optionsToolStripMenuItem, 
				this->integrationToolStripMenuItem});
			this->menuStrip1->Location = System::Drawing::Point(0, 0);
			this->menuStrip1->Name = L"menuStrip1";
			this->menuStrip1->RenderMode = System::Windows::Forms::ToolStripRenderMode::System;
			this->menuStrip1->Size = System::Drawing::Size(1157, 24);
			this->menuStrip1->TabIndex = 0;
			this->menuStrip1->Text = L"menuStrip1";
			// 
			// exportToolStripMenuItem
			// 
			this->exportToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(11) {this->importParametersToolStripMenuItem, 
				this->importBaselineToolStripMenuItem, this->saveParametersToolStripMenuItem, this->saveParametersAsToolStripMenuItem, this->loadFileAsFormFactorToolStripMenuItem, 
				this->toolStripSeparator2, this->plotElectronDensityProfileToolStripMenuItem, this->plotFittingResultsToolStripMenuItem, this->toolStripSeparator5, 
				this->pauseFittingToolStripMenuItem, this->fittingEmergencyStopToolStripMenuItem});
			this->exportToolStripMenuItem->Name = L"exportToolStripMenuItem";
			this->exportToolStripMenuItem->Size = System::Drawing::Size(37, 20);
			this->exportToolStripMenuItem->Text = L"File";
			// 
			// importParametersToolStripMenuItem
			// 
			this->importParametersToolStripMenuItem->Name = L"importParametersToolStripMenuItem";
			this->importParametersToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->importParametersToolStripMenuItem->Text = L"Import Parameters...";
			this->importParametersToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::importParametersToolStripMenuItem_Click);
			// 
			// importBaselineToolStripMenuItem
			// 
			this->importBaselineToolStripMenuItem->Name = L"importBaselineToolStripMenuItem";
			this->importBaselineToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->importBaselineToolStripMenuItem->Text = L"Import Baseline...";
			this->importBaselineToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::importBaselineToolStripMenuItem_Click);
			// 
			// saveParametersToolStripMenuItem
			// 
			this->saveParametersToolStripMenuItem->Name = L"saveParametersToolStripMenuItem";
			this->saveParametersToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->saveParametersToolStripMenuItem->Text = L"Save Parameters";
			this->saveParametersToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::save_Click);
			// 
			// saveParametersAsToolStripMenuItem
			// 
			this->saveParametersAsToolStripMenuItem->Name = L"saveParametersAsToolStripMenuItem";
			this->saveParametersAsToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->saveParametersAsToolStripMenuItem->Text = L"Save Parameters As...";
			this->saveParametersAsToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::save_Click);
			// 
			// loadFileAsFormFactorToolStripMenuItem
			// 
			this->loadFileAsFormFactorToolStripMenuItem->Name = L"loadFileAsFormFactorToolStripMenuItem";
			this->loadFileAsFormFactorToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->loadFileAsFormFactorToolStripMenuItem->Text = L"Load File As Form Factor...";
			this->loadFileAsFormFactorToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::loadFileAsFormFactorToolStripMenuItem_Click);
			// 
			// toolStripSeparator2
			// 
			this->toolStripSeparator2->Name = L"toolStripSeparator2";
			this->toolStripSeparator2->Size = System::Drawing::Size(226, 6);
			// 
			// plotElectronDensityProfileToolStripMenuItem
			// 
			this->plotElectronDensityProfileToolStripMenuItem->Name = L"plotElectronDensityProfileToolStripMenuItem";
			this->plotElectronDensityProfileToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->plotElectronDensityProfileToolStripMenuItem->Text = L"Plot Electron Density Profile...";
			this->plotElectronDensityProfileToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::plotElectronDensityProfileToolStripMenuItem_Click);
			// 
			// plotFittingResultsToolStripMenuItem
			// 
			this->plotFittingResultsToolStripMenuItem->Enabled = false;
			this->plotFittingResultsToolStripMenuItem->Name = L"plotFittingResultsToolStripMenuItem";
			this->plotFittingResultsToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->plotFittingResultsToolStripMenuItem->Text = L"Plot Fitting Results...";
			this->plotFittingResultsToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::plotFittingResultsToolStripMenuItem_Click);
			// 
			// toolStripSeparator5
			// 
			this->toolStripSeparator5->Name = L"toolStripSeparator5";
			this->toolStripSeparator5->Size = System::Drawing::Size(226, 6);
			// 
			// pauseFittingToolStripMenuItem
			// 
			this->pauseFittingToolStripMenuItem->Enabled = false;
			this->pauseFittingToolStripMenuItem->Name = L"pauseFittingToolStripMenuItem";
			this->pauseFittingToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->pauseFittingToolStripMenuItem->Text = L"Pause Fitting";
			this->pauseFittingToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::pauseFittingToolStripMenuItem_Click);
			// 
			// fittingEmergencyStopToolStripMenuItem
			// 
			this->fittingEmergencyStopToolStripMenuItem->Enabled = false;
			this->fittingEmergencyStopToolStripMenuItem->Name = L"fittingEmergencyStopToolStripMenuItem";
			this->fittingEmergencyStopToolStripMenuItem->Size = System::Drawing::Size(229, 22);
			this->fittingEmergencyStopToolStripMenuItem->Text = L"Fitting Emergency Stop";
			this->fittingEmergencyStopToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::fittingEmergencyStopToolStripMenuItem_Click);
			// 
			// generationToolStripMenuItem
			// 
			this->generationToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->gridResolutionToolStripMenuItem});
			this->generationToolStripMenuItem->Name = L"generationToolStripMenuItem";
			this->generationToolStripMenuItem->Size = System::Drawing::Size(77, 20);
			this->generationToolStripMenuItem->Text = L"Generation";
			this->generationToolStripMenuItem->Visible = false;
			// 
			// gridResolutionToolStripMenuItem
			// 
			this->gridResolutionToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->toolStripTextBox2});
			this->gridResolutionToolStripMenuItem->Name = L"gridResolutionToolStripMenuItem";
			this->gridResolutionToolStripMenuItem->Size = System::Drawing::Size(155, 22);
			this->gridResolutionToolStripMenuItem->Text = L"Grid Resolution";
			// 
			// toolStripTextBox2
			// 
			this->toolStripTextBox2->Name = L"toolStripTextBox2";
			this->toolStripTextBox2->Size = System::Drawing::Size(100, 23);
			this->toolStripTextBox2->Text = L"500";
			// 
			// exportToolStripMenuItem1
			// 
			this->exportToolStripMenuItem1->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(10) {this->exportSignalToolStripMenuItem, 
				this->exportModelToolStripMenuItem1, this->exportBackgroundToolStripMenuItem, this->exportGraphToolStripMenuItem, this->exportElectronDensityProfileToolStripMenuItem1, 
				this->exportFormFactorToolStripMenuItem, this->exportStructureFactorToolStripMenuItem, this->exportDecomposedToolStripMenuItem, 
				this->exportSigModBLToolStripMenuItem, this->exportAllParametersAsCSVFileToolStripMenuItem});
			this->exportToolStripMenuItem1->Name = L"exportToolStripMenuItem1";
			this->exportToolStripMenuItem1->Size = System::Drawing::Size(52, 20);
			this->exportToolStripMenuItem1->Text = L"Export";
			// 
			// exportSignalToolStripMenuItem
			// 
			this->exportSignalToolStripMenuItem->Enabled = false;
			this->exportSignalToolStripMenuItem->Name = L"exportSignalToolStripMenuItem";
			this->exportSignalToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportSignalToolStripMenuItem->Text = L"Export Signal...";
			this->exportSignalToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportModelToolStripMenuItem1
			// 
			this->exportModelToolStripMenuItem1->Enabled = false;
			this->exportModelToolStripMenuItem1->Name = L"exportModelToolStripMenuItem1";
			this->exportModelToolStripMenuItem1->Size = System::Drawing::Size(249, 22);
			this->exportModelToolStripMenuItem1->Text = L"Export Model...";
			this->exportModelToolStripMenuItem1->Click += gcnew System::EventHandler(this, &FormFactor::exportModelToolStripMenuItem1_Click);
			// 
			// exportBackgroundToolStripMenuItem
			// 
			this->exportBackgroundToolStripMenuItem->Enabled = false;
			this->exportBackgroundToolStripMenuItem->Name = L"exportBackgroundToolStripMenuItem";
			this->exportBackgroundToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportBackgroundToolStripMenuItem->Text = L"Export Background...";
			this->exportBackgroundToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportGraphToolStripMenuItem
			// 
			this->exportGraphToolStripMenuItem->Enabled = false;
			this->exportGraphToolStripMenuItem->Name = L"exportGraphToolStripMenuItem";
			this->exportGraphToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportGraphToolStripMenuItem->Text = L"Export Graph...";
			this->exportGraphToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportGraphToolStripMenuItem_Click);
			// 
			// exportElectronDensityProfileToolStripMenuItem1
			// 
			this->exportElectronDensityProfileToolStripMenuItem1->Name = L"exportElectronDensityProfileToolStripMenuItem1";
			this->exportElectronDensityProfileToolStripMenuItem1->Size = System::Drawing::Size(249, 22);
			this->exportElectronDensityProfileToolStripMenuItem1->Text = L"Export Electron Density Profile...";
			this->exportElectronDensityProfileToolStripMenuItem1->Click += gcnew System::EventHandler(this, &FormFactor::exportElectronDensityProfileToolStripMenuItem1_Click);
			// 
			// exportFormFactorToolStripMenuItem
			// 
			this->exportFormFactorToolStripMenuItem->Enabled = false;
			this->exportFormFactorToolStripMenuItem->Name = L"exportFormFactorToolStripMenuItem";
			this->exportFormFactorToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportFormFactorToolStripMenuItem->Text = L"Export Form Factor...";
			this->exportFormFactorToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportStructureFactorToolStripMenuItem
			// 
			this->exportStructureFactorToolStripMenuItem->Enabled = false;
			this->exportStructureFactorToolStripMenuItem->Name = L"exportStructureFactorToolStripMenuItem";
			this->exportStructureFactorToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportStructureFactorToolStripMenuItem->Text = L"Export Structure Factor...";
			this->exportStructureFactorToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportDecomposedToolStripMenuItem
			// 
			this->exportDecomposedToolStripMenuItem->Enabled = false;
			this->exportDecomposedToolStripMenuItem->Name = L"exportDecomposedToolStripMenuItem";
			this->exportDecomposedToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportDecomposedToolStripMenuItem->Text = L"Export Decomposed Signal...";
			this->exportDecomposedToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportSigModBLToolStripMenuItem
			// 
			this->exportSigModBLToolStripMenuItem->Enabled = false;
			this->exportSigModBLToolStripMenuItem->Name = L"exportSigModBLToolStripMenuItem";
			this->exportSigModBLToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportSigModBLToolStripMenuItem->Text = L"Export Sig, Mod, BL...";
			this->exportSigModBLToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportDataFileToolStripMenuItem_Click);
			// 
			// exportAllParametersAsCSVFileToolStripMenuItem
			// 
			this->exportAllParametersAsCSVFileToolStripMenuItem->Name = L"exportAllParametersAsCSVFileToolStripMenuItem";
			this->exportAllParametersAsCSVFileToolStripMenuItem->Size = System::Drawing::Size(249, 22);
			this->exportAllParametersAsCSVFileToolStripMenuItem->Text = L"Export all parameters as TSV file...";
			this->exportAllParametersAsCSVFileToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::exportAllParametersAsCSVFileToolStripMenuItem_Click);
			// 
			// settingsToolStripMenuItem
			// 
			this->settingsToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {this->dragToZoomToolStripMenuItem, 
				this->logarToolStripMenuItem, this->liveRefreshToolStripMenuItem, this->liveFittingToolStripMenuItem, this->a1nm1ToolStripMenuItem});
			this->settingsToolStripMenuItem->Name = L"settingsToolStripMenuItem";
			this->settingsToolStripMenuItem->Size = System::Drawing::Size(61, 20);
			this->settingsToolStripMenuItem->Text = L"Settings";
			this->settingsToolStripMenuItem->DropDownOpening += gcnew System::EventHandler(this, &FormFactor::settingsToolStripMenuItem_Click);
			// 
			// dragToZoomToolStripMenuItem
			// 
			this->dragToZoomToolStripMenuItem->CheckOnClick = true;
			this->dragToZoomToolStripMenuItem->Name = L"dragToZoomToolStripMenuItem";
			this->dragToZoomToolStripMenuItem->Size = System::Drawing::Size(215, 22);
			this->dragToZoomToolStripMenuItem->Text = L"Drag to Zoom";
			this->dragToZoomToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::dragToZoomToolStripMenuItem_Click);
			// 
			// logarToolStripMenuItem
			// 
			this->logarToolStripMenuItem->CheckOnClick = true;
			this->logarToolStripMenuItem->Name = L"logarToolStripMenuItem";
			this->logarToolStripMenuItem->Size = System::Drawing::Size(215, 22);
			this->logarToolStripMenuItem->Text = L"Logarithmic Scale";
			this->logarToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::logscaleToolStripMenuItem_Click);
			// 
			// liveRefreshToolStripMenuItem
			// 
			this->liveRefreshToolStripMenuItem->Checked = true;
			this->liveRefreshToolStripMenuItem->CheckOnClick = true;
			this->liveRefreshToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->liveRefreshToolStripMenuItem->Name = L"liveRefreshToolStripMenuItem";
			this->liveRefreshToolStripMenuItem->Size = System::Drawing::Size(215, 22);
			this->liveRefreshToolStripMenuItem->Text = L"Live Refresh";
			// 
			// liveFittingToolStripMenuItem
			// 
			this->liveFittingToolStripMenuItem->Checked = true;
			this->liveFittingToolStripMenuItem->CheckOnClick = true;
			this->liveFittingToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->liveFittingToolStripMenuItem->Name = L"liveFittingToolStripMenuItem";
			this->liveFittingToolStripMenuItem->Size = System::Drawing::Size(215, 22);
			this->liveFittingToolStripMenuItem->Text = L"Update Graph while Fitting";
			// 
			// a1nm1ToolStripMenuItem
			// 
			this->a1nm1ToolStripMenuItem->CheckOnClick = true;
			this->a1nm1ToolStripMenuItem->Name = L"a1nm1ToolStripMenuItem";
			this->a1nm1ToolStripMenuItem->Size = System::Drawing::Size(215, 22);
			this->a1nm1ToolStripMenuItem->Text = L"(A^-1) --> (nm^-1)";
			this->a1nm1ToolStripMenuItem->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::a1nm1ToolStripMenuItem_CheckedChanged);
			// 
			// optionsToolStripMenuItem
			// 
			this->optionsToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(13) {this->relatedModelsToolStripMenuItem, 
				this->electronDensityProfileToolStripMenuItem, this->toolStripSeparator1, this->accurateDerivativeToolStripMenuItem, this->accurateFittingToolStripMenuItem, 
				this->chiSquaredBasedFittingToolStripMenuItem, this->logScaledFittingParamToolStripMenuItem, this->fittingMethodToolStripMenuItem, 
				this->computingToolStripMenuItem, this->minimumSignalToolStripMenuItem, this->ExperimentResToolStripMenuItem, this->sigmaFWHMToolStripMenuItem, 
				this->polydiToolStripMenuItem});
			this->optionsToolStripMenuItem->Name = L"optionsToolStripMenuItem";
			this->optionsToolStripMenuItem->Size = System::Drawing::Size(61, 20);
			this->optionsToolStripMenuItem->Text = L"Options";
			// 
			// relatedModelsToolStripMenuItem
			// 
			this->relatedModelsToolStripMenuItem->Name = L"relatedModelsToolStripMenuItem";
			this->relatedModelsToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->relatedModelsToolStripMenuItem->Text = L"Related Models";
			// 
			// electronDensityProfileToolStripMenuItem
			// 
			this->electronDensityProfileToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {this->discreteStepsToolStripMenuItem, 
				this->gaussiansToolStripMenuItem, this->hyperbolictangentSmoothStepsToolStripMenuItem, this->toolStripSeparator6, this->stepResolutionToolStripMenuItem});
			this->electronDensityProfileToolStripMenuItem->Name = L"electronDensityProfileToolStripMenuItem";
			this->electronDensityProfileToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->electronDensityProfileToolStripMenuItem->Text = L"Electron Density Profile";
			// 
			// discreteStepsToolStripMenuItem
			// 
			this->discreteStepsToolStripMenuItem->Checked = true;
			this->discreteStepsToolStripMenuItem->CheckOnClick = true;
			this->discreteStepsToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->discreteStepsToolStripMenuItem->Name = L"discreteStepsToolStripMenuItem";
			this->discreteStepsToolStripMenuItem->Size = System::Drawing::Size(254, 22);
			this->discreteStepsToolStripMenuItem->Text = L"Discrete Steps";
			this->discreteStepsToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::discreteStepsToolStripMenuItem_Click);
			// 
			// gaussiansToolStripMenuItem
			// 
			this->gaussiansToolStripMenuItem->CheckOnClick = true;
			this->gaussiansToolStripMenuItem->Name = L"gaussiansToolStripMenuItem";
			this->gaussiansToolStripMenuItem->Size = System::Drawing::Size(254, 22);
			this->gaussiansToolStripMenuItem->Text = L"Gaussians";
			this->gaussiansToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::gaussiansToolStripMenuItem_Click);
			// 
			// hyperbolictangentSmoothStepsToolStripMenuItem
			// 
			this->hyperbolictangentSmoothStepsToolStripMenuItem->CheckOnClick = true;
			this->hyperbolictangentSmoothStepsToolStripMenuItem->Name = L"hyperbolictangentSmoothStepsToolStripMenuItem";
			this->hyperbolictangentSmoothStepsToolStripMenuItem->Size = System::Drawing::Size(254, 22);
			this->hyperbolictangentSmoothStepsToolStripMenuItem->Text = L"Hyperbolic-tangent Smooth Steps";
			this->hyperbolictangentSmoothStepsToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::hyperbolictangentSmoothStepsToolStripMenuItem_Click);
			// 
			// toolStripSeparator6
			// 
			this->toolStripSeparator6->Name = L"toolStripSeparator6";
			this->toolStripSeparator6->Size = System::Drawing::Size(251, 6);
			// 
			// stepResolutionToolStripMenuItem
			// 
			this->stepResolutionToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->edpResolution, 
				this->toolStripSeparator7, this->adaptiveToolStripMenuItem});
			this->stepResolutionToolStripMenuItem->Name = L"stepResolutionToolStripMenuItem";
			this->stepResolutionToolStripMenuItem->Size = System::Drawing::Size(254, 22);
			this->stepResolutionToolStripMenuItem->Text = L"Profile Resolution";
			// 
			// edpResolution
			// 
			this->edpResolution->Name = L"edpResolution";
			this->edpResolution->Size = System::Drawing::Size(100, 23);
			this->edpResolution->Text = L"157";
			this->edpResolution->LostFocus += gcnew System::EventHandler(this, &FormFactor::edpResolution_TextChanged);
			// 
			// toolStripSeparator7
			// 
			this->toolStripSeparator7->Name = L"toolStripSeparator7";
			this->toolStripSeparator7->Size = System::Drawing::Size(157, 6);
			// 
			// adaptiveToolStripMenuItem
			// 
			this->adaptiveToolStripMenuItem->CheckOnClick = true;
			this->adaptiveToolStripMenuItem->Name = L"adaptiveToolStripMenuItem";
			this->adaptiveToolStripMenuItem->Size = System::Drawing::Size(160, 22);
			this->adaptiveToolStripMenuItem->Text = L"Adaptive";
			this->adaptiveToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::adaptiveToolStripMenuItem_Click);
			// 
			// toolStripSeparator1
			// 
			this->toolStripSeparator1->Name = L"toolStripSeparator1";
			this->toolStripSeparator1->Size = System::Drawing::Size(223, 6);
			// 
			// accurateDerivativeToolStripMenuItem
			// 
			this->accurateDerivativeToolStripMenuItem->CheckOnClick = true;
			this->accurateDerivativeToolStripMenuItem->Name = L"accurateDerivativeToolStripMenuItem";
			this->accurateDerivativeToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->accurateDerivativeToolStripMenuItem->Text = L"Accurate Derivative";
			this->accurateDerivativeToolStripMenuItem->Visible = false;
			this->accurateDerivativeToolStripMenuItem->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::accurateDerivativeToolStripMenuItem_CheckedChanged);
			// 
			// accurateFittingToolStripMenuItem
			// 
			this->accurateFittingToolStripMenuItem->Checked = true;
			this->accurateFittingToolStripMenuItem->CheckOnClick = true;
			this->accurateFittingToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->accurateFittingToolStripMenuItem->Name = L"accurateFittingToolStripMenuItem";
			this->accurateFittingToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->accurateFittingToolStripMenuItem->Text = L"Accurate Fitting (SVD-based)";
			// 
			// chiSquaredBasedFittingToolStripMenuItem
			// 
			this->chiSquaredBasedFittingToolStripMenuItem->CheckOnClick = true;
			this->chiSquaredBasedFittingToolStripMenuItem->Name = L"chiSquaredBasedFittingToolStripMenuItem";
			this->chiSquaredBasedFittingToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->chiSquaredBasedFittingToolStripMenuItem->Text = L"Chi-Squared Based Fitting";
			// 
			// logScaledFittingParamToolStripMenuItem
			// 
			this->logScaledFittingParamToolStripMenuItem->CheckOnClick = true;
			this->logScaledFittingParamToolStripMenuItem->Name = L"logScaledFittingParamToolStripMenuItem";
			this->logScaledFittingParamToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->logScaledFittingParamToolStripMenuItem->Text = L"Log fitting parameter";
			this->logScaledFittingParamToolStripMenuItem->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::logScaledFittingParamToolStripMenuItem_CheckedChanged);
			// 
			// fittingMethodToolStripMenuItem
			// 
			this->fittingMethodToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->fittingToolStripMenuItem, 
				this->phaseFittingToolStripMenuItem});
			this->fittingMethodToolStripMenuItem->Name = L"fittingMethodToolStripMenuItem";
			this->fittingMethodToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->fittingMethodToolStripMenuItem->Text = L"Fitting Method";
			// 
			// fittingToolStripMenuItem
			// 
			this->fittingToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {this->levmar, 
				this->diffEvo, this->raindrop, this->toolStripSeparator3, this->iterationsToolStripMenuItem});
			this->fittingToolStripMenuItem->Name = L"fittingToolStripMenuItem";
			this->fittingToolStripMenuItem->Size = System::Drawing::Size(142, 22);
			this->fittingToolStripMenuItem->Text = L"Fitting";
			// 
			// levmar
			// 
			this->levmar->Checked = true;
			this->levmar->CheckOnClick = true;
			this->levmar->CheckState = System::Windows::Forms::CheckState::Checked;
			this->levmar->Name = L"levmar";
			this->levmar->Size = System::Drawing::Size(190, 22);
			this->levmar->Text = L"Levenberg-Marquardt";
			this->levmar->Click += gcnew System::EventHandler(this, &FormFactor::levmar_Click);
			// 
			// diffEvo
			// 
			this->diffEvo->Enabled = false;
			this->diffEvo->Name = L"diffEvo";
			this->diffEvo->Size = System::Drawing::Size(190, 22);
			this->diffEvo->Text = L"Differential Evolution";
			// 
			// raindrop
			// 
			this->raindrop->CheckOnClick = true;
			this->raindrop->Name = L"raindrop";
			this->raindrop->Size = System::Drawing::Size(190, 22);
			this->raindrop->Text = L"Raindrop Method";
			this->raindrop->Click += gcnew System::EventHandler(this, &FormFactor::raindrop_Click);
			// 
			// toolStripSeparator3
			// 
			this->toolStripSeparator3->Name = L"toolStripSeparator3";
			this->toolStripSeparator3->Size = System::Drawing::Size(187, 6);
			// 
			// iterationsToolStripMenuItem
			// 
			this->iterationsToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->fittingIter});
			this->iterationsToolStripMenuItem->Name = L"iterationsToolStripMenuItem";
			this->iterationsToolStripMenuItem->Size = System::Drawing::Size(190, 22);
			this->iterationsToolStripMenuItem->Text = L"Iterations";
			// 
			// fittingIter
			// 
			this->fittingIter->Name = L"fittingIter";
			this->fittingIter->Size = System::Drawing::Size(152, 23);
			this->fittingIter->Text = L"20";
			// 
			// phaseFittingToolStripMenuItem
			// 
			this->phaseFittingToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {this->pLevmar, 
				this->pDiffEvo, this->pRaindrop, this->toolStripSeparator4, this->iterationsToolStripMenuItem1});
			this->phaseFittingToolStripMenuItem->Name = L"phaseFittingToolStripMenuItem";
			this->phaseFittingToolStripMenuItem->Size = System::Drawing::Size(142, 22);
			this->phaseFittingToolStripMenuItem->Text = L"Phase Fitting";
			// 
			// pLevmar
			// 
			this->pLevmar->CheckOnClick = true;
			this->pLevmar->Name = L"pLevmar";
			this->pLevmar->Size = System::Drawing::Size(190, 22);
			this->pLevmar->Text = L"Levenberg-Marquardt";
			this->pLevmar->Click += gcnew System::EventHandler(this, &FormFactor::pLevmar_Click);
			// 
			// pDiffEvo
			// 
			this->pDiffEvo->Enabled = false;
			this->pDiffEvo->Name = L"pDiffEvo";
			this->pDiffEvo->Size = System::Drawing::Size(190, 22);
			this->pDiffEvo->Text = L"Differential Evolution";
			// 
			// pRaindrop
			// 
			this->pRaindrop->Checked = true;
			this->pRaindrop->CheckOnClick = true;
			this->pRaindrop->CheckState = System::Windows::Forms::CheckState::Checked;
			this->pRaindrop->Name = L"pRaindrop";
			this->pRaindrop->Size = System::Drawing::Size(190, 22);
			this->pRaindrop->Text = L"Raindrop Method";
			this->pRaindrop->Click += gcnew System::EventHandler(this, &FormFactor::pRaindrop_Click);
			// 
			// toolStripSeparator4
			// 
			this->toolStripSeparator4->Name = L"toolStripSeparator4";
			this->toolStripSeparator4->Size = System::Drawing::Size(187, 6);
			// 
			// iterationsToolStripMenuItem1
			// 
			this->iterationsToolStripMenuItem1->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->phaseIterations});
			this->iterationsToolStripMenuItem1->Name = L"iterationsToolStripMenuItem1";
			this->iterationsToolStripMenuItem1->Size = System::Drawing::Size(190, 22);
			this->iterationsToolStripMenuItem1->Text = L"Iterations";
			// 
			// phaseIterations
			// 
			this->phaseIterations->Name = L"phaseIterations";
			this->phaseIterations->Size = System::Drawing::Size(152, 23);
			this->phaseIterations->Text = L"200";
			// 
			// computingToolStripMenuItem
			// 
			this->computingToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->cPUToolStripMenuItem, 
				this->gPUToolStripMenuItem});
			this->computingToolStripMenuItem->Name = L"computingToolStripMenuItem";
			this->computingToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->computingToolStripMenuItem->Text = L"Computing Backend";
			// 
			// cPUToolStripMenuItem
			// 
			this->cPUToolStripMenuItem->Checked = true;
			this->cPUToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->cPUToolStripMenuItem->Name = L"cPUToolStripMenuItem";
			this->cPUToolStripMenuItem->Size = System::Drawing::Size(97, 22);
			this->cPUToolStripMenuItem->Text = L"CPU";
			this->cPUToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::cPUToolStripMenuItem_Click);
			// 
			// gPUToolStripMenuItem
			// 
			this->gPUToolStripMenuItem->Enabled = false;
			this->gPUToolStripMenuItem->Name = L"gPUToolStripMenuItem";
			this->gPUToolStripMenuItem->Size = System::Drawing::Size(97, 22);
			this->gPUToolStripMenuItem->Text = L"GPU";
			this->gPUToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::gPUToolStripMenuItem_Click);
			// 
			// minimumSignalToolStripMenuItem
			// 
			this->minimumSignalToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->minimumSignalTextbox});
			this->minimumSignalToolStripMenuItem->Name = L"minimumSignalToolStripMenuItem";
			this->minimumSignalToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->minimumSignalToolStripMenuItem->Text = L"Minimum Signal";
			this->minimumSignalToolStripMenuItem->DropDownClosed += gcnew System::EventHandler(this, &FormFactor::minimumSignalTextbox_Leave);
			this->minimumSignalToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::minimumSignalToolStripMenuItem_Click);
			// 
			// minimumSignalTextbox
			// 
			this->minimumSignalTextbox->Name = L"minimumSignalTextbox";
			this->minimumSignalTextbox->Size = System::Drawing::Size(152, 23);
			this->minimumSignalTextbox->Text = L"5.0";
			this->minimumSignalTextbox->Enter += gcnew System::EventHandler(this, &FormFactor::minimumSignalTextbox_Enter);
			// 
			// ExperimentResToolStripMenuItem
			// 
			this->ExperimentResToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->expResToolStripTextBox});
			this->ExperimentResToolStripMenuItem->Name = L"ExperimentResToolStripMenuItem";
			this->ExperimentResToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->ExperimentResToolStripMenuItem->Text = L"Experimental error";
			this->ExperimentResToolStripMenuItem->DropDownClosed += gcnew System::EventHandler(this, &FormFactor::expResToolStripTextBox_TextChanged);
			// 
			// expResToolStripTextBox
			// 
			this->expResToolStripTextBox->Name = L"expResToolStripTextBox";
			this->expResToolStripTextBox->Size = System::Drawing::Size(152, 23);
			this->expResToolStripTextBox->Text = L"0.0";
			this->expResToolStripTextBox->Enter += gcnew System::EventHandler(this, &FormFactor::expResToolStripTextBox_Enter);
			// 
			// sigmaFWHMToolStripMenuItem
			// 
			this->sigmaFWHMToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->sigmaToolStripMenuItem, 
				this->fWHMToolStripMenuItem});
			this->sigmaFWHMToolStripMenuItem->Name = L"sigmaFWHMToolStripMenuItem";
			this->sigmaFWHMToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->sigmaFWHMToolStripMenuItem->Text = L"Sigma/FWHM";
			// 
			// sigmaToolStripMenuItem
			// 
			this->sigmaToolStripMenuItem->Checked = true;
			this->sigmaToolStripMenuItem->CheckOnClick = true;
			this->sigmaToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->sigmaToolStripMenuItem->Name = L"sigmaToolStripMenuItem";
			this->sigmaToolStripMenuItem->Size = System::Drawing::Size(111, 22);
			this->sigmaToolStripMenuItem->Text = L"Sigma";
			this->sigmaToolStripMenuItem->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::sigmaToolStripMenuItem_CheckedChanged);
			// 
			// fWHMToolStripMenuItem
			// 
			this->fWHMToolStripMenuItem->CheckOnClick = true;
			this->fWHMToolStripMenuItem->Name = L"fWHMToolStripMenuItem";
			this->fWHMToolStripMenuItem->Size = System::Drawing::Size(111, 22);
			this->fWHMToolStripMenuItem->Text = L"FWHM";
			this->fWHMToolStripMenuItem->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::sigmaToolStripMenuItem_CheckedChanged);
			// 
			// integrationToolStripMenuItem
			// 
			this->integrationToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {this->integrationTypeToolStripMenuItem, 
				this->quadratureResolutionToolStripMenuItem});
			this->integrationToolStripMenuItem->Name = L"integrationToolStripMenuItem";
			this->integrationToolStripMenuItem->Size = System::Drawing::Size(77, 20);
			this->integrationToolStripMenuItem->Text = L"Integration";
			this->integrationToolStripMenuItem->Visible = false;
			// 
			// integrationTypeToolStripMenuItem
			// 
			this->integrationTypeToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->gaussLegendreToolStripMenuItem, 
				this->monteCarloToolStripMenuItem, this->simpsonsRuleToolStripMenuItem});
			this->integrationTypeToolStripMenuItem->Name = L"integrationTypeToolStripMenuItem";
			this->integrationTypeToolStripMenuItem->Size = System::Drawing::Size(193, 22);
			this->integrationTypeToolStripMenuItem->Text = L"Integration Method";
			// 
			// gaussLegendreToolStripMenuItem
			// 
			this->gaussLegendreToolStripMenuItem->Checked = true;
			this->gaussLegendreToolStripMenuItem->CheckOnClick = true;
			this->gaussLegendreToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->gaussLegendreToolStripMenuItem->Name = L"gaussLegendreToolStripMenuItem";
			this->gaussLegendreToolStripMenuItem->Size = System::Drawing::Size(159, 22);
			this->gaussLegendreToolStripMenuItem->Text = L"Gauss-Legendre";
			this->gaussLegendreToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::gaussLegendreToolStripMenuItem_Click);
			// 
			// monteCarloToolStripMenuItem
			// 
			this->monteCarloToolStripMenuItem->CheckOnClick = true;
			this->monteCarloToolStripMenuItem->Name = L"monteCarloToolStripMenuItem";
			this->monteCarloToolStripMenuItem->Size = System::Drawing::Size(159, 22);
			this->monteCarloToolStripMenuItem->Text = L"Monte-Carlo";
			this->monteCarloToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::monteCarloToolStripMenuItem_Click);
			// 
			// simpsonsRuleToolStripMenuItem
			// 
			this->simpsonsRuleToolStripMenuItem->CheckOnClick = true;
			this->simpsonsRuleToolStripMenuItem->Name = L"simpsonsRuleToolStripMenuItem";
			this->simpsonsRuleToolStripMenuItem->Size = System::Drawing::Size(159, 22);
			this->simpsonsRuleToolStripMenuItem->Text = L"Simpson\'s Rule";
			this->simpsonsRuleToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::simpsonsRuleToolStripMenuItem_Click);
			// 
			// quadratureResolutionToolStripMenuItem
			// 
			this->quadratureResolutionToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) {this->toolStripTextBox1});
			this->quadratureResolutionToolStripMenuItem->Name = L"quadratureResolutionToolStripMenuItem";
			this->quadratureResolutionToolStripMenuItem->Size = System::Drawing::Size(193, 22);
			this->quadratureResolutionToolStripMenuItem->Text = L"Quadrature Resolution";
			// 
			// toolStripTextBox1
			// 
			this->toolStripTextBox1->Name = L"toolStripTextBox1";
			this->toolStripTextBox1->Size = System::Drawing::Size(100, 23);
			this->toolStripTextBox1->Text = L"200";
			this->toolStripTextBox1->TextChanged += gcnew System::EventHandler(this, &FormFactor::quadrestoolStripTextBox_TextChanged);
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->ColumnCount = 2;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				55.74713F)));
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				44.25287F)));
			this->tableLayoutPanel1->Controls->Add(this->panel2, 0, 0);
			this->tableLayoutPanel1->Controls->Add(this->movetophases, 1, 1);
			this->tableLayoutPanel1->Controls->Add(this->groupBox1, 1, 0);
			this->tableLayoutPanel1->Controls->Add(this->panel3, 0, 1);
			this->tableLayoutPanel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel1->Location = System::Drawing::Point(0, 24);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->RowCount = 2;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 83)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(1157, 692);
			this->tableLayoutPanel1->TabIndex = 1;
			// 
			// panel2
			// 
			this->panel2->Controls->Add(this->tabControl1);
			this->panel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel2->Location = System::Drawing::Point(3, 3);
			this->panel2->Name = L"panel2";
			this->panel2->Size = System::Drawing::Size(638, 603);
			this->panel2->TabIndex = 1;
			// 
			// tabControl1
			// 
			this->tabControl1->Controls->Add(this->FFTab);
			this->tabControl1->Controls->Add(this->SFTab);
			this->tabControl1->Controls->Add(this->BGTab);
			this->tabControl1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tabControl1->Location = System::Drawing::Point(0, 0);
			this->tabControl1->Name = L"tabControl1";
			this->tabControl1->SelectedIndex = 0;
			this->tabControl1->Size = System::Drawing::Size(638, 603);
			this->tabControl1->TabIndex = 10;
			this->tabControl1->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::tabControl1_TabIndexChanged);
			this->tabControl1->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &FormFactor::General_KeyDown);
			// 
			// FFTab
			// 
			this->FFTab->BackColor = System::Drawing::Color::MintCream;
			this->FFTab->Controls->Add(this->ffUseCheckBox);
			this->FFTab->Controls->Add(this->FFGroupbox);
			this->FFTab->Location = System::Drawing::Point(4, 22);
			this->FFTab->Name = L"FFTab";
			this->FFTab->Padding = System::Windows::Forms::Padding(3);
			this->FFTab->Size = System::Drawing::Size(630, 577);
			this->FFTab->TabIndex = 0;
			this->FFTab->Text = L"Form Factor";
			// 
			// ffUseCheckBox
			// 
			this->ffUseCheckBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->ffUseCheckBox->AutoSize = true;
			this->ffUseCheckBox->Checked = true;
			this->ffUseCheckBox->CheckState = System::Windows::Forms::CheckState::Checked;
			this->ffUseCheckBox->Enabled = false;
			this->ffUseCheckBox->Location = System::Drawing::Point(408, 15);
			this->ffUseCheckBox->Name = L"ffUseCheckBox";
			this->ffUseCheckBox->Size = System::Drawing::Size(45, 17);
			this->ffUseCheckBox->TabIndex = 759;
			this->ffUseCheckBox->Text = L"Use";
			this->ffUseCheckBox->UseVisualStyleBackColor = true;
			this->ffUseCheckBox->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::useCheckBox_CheckedChanged);
			// 
			// FFGroupbox
			// 
			this->FFGroupbox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->FFGroupbox->Controls->Add(this->paramPanel);
			this->FFGroupbox->Controls->Add(this->listView_display);
			this->FFGroupbox->Controls->Add(this->freezeFFCheckBox);
			this->FFGroupbox->Controls->Add(this->useModelFFButton);
			this->FFGroupbox->Controls->Add(this->slowModelGroupbox);
			this->FFGroupbox->Controls->Add(this->label2);
			this->FFGroupbox->Controls->Add(this->genRangeBox);
			this->FFGroupbox->Controls->Add(this->consGroupBox);
			this->FFGroupbox->Controls->Add(this->EDAreaGroup);
			this->FFGroupbox->Controls->Add(this->previewBox);
			this->FFGroupbox->Controls->Add(this->listViewFF);
			this->FFGroupbox->Controls->Add(this->paramLabel);
			this->FFGroupbox->Controls->Add(this->globalParamtersGroupBox);
			this->FFGroupbox->Controls->Add(this->listView_Extraparams);
			this->FFGroupbox->Controls->Add(this->edpBox);
			this->FFGroupbox->Controls->Add(this->removeLayer);
			this->FFGroupbox->Controls->Add(this->addLayer);
			this->FFGroupbox->Location = System::Drawing::Point(0, 3);
			this->FFGroupbox->Name = L"FFGroupbox";
			this->FFGroupbox->Size = System::Drawing::Size(629, 575);
			this->FFGroupbox->TabIndex = 760;
			this->FFGroupbox->TabStop = false;
			this->FFGroupbox->Text = L"Form Factor";
			// 
			// paramPanel
			// 
			this->paramPanel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->paramPanel->AutoScroll = true;
			this->paramPanel->FlowDirection = System::Windows::Forms::FlowDirection::TopDown;
			this->paramPanel->Location = System::Drawing::Point(67, 10);
			this->paramPanel->Name = L"paramPanel";
			this->paramPanel->Size = System::Drawing::Size(322, 95);
			this->paramPanel->TabIndex = 763;
			// 
			// listView_display
			// 
			this->listView_display->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->listView_display->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(2) {this->columnHeader16, 
				this->columnHeader17});
			this->listView_display->FullRowSelect = true;
			this->listView_display->Location = System::Drawing::Point(191, 440);
			this->listView_display->Name = L"listView_display";
			this->listView_display->Size = System::Drawing::Size(155, 131);
			this->listView_display->TabIndex = 762;
			this->listView_display->UseCompatibleStateImageBehavior = false;
			this->listView_display->View = System::Windows::Forms::View::Details;
			// 
			// columnHeader16
			// 
			this->columnHeader16->Text = L"Name";
			this->columnHeader16->Width = 69;
			// 
			// columnHeader17
			// 
			this->columnHeader17->Text = L"Value";
			this->columnHeader17->Width = 81;
			// 
			// freezeFFCheckBox
			// 
			this->freezeFFCheckBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->freezeFFCheckBox->AutoSize = true;
			this->freezeFFCheckBox->Location = System::Drawing::Point(408, 34);
			this->freezeFFCheckBox->Name = L"freezeFFCheckBox";
			this->freezeFFCheckBox->Size = System::Drawing::Size(82, 30);
			this->freezeFFCheckBox->TabIndex = 759;
			this->freezeFFCheckBox->Text = L"Freeze\nForm Factor";
			this->freezeFFCheckBox->UseVisualStyleBackColor = true;
			this->freezeFFCheckBox->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::freezeFFCheckBox_CheckedChanged);
			// 
			// useModelFFButton
			// 
			this->useModelFFButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->useModelFFButton->Location = System::Drawing::Point(405, 66);
			this->useModelFFButton->Name = L"useModelFFButton";
			this->useModelFFButton->Size = System::Drawing::Size(72, 22);
			this->useModelFFButton->TabIndex = 150;
			this->useModelFFButton->Text = L"Use Model";
			this->useModelFFButton->UseVisualStyleBackColor = true;
			this->useModelFFButton->Visible = false;
			this->useModelFFButton->Click += gcnew System::EventHandler(this, &FormFactor::useModelFFButton_Click);
			// 
			// slowModelGroupbox
			// 
			this->slowModelGroupbox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->slowModelGroupbox->BackColor = System::Drawing::Color::OrangeRed;
			this->slowModelGroupbox->Controls->Add(this->warningLabel);
			this->slowModelGroupbox->Location = System::Drawing::Point(496, 256);
			this->slowModelGroupbox->Name = L"slowModelGroupbox";
			this->slowModelGroupbox->Size = System::Drawing::Size(121, 109);
			this->slowModelGroupbox->TabIndex = 759;
			this->slowModelGroupbox->TabStop = false;
			this->slowModelGroupbox->Text = L"Warning!!";
			this->slowModelGroupbox->Visible = false;
			// 
			// warningLabel
			// 
			this->warningLabel->AutoSize = true;
			this->warningLabel->Location = System::Drawing::Point(7, 22);
			this->warningLabel->Name = L"warningLabel";
			this->warningLabel->Size = System::Drawing::Size(107, 65);
			this->warningLabel->TabIndex = 0;
			this->warningLabel->Text = L"This model requires a\r\nlot of CPU time to\r\ncalculate.  It is not\r\nrecommended to " 
				L"use\r\nthe slidebars.";
			this->warningLabel->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// label2
			// 
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(6, 15);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(55, 13);
			this->label2->TabIndex = 719;
			this->label2->Text = L"Parameter";
			// 
			// genRangeBox
			// 
			this->genRangeBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->genRangeBox->Controls->Add(this->label5);
			this->genRangeBox->Controls->Add(this->startGen);
			this->genRangeBox->Controls->Add(this->endGen);
			this->genRangeBox->Location = System::Drawing::Point(462, 392);
			this->genRangeBox->Name = L"genRangeBox";
			this->genRangeBox->Size = System::Drawing::Size(166, 44);
			this->genRangeBox->TabIndex = 729;
			this->genRangeBox->TabStop = false;
			this->genRangeBox->Text = L"Generation Range:";
			this->genRangeBox->Visible = false;
			// 
			// label5
			// 
			this->label5->AutoSize = true;
			this->label5->Location = System::Drawing::Point(70, 21);
			this->label5->Name = L"label5";
			this->label5->Size = System::Drawing::Size(19, 13);
			this->label5->TabIndex = 20;
			this->label5->Text = L"-->";
			// 
			// startGen
			// 
			this->startGen->Location = System::Drawing::Point(6, 18);
			this->startGen->Name = L"startGen";
			this->startGen->Size = System::Drawing::Size(61, 20);
			this->startGen->TabIndex = 260;
			this->startGen->Text = L"0.100000";
			this->startGen->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->startGen->Leave += gcnew System::EventHandler(this, &FormFactor::double_TextChanged);
			// 
			// endGen
			// 
			this->endGen->Location = System::Drawing::Point(95, 18);
			this->endGen->Name = L"endGen";
			this->endGen->Size = System::Drawing::Size(61, 20);
			this->endGen->TabIndex = 270;
			this->endGen->Text = L"5.000000";
			this->endGen->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->endGen->Leave += gcnew System::EventHandler(this, &FormFactor::double_TextChanged);
			// 
			// consGroupBox
			// 
			this->consGroupBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->consGroupBox->Controls->Add(this->constraints);
			this->consGroupBox->Controls->Add(this->fitRange);
			this->consGroupBox->Location = System::Drawing::Point(7, 390);
			this->consGroupBox->Name = L"consGroupBox";
			this->consGroupBox->Size = System::Drawing::Size(151, 45);
			this->consGroupBox->TabIndex = 756;
			this->consGroupBox->TabStop = false;
			this->consGroupBox->Text = L"Fit Constraints";
			// 
			// constraints
			// 
			this->constraints->AutoSize = true;
			this->constraints->Location = System::Drawing::Point(6, 19);
			this->constraints->Name = L"constraints";
			this->constraints->Size = System::Drawing::Size(59, 17);
			this->constraints->TabIndex = 140;
			this->constraints->Text = L"Enable";
			this->constraints->UseVisualStyleBackColor = true;
			this->constraints->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::constraints_CheckedChanged);
			// 
			// fitRange
			// 
			this->fitRange->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->fitRange->Enabled = false;
			this->fitRange->Location = System::Drawing::Point(73, 15);
			this->fitRange->Name = L"fitRange";
			this->fitRange->Size = System::Drawing::Size(72, 22);
			this->fitRange->TabIndex = 150;
			this->fitRange->Text = L"Fit Range...";
			this->fitRange->UseVisualStyleBackColor = true;
			this->fitRange->Click += gcnew System::EventHandler(this, &FormFactor::fitRange_Click);
			// 
			// EDAreaGroup
			// 
			this->EDAreaGroup->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->EDAreaGroup->Controls->Add(this->AreaText);
			this->EDAreaGroup->Location = System::Drawing::Point(164, 392);
			this->EDAreaGroup->Name = L"EDAreaGroup";
			this->EDAreaGroup->Size = System::Drawing::Size(77, 45);
			this->EDAreaGroup->TabIndex = 755;
			this->EDAreaGroup->TabStop = false;
			this->EDAreaGroup->Text = L"ED Area";
			// 
			// AreaText
			// 
			this->AreaText->Location = System::Drawing::Point(6, 17);
			this->AreaText->Name = L"AreaText";
			this->AreaText->ReadOnly = true;
			this->AreaText->Size = System::Drawing::Size(64, 20);
			this->AreaText->TabIndex = 160;
			// 
			// previewBox
			// 
			this->previewBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->previewBox->Controls->Add(this->oglPanel);
			this->previewBox->Location = System::Drawing::Point(352, 441);
			this->previewBox->Name = L"previewBox";
			this->previewBox->Size = System::Drawing::Size(136, 131);
			this->previewBox->TabIndex = 728;
			this->previewBox->TabStop = false;
			this->previewBox->Text = L"3D Preview";
			// 
			// oglPanel
			// 
			this->oglPanel->Dock = System::Windows::Forms::DockStyle::Fill;
			this->oglPanel->Location = System::Drawing::Point(3, 16);
			this->oglPanel->Name = L"oglPanel";
			this->oglPanel->Size = System::Drawing::Size(130, 112);
			this->oglPanel->TabIndex = 0;
			// 
			// listViewFF
			// 
			this->listViewFF->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->listViewFF->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(1) {this->ParameterCol});
			this->listViewFF->FullRowSelect = true;
			this->listViewFF->GridLines = true;
			this->listViewFF->HideSelection = false;
			this->listViewFF->Location = System::Drawing::Point(13, 111);
			this->listViewFF->Name = L"listViewFF";
			this->listViewFF->Size = System::Drawing::Size(477, 234);
			this->listViewFF->TabIndex = 80;
			this->listViewFF->UseCompatibleStateImageBehavior = false;
			this->listViewFF->View = System::Windows::Forms::View::Details;
			this->listViewFF->MouseDoubleClick += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::listViewFF_DoubleClick);
			this->listViewFF->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::listViewFF_SelectedIndexChanged);
			this->listViewFF->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &FormFactor::listViewFF_KeyDown);
			// 
			// ParameterCol
			// 
			this->ParameterCol->Text = L"Parameter";
			this->ParameterCol->Width = 63;
			// 
			// paramLabel
			// 
			this->paramLabel->AutoSize = true;
			this->paramLabel->Location = System::Drawing::Point(6, 34);
			this->paramLabel->Name = L"paramLabel";
			this->paramLabel->Size = System::Drawing::Size(45, 13);
			this->paramLabel->TabIndex = 720;
			this->paramLabel->Text = L"<None>";
			// 
			// globalParamtersGroupBox
			// 
			this->globalParamtersGroupBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->globalParamtersGroupBox->Controls->Add(this->paramBox);
			this->globalParamtersGroupBox->Controls->Add(this->minim);
			this->globalParamtersGroupBox->Controls->Add(this->maxim);
			this->globalParamtersGroupBox->Controls->Add(this->exmax);
			this->globalParamtersGroupBox->Controls->Add(this->exmin);
			this->globalParamtersGroupBox->Controls->Add(this->infExtraParam);
			this->globalParamtersGroupBox->Location = System::Drawing::Point(496, 15);
			this->globalParamtersGroupBox->Name = L"globalParamtersGroupBox";
			this->globalParamtersGroupBox->Size = System::Drawing::Size(121, 238);
			this->globalParamtersGroupBox->TabIndex = 732;
			this->globalParamtersGroupBox->TabStop = false;
			this->globalParamtersGroupBox->Text = L"Global Parameters:";
			// 
			// paramBox
			// 
			this->paramBox->AllowDrop = true;
			this->paramBox->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->paramBox->FormattingEnabled = true;
			this->paramBox->Location = System::Drawing::Point(6, 19);
			this->paramBox->Name = L"paramBox";
			this->paramBox->Size = System::Drawing::Size(111, 21);
			this->paramBox->TabIndex = 190;
			this->paramBox->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::paramBox_SelectedIndexChanged);
			// 
			// minim
			// 
			this->minim->AutoSize = true;
			this->minim->Location = System::Drawing::Point(3, 131);
			this->minim->Name = L"minim";
			this->minim->Size = System::Drawing::Size(51, 13);
			this->minim->TabIndex = 705;
			this->minim->Text = L"Minimum:";
			// 
			// maxim
			// 
			this->maxim->AutoSize = true;
			this->maxim->Location = System::Drawing::Point(3, 170);
			this->maxim->Name = L"maxim";
			this->maxim->Size = System::Drawing::Size(54, 13);
			this->maxim->TabIndex = 707;
			this->maxim->Text = L"Maximum:";
			// 
			// exmax
			// 
			this->exmax->Location = System::Drawing::Point(6, 186);
			this->exmax->Name = L"exmax";
			this->exmax->Size = System::Drawing::Size(106, 20);
			this->exmax->TabIndex = 240;
			this->exmax->Text = L"0.000000";
			this->exmax->Leave += gcnew System::EventHandler(this, &FormFactor::ExtraParameter_TextChanged);
			// 
			// exmin
			// 
			this->exmin->Location = System::Drawing::Point(6, 147);
			this->exmin->Name = L"exmin";
			this->exmin->Size = System::Drawing::Size(106, 20);
			this->exmin->TabIndex = 230;
			this->exmin->Text = L"0.000000";
			this->exmin->Leave += gcnew System::EventHandler(this, &FormFactor::ExtraParameter_TextChanged);
			// 
			// infExtraParam
			// 
			this->infExtraParam->AutoSize = true;
			this->infExtraParam->Location = System::Drawing::Point(6, 213);
			this->infExtraParam->Name = L"infExtraParam";
			this->infExtraParam->Size = System::Drawing::Size(57, 17);
			this->infExtraParam->TabIndex = 250;
			this->infExtraParam->Text = L"Infinite";
			this->infExtraParam->UseVisualStyleBackColor = true;
			this->infExtraParam->Visible = false;
			this->infExtraParam->Click += gcnew System::EventHandler(this, &FormFactor::infExtraParam_CheckedChanged);
			// 
			// listView_Extraparams
			// 
			this->listView_Extraparams->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->listView_Extraparams->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(2) {this->Param, 
				this->Value});
			this->listView_Extraparams->FullRowSelect = true;
			this->listView_Extraparams->Location = System::Drawing::Point(7, 441);
			this->listView_Extraparams->Name = L"listView_Extraparams";
			this->listView_Extraparams->Size = System::Drawing::Size(178, 131);
			this->listView_Extraparams->TabIndex = 180;
			this->listView_Extraparams->UseCompatibleStateImageBehavior = false;
			this->listView_Extraparams->View = System::Windows::Forms::View::Details;
			this->listView_Extraparams->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::listView_Extraparams_SelectedIndexChanged);
			// 
			// Param
			// 
			this->Param->Text = L"Parameter";
			this->Param->Width = 88;
			// 
			// Value
			// 
			this->Value->Text = L"Value";
			this->Value->Width = 81;
			// 
			// edpBox
			// 
			this->edpBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->edpBox->Location = System::Drawing::Point(494, 441);
			this->edpBox->Name = L"edpBox";
			this->edpBox->Size = System::Drawing::Size(132, 131);
			this->edpBox->TabIndex = 730;
			this->edpBox->TabStop = false;
			this->edpBox->Text = L"Electron Density Profile";
			// 
			// removeLayer
			// 
			this->removeLayer->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->removeLayer->Enabled = false;
			this->removeLayer->Location = System::Drawing::Point(37, 351);
			this->removeLayer->Name = L"removeLayer";
			this->removeLayer->Size = System::Drawing::Size(24, 23);
			this->removeLayer->TabIndex = 100;
			this->removeLayer->Text = L"-";
			this->removeLayer->UseVisualStyleBackColor = true;
			this->removeLayer->Click += gcnew System::EventHandler(this, &FormFactor::removeLayer_Click);
			// 
			// addLayer
			// 
			this->addLayer->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->addLayer->Location = System::Drawing::Point(9, 351);
			this->addLayer->Name = L"addLayer";
			this->addLayer->Size = System::Drawing::Size(24, 23);
			this->addLayer->TabIndex = 90;
			this->addLayer->Text = L"+";
			this->addLayer->UseVisualStyleBackColor = true;
			this->addLayer->Click += gcnew System::EventHandler(this, &FormFactor::addLayer_Click);
			// 
			// SFTab
			// 
			this->SFTab->BackColor = System::Drawing::Color::AntiqueWhite;
			this->SFTab->Controls->Add(this->sfUseCheckBox);
			this->SFTab->Controls->Add(this->phasefitter);
			this->SFTab->Controls->Add(this->Peakfitter);
			this->SFTab->Controls->Add(this->cailleGroupbox);
			this->SFTab->Location = System::Drawing::Point(4, 22);
			this->SFTab->Name = L"SFTab";
			this->SFTab->Padding = System::Windows::Forms::Padding(3);
			this->SFTab->Size = System::Drawing::Size(630, 577);
			this->SFTab->TabIndex = 1;
			this->SFTab->Text = L"Structure Factor";
			// 
			// sfUseCheckBox
			// 
			this->sfUseCheckBox->AutoSize = true;
			this->sfUseCheckBox->Checked = true;
			this->sfUseCheckBox->CheckState = System::Windows::Forms::CheckState::Checked;
			this->sfUseCheckBox->Enabled = false;
			this->sfUseCheckBox->Location = System::Drawing::Point(408, 15);
			this->sfUseCheckBox->Name = L"sfUseCheckBox";
			this->sfUseCheckBox->Size = System::Drawing::Size(45, 17);
			this->sfUseCheckBox->TabIndex = 767;
			this->sfUseCheckBox->Text = L"Use";
			this->sfUseCheckBox->UseVisualStyleBackColor = true;
			this->sfUseCheckBox->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::useCheckBox_CheckedChanged);
			// 
			// phasefitter
			// 
			this->phasefitter->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->phasefitter->Controls->Add(this->order);
			this->phasefitter->Controls->Add(this->Volume);
			this->phasefitter->Controls->Add(this->phaseErrorTextBox);
			this->phasefitter->Controls->Add(this->Vollabel);
			this->phasefitter->Controls->Add(this->phaseErrorLabel);
			this->phasefitter->Controls->Add(this->Caille_button);
			this->phasefitter->Controls->Add(this->undoPhases);
			this->phasefitter->Controls->Add(this->clearPositionsButton);
			this->phasefitter->Controls->Add(this->listView_PeakPosition);
			this->phasefitter->Controls->Add(this->phasesParamsGroupBox);
			this->phasefitter->Controls->Add(this->fitphase);
			this->phasefitter->Controls->Add(this->label10);
			this->phasefitter->Controls->Add(this->listView_phases);
			this->phasefitter->Location = System::Drawing::Point(6, 288);
			this->phasefitter->Name = L"phasefitter";
			this->phasefitter->Size = System::Drawing::Size(563, 283);
			this->phasefitter->TabIndex = 758;
			this->phasefitter->TabStop = false;
			this->phasefitter->Text = L"Phase fitter:";
			// 
			// order
			// 
			this->order->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->order->FormattingEnabled = true;
			this->order->Items->AddRange(gcnew cli::array< System::Object^  >(14) {L"[None]", L"Lamellar", L"General 2D", L"Rectangular 2D", 
				L"Cent. Rect. 2D", L"Squared 2D", L"Hexagonal 2D", L"General 3D", L"Rhombohegral 3D", L"Hexagonal 3D", L"Monoclinic 3D", L"Orthorombic 3D", 
				L"Tetragonal 3D", L"Cubic 3D"});
			this->order->Location = System::Drawing::Point(41, 16);
			this->order->MaxDropDownItems = 13;
			this->order->Name = L"order";
			this->order->Size = System::Drawing::Size(118, 21);
			this->order->TabIndex = 729;
			this->order->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::phaseorder_SelectedIndexChanged);
			// 
			// Volume
			// 
			this->Volume->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->Volume->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 6.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(177)));
			this->Volume->Location = System::Drawing::Point(258, 244);
			this->Volume->Name = L"Volume";
			this->Volume->ReadOnly = true;
			this->Volume->Size = System::Drawing::Size(53, 17);
			this->Volume->TabIndex = 764;
			this->Volume->Text = L"-";
			this->Volume->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// phaseErrorTextBox
			// 
			this->phaseErrorTextBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->phaseErrorTextBox->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 6.5F, System::Drawing::FontStyle::Regular, 
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(177)));
			this->phaseErrorTextBox->Location = System::Drawing::Point(504, 32);
			this->phaseErrorTextBox->Name = L"phaseErrorTextBox";
			this->phaseErrorTextBox->ReadOnly = true;
			this->phaseErrorTextBox->Size = System::Drawing::Size(53, 17);
			this->phaseErrorTextBox->TabIndex = 764;
			this->phaseErrorTextBox->Text = L"-";
			this->phaseErrorTextBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			// 
			// Vollabel
			// 
			this->Vollabel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->Vollabel->AutoSize = true;
			this->Vollabel->Location = System::Drawing::Point(257, 226);
			this->Vollabel->Name = L"Vollabel";
			this->Vollabel->Size = System::Drawing::Size(96, 13);
			this->Vollabel->TabIndex = 763;
			this->Vollabel->Text = L"Volume of unit cell:";
			// 
			// phaseErrorLabel
			// 
			this->phaseErrorLabel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->phaseErrorLabel->AutoSize = true;
			this->phaseErrorLabel->Location = System::Drawing::Point(505, 16);
			this->phaseErrorLabel->Name = L"phaseErrorLabel";
			this->phaseErrorLabel->Size = System::Drawing::Size(32, 13);
			this->phaseErrorLabel->TabIndex = 763;
			this->phaseErrorLabel->Text = L"Error:";
			// 
			// Caille_button
			// 
			this->Caille_button->Enabled = false;
			this->Caille_button->Location = System::Drawing::Point(163, 15);
			this->Caille_button->Name = L"Caille_button";
			this->Caille_button->Size = System::Drawing::Size(47, 21);
			this->Caille_button->TabIndex = 16;
			this->Caille_button->Text = L"Caille";
			this->Caille_button->UseVisualStyleBackColor = true;
			this->Caille_button->Visible = false;
			this->Caille_button->Click += gcnew System::EventHandler(this, &FormFactor::Caille_button_Click);
			// 
			// undoPhases
			// 
			this->undoPhases->Enabled = false;
			this->undoPhases->Location = System::Drawing::Point(76, 43);
			this->undoPhases->Name = L"undoPhases";
			this->undoPhases->Size = System::Drawing::Size(60, 25);
			this->undoPhases->TabIndex = 762;
			this->undoPhases->Text = L"Undo";
			this->undoPhases->UseVisualStyleBackColor = true;
			this->undoPhases->Click += gcnew System::EventHandler(this, &FormFactor::fitphase_Click);
			// 
			// clearPositionsButton
			// 
			this->clearPositionsButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->clearPositionsButton->Location = System::Drawing::Point(138, 43);
			this->clearPositionsButton->Name = L"clearPositionsButton";
			this->clearPositionsButton->Size = System::Drawing::Size(60, 25);
			this->clearPositionsButton->TabIndex = 757;
			this->clearPositionsButton->Text = L"Clear";
			this->clearPositionsButton->UseVisualStyleBackColor = true;
			this->clearPositionsButton->Click += gcnew System::EventHandler(this, &FormFactor::clearPhases_Click);
			// 
			// listView_PeakPosition
			// 
			this->listView_PeakPosition->Anchor = static_cast<System::Windows::Forms::AnchorStyles>(((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left));
			this->listView_PeakPosition->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(3) {this->columnHeader9, 
				this->columnHeader8, this->columnHeader14});
			this->listView_PeakPosition->FullRowSelect = true;
			this->listView_PeakPosition->GridLines = true;
			this->listView_PeakPosition->HideSelection = false;
			this->listView_PeakPosition->Location = System::Drawing::Point(12, 73);
			this->listView_PeakPosition->Name = L"listView_PeakPosition";
			this->listView_PeakPosition->Size = System::Drawing::Size(231, 205);
			this->listView_PeakPosition->TabIndex = 734;
			this->listView_PeakPosition->UseCompatibleStateImageBehavior = false;
			this->listView_PeakPosition->View = System::Windows::Forms::View::Details;
			this->listView_PeakPosition->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::listView_PeakPosition_SelectedIndexChanged);
			// 
			// columnHeader9
			// 
			this->columnHeader9->Text = L"Position";
			this->columnHeader9->Width = 54;
			// 
			// columnHeader8
			// 
			this->columnHeader8->Text = L"Indices";
			this->columnHeader8->Width = 94;
			// 
			// columnHeader14
			// 
			this->columnHeader14->Text = L"Generated";
			this->columnHeader14->Width = 69;
			// 
			// phasesParamsGroupBox
			// 
			this->phasesParamsGroupBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->phasesParamsGroupBox->Controls->Add(this->MutPhases);
			this->phasesParamsGroupBox->Controls->Add(this->label21);
			this->phasesParamsGroupBox->Controls->Add(this->ValPhases);
			this->phasesParamsGroupBox->Controls->Add(this->comboPhases);
			this->phasesParamsGroupBox->Controls->Add(this->label22);
			this->phasesParamsGroupBox->Controls->Add(this->label23);
			this->phasesParamsGroupBox->Controls->Add(this->MaxPhases);
			this->phasesParamsGroupBox->Controls->Add(this->MinPhases);
			this->phasesParamsGroupBox->Location = System::Drawing::Point(220, 7);
			this->phasesParamsGroupBox->Name = L"phasesParamsGroupBox";
			this->phasesParamsGroupBox->Size = System::Drawing::Size(279, 61);
			this->phasesParamsGroupBox->TabIndex = 761;
			this->phasesParamsGroupBox->TabStop = false;
			this->phasesParamsGroupBox->Text = L"Parameters:";
			// 
			// MutPhases
			// 
			this->MutPhases->AutoSize = true;
			this->MutPhases->Location = System::Drawing::Point(5, 41);
			this->MutPhases->Name = L"MutPhases";
			this->MutPhases->Size = System::Drawing::Size(64, 17);
			this->MutPhases->TabIndex = 712;
			this->MutPhases->Text = L"Mutable";
			this->MutPhases->UseVisualStyleBackColor = true;
			this->MutPhases->Click += gcnew System::EventHandler(this, &FormFactor::MutPhases_Click);
			// 
			// label21
			// 
			this->label21->AutoSize = true;
			this->label21->Location = System::Drawing::Point(74, 15);
			this->label21->Name = L"label21";
			this->label21->Size = System::Drawing::Size(37, 13);
			this->label21->TabIndex = 710;
			this->label21->Text = L"Value:";
			// 
			// ValPhases
			// 
			this->ValPhases->Location = System::Drawing::Point(77, 31);
			this->ValPhases->Name = L"ValPhases";
			this->ValPhases->Size = System::Drawing::Size(60, 20);
			this->ValPhases->TabIndex = 709;
			this->ValPhases->Text = L"0.000000";
			this->ValPhases->Leave += gcnew System::EventHandler(this, &FormFactor::ValPhases_Leave);
			// 
			// comboPhases
			// 
			this->comboPhases->AllowDrop = true;
			this->comboPhases->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->comboPhases->FormattingEnabled = true;
			this->comboPhases->Location = System::Drawing::Point(6, 14);
			this->comboPhases->Name = L"comboPhases";
			this->comboPhases->Size = System::Drawing::Size(65, 21);
			this->comboPhases->TabIndex = 708;
			this->comboPhases->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::comboPhases_SelectedIndexChanged);
			// 
			// label22
			// 
			this->label22->AutoSize = true;
			this->label22->Location = System::Drawing::Point(140, 15);
			this->label22->Name = L"label22";
			this->label22->Size = System::Drawing::Size(51, 13);
			this->label22->TabIndex = 705;
			this->label22->Text = L"Minimum:";
			// 
			// label23
			// 
			this->label23->AutoSize = true;
			this->label23->Location = System::Drawing::Point(206, 15);
			this->label23->Name = L"label23";
			this->label23->Size = System::Drawing::Size(54, 13);
			this->label23->TabIndex = 707;
			this->label23->Text = L"Maximum:";
			// 
			// MaxPhases
			// 
			this->MaxPhases->Location = System::Drawing::Point(209, 31);
			this->MaxPhases->Name = L"MaxPhases";
			this->MaxPhases->Size = System::Drawing::Size(60, 20);
			this->MaxPhases->TabIndex = 703;
			this->MaxPhases->Text = L"0.000000";
			this->MaxPhases->Leave += gcnew System::EventHandler(this, &FormFactor::ValPhases_Leave);
			// 
			// MinPhases
			// 
			this->MinPhases->Location = System::Drawing::Point(143, 31);
			this->MinPhases->Name = L"MinPhases";
			this->MinPhases->Size = System::Drawing::Size(60, 20);
			this->MinPhases->TabIndex = 704;
			this->MinPhases->Text = L"0.000000";
			this->MinPhases->Leave += gcnew System::EventHandler(this, &FormFactor::ValPhases_Leave);
			// 
			// fitphase
			// 
			this->fitphase->Enabled = false;
			this->fitphase->Location = System::Drawing::Point(14, 43);
			this->fitphase->Name = L"fitphase";
			this->fitphase->Size = System::Drawing::Size(60, 25);
			this->fitphase->TabIndex = 759;
			this->fitphase->Text = L"Fit Phase";
			this->fitphase->UseVisualStyleBackColor = true;
			this->fitphase->Click += gcnew System::EventHandler(this, &FormFactor::fitphase_Click);
			// 
			// label10
			// 
			this->label10->AutoSize = true;
			this->label10->Location = System::Drawing::Point(3, 19);
			this->label10->Name = L"label10";
			this->label10->Size = System::Drawing::Size(40, 13);
			this->label10->TabIndex = 732;
			this->label10->Text = L"Phase:";
			// 
			// listView_phases
			// 
			this->listView_phases->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->listView_phases->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(6) {this->columnHeader10, 
				this->columnHeader13, this->columnHeader12, this->columnHeader11, this->columnHeader15, this->recipColumnHeader});
			this->listView_phases->FullRowSelect = true;
			this->listView_phases->GridLines = true;
			this->listView_phases->HideSelection = false;
			this->listView_phases->Location = System::Drawing::Point(258, 73);
			this->listView_phases->MultiSelect = false;
			this->listView_phases->Name = L"listView_phases";
			this->listView_phases->Size = System::Drawing::Size(296, 139);
			this->listView_phases->TabIndex = 1;
			this->listView_phases->UseCompatibleStateImageBehavior = false;
			this->listView_phases->View = System::Windows::Forms::View::Details;
			this->listView_phases->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::listView_phases_SelectedIndexChanged);
			// 
			// columnHeader10
			// 
			this->columnHeader10->Text = L"Parameter";
			this->columnHeader10->Width = 45;
			// 
			// columnHeader13
			// 
			this->columnHeader13->Text = L"Value";
			this->columnHeader13->Width = 49;
			// 
			// columnHeader12
			// 
			this->columnHeader12->Text = L"M";
			this->columnHeader12->Width = 25;
			// 
			// columnHeader11
			// 
			this->columnHeader11->Text = L"Min";
			this->columnHeader11->Width = 49;
			// 
			// columnHeader15
			// 
			this->columnHeader15->Text = L"Max";
			this->columnHeader15->Width = 49;
			// 
			// recipColumnHeader
			// 
			this->recipColumnHeader->Text = L"Reciprocal";
			this->recipColumnHeader->Width = 66;
			// 
			// Peakfitter
			// 
			this->Peakfitter->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->Peakfitter->Controls->Add(this->showCailleButton);
			this->Peakfitter->Controls->Add(this->PeakShapeGroupbox);
			this->Peakfitter->Controls->Add(this->SortButton);
			this->Peakfitter->Controls->Add(this->automaticPeakFinderButton);
			this->Peakfitter->Controls->Add(this->Move2Phases);
			this->Peakfitter->Controls->Add(this->thresholdBox2);
			this->Peakfitter->Controls->Add(this->Threshold_label2);
			this->Peakfitter->Controls->Add(this->thresholdBox1);
			this->Peakfitter->Controls->Add(this->Threshold_label1);
			this->Peakfitter->Controls->Add(this->PeakPicker);
			this->Peakfitter->Controls->Add(this->removePeak);
			this->Peakfitter->Controls->Add(this->addPeak);
			this->Peakfitter->Controls->Add(this->centerTrack);
			this->Peakfitter->Controls->Add(this->centerMut);
			this->Peakfitter->Controls->Add(this->centerBox);
			this->Peakfitter->Controls->Add(this->label11);
			this->Peakfitter->Controls->Add(this->fwhmMut);
			this->Peakfitter->Controls->Add(this->ampMut);
			this->Peakfitter->Controls->Add(this->fwhmBox);
			this->Peakfitter->Controls->Add(this->label12);
			this->Peakfitter->Controls->Add(this->ampBox);
			this->Peakfitter->Controls->Add(this->label13);
			this->Peakfitter->Controls->Add(this->label14);
			this->Peakfitter->Controls->Add(this->label15);
			this->Peakfitter->Controls->Add(this->listView_peaks);
			this->Peakfitter->Controls->Add(this->fwhmTrack);
			this->Peakfitter->Controls->Add(this->ampTrack);
			this->Peakfitter->Location = System::Drawing::Point(5, 5);
			this->Peakfitter->Name = L"Peakfitter";
			this->Peakfitter->Size = System::Drawing::Size(560, 277);
			this->Peakfitter->TabIndex = 757;
			this->Peakfitter->TabStop = false;
			this->Peakfitter->Text = L"Peak fitter:";
			// 
			// showCailleButton
			// 
			this->showCailleButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->showCailleButton->Location = System::Drawing::Point(476, 203);
			this->showCailleButton->Name = L"showCailleButton";
			this->showCailleButton->Size = System::Drawing::Size(75, 23);
			this->showCailleButton->TabIndex = 766;
			this->showCailleButton->Text = L"Show Caille";
			this->showCailleButton->UseVisualStyleBackColor = true;
			this->showCailleButton->Visible = false;
			this->showCailleButton->Click += gcnew System::EventHandler(this, &FormFactor::showCailleButton_Click);
			// 
			// PeakShapeGroupbox
			// 
			this->PeakShapeGroupbox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->PeakShapeGroupbox->Controls->Add(this->peakfit);
			this->PeakShapeGroupbox->Location = System::Drawing::Point(430, 26);
			this->PeakShapeGroupbox->Name = L"PeakShapeGroupbox";
			this->PeakShapeGroupbox->Size = System::Drawing::Size(121, 42);
			this->PeakShapeGroupbox->TabIndex = 753;
			this->PeakShapeGroupbox->TabStop = false;
			this->PeakShapeGroupbox->Text = L"Peak Shape:";
			// 
			// peakfit
			// 
			this->peakfit->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->peakfit->FormattingEnabled = true;
			this->peakfit->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Gaussian", L"Lorentzian", L"Lorentzian Squared"});
			this->peakfit->Location = System::Drawing::Point(4, 15);
			this->peakfit->Name = L"peakfit";
			this->peakfit->Size = System::Drawing::Size(111, 21);
			this->peakfit->TabIndex = 730;
			this->peakfit->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::peakfit_SelectedIndexChanged);
			// 
			// SortButton
			// 
			this->SortButton->Location = System::Drawing::Point(276, 77);
			this->SortButton->Name = L"SortButton";
			this->SortButton->Size = System::Drawing::Size(44, 23);
			this->SortButton->TabIndex = 765;
			this->SortButton->Text = L"Sort";
			this->SortButton->UseVisualStyleBackColor = true;
			this->SortButton->Click += gcnew System::EventHandler(this, &FormFactor::SortButton_Click);
			// 
			// automaticPeakFinderButton
			// 
			this->automaticPeakFinderButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->automaticPeakFinderButton->Location = System::Drawing::Point(477, 144);
			this->automaticPeakFinderButton->Name = L"automaticPeakFinderButton";
			this->automaticPeakFinderButton->Size = System::Drawing::Size(74, 23);
			this->automaticPeakFinderButton->TabIndex = 764;
			this->automaticPeakFinderButton->Text = L"Auto Peaks";
			this->automaticPeakFinderButton->UseVisualStyleBackColor = true;
			this->automaticPeakFinderButton->Click += gcnew System::EventHandler(this, &FormFactor::automaticPeakFinderButton_Click);
			// 
			// Move2Phases
			// 
			this->Move2Phases->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->Move2Phases->Location = System::Drawing::Point(477, 232);
			this->Move2Phases->Name = L"Move2Phases";
			this->Move2Phases->Size = System::Drawing::Size(74, 36);
			this->Move2Phases->TabIndex = 757;
			this->Move2Phases->Text = L"Move to Phase Fitter";
			this->Move2Phases->UseVisualStyleBackColor = true;
			this->Move2Phases->Click += gcnew System::EventHandler(this, &FormFactor::Move2Phases_Click);
			// 
			// thresholdBox2
			// 
			this->thresholdBox2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->thresholdBox2->Location = System::Drawing::Point(476, 119);
			this->thresholdBox2->Name = L"thresholdBox2";
			this->thresholdBox2->Size = System::Drawing::Size(74, 20);
			this->thresholdBox2->TabIndex = 761;
			this->thresholdBox2->Text = L"0.001000";
			this->thresholdBox2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->thresholdBox2->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// Threshold_label2
			// 
			this->Threshold_label2->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->Threshold_label2->AutoSize = true;
			this->Threshold_label2->Location = System::Drawing::Point(473, 105);
			this->Threshold_label2->Name = L"Threshold_label2";
			this->Threshold_label2->Size = System::Drawing::Size(45, 13);
			this->Threshold_label2->TabIndex = 762;
			this->Threshold_label2->Text = L"Fraction";
			// 
			// thresholdBox1
			// 
			this->thresholdBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->thresholdBox1->Location = System::Drawing::Point(475, 82);
			this->thresholdBox1->Name = L"thresholdBox1";
			this->thresholdBox1->Size = System::Drawing::Size(74, 20);
			this->thresholdBox1->TabIndex = 758;
			this->thresholdBox1->Text = L"0.010000";
			this->thresholdBox1->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->thresholdBox1->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// Threshold_label1
			// 
			this->Threshold_label1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->Threshold_label1->AutoSize = true;
			this->Threshold_label1->Location = System::Drawing::Point(472, 67);
			this->Threshold_label1->Name = L"Threshold_label1";
			this->Threshold_label1->Size = System::Drawing::Size(72, 13);
			this->Threshold_label1->TabIndex = 759;
			this->Threshold_label1->Text = L"Segment Size";
			// 
			// PeakPicker
			// 
			this->PeakPicker->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->PeakPicker->Enabled = false;
			this->PeakPicker->Location = System::Drawing::Point(477, 172);
			this->PeakPicker->Name = L"PeakPicker";
			this->PeakPicker->Size = System::Drawing::Size(74, 23);
			this->PeakPicker->TabIndex = 756;
			this->PeakPicker->Text = L"Peak Finder";
			this->PeakPicker->UseVisualStyleBackColor = true;
			this->PeakPicker->Click += gcnew System::EventHandler(this, &FormFactor::PeakPicker_Click);
			// 
			// removePeak
			// 
			this->removePeak->Enabled = false;
			this->removePeak->Location = System::Drawing::Point(37, 73);
			this->removePeak->Name = L"removePeak";
			this->removePeak->Size = System::Drawing::Size(26, 23);
			this->removePeak->TabIndex = 755;
			this->removePeak->Text = L"-";
			this->removePeak->UseVisualStyleBackColor = true;
			this->removePeak->Click += gcnew System::EventHandler(this, &FormFactor::removePeak_Click);
			// 
			// addPeak
			// 
			this->addPeak->Location = System::Drawing::Point(5, 73);
			this->addPeak->Name = L"addPeak";
			this->addPeak->Size = System::Drawing::Size(26, 23);
			this->addPeak->TabIndex = 754;
			this->addPeak->Text = L"+";
			this->addPeak->UseVisualStyleBackColor = true;
			this->addPeak->Click += gcnew System::EventHandler(this, &FormFactor::addPeak_Click);
			// 
			// centerTrack
			// 
			this->centerTrack->Enabled = false;
			this->centerTrack->Location = System::Drawing::Point(234, 52);
			this->centerTrack->Maximum = 1000;
			this->centerTrack->Name = L"centerTrack";
			this->centerTrack->Size = System::Drawing::Size(74, 45);
			this->centerTrack->TabIndex = 750;
			this->centerTrack->TickStyle = System::Windows::Forms::TickStyle::None;
			this->centerTrack->Value = 500;
			this->centerTrack->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
			// 
			// centerMut
			// 
			this->centerMut->AutoSize = true;
			this->centerMut->Enabled = false;
			this->centerMut->Location = System::Drawing::Point(293, 12);
			this->centerMut->Name = L"centerMut";
			this->centerMut->Size = System::Drawing::Size(15, 14);
			this->centerMut->TabIndex = 747;
			this->centerMut->UseVisualStyleBackColor = true;
			this->centerMut->Click += gcnew System::EventHandler(this, &FormFactor::centerMut_Click);
			// 
			// centerBox
			// 
			this->centerBox->Enabled = false;
			this->centerBox->Location = System::Drawing::Point(234, 28);
			this->centerBox->Name = L"centerBox";
			this->centerBox->Size = System::Drawing::Size(74, 20);
			this->centerBox->TabIndex = 745;
			this->centerBox->Text = L"0.000000";
			this->centerBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->centerBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// label11
			// 
			this->label11->AutoSize = true;
			this->label11->Location = System::Drawing::Point(231, 12);
			this->label11->Name = L"label11";
			this->label11->Size = System::Drawing::Size(38, 13);
			this->label11->TabIndex = 746;
			this->label11->Text = L"Center";
			// 
			// fwhmMut
			// 
			this->fwhmMut->AutoSize = true;
			this->fwhmMut->Enabled = false;
			this->fwhmMut->Location = System::Drawing::Point(208, 13);
			this->fwhmMut->Name = L"fwhmMut";
			this->fwhmMut->Size = System::Drawing::Size(15, 14);
			this->fwhmMut->TabIndex = 740;
			this->fwhmMut->UseVisualStyleBackColor = true;
			this->fwhmMut->Click += gcnew System::EventHandler(this, &FormFactor::fwhmMut_Click);
			// 
			// ampMut
			// 
			this->ampMut->AutoSize = true;
			this->ampMut->Enabled = false;
			this->ampMut->Location = System::Drawing::Point(122, 12);
			this->ampMut->Name = L"ampMut";
			this->ampMut->Size = System::Drawing::Size(15, 14);
			this->ampMut->TabIndex = 738;
			this->ampMut->UseVisualStyleBackColor = true;
			this->ampMut->Click += gcnew System::EventHandler(this, &FormFactor::ampMut_Click);
			// 
			// fwhmBox
			// 
			this->fwhmBox->Enabled = false;
			this->fwhmBox->Location = System::Drawing::Point(148, 28);
			this->fwhmBox->Name = L"fwhmBox";
			this->fwhmBox->Size = System::Drawing::Size(74, 20);
			this->fwhmBox->TabIndex = 739;
			this->fwhmBox->Text = L"0.000000";
			this->fwhmBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->fwhmBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// label12
			// 
			this->label12->AutoSize = true;
			this->label12->Location = System::Drawing::Point(145, 12);
			this->label12->Name = L"label12";
			this->label12->Size = System::Drawing::Size(41, 13);
			this->label12->TabIndex = 744;
			this->label12->Text = L"FWHM";
			// 
			// ampBox
			// 
			this->ampBox->Enabled = false;
			this->ampBox->Location = System::Drawing::Point(63, 28);
			this->ampBox->Name = L"ampBox";
			this->ampBox->Size = System::Drawing::Size(74, 20);
			this->ampBox->TabIndex = 737;
			this->ampBox->Text = L"0.000000";
			this->ampBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->ampBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// label13
			// 
			this->label13->AutoSize = true;
			this->label13->Location = System::Drawing::Point(60, 12);
			this->label13->Name = L"label13";
			this->label13->Size = System::Drawing::Size(53, 13);
			this->label13->TabIndex = 743;
			this->label13->Text = L"Amplitude";
			// 
			// label14
			// 
			this->label14->AutoSize = true;
			this->label14->Location = System::Drawing::Point(6, 31);
			this->label14->Name = L"label14";
			this->label14->Size = System::Drawing::Size(45, 13);
			this->label14->TabIndex = 742;
			this->label14->Text = L"<None>";
			// 
			// label15
			// 
			this->label15->AutoSize = true;
			this->label15->Location = System::Drawing::Point(6, 12);
			this->label15->Name = L"label15";
			this->label15->Size = System::Drawing::Size(32, 13);
			this->label15->TabIndex = 741;
			this->label15->Text = L"Peak";
			// 
			// listView_peaks
			// 
			this->listView_peaks->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->listView_peaks->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(7) {this->columnHeader1, 
				this->columnHeader2, this->columnHeader3, this->columnHeader4, this->columnHeader5, this->columnHeader6, this->columnHeader7});
			this->listView_peaks->FullRowSelect = true;
			this->listView_peaks->GridLines = true;
			this->listView_peaks->HideSelection = false;
			this->listView_peaks->Location = System::Drawing::Point(8, 102);
			this->listView_peaks->Name = L"listView_peaks";
			this->listView_peaks->Size = System::Drawing::Size(456, 169);
			this->listView_peaks->TabIndex = 736;
			this->listView_peaks->UseCompatibleStateImageBehavior = false;
			this->listView_peaks->View = System::Windows::Forms::View::Details;
			this->listView_peaks->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::listView_peaks_SelectedIndexChanged);
			this->listView_peaks->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &FormFactor::listView_peaks_KeyDown);
			// 
			// columnHeader1
			// 
			this->columnHeader1->Text = L"Peak #";
			this->columnHeader1->Width = 61;
			// 
			// columnHeader2
			// 
			this->columnHeader2->Text = L"Amplitude";
			this->columnHeader2->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader2->Width = 75;
			// 
			// columnHeader3
			// 
			this->columnHeader3->Text = L"M";
			this->columnHeader3->Width = 25;
			// 
			// columnHeader4
			// 
			this->columnHeader4->Text = L"FWHM";
			this->columnHeader4->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader4->Width = 73;
			// 
			// columnHeader5
			// 
			this->columnHeader5->Text = L"M";
			this->columnHeader5->Width = 25;
			// 
			// columnHeader6
			// 
			this->columnHeader6->Text = L"Center";
			this->columnHeader6->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->columnHeader6->Width = 57;
			// 
			// columnHeader7
			// 
			this->columnHeader7->Text = L"M";
			this->columnHeader7->Width = 26;
			// 
			// fwhmTrack
			// 
			this->fwhmTrack->Enabled = false;
			this->fwhmTrack->Location = System::Drawing::Point(148, 52);
			this->fwhmTrack->Maximum = 1000;
			this->fwhmTrack->Name = L"fwhmTrack";
			this->fwhmTrack->Size = System::Drawing::Size(75, 45);
			this->fwhmTrack->TabIndex = 750;
			this->fwhmTrack->TickStyle = System::Windows::Forms::TickStyle::None;
			this->fwhmTrack->Value = 500;
			this->fwhmTrack->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
			// 
			// ampTrack
			// 
			this->ampTrack->Enabled = false;
			this->ampTrack->Location = System::Drawing::Point(63, 52);
			this->ampTrack->Maximum = 1000;
			this->ampTrack->Name = L"ampTrack";
			this->ampTrack->Size = System::Drawing::Size(74, 45);
			this->ampTrack->TabIndex = 750;
			this->ampTrack->TickStyle = System::Windows::Forms::TickStyle::None;
			this->ampTrack->Value = 500;
			this->ampTrack->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
			// 
			// cailleGroupbox
			// 
			this->cailleGroupbox->Controls->Add(this->PeakFinderCailleButton);
			this->cailleGroupbox->Controls->Add(this->cailleN_diffusedCheckBox);
			this->cailleGroupbox->Controls->Add(this->cailleSigCheckbox);
			this->cailleGroupbox->Controls->Add(this->cailleNCheckbox);
			this->cailleGroupbox->Controls->Add(this->cailleEtaCheckbox);
			this->cailleGroupbox->Controls->Add(this->cailleAmpCheckbox);
			this->cailleGroupbox->Controls->Add(this->mutLabel);
			this->cailleGroupbox->Controls->Add(this->returnToLineshapeButton);
			this->cailleGroupbox->Controls->Add(this->cailleParamListView);
			this->cailleGroupbox->Controls->Add(this->caillePeaksListView);
			this->cailleGroupbox->Controls->Add(this->cailleMaxGroupbox);
			this->cailleGroupbox->Controls->Add(this->cailleMinGroupbox);
			this->cailleGroupbox->Controls->Add(this->cailleValueGroupbox);
			this->cailleGroupbox->Enabled = false;
			this->cailleGroupbox->Location = System::Drawing::Point(5, 5);
			this->cailleGroupbox->Name = L"cailleGroupbox";
			this->cailleGroupbox->Size = System::Drawing::Size(555, 275);
			this->cailleGroupbox->TabIndex = 762;
			this->cailleGroupbox->TabStop = false;
			this->cailleGroupbox->Text = L"Caille Peaks";
			this->cailleGroupbox->Visible = false;
			// 
			// PeakFinderCailleButton
			// 
			this->PeakFinderCailleButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->PeakFinderCailleButton->Enabled = false;
			this->PeakFinderCailleButton->Location = System::Drawing::Point(474, 201);
			this->PeakFinderCailleButton->Name = L"PeakFinderCailleButton";
			this->PeakFinderCailleButton->Size = System::Drawing::Size(74, 23);
			this->PeakFinderCailleButton->TabIndex = 758;
			this->PeakFinderCailleButton->Text = L"Peak Finder";
			this->PeakFinderCailleButton->UseVisualStyleBackColor = true;
			this->PeakFinderCailleButton->Click += gcnew System::EventHandler(this, &FormFactor::PeakPicker_Click);
			// 
			// cailleN_diffusedCheckBox
			// 
			this->cailleN_diffusedCheckBox->AutoSize = true;
			this->cailleN_diffusedCheckBox->Location = System::Drawing::Point(166, 112);
			this->cailleN_diffusedCheckBox->Name = L"cailleN_diffusedCheckBox";
			this->cailleN_diffusedCheckBox->Size = System::Drawing::Size(15, 14);
			this->cailleN_diffusedCheckBox->TabIndex = 757;
			this->cailleN_diffusedCheckBox->UseVisualStyleBackColor = true;
			// 
			// cailleSigCheckbox
			// 
			this->cailleSigCheckbox->AutoSize = true;
			this->cailleSigCheckbox->Location = System::Drawing::Point(166, 98);
			this->cailleSigCheckbox->Name = L"cailleSigCheckbox";
			this->cailleSigCheckbox->Size = System::Drawing::Size(15, 14);
			this->cailleSigCheckbox->TabIndex = 757;
			this->cailleSigCheckbox->UseVisualStyleBackColor = true;
			// 
			// cailleNCheckbox
			// 
			this->cailleNCheckbox->AutoSize = true;
			this->cailleNCheckbox->Location = System::Drawing::Point(166, 84);
			this->cailleNCheckbox->Name = L"cailleNCheckbox";
			this->cailleNCheckbox->Size = System::Drawing::Size(15, 14);
			this->cailleNCheckbox->TabIndex = 757;
			this->cailleNCheckbox->UseVisualStyleBackColor = true;
			// 
			// cailleEtaCheckbox
			// 
			this->cailleEtaCheckbox->AutoSize = true;
			this->cailleEtaCheckbox->Location = System::Drawing::Point(166, 70);
			this->cailleEtaCheckbox->Name = L"cailleEtaCheckbox";
			this->cailleEtaCheckbox->Size = System::Drawing::Size(15, 14);
			this->cailleEtaCheckbox->TabIndex = 757;
			this->cailleEtaCheckbox->UseVisualStyleBackColor = true;
			// 
			// cailleAmpCheckbox
			// 
			this->cailleAmpCheckbox->AutoSize = true;
			this->cailleAmpCheckbox->Location = System::Drawing::Point(166, 56);
			this->cailleAmpCheckbox->Name = L"cailleAmpCheckbox";
			this->cailleAmpCheckbox->Size = System::Drawing::Size(15, 14);
			this->cailleAmpCheckbox->TabIndex = 757;
			this->cailleAmpCheckbox->UseVisualStyleBackColor = true;
			// 
			// mutLabel
			// 
			this->mutLabel->AutoSize = true;
			this->mutLabel->Location = System::Drawing::Point(160, 40);
			this->mutLabel->Name = L"mutLabel";
			this->mutLabel->Size = System::Drawing::Size(25, 13);
			this->mutLabel->TabIndex = 756;
			this->mutLabel->Text = L"Mut";
			// 
			// returnToLineshapeButton
			// 
			this->returnToLineshapeButton->Location = System::Drawing::Point(442, 233);
			this->returnToLineshapeButton->Name = L"returnToLineshapeButton";
			this->returnToLineshapeButton->Size = System::Drawing::Size(106, 36);
			this->returnToLineshapeButton->TabIndex = 755;
			this->returnToLineshapeButton->Text = L"Return to Lineshape Pane";
			this->returnToLineshapeButton->UseVisualStyleBackColor = true;
			this->returnToLineshapeButton->Click += gcnew System::EventHandler(this, &FormFactor::returnToLineshapeButton_Click);
			// 
			// cailleParamListView
			// 
			this->cailleParamListView->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->cailleParamListView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(4) {this->cailleParamColumnHeader, 
				this->cailleValuecolumnHeader, this->cailleMinColumnHeader, this->cailleMaxColumnHeader});
			this->cailleParamListView->FullRowSelect = true;
			this->cailleParamListView->GridLines = true;
			this->cailleParamListView->HideSelection = false;
			this->cailleParamListView->Location = System::Drawing::Point(185, 36);
			this->cailleParamListView->MultiSelect = false;
			this->cailleParamListView->Name = L"cailleParamListView";
			this->cailleParamListView->Size = System::Drawing::Size(361, 102);
			this->cailleParamListView->TabIndex = 754;
			this->cailleParamListView->UseCompatibleStateImageBehavior = false;
			this->cailleParamListView->View = System::Windows::Forms::View::Details;
			this->cailleParamListView->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::cailleParamListView_SelectedIndexChanged);
			// 
			// cailleParamColumnHeader
			// 
			this->cailleParamColumnHeader->Text = L"Parameter";
			this->cailleParamColumnHeader->Width = 88;
			// 
			// cailleValuecolumnHeader
			// 
			this->cailleValuecolumnHeader->Text = L"Value";
			this->cailleValuecolumnHeader->Width = 88;
			// 
			// cailleMinColumnHeader
			// 
			this->cailleMinColumnHeader->Text = L"Minimum";
			this->cailleMinColumnHeader->Width = 88;
			// 
			// cailleMaxColumnHeader
			// 
			this->cailleMaxColumnHeader->Text = L"Maximum";
			this->cailleMaxColumnHeader->Width = 88;
			// 
			// caillePeaksListView
			// 
			this->caillePeaksListView->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->caillePeaksListView->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(2) {this->hColumnHeader, 
				this->cailleCenterColumnHeader});
			this->caillePeaksListView->GridLines = true;
			this->caillePeaksListView->Location = System::Drawing::Point(12, 21);
			this->caillePeaksListView->Name = L"caillePeaksListView";
			this->caillePeaksListView->Size = System::Drawing::Size(139, 248);
			this->caillePeaksListView->TabIndex = 754;
			this->caillePeaksListView->UseCompatibleStateImageBehavior = false;
			this->caillePeaksListView->View = System::Windows::Forms::View::Details;
			// 
			// hColumnHeader
			// 
			this->hColumnHeader->Text = L"h";
			this->hColumnHeader->Width = 39;
			// 
			// cailleCenterColumnHeader
			// 
			this->cailleCenterColumnHeader->Text = L"Peak Center";
			this->cailleCenterColumnHeader->Width = 84;
			// 
			// cailleMaxGroupbox
			// 
			this->cailleMaxGroupbox->Controls->Add(this->cailleMaxTextBox);
			this->cailleMaxGroupbox->Location = System::Drawing::Point(444, 144);
			this->cailleMaxGroupbox->Name = L"cailleMaxGroupbox";
			this->cailleMaxGroupbox->Size = System::Drawing::Size(99, 46);
			this->cailleMaxGroupbox->TabIndex = 753;
			this->cailleMaxGroupbox->TabStop = false;
			this->cailleMaxGroupbox->Text = L"Maximum";
			// 
			// cailleMaxTextBox
			// 
			this->cailleMaxTextBox->Enabled = false;
			this->cailleMaxTextBox->Location = System::Drawing::Point(12, 16);
			this->cailleMaxTextBox->Name = L"cailleMaxTextBox";
			this->cailleMaxTextBox->Size = System::Drawing::Size(74, 20);
			this->cailleMaxTextBox->TabIndex = 751;
			this->cailleMaxTextBox->Text = L"0.000000";
			this->cailleMaxTextBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->cailleMaxTextBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// cailleMinGroupbox
			// 
			this->cailleMinGroupbox->Controls->Add(this->cailleMinTextBox);
			this->cailleMinGroupbox->Location = System::Drawing::Point(339, 144);
			this->cailleMinGroupbox->Name = L"cailleMinGroupbox";
			this->cailleMinGroupbox->Size = System::Drawing::Size(99, 46);
			this->cailleMinGroupbox->TabIndex = 753;
			this->cailleMinGroupbox->TabStop = false;
			this->cailleMinGroupbox->Text = L"Minimum";
			// 
			// cailleMinTextBox
			// 
			this->cailleMinTextBox->Enabled = false;
			this->cailleMinTextBox->Location = System::Drawing::Point(12, 16);
			this->cailleMinTextBox->Name = L"cailleMinTextBox";
			this->cailleMinTextBox->Size = System::Drawing::Size(74, 20);
			this->cailleMinTextBox->TabIndex = 751;
			this->cailleMinTextBox->Text = L"0.000000";
			this->cailleMinTextBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->cailleMinTextBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// cailleValueGroupbox
			// 
			this->cailleValueGroupbox->Controls->Add(this->cailleValueTrackBar);
			this->cailleValueGroupbox->Controls->Add(this->cailleValueTextBox);
			this->cailleValueGroupbox->Location = System::Drawing::Point(234, 144);
			this->cailleValueGroupbox->Name = L"cailleValueGroupbox";
			this->cailleValueGroupbox->Size = System::Drawing::Size(99, 86);
			this->cailleValueGroupbox->TabIndex = 753;
			this->cailleValueGroupbox->TabStop = false;
			this->cailleValueGroupbox->Text = L"Parameter Value";
			// 
			// cailleValueTrackBar
			// 
			this->cailleValueTrackBar->Enabled = false;
			this->cailleValueTrackBar->Location = System::Drawing::Point(12, 40);
			this->cailleValueTrackBar->Maximum = 1000;
			this->cailleValueTrackBar->Name = L"cailleValueTrackBar";
			this->cailleValueTrackBar->Size = System::Drawing::Size(74, 45);
			this->cailleValueTrackBar->TabIndex = 752;
			this->cailleValueTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->cailleValueTrackBar->Value = 500;
			this->cailleValueTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::centerTrackBar);
			// 
			// cailleValueTextBox
			// 
			this->cailleValueTextBox->Enabled = false;
			this->cailleValueTextBox->Location = System::Drawing::Point(12, 16);
			this->cailleValueTextBox->Name = L"cailleValueTextBox";
			this->cailleValueTextBox->Size = System::Drawing::Size(74, 20);
			this->cailleValueTextBox->TabIndex = 751;
			this->cailleValueTextBox->Text = L"0.000000";
			this->cailleValueTextBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->cailleValueTextBox->Leave += gcnew System::EventHandler(this, &FormFactor::StructureFactor_ParamTextLeave);
			// 
			// BGTab
			// 
			this->BGTab->BackColor = System::Drawing::Color::PaleGoldenrod;
			this->BGTab->Controls->Add(this->bgUseCheckBox);
			this->BGTab->Controls->Add(this->label26);
			this->BGTab->Controls->Add(this->manipBox);
			this->BGTab->Controls->Add(this->label25);
			this->BGTab->Controls->Add(this->label24);
			this->BGTab->Controls->Add(this->DEBUG_label);
			this->BGTab->Controls->Add(this->functionsGroupBox);
			this->BGTab->Location = System::Drawing::Point(4, 22);
			this->BGTab->Name = L"BGTab";
			this->BGTab->Padding = System::Windows::Forms::Padding(3);
			this->BGTab->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->BGTab->Size = System::Drawing::Size(630, 577);
			this->BGTab->TabIndex = 2;
			this->BGTab->Text = L"Background";
			// 
			// bgUseCheckBox
			// 
			this->bgUseCheckBox->AutoSize = true;
			this->bgUseCheckBox->Checked = true;
			this->bgUseCheckBox->CheckState = System::Windows::Forms::CheckState::Checked;
			this->bgUseCheckBox->Enabled = false;
			this->bgUseCheckBox->Location = System::Drawing::Point(408, 15);
			this->bgUseCheckBox->Name = L"bgUseCheckBox";
			this->bgUseCheckBox->Size = System::Drawing::Size(45, 17);
			this->bgUseCheckBox->TabIndex = 773;
			this->bgUseCheckBox->Text = L"Use";
			this->bgUseCheckBox->UseVisualStyleBackColor = true;
			this->bgUseCheckBox->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::useCheckBox_CheckedChanged);
			// 
			// label26
			// 
			this->label26->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->label26->AutoSize = true;
			this->label26->Location = System::Drawing::Point(360, 555);
			this->label26->Name = L"label26";
			this->label26->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->label26->Size = System::Drawing::Size(205, 13);
			this->label26->TabIndex = 775;
			this->label26->Text = L"Exponent(q) = amp*e^[-(q-xcenter)/decay]";
			// 
			// manipBox
			// 
			this->manipBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->manipBox->Controls->Add(this->fitToBaseline);
			this->manipBox->Controls->Add(this->smooth);
			this->manipBox->Controls->Add(this->baseline);
			this->manipBox->Controls->Add(this->zeroBG);
			this->manipBox->Location = System::Drawing::Point(5, 492);
			this->manipBox->Name = L"manipBox";
			this->manipBox->Size = System::Drawing::Size(271, 79);
			this->manipBox->TabIndex = 727;
			this->manipBox->TabStop = false;
			this->manipBox->Text = L"Data Manipulation:";
			// 
			// fitToBaseline
			// 
			this->fitToBaseline->Enabled = false;
			this->fitToBaseline->Location = System::Drawing::Point(123, 48);
			this->fitToBaseline->Name = L"fitToBaseline";
			this->fitToBaseline->Size = System::Drawing::Size(142, 23);
			this->fitToBaseline->TabIndex = 776;
			this->fitToBaseline->Text = L"Fit Background to Baseline";
			this->fitToBaseline->UseVisualStyleBackColor = true;
			this->fitToBaseline->Click += gcnew System::EventHandler(this, &FormFactor::fitToBaseline_Click);
			// 
			// smooth
			// 
			this->smooth->Enabled = false;
			this->smooth->Location = System::Drawing::Point(6, 48);
			this->smooth->Name = L"smooth";
			this->smooth->Size = System::Drawing::Size(111, 23);
			this->smooth->TabIndex = 19;
			this->smooth->Text = L"Smooth Signal...";
			this->smooth->UseVisualStyleBackColor = true;
			this->smooth->Click += gcnew System::EventHandler(this, &FormFactor::smooth_Click);
			// 
			// baseline
			// 
			this->baseline->Location = System::Drawing::Point(123, 19);
			this->baseline->Name = L"baseline";
			this->baseline->Size = System::Drawing::Size(142, 23);
			this->baseline->TabIndex = 18;
			this->baseline->Text = L"Crop / Extract Baseline...";
			this->baseline->UseVisualStyleBackColor = true;
			this->baseline->Click += gcnew System::EventHandler(this, &FormFactor::baseline_Click);
			// 
			// zeroBG
			// 
			this->zeroBG->Location = System::Drawing::Point(6, 19);
			this->zeroBG->Name = L"zeroBG";
			this->zeroBG->Size = System::Drawing::Size(111, 23);
			this->zeroBG->TabIndex = 17;
			this->zeroBG->Text = L"Zero Background";
			this->zeroBG->UseVisualStyleBackColor = true;
			this->zeroBG->Click += gcnew System::EventHandler(this, &FormFactor::zeroBG_Click);
			// 
			// label25
			// 
			this->label25->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->label25->AutoSize = true;
			this->label25->Location = System::Drawing::Point(431, 516);
			this->label25->Name = L"label25";
			this->label25->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->label25->Size = System::Drawing::Size(134, 13);
			this->label25->TabIndex = 774;
			this->label25->Text = L"Linear(q) = -amp*(q)+decay";
			// 
			// label24
			// 
			this->label24->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->label24->AutoSize = true;
			this->label24->Location = System::Drawing::Point(386, 534);
			this->label24->Name = L"label24";
			this->label24->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->label24->Size = System::Drawing::Size(179, 13);
			this->label24->TabIndex = 773;
			this->label24->Text = L"Power(q) = amp*(q-xcenter)^(-decay)";
			// 
			// DEBUG_label
			// 
			this->DEBUG_label->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->DEBUG_label->AutoSize = true;
			this->DEBUG_label->Location = System::Drawing::Point(374, 468);
			this->DEBUG_label->Name = L"DEBUG_label";
			this->DEBUG_label->RightToLeft = System::Windows::Forms::RightToLeft::No;
			this->DEBUG_label->Size = System::Drawing::Size(196, 13);
			this->DEBUG_label->TabIndex = 1;
			this->DEBUG_label->Text = L"Signal(q) = a * [(FF(q) + b) * SF(q)] + B(q)";
			// 
			// functionsGroupBox
			// 
			this->functionsGroupBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->functionsGroupBox->Controls->Add(this->maxLabel);
			this->functionsGroupBox->Controls->Add(this->minLabel);
			this->functionsGroupBox->Controls->Add(this->baseLabel);
			this->functionsGroupBox->Controls->Add(this->baseMaxBox);
			this->functionsGroupBox->Controls->Add(this->baseMinBox);
			this->functionsGroupBox->Controls->Add(this->baseTextbox);
			this->functionsGroupBox->Controls->Add(this->funcTypeBox);
			this->functionsGroupBox->Controls->Add(this->baseMut);
			this->functionsGroupBox->Controls->Add(this->xCenterLabel);
			this->functionsGroupBox->Controls->Add(this->xCenterMut);
			this->functionsGroupBox->Controls->Add(this->xcMaxBox);
			this->functionsGroupBox->Controls->Add(this->xcMinBox);
			this->functionsGroupBox->Controls->Add(this->decMaxBox);
			this->functionsGroupBox->Controls->Add(this->xCenterTextbox);
			this->functionsGroupBox->Controls->Add(this->decMinBox);
			this->functionsGroupBox->Controls->Add(this->decayMut);
			this->functionsGroupBox->Controls->Add(this->decayTextbox);
			this->functionsGroupBox->Controls->Add(this->decayLabel);
			this->functionsGroupBox->Controls->Add(this->removeFuncButton);
			this->functionsGroupBox->Controls->Add(this->addFuncButton);
			this->functionsGroupBox->Controls->Add(this->BGListview);
			this->functionsGroupBox->Controls->Add(this->baseTrackBar);
			this->functionsGroupBox->Controls->Add(this->decayTrackBar);
			this->functionsGroupBox->Controls->Add(this->xCenterTrackBar);
			this->functionsGroupBox->Location = System::Drawing::Point(0, 3);
			this->functionsGroupBox->Name = L"functionsGroupBox";
			this->functionsGroupBox->Size = System::Drawing::Size(565, 462);
			this->functionsGroupBox->TabIndex = 0;
			this->functionsGroupBox->TabStop = false;
			this->functionsGroupBox->Text = L"Background Functions";
			// 
			// maxLabel
			// 
			this->maxLabel->AutoSize = true;
			this->maxLabel->Location = System::Drawing::Point(77, 129);
			this->maxLabel->Name = L"maxLabel";
			this->maxLabel->Size = System::Drawing::Size(54, 13);
			this->maxLabel->TabIndex = 764;
			this->maxLabel->Text = L"Maximum:";
			// 
			// minLabel
			// 
			this->minLabel->AutoSize = true;
			this->minLabel->Location = System::Drawing::Point(77, 103);
			this->minLabel->Name = L"minLabel";
			this->minLabel->Size = System::Drawing::Size(51, 13);
			this->minLabel->TabIndex = 764;
			this->minLabel->Text = L"Minimum:";
			// 
			// baseLabel
			// 
			this->baseLabel->AutoSize = true;
			this->baseLabel->Location = System::Drawing::Point(133, 38);
			this->baseLabel->Name = L"baseLabel";
			this->baseLabel->Size = System::Drawing::Size(53, 13);
			this->baseLabel->TabIndex = 764;
			this->baseLabel->Text = L"Amplitude";
			// 
			// baseMaxBox
			// 
			this->baseMaxBox->Enabled = false;
			this->baseMaxBox->Location = System::Drawing::Point(136, 129);
			this->baseMaxBox->Name = L"baseMaxBox";
			this->baseMaxBox->Size = System::Drawing::Size(74, 20);
			this->baseMaxBox->TabIndex = 758;
			this->baseMaxBox->Text = L"0.000000";
			this->baseMaxBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->baseMaxBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// baseMinBox
			// 
			this->baseMinBox->Enabled = false;
			this->baseMinBox->Location = System::Drawing::Point(136, 103);
			this->baseMinBox->Name = L"baseMinBox";
			this->baseMinBox->Size = System::Drawing::Size(74, 20);
			this->baseMinBox->TabIndex = 758;
			this->baseMinBox->Text = L"0.000000";
			this->baseMinBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->baseMinBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// baseTextbox
			// 
			this->baseTextbox->Enabled = false;
			this->baseTextbox->Location = System::Drawing::Point(136, 54);
			this->baseTextbox->Name = L"baseTextbox";
			this->baseTextbox->Size = System::Drawing::Size(74, 20);
			this->baseTextbox->TabIndex = 758;
			this->baseTextbox->Text = L"0.000000";
			this->baseTextbox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->baseTextbox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// funcTypeBox
			// 
			this->funcTypeBox->Controls->Add(this->funcTypeList);
			this->funcTypeBox->Location = System::Drawing::Point(7, 43);
			this->funcTypeBox->Name = L"funcTypeBox";
			this->funcTypeBox->Size = System::Drawing::Size(121, 42);
			this->funcTypeBox->TabIndex = 772;
			this->funcTypeBox->TabStop = false;
			this->funcTypeBox->Text = L"Func. Type:";
			// 
			// funcTypeList
			// 
			this->funcTypeList->DropDownStyle = System::Windows::Forms::ComboBoxStyle::DropDownList;
			this->funcTypeList->FormattingEnabled = true;
			this->funcTypeList->Items->AddRange(gcnew cli::array< System::Object^  >(3) {L"Exponent", L"Linear", L"Power"});
			this->funcTypeList->Location = System::Drawing::Point(4, 15);
			this->funcTypeList->Name = L"funcTypeList";
			this->funcTypeList->Size = System::Drawing::Size(111, 21);
			this->funcTypeList->TabIndex = 730;
			this->funcTypeList->SelectionChangeCommitted += gcnew System::EventHandler(this, &FormFactor::funcTypeList_SelectedIndexChanged);
			// 
			// baseMut
			// 
			this->baseMut->AutoSize = true;
			this->baseMut->Enabled = false;
			this->baseMut->Location = System::Drawing::Point(195, 39);
			this->baseMut->Name = L"baseMut";
			this->baseMut->Size = System::Drawing::Size(15, 14);
			this->baseMut->TabIndex = 759;
			this->baseMut->UseVisualStyleBackColor = true;
			this->baseMut->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::BGCheckedChanged);
			// 
			// xCenterLabel
			// 
			this->xCenterLabel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xCenterLabel->AutoSize = true;
			this->xCenterLabel->Location = System::Drawing::Point(458, 39);
			this->xCenterLabel->Name = L"xCenterLabel";
			this->xCenterLabel->Size = System::Drawing::Size(48, 13);
			this->xCenterLabel->TabIndex = 771;
			this->xCenterLabel->Text = L"X Center";
			// 
			// xCenterMut
			// 
			this->xCenterMut->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xCenterMut->AutoSize = true;
			this->xCenterMut->Enabled = false;
			this->xCenterMut->Location = System::Drawing::Point(518, 39);
			this->xCenterMut->Name = L"xCenterMut";
			this->xCenterMut->Size = System::Drawing::Size(15, 14);
			this->xCenterMut->TabIndex = 767;
			this->xCenterMut->UseVisualStyleBackColor = true;
			this->xCenterMut->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::BGCheckedChanged);
			// 
			// xcMaxBox
			// 
			this->xcMaxBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xcMaxBox->Enabled = false;
			this->xcMaxBox->Location = System::Drawing::Point(459, 130);
			this->xcMaxBox->Name = L"xcMaxBox";
			this->xcMaxBox->Size = System::Drawing::Size(74, 20);
			this->xcMaxBox->TabIndex = 766;
			this->xcMaxBox->Text = L"0.000000";
			this->xcMaxBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->xcMaxBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// xcMinBox
			// 
			this->xcMinBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xcMinBox->Enabled = false;
			this->xcMinBox->Location = System::Drawing::Point(459, 104);
			this->xcMinBox->Name = L"xcMinBox";
			this->xcMinBox->Size = System::Drawing::Size(74, 20);
			this->xcMinBox->TabIndex = 766;
			this->xcMinBox->Text = L"0.000000";
			this->xcMinBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->xcMinBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// decMaxBox
			// 
			this->decMaxBox->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decMaxBox->Enabled = false;
			this->decMaxBox->Location = System::Drawing::Point(294, 129);
			this->decMaxBox->Name = L"decMaxBox";
			this->decMaxBox->Size = System::Drawing::Size(74, 20);
			this->decMaxBox->TabIndex = 760;
			this->decMaxBox->Text = L"0.000000";
			this->decMaxBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->decMaxBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// xCenterTextbox
			// 
			this->xCenterTextbox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xCenterTextbox->Enabled = false;
			this->xCenterTextbox->Location = System::Drawing::Point(459, 55);
			this->xCenterTextbox->Name = L"xCenterTextbox";
			this->xCenterTextbox->Size = System::Drawing::Size(74, 20);
			this->xCenterTextbox->TabIndex = 766;
			this->xCenterTextbox->Text = L"0.000000";
			this->xCenterTextbox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->xCenterTextbox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// decMinBox
			// 
			this->decMinBox->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decMinBox->Enabled = false;
			this->decMinBox->Location = System::Drawing::Point(294, 103);
			this->decMinBox->Name = L"decMinBox";
			this->decMinBox->Size = System::Drawing::Size(74, 20);
			this->decMinBox->TabIndex = 760;
			this->decMinBox->Text = L"0.000000";
			this->decMinBox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->decMinBox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// decayMut
			// 
			this->decayMut->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decayMut->AutoSize = true;
			this->decayMut->Enabled = false;
			this->decayMut->Location = System::Drawing::Point(354, 39);
			this->decayMut->Name = L"decayMut";
			this->decayMut->Size = System::Drawing::Size(15, 14);
			this->decayMut->TabIndex = 761;
			this->decayMut->UseVisualStyleBackColor = true;
			this->decayMut->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::BGCheckedChanged);
			// 
			// decayTextbox
			// 
			this->decayTextbox->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decayTextbox->Enabled = false;
			this->decayTextbox->Location = System::Drawing::Point(294, 54);
			this->decayTextbox->Name = L"decayTextbox";
			this->decayTextbox->Size = System::Drawing::Size(74, 20);
			this->decayTextbox->TabIndex = 760;
			this->decayTextbox->Text = L"0.000000";
			this->decayTextbox->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->decayTextbox->Leave += gcnew System::EventHandler(this, &FormFactor::BGTextBox_Leave);
			// 
			// decayLabel
			// 
			this->decayLabel->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decayLabel->AutoSize = true;
			this->decayLabel->Location = System::Drawing::Point(291, 38);
			this->decayLabel->Name = L"decayLabel";
			this->decayLabel->Size = System::Drawing::Size(38, 13);
			this->decayLabel->TabIndex = 765;
			this->decayLabel->Text = L"Decay";
			// 
			// removeFuncButton
			// 
			this->removeFuncButton->Enabled = false;
			this->removeFuncButton->Location = System::Drawing::Point(89, 149);
			this->removeFuncButton->Name = L"removeFuncButton";
			this->removeFuncButton->Size = System::Drawing::Size(26, 23);
			this->removeFuncButton->TabIndex = 757;
			this->removeFuncButton->Text = L"-";
			this->removeFuncButton->UseVisualStyleBackColor = true;
			this->removeFuncButton->Click += gcnew System::EventHandler(this, &FormFactor::removeFuncButton_Click);
			// 
			// addFuncButton
			// 
			this->addFuncButton->Location = System::Drawing::Point(57, 149);
			this->addFuncButton->Name = L"addFuncButton";
			this->addFuncButton->Size = System::Drawing::Size(26, 23);
			this->addFuncButton->TabIndex = 756;
			this->addFuncButton->Text = L"+";
			this->addFuncButton->UseVisualStyleBackColor = true;
			this->addFuncButton->Click += gcnew System::EventHandler(this, &FormFactor::addFuncButton_Click);
			// 
			// BGListview
			// 
			this->BGListview->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->BGListview->Columns->AddRange(gcnew cli::array< System::Windows::Forms::ColumnHeader^  >(8) {this->funcNumHeader, this->funcHeader, 
				this->BaseHeader, this->BaseMutHeader, this->DecHeader, this->DecMutHeader, this->xCenterHeader, this->xCenterMutHeader});
			this->BGListview->FullRowSelect = true;
			this->BGListview->GridLines = true;
			this->BGListview->HideSelection = false;
			this->BGListview->Location = System::Drawing::Point(57, 190);
			this->BGListview->Name = L"BGListview";
			this->BGListview->Size = System::Drawing::Size(435, 266);
			this->BGListview->TabIndex = 0;
			this->BGListview->UseCompatibleStateImageBehavior = false;
			this->BGListview->View = System::Windows::Forms::View::Details;
			this->BGListview->SelectedIndexChanged += gcnew System::EventHandler(this, &FormFactor::BGListview_SelectedIndexChanged);
			this->BGListview->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &FormFactor::BGListview_KeyDown);
			// 
			// funcNumHeader
			// 
			this->funcNumHeader->Text = L"#";
			this->funcNumHeader->Width = 28;
			// 
			// funcHeader
			// 
			this->funcHeader->Text = L"Func. Type";
			this->funcHeader->Width = 76;
			// 
			// BaseHeader
			// 
			this->BaseHeader->Text = L"Amplitude";
			this->BaseHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->BaseHeader->Width = 89;
			// 
			// BaseMutHeader
			// 
			this->BaseMutHeader->Text = L"M";
			this->BaseMutHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->BaseMutHeader->Width = 27;
			// 
			// DecHeader
			// 
			this->DecHeader->Text = L"Decay";
			this->DecHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->DecHeader->Width = 79;
			// 
			// DecMutHeader
			// 
			this->DecMutHeader->Text = L"M";
			this->DecMutHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->DecMutHeader->Width = 27;
			// 
			// xCenterHeader
			// 
			this->xCenterHeader->Text = L"X Center";
			this->xCenterHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->xCenterHeader->Width = 76;
			// 
			// xCenterMutHeader
			// 
			this->xCenterMutHeader->Text = L"M";
			this->xCenterMutHeader->TextAlign = System::Windows::Forms::HorizontalAlignment::Center;
			this->xCenterMutHeader->Width = 29;
			// 
			// baseTrackBar
			// 
			this->baseTrackBar->Enabled = false;
			this->baseTrackBar->Location = System::Drawing::Point(136, 78);
			this->baseTrackBar->Maximum = 1000;
			this->baseTrackBar->Name = L"baseTrackBar";
			this->baseTrackBar->Size = System::Drawing::Size(74, 45);
			this->baseTrackBar->TabIndex = 768;
			this->baseTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->baseTrackBar->Value = 500;
			this->baseTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::BGTrackBar_MouseUp);
			// 
			// decayTrackBar
			// 
			this->decayTrackBar->Anchor = System::Windows::Forms::AnchorStyles::Top;
			this->decayTrackBar->Enabled = false;
			this->decayTrackBar->Location = System::Drawing::Point(294, 78);
			this->decayTrackBar->Maximum = 1000;
			this->decayTrackBar->Name = L"decayTrackBar";
			this->decayTrackBar->Size = System::Drawing::Size(75, 45);
			this->decayTrackBar->TabIndex = 769;
			this->decayTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->decayTrackBar->Value = 500;
			this->decayTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::BGTrackBar_MouseUp);
			// 
			// xCenterTrackBar
			// 
			this->xCenterTrackBar->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->xCenterTrackBar->Enabled = false;
			this->xCenterTrackBar->Location = System::Drawing::Point(459, 79);
			this->xCenterTrackBar->Maximum = 1000;
			this->xCenterTrackBar->Name = L"xCenterTrackBar";
			this->xCenterTrackBar->Size = System::Drawing::Size(74, 45);
			this->xCenterTrackBar->TabIndex = 770;
			this->xCenterTrackBar->TickStyle = System::Windows::Forms::TickStyle::None;
			this->xCenterTrackBar->Value = 500;
			this->xCenterTrackBar->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &FormFactor::BGTrackBar_MouseUp);
			// 
			// movetophases
			// 
			this->movetophases->Controls->Add(this->changeModel);
			this->movetophases->Controls->Add(this->maskButton);
			this->movetophases->Controls->Add(this->rsquared);
			this->movetophases->Controls->Add(this->LocOnGraph);
			this->movetophases->Controls->Add(this->logXCheckBox);
			this->movetophases->Controls->Add(this->logScale);
			this->movetophases->Controls->Add(this->changeData);
			this->movetophases->Controls->Add(this->save);
			this->movetophases->Controls->Add(this->wssr);
			this->movetophases->Dock = System::Windows::Forms::DockStyle::Fill;
			this->movetophases->Location = System::Drawing::Point(647, 612);
			this->movetophases->Name = L"movetophases";
			this->movetophases->Size = System::Drawing::Size(507, 77);
			this->movetophases->TabIndex = 0;
			// 
			// changeModel
			// 
			this->changeModel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->changeModel->Location = System::Drawing::Point(303, 51);
			this->changeModel->Name = L"changeModel";
			this->changeModel->Size = System::Drawing::Size(98, 23);
			this->changeModel->TabIndex = 362;
			this->changeModel->Text = L"Change Model...";
			this->changeModel->UseVisualStyleBackColor = true;
			this->changeModel->Click += gcnew System::EventHandler(this, &FormFactor::changeModel_Click);
			// 
			// maskButton
			// 
			this->maskButton->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->maskButton->Location = System::Drawing::Point(125, 51);
			this->maskButton->Name = L"maskButton";
			this->maskButton->Size = System::Drawing::Size(75, 23);
			this->maskButton->TabIndex = 361;
			this->maskButton->Text = L"Mask...";
			this->maskButton->UseVisualStyleBackColor = true;
			this->maskButton->Click += gcnew System::EventHandler(this, &FormFactor::maskButton_Click);
			// 
			// rsquared
			// 
			this->rsquared->AutoSize = true;
			this->rsquared->Font = (gcnew System::Drawing::Font(L"Times New Roman", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(161)));
			this->rsquared->Location = System::Drawing::Point(3, 26);
			this->rsquared->Name = L"rsquared";
			this->rsquared->Size = System::Drawing::Size(20, 15);
			this->rsquared->TabIndex = 27;
			this->rsquared->Text = L" = ";
			// 
			// LocOnGraph
			// 
			this->LocOnGraph->AutoSize = true;
			this->LocOnGraph->Location = System::Drawing::Point(156, 3);
			this->LocOnGraph->Name = L"LocOnGraph";
			this->LocOnGraph->Size = System::Drawing::Size(28, 13);
			this->LocOnGraph->TabIndex = 0;
			this->LocOnGraph->Text = L"(0,0)";
			// 
			// logXCheckBox
			// 
			this->logXCheckBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->logXCheckBox->AutoSize = true;
			this->logXCheckBox->Location = System::Drawing::Point(449, 32);
			this->logXCheckBox->Name = L"logXCheckBox";
			this->logXCheckBox->Size = System::Drawing::Size(56, 17);
			this->logXCheckBox->TabIndex = 340;
			this->logXCheckBox->Text = L"Log(q)";
			this->logXCheckBox->UseVisualStyleBackColor = true;
			this->logXCheckBox->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::logScale_CheckedChanged);
			// 
			// logScale
			// 
			this->logScale->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->logScale->AutoSize = true;
			this->logScale->Location = System::Drawing::Point(390, 31);
			this->logScale->Name = L"logScale";
			this->logScale->Size = System::Drawing::Size(53, 17);
			this->logScale->TabIndex = 340;
			this->logScale->Text = L"Log(I)";
			this->logScale->UseVisualStyleBackColor = true;
			this->logScale->CheckedChanged += gcnew System::EventHandler(this, &FormFactor::logScale_CheckedChanged);
			// 
			// changeData
			// 
			this->changeData->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->changeData->Location = System::Drawing::Point(206, 51);
			this->changeData->Name = L"changeData";
			this->changeData->Size = System::Drawing::Size(91, 23);
			this->changeData->TabIndex = 350;
			this->changeData->Text = L"Change Data...";
			this->changeData->UseVisualStyleBackColor = true;
			this->changeData->Click += gcnew System::EventHandler(this, &FormFactor::changeData_Click);
			// 
			// save
			// 
			this->save->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->save->Enabled = false;
			this->save->Location = System::Drawing::Point(407, 51);
			this->save->Name = L"save";
			this->save->Size = System::Drawing::Size(97, 23);
			this->save->TabIndex = 360;
			this->save->Text = L"Save Parameters";
			this->save->UseVisualStyleBackColor = true;
			this->save->Click += gcnew System::EventHandler(this, &FormFactor::save_Click);
			// 
			// wssr
			// 
			this->wssr->AutoSize = true;
			this->wssr->Font = (gcnew System::Drawing::Font(L"Times New Roman", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point, 
				static_cast<System::Byte>(161)));
			this->wssr->Location = System::Drawing::Point(3, 2);
			this->wssr->Name = L"wssr";
			this->wssr->Size = System::Drawing::Size(20, 15);
			this->wssr->TabIndex = 0;
			this->wssr->Text = L" = ";
			// 
			// groupBox1
			// 
			this->groupBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom) 
				| System::Windows::Forms::AnchorStyles::Left) 
				| System::Windows::Forms::AnchorStyles::Right));
			this->groupBox1->Controls->Add(this->maskToolStrip);
			this->groupBox1->Controls->Add(this->label1);
			this->groupBox1->Location = System::Drawing::Point(647, 3);
			this->groupBox1->Name = L"groupBox1";
			this->groupBox1->Size = System::Drawing::Size(507, 603);
			this->groupBox1->TabIndex = 1000;
			this->groupBox1->TabStop = false;
			this->groupBox1->Text = L"Form Factor";
			// 
			// maskToolStrip
			// 
			this->maskToolStrip->Anchor = System::Windows::Forms::AnchorStyles::None;
			this->maskToolStrip->Dock = System::Windows::Forms::DockStyle::None;
			this->maskToolStrip->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(4) {this->addMaskButton, 
				this->removeMaskButton, this->invertMaskButton, this->clearMaskButton});
			this->maskToolStrip->Location = System::Drawing::Point(346, 12);
			this->maskToolStrip->Name = L"maskToolStrip";
			this->maskToolStrip->Size = System::Drawing::Size(104, 25);
			this->maskToolStrip->TabIndex = 1;
			this->maskToolStrip->Text = L"Mask Toolstrip";
			// 
			// addMaskButton
			// 
			this->addMaskButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->addMaskButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"addMaskButton.Image")));
			this->addMaskButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->addMaskButton->Name = L"addMaskButton";
			this->addMaskButton->Size = System::Drawing::Size(23, 22);
			this->addMaskButton->Text = L"toolStripButton1";
			this->addMaskButton->ToolTipText = L"Add masked regions to the data";
			this->addMaskButton->Click += gcnew System::EventHandler(this, &FormFactor::maskPanel_Click);
			// 
			// removeMaskButton
			// 
			this->removeMaskButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->removeMaskButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"removeMaskButton.Image")));
			this->removeMaskButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->removeMaskButton->Name = L"removeMaskButton";
			this->removeMaskButton->Size = System::Drawing::Size(23, 22);
			this->removeMaskButton->Text = L"toolStripButton2";
			this->removeMaskButton->ToolTipText = L"Remove masked regions from data";
			this->removeMaskButton->Click += gcnew System::EventHandler(this, &FormFactor::maskPanel_Click);
			// 
			// invertMaskButton
			// 
			this->invertMaskButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->invertMaskButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"invertMaskButton.Image")));
			this->invertMaskButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->invertMaskButton->Name = L"invertMaskButton";
			this->invertMaskButton->Size = System::Drawing::Size(23, 22);
			this->invertMaskButton->Text = L"toolStripButton3";
			this->invertMaskButton->ToolTipText = L"Invert masked and unmasked regions of the data";
			this->invertMaskButton->Click += gcnew System::EventHandler(this, &FormFactor::maskPanel_Click);
			// 
			// clearMaskButton
			// 
			this->clearMaskButton->DisplayStyle = System::Windows::Forms::ToolStripItemDisplayStyle::Image;
			this->clearMaskButton->Image = (cli::safe_cast<System::Drawing::Image^  >(resources->GetObject(L"clearMaskButton.Image")));
			this->clearMaskButton->ImageTransparentColor = System::Drawing::Color::Magenta;
			this->clearMaskButton->Name = L"clearMaskButton";
			this->clearMaskButton->Size = System::Drawing::Size(23, 22);
			this->clearMaskButton->Text = L"toolStripButton4";
			this->clearMaskButton->ToolTipText = L"Clear all masked regions";
			this->clearMaskButton->Click += gcnew System::EventHandler(this, &FormFactor::maskPanel_Click);
			// 
			// label1
			// 
			this->label1->Anchor = System::Windows::Forms::AnchorStyles::Right;
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(153, 288);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(247, 26);
			this->label1->TabIndex = 0;
			this->label1->Text = L"In order to fit the model data, define its background\r\nusing \"Data Manipulation\" " 
				L"to the left.";
			this->label1->TextAlign = System::Drawing::ContentAlignment::MiddleCenter;
			// 
			// panel3
			// 
			this->panel3->Controls->Add(this->CalcComboBox);
			this->panel3->Controls->Add(this->iterProgressBar);
			this->panel3->Controls->Add(this->progressBar1);
			this->panel3->Controls->Add(this->label6);
			this->panel3->Controls->Add(this->reportButton);
			this->panel3->Controls->Add(this->undo);
			this->panel3->Controls->Add(this->calculate);
			this->panel3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->panel3->Location = System::Drawing::Point(3, 612);
			this->panel3->Name = L"panel3";
			this->panel3->Size = System::Drawing::Size(638, 77);
			this->panel3->TabIndex = 3;
			// 
			// CalcComboBox
			// 
			this->CalcComboBox->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Left));
			this->CalcComboBox->FormattingEnabled = true;
			this->CalcComboBox->Items->AddRange(gcnew cli::array< System::Object^  >(5) {L"This Tab", L"FF+SF", L"FF+BG", L"SF+BG", L"All Tabs"});
			this->CalcComboBox->Location = System::Drawing::Point(3, 7);
			this->CalcComboBox->Name = L"CalcComboBox";
			this->CalcComboBox->Size = System::Drawing::Size(101, 21);
			this->CalcComboBox->TabIndex = 331;
			this->CalcComboBox->Text = L"This Tab";
			// 
			// iterProgressBar
			// 
			this->iterProgressBar->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->iterProgressBar->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)), 
				static_cast<System::Int32>(static_cast<System::Byte>(0)));
			this->iterProgressBar->Location = System::Drawing::Point(513, 51);
			this->iterProgressBar->Name = L"iterProgressBar";
			this->iterProgressBar->Size = System::Drawing::Size(104, 22);
			this->iterProgressBar->TabIndex = 8;
			this->iterProgressBar->Visible = false;
			// 
			// progressBar1
			// 
			this->progressBar1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->progressBar1->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(192)), static_cast<System::Int32>(static_cast<System::Byte>(0)), 
				static_cast<System::Int32>(static_cast<System::Byte>(0)));
			this->progressBar1->Location = System::Drawing::Point(513, 27);
			this->progressBar1->Name = L"progressBar1";
			this->progressBar1->Size = System::Drawing::Size(104, 22);
			this->progressBar1->TabIndex = 8;
			this->progressBar1->Visible = false;
			// 
			// label6
			// 
			this->label6->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->label6->AutoSize = true;
			this->label6->Location = System::Drawing::Point(518, 10);
			this->label6->Name = L"label6";
			this->label6->Size = System::Drawing::Size(99, 13);
			this->label6->TabIndex = 7;
			this->label6->Text = L"Fitting in Progress...";
			this->label6->TextAlign = System::Drawing::ContentAlignment::TopCenter;
			this->label6->Visible = false;
			// 
			// reportButton
			// 
			this->reportButton->Cursor = System::Windows::Forms::Cursors::Default;
			this->reportButton->Location = System::Drawing::Point(280, 5);
			this->reportButton->Name = L"reportButton";
			this->reportButton->Size = System::Drawing::Size(83, 23);
			this->reportButton->TabIndex = 330;
			this->reportButton->Text = L"Param. Report";
			this->reportButton->UseVisualStyleBackColor = true;
			this->reportButton->Click += gcnew System::EventHandler(this, &FormFactor::reportButton_Click);
			// 
			// undo
			// 
			this->undo->Enabled = false;
			this->undo->Location = System::Drawing::Point(214, 5);
			this->undo->Name = L"undo";
			this->undo->Size = System::Drawing::Size(60, 23);
			this->undo->TabIndex = 320;
			this->undo->Text = L"Undo";
			this->undo->UseVisualStyleBackColor = true;
			this->undo->Click += gcnew System::EventHandler(this, &FormFactor::calculate_Click);
			// 
			// calculate
			// 
			this->calculate->Enabled = false;
			this->calculate->Location = System::Drawing::Point(126, 5);
			this->calculate->Name = L"calculate";
			this->calculate->Size = System::Drawing::Size(82, 23);
			this->calculate->TabIndex = 310;
			this->calculate->Text = L"(Re)Calculate";
			this->calculate->UseVisualStyleBackColor = true;
			this->calculate->Click += gcnew System::EventHandler(this, &FormFactor::calculate_Click);
			// 
			// timer1
			// 
			this->timer1->Tick += gcnew System::EventHandler(this, &FormFactor::timer1_Tick);
			// 
			// sfd
			// 
			this->sfd->Filter = L"Output Files (*.out)|*.out|Data Files (*.dat, *.chi)|*.dat;*.chi|All files|*.*";
			// 
			// button1
			// 
			this->button1->Location = System::Drawing::Point(6, 48);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(111, 23);
			this->button1->TabIndex = 18;
			this->button1->Text = L"Extract Baseline...";
			this->button1->UseVisualStyleBackColor = true;
			// 
			// button2
			// 
			this->button2->Location = System::Drawing::Point(6, 19);
			this->button2->Name = L"button2";
			this->button2->Size = System::Drawing::Size(111, 23);
			this->button2->TabIndex = 17;
			this->button2->Text = L"Zero Background";
			this->button2->UseVisualStyleBackColor = true;
			// 
			// scrollerTimer
			// 
			this->scrollerTimer->Enabled = true;
			this->scrollerTimer->Interval = 50;
			this->scrollerTimer->Tick += gcnew System::EventHandler(this, &FormFactor::scrollerTimer_Tick);
			// 
			// polydiToolStripMenuItem
			// 
			this->polydiToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {this->uniformPDToolStripMenuItem, 
				this->gaussianPDToolStripMenuItem, this->lorentzianPDToolStripMenuItem});
			this->polydiToolStripMenuItem->Name = L"polydiToolStripMenuItem";
			this->polydiToolStripMenuItem->Size = System::Drawing::Size(226, 22);
			this->polydiToolStripMenuItem->Text = L"Polydispersity Function";
			// 
			// uniformPDToolStripMenuItem
			// 
			this->uniformPDToolStripMenuItem->Name = L"uniformPDToolStripMenuItem";
			this->uniformPDToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->uniformPDToolStripMenuItem->Text = L"Uniform";
			this->uniformPDToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::uniformPDToolStripMenuItem_Click);
			// 
			// gaussianPDToolStripMenuItem
			// 
			this->gaussianPDToolStripMenuItem->Checked = true;
			this->gaussianPDToolStripMenuItem->CheckState = System::Windows::Forms::CheckState::Checked;
			this->gaussianPDToolStripMenuItem->Name = L"gaussianPDToolStripMenuItem";
			this->gaussianPDToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->gaussianPDToolStripMenuItem->Text = L"Gaussian";
			this->gaussianPDToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::gaussianPDToolStripMenuItem_Click);
			// 
			// lorentzianPDToolStripMenuItem
			// 
			this->lorentzianPDToolStripMenuItem->Name = L"lorentzianPDToolStripMenuItem";
			this->lorentzianPDToolStripMenuItem->Size = System::Drawing::Size(152, 22);
			this->lorentzianPDToolStripMenuItem->Text = L"Lorentzian";
			this->lorentzianPDToolStripMenuItem->Click += gcnew System::EventHandler(this, &FormFactor::lorentzianPDToolStripMenuItem_Click);
			// 
			// FormFactor
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1157, 716);
			this->Controls->Add(this->tableLayoutPanel1);
			this->Controls->Add(this->menuStrip1);
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->MainMenuStrip = this->menuStrip1;
			this->MinimumSize = System::Drawing::Size(1024, 719);
			this->Name = L"FormFactor";
			this->Text = L"X+ -- Form Factor";
			this->Load += gcnew System::EventHandler(this, &FormFactor::FormFactor_Load);
			this->FormClosed += gcnew System::Windows::Forms::FormClosedEventHandler(this, &FormFactor::FormFactor_FormClosed);
			this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &FormFactor::FormFactor_KeyDown);
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->tableLayoutPanel1->ResumeLayout(false);
			this->panel2->ResumeLayout(false);
			this->tabControl1->ResumeLayout(false);
			this->FFTab->ResumeLayout(false);
			this->FFTab->PerformLayout();
			this->FFGroupbox->ResumeLayout(false);
			this->FFGroupbox->PerformLayout();
			this->slowModelGroupbox->ResumeLayout(false);
			this->slowModelGroupbox->PerformLayout();
			this->genRangeBox->ResumeLayout(false);
			this->genRangeBox->PerformLayout();
			this->consGroupBox->ResumeLayout(false);
			this->consGroupBox->PerformLayout();
			this->EDAreaGroup->ResumeLayout(false);
			this->EDAreaGroup->PerformLayout();
			this->previewBox->ResumeLayout(false);
			this->globalParamtersGroupBox->ResumeLayout(false);
			this->globalParamtersGroupBox->PerformLayout();
			this->SFTab->ResumeLayout(false);
			this->SFTab->PerformLayout();
			this->phasefitter->ResumeLayout(false);
			this->phasefitter->PerformLayout();
			this->phasesParamsGroupBox->ResumeLayout(false);
			this->phasesParamsGroupBox->PerformLayout();
			this->Peakfitter->ResumeLayout(false);
			this->Peakfitter->PerformLayout();
			this->PeakShapeGroupbox->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->centerTrack))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->fwhmTrack))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->ampTrack))->EndInit();
			this->cailleGroupbox->ResumeLayout(false);
			this->cailleGroupbox->PerformLayout();
			this->cailleMaxGroupbox->ResumeLayout(false);
			this->cailleMaxGroupbox->PerformLayout();
			this->cailleMinGroupbox->ResumeLayout(false);
			this->cailleMinGroupbox->PerformLayout();
			this->cailleValueGroupbox->ResumeLayout(false);
			this->cailleValueGroupbox->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->cailleValueTrackBar))->EndInit();
			this->BGTab->ResumeLayout(false);
			this->BGTab->PerformLayout();
			this->manipBox->ResumeLayout(false);
			this->functionsGroupBox->ResumeLayout(false);
			this->functionsGroupBox->PerformLayout();
			this->funcTypeBox->ResumeLayout(false);
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->baseTrackBar))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->decayTrackBar))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->xCenterTrackBar))->EndInit();
			this->movetophases->ResumeLayout(false);
			this->movetophases->PerformLayout();
			this->groupBox1->ResumeLayout(false);
			this->groupBox1->PerformLayout();
			this->maskToolStrip->ResumeLayout(false);
			this->maskToolStrip->PerformLayout();
			this->panel3->ResumeLayout(false);
			this->panel3->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion


// UI Methods
	 
private:
#pragma region Form Factor Methods
/////////////////////////////////////////////////////
// Form factor functions                           //
/////////////////////////////////////////////////////

	// Helper functions
	//////////////////////
	
	// Function that is called upon model change
	void PrepareModelUI();

	void UItoParameters(paramStruct *p);
	void ParametersToUI(const paramStruct *p);

	void UpdateEDPreview();
	void InitializeEDProfile();	
	
	// Calculates the scale and background of a frozen model
	void ScaleandBGFF();
	void loadFileAsFormFactor(std::wstring fname, std::vector<double> tmpX, std::vector<double> tmpY);
	void linkedParameterCheck(ListView ^lv, int layer);
	void linkedParameterChangedCheck(ListView::ListViewItemCollection ^s, int layer);

	// Called upon any change in the form factor parameters
	void FFParameterUpdateHandler();
	// Called upon any change in the structure factor parameters
	void SFParameterUpdateHandler();
	// Called upon any change in the background parameters
	void BGParameterUpdateHandler();
	// Updates the param box according to the listview
	void UpdateExtraParamBox();

	void EDU();
	void AddParamLayer(std::vector<Parameter> layer);
	void AddParamLayer();

	// UI events
	//////////////////////
	System::Void Mut_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void constraints_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		fitRange->Enabled = constraints->Checked && (listViewFF->SelectedIndices->Count == 1);
	}

	System::Void listViewFF_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);
	System::Void changeData_Click(System::Object^  sender, System::EventArgs^  e);	
	System::Void changeModel_Click(System::Object^  sender, System::EventArgs^  e);	
	System::Void listView_Extraparams_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void paramBox_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void fitRange_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void listViewFF_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void Parameter_TextChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void PDRadioChanged(System::Object^ sender, System::EventArgs^ e);
	System::Void ExtraParameter_TextChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void ExtraParameter_Enter(System::Object^  sender, System::EventArgs^  e);
	System::Void infExtraParam_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void relatedModelToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e);
	void loadFileAsFormFactorToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	void freezeFFCheckBox_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	void useModelFFButton_Click(System::Object^  sender, System::EventArgs^  e);

#pragma endregion

#pragma region Structure Factor Methods
/////////////////////////////////////////////////////
// Structure factor functions                      //
/////////////////////////////////////////////////////

	// Helper functions
	//////////////////////
	void calculateRecipVectors();
	double calcPhaseErr(std::vector<double> O, std::vector<double> E);
	void SetPhaseType(PhaseType a);
	void SetCaillePeakstoGUI(peakStruct *p);
	void GetCaillePeaksFromGUI(peakStruct *p);
	void returnToLineshapeButton_Click(System::Object^  sender, System::EventArgs^  e);
	void AddCailleParam(System::String ^str, ParamMode mode, double defaultValue);
	void PhasesCompleted();
	void UpdateGeneratedPhasePeaks();
	void AddCaille(double eta, int h);
	void SetCailletoGUI(graphTable *BGs, cailleParamStruct *drawing);
	void GetCailleFromGUI(graphTable *Caille, cailleParamStruct *cailleP);
	void SetPhases(phaseStruct *phase);
	void AddPhasesParam(System::String ^str, ParamMode mode, double defaultValue, double min, double max);
	void GetPhasesParameters(vector<double> &exParams);
	void SetPhasesParameters(const vector<double> &exParams);
	void GetPhasesParameters(vector<double> &exParams, vector<bool>& mutex);
	void GetPhasesFromListView(phaseStruct *phase);
	PhaseType GetPhase(); 
	FitMethod GetPhaseFitMethod();
	void StructureFactor_Load();
	void AddPeak(double amp, char ampMut, double fwhm, char fwhmMut,
				 double center, char centerMut);
	void AddPeak();
	void GetPeaksFromListView(peakStruct *peaks);
	void initPhasesParams();
	void SetPeaks(peakStruct *peaks);

	// Function that will try to automatically find the SF peaks.
	void AutoFindPeaks();


		 
	// UI events
	//////////////////////
	System::Void sigmaToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void showCailleButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void clearPhases_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void cailleParamListView_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void removeCaille_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void listView_PeakPosition_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e); 
	System::Void caillePeaksListView_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);
	System::Void caillePeaksListView_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);

	// Sort the peak list view based on the xCenter values
	System::Void SortButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void peaksForPhases_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void Caille_button_Click(System::Object^  sender, System::EventArgs^  e);		 
	System::Void listView_phases_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) ;
	System::Void ValPhases_Leave(System::Object^  sender, System::EventArgs^  e) ;	 		 
	System::Void MutPhases_Click(System::Object^  sender, System::EventArgs^  e) ; 
	System::Void comboPhases_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) ;
	System::Void phaseorder_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) ;
	System::Void automaticPeakFinderButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void addPeak_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void removePeak_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void listView_peaks_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void ampMut_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void fwhmMut_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void centerMut_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void StructureFactor_ParamTextLeave(System::Object^  sender, System::EventArgs^  e);
	System::Void peakfit_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void PeakPicker_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void Move2Phases_Click(System::Object^  sender, System::EventArgs^  e) ;
	System::Void fitphase_Click(System::Object^  sender, System::EventArgs^  e) ;	 
	System::Void listView_peaks_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);

#pragma endregion


#pragma region Background Methods
/////////////////////////////////////////////////////
// Background functions                            //
/////////////////////////////////////////////////////

	// Helper functions
	//////////////////////
	void Background_Load();
	void AddBGFunction(BGFuncType type, double amp, char ampMut, 
					   double dec, char decMut, double xc, char xcMut,
					   double ampmin, double ampmax, double decmin, 
					   double decmax, double centermin, double centermax);
	BGFuncType GetFuncType(System::String ^str);
	System::String ^GetBGFuncString(BGFuncType type);

	void GetBGFromGUI(bgStruct *BGs);
	void SetBGtoGUI(bgStruct *BGs);


	// UI events
	//////////////////////
	System::Void BGTextBox_Leave(System::Object^  sender, System::EventArgs^  e);
	System::Void BGTrackBar_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);
	System::Void addFuncButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void removeFuncButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void BGListview_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void BGListview_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);
	System::Void BGCheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void funcTypeList_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void fitToBaseline_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void smooth_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void addLayer_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void removeLayer_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void zeroBG_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void baseline_Click(System::Object^  sender, System::EventArgs^  e);


#pragma endregion

#pragma region General, Options and UI Methods
/////////////////////////////////////////////////////
// General option and UI functions                 //
/////////////////////////////////////////////////////

	// Helper functions
	//////////////////////
	void dealWithTrackBar(ListViewItem ^lvi, int subItem, TrackBar ^tb, TextBox ^txt, double factor, ExtraParam desc);
	void SortListView(ListView ^l,int sub, bool firstCol);
	FitMethod GetFitMethod();	
	void addValueToVector(std::vector<double> &vec, double val);
	void handleModelChange(FFModel &newModel);
	void ChangeModel(paramStruct *p, Model *newModel);	// Fills the paramStruct with values for the new model
	void ReloadModelUI(); // Reloads the same model (for ED profile function modification)

	// Export all values to a csv file for the user to use at his/her convenience
	void WriteCSVParamsFile();

	void modelFitter_threadFunc(System::Object ^args);
	void plotFittingResults();
	void plotGenerateResults();
	void RenderPreviewScene();
	void InitializeGraph(bool bZero, vector<double>& bgx, vector<double>& bgy, std::vector<double>& ffy);		
	void InitializeFitGraph();
	void InitializeFitGraph(vector<double>& x, vector<double>& y);		
	void UpdateGraph();
	void UpdateGraph(bool calcAll);

	// Makes the wgtFit graph be redrawn without recalculating any models.
	void RedrawGraph();
	void UpdateChisq(double chisq);
	void UpdateRSquared(double rsquared);
	void AddVectors(vector<double> &result, const vector<double> &a, const vector<double> &b);
	void SubtractVectors(vector<double> &result, const vector<double> &a, const vector<double> &b);
	void MultiplyVectors(vector<double> &result, const vector<double> &a, const vector<double> &b);
	void DivideVectors(vector<double> &result, const vector<double> &numerators, const vector<double> &denominators);
	void multiplyVectorByValue(std::vector<double> &vec, double val);

	// UI events
	//////////////////////
	
	System::Void FormFactor_Load(System::Object^  sender, System::EventArgs^  e);

	System::Void wgtFit_MouseMove(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e); 
	System::Void wgtFit_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);
	System::Void wgtFit_MouseUp(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);

protected: 
	virtual void OnFormClosing(FormClosingEventArgs^ e) override {
		if(fitterThread && fitterThread->IsAlive)
			fitterThread->Abort();
		System::Windows::Forms::Form::OnFormClosing(e);
	}
private:

	System::Void centerTrackBar(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);

	System::Void reportButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void levmar_Click(System::Object^  sender, System::EventArgs^  e) {
		levmar->Checked = true;
		raindrop->Checked = false;
		diffEvo->Checked = false;
	}

	System::Void raindrop_Click(System::Object^  sender, System::EventArgs^  e) {
		levmar->Checked = false;
		raindrop->Checked = true;
		diffEvo->Checked = false;
	}

	System::Void pLevmar_Click(System::Object^  sender, System::EventArgs^  e) {
		pLevmar->Checked = true;
		pRaindrop->Checked = false;
		pDiffEvo->Checked = false;
	}

	System::Void pRaindrop_Click(System::Object^  sender, System::EventArgs^  e) {
		pLevmar->Checked = false;
		pRaindrop->Checked = true;
		pDiffEvo->Checked = false;
	}

	System::Void discreteStepsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void gaussiansToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void hyperbolictangentSmoothStepsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void adaptiveToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);

	System::Void edpResolution_TextChanged(System::Object^  sender, System::EventArgs^  e);

	System::Void quadrestoolStripTextBox_TextChanged(System::Object^  sender, System::EventArgs^  e) {
		save->Enabled = true;
	}

	System::Void pauseFittingToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void fittingEmergencyStopToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void minimumSignalToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) ;
	System::Void cPUToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		gPUToolStripMenuItem->Checked = false;
		cPUToolStripMenuItem->Checked = true;
		SetGPUBackend(false);
	}

	System::Void gPUToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(hasGPUBackend()) {
			gPUToolStripMenuItem->Checked = true;
			cPUToolStripMenuItem->Checked = false;
			SetGPUBackend(true);
		}
	}
	System::Void gaussLegendreToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!gaussLegendreToolStripMenuItem->Checked)
			gaussLegendreToolStripMenuItem->Checked = true;
		else {
			monteCarloToolStripMenuItem->Checked = false;
			simpsonsRuleToolStripMenuItem->Checked = false;
			ClassifyQuadratureMethod(QUAD_GAUSSLEGENDRE);
		}
		save->Enabled = true;
	}
	System::Void monteCarloToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!monteCarloToolStripMenuItem->Checked)
			monteCarloToolStripMenuItem->Checked = true;
		else {
			gaussLegendreToolStripMenuItem->Checked = false;
			simpsonsRuleToolStripMenuItem->Checked = false;
			ClassifyQuadratureMethod(QUAD_MONTECARLO);
		}
		save->Enabled = true;
	}

	System::Void simpsonsRuleToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(!simpsonsRuleToolStripMenuItem->Checked)
			simpsonsRuleToolStripMenuItem->Checked = true;
		else {
			gaussLegendreToolStripMenuItem->Checked = false;
			monteCarloToolStripMenuItem->Checked = false;
			ClassifyQuadratureMethod(QUAD_SIMPSON);
		}
		save->Enabled = true;
	}

	System::Void uniformPDToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		uniformPDToolStripMenuItem->Checked = true;
		gaussianPDToolStripMenuItem->Checked = false;
		lorentzianPDToolStripMenuItem->Checked = false;
		SetPDFunc(SHAPE_LORENTZIAN_SQUARED);
		FFParameterUpdateHandler();
		save->Enabled = true;
	}
	
	System::Void gaussianPDToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		uniformPDToolStripMenuItem->Checked = false;
		gaussianPDToolStripMenuItem->Checked = true;
		lorentzianPDToolStripMenuItem->Checked = false;
		SetPDFunc(SHAPE_GAUSSIAN);
		FFParameterUpdateHandler();
		save->Enabled = true;
	}

	System::Void lorentzianPDToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		uniformPDToolStripMenuItem->Checked = false;
		gaussianPDToolStripMenuItem->Checked = false;
		lorentzianPDToolStripMenuItem->Checked = true;
		SetPDFunc(SHAPE_LORENTZIAN);
		FFParameterUpdateHandler();
		save->Enabled = true;
	}

	System::Void timer1_Tick(System::Object^  sender, System::EventArgs^  e) {
		if(_bChanging)
			return;
		oglPreview->Render();
		oglPreview->SwapOpenGLBuffers();
	}

	System::Void oglPanel_Resize(System::Object^  sender, System::EventArgs^  e) {
		oglPreview->ReSizeGLScene(oglPanel->Width, oglPanel->Height);
	}

	System::Void double_TextChanged(System::Object^  sender, System::EventArgs^  e);

	System::Void maskButton_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void maskPanel_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void expResToolStripTextBox_TextChanged(System::Object^  sender, System::EventArgs^  e) ;
	System::Void expResToolStripTextBox_Enter(System::Object^  sender, System::EventArgs^  e) ;
	System::Void logScaledFittingParamToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e) ;
	System::Void useCheckBox_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void General_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);
	System::Void exportAllParametersAsCSVFileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void a1nm1ToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void minimumSignalTextbox_Leave(System::Object^  sender, System::EventArgs^  e);
	System::Void minimumSignalTextbox_Enter(System::Object^  sender, System::EventArgs^  e);
	System::Void accurateDerivativeToolStripMenuItem_CheckedChanged(System::Object^  sender, System::EventArgs^  e); 
	System::Void exportGraphToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void scrollerTimer_Tick(System::Object^  sender, System::EventArgs^  e);
	System::Void tabControl1_TabIndexChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void exmut_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
	System::Void importParametersToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void importBaselineToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void modelFitter_DoWork(System::Object^  sender, System::ComponentModel::DoWorkEventArgs^  e);		 
	System::Void exportDataFileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void exportModelToolStripMenuItem1_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void exportElectronDensityProfileToolStripMenuItem1_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void FormFactor_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e);
	System::Void save_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void plotElectronDensityProfileToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void listViewFF_DoubleClick(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e);
	System::Void plotFittingResultsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		plotFittingResults();
	}

	System::Void calculate_Click(System::Object^  sender, System::EventArgs^  e);
	System::Void logScale_CheckedChanged(System::Object^  sender, System::EventArgs^  e);

	System::Void dragToZoomToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		if(wgtFit && wgtFit->graph)
			wgtFit->setDragToZoom(dragToZoomToolStripMenuItem->Checked);
		g_bDragToZoom = dragToZoomToolStripMenuItem->Checked;
	}

	System::Void settingsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		dragToZoomToolStripMenuItem->Checked = g_bDragToZoom;
		if(wgtFit && wgtFit->graph)
			wgtFit->setDragToZoom(dragToZoomToolStripMenuItem->Checked);
		logarToolStripMenuItem->Checked = logScale->Checked;
	}

	System::Void logscaleToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
		logScale->Checked = logarToolStripMenuItem->Checked;
		if(wgtFit && wgtFit->graph) {
			wgtFit->graph->SetScale(0, logScale->Checked ? SCALE_LOG : SCALE_LIN);
			wgtFit->Invalidate();
		}
	}

	System::Void FormFactor_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e);


// Public methods, for use by the fitting thread
public:
	System::Void modelFitter_ProgressChanged(System::Object^  sender, System::ComponentModel::ProgressChangedEventArgs^  e) {
		int progress = e->ProgressPercentage;

		progressBar1->Value = progress;

		wgtFit->Invalidate();
	}	
	System::Void modelFitter_RunWorkerCompleted(System::Object^  sender, System::ComponentModel::RunWorkerCompletedEventArgs^  e);


#pragma endregion
};
}
