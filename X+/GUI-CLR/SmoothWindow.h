#pragma once

#include <vector>

#include "WGTControl.h"

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;

struct vec {
	std::vector<double> x;
	std::vector<double> y;
};

namespace GUICLR {

	/// <summary>
	/// Summary for SmoothWindow
	///
	/// WARNING: If you change the name of this class, you will need to change the
	///          'Resource File Name' property for the managed resource compiler tool
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public ref class SmoothWindow : public System::Windows::Forms::Form
	{
	System::String ^_dataFile;
	private: System::Windows::Forms::SaveFileDialog^  saveFileDialog1;
			 struct vec *origY;
	private: System::Windows::Forms::CheckBox^  logscaleX;
			 bool _bOverwrite;
	public:
		SmoothWindow(const wchar_t *data, bool bOverwrite)
		{
			_dataFile = gcnew System::String(data);
			origY = new struct vec;
			_bOverwrite = bOverwrite;
			InitializeComponent();
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~SmoothWindow()
		{
			delete origY;
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel1;
	protected: 
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel2;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::Button^  saveAs;


	private: System::Windows::Forms::CheckBox^  logScale;
	private: System::Windows::Forms::TableLayoutPanel^  tableLayoutPanel3;
	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::TrackBar^  trackBar1;








	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->tableLayoutPanel2 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->saveAs = (gcnew System::Windows::Forms::Button());
			this->logScale = (gcnew System::Windows::Forms::CheckBox());
			this->tableLayoutPanel3 = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->trackBar1 = (gcnew System::Windows::Forms::TrackBar());
			this->saveFileDialog1 = (gcnew System::Windows::Forms::SaveFileDialog());
			this->logscaleX = (gcnew System::Windows::Forms::CheckBox());
			this->tableLayoutPanel1->SuspendLayout();
			this->tableLayoutPanel2->SuspendLayout();
			this->tableLayoutPanel3->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->trackBar1))->BeginInit();
			this->SuspendLayout();
			// 
			// tableLayoutPanel1
			// 
			this->tableLayoutPanel1->ColumnCount = 1;
			this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				50)));
			this->tableLayoutPanel1->Controls->Add(this->tableLayoutPanel2, 0, 1);
			this->tableLayoutPanel1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel1->Location = System::Drawing::Point(0, 0);
			this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
			this->tableLayoutPanel1->RowCount = 2;
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 90.54054F)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 9.459459F)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 20)));
			this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Absolute, 20)));
			this->tableLayoutPanel1->Size = System::Drawing::Size(568, 444);
			this->tableLayoutPanel1->TabIndex = 0;
			// 
			// tableLayoutPanel2
			// 
			this->tableLayoutPanel2->ColumnCount = 5;
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				48.75445F)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				11.56584F)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				11.20996F)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				13.7229F)));
			this->tableLayoutPanel2->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				14.41773F)));
			this->tableLayoutPanel2->Controls->Add(this->logscaleX, 0, 0);
			this->tableLayoutPanel2->Controls->Add(this->button1, 4, 0);
			this->tableLayoutPanel2->Controls->Add(this->saveAs, 3, 0);
			this->tableLayoutPanel2->Controls->Add(this->tableLayoutPanel3, 0, 0);
			this->tableLayoutPanel2->Controls->Add(this->logScale, 2, 0);
			this->tableLayoutPanel2->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel2->Location = System::Drawing::Point(3, 405);
			this->tableLayoutPanel2->Name = L"tableLayoutPanel2";
			this->tableLayoutPanel2->RowCount = 1;
			this->tableLayoutPanel2->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 100)));
			this->tableLayoutPanel2->Size = System::Drawing::Size(562, 36);
			this->tableLayoutPanel2->TabIndex = 0;
			// 
			// button1
			// 
			this->button1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->button1->Location = System::Drawing::Point(484, 10);
			this->button1->Name = L"button1";
			this->button1->Size = System::Drawing::Size(75, 23);
			this->button1->TabIndex = 0;
			this->button1->Text = L"Cancel";
			this->button1->UseVisualStyleBackColor = true;
			this->button1->Click += gcnew System::EventHandler(this, &SmoothWindow::button1_Click);
			// 
			// saveAs
			// 
			this->saveAs->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->saveAs->Location = System::Drawing::Point(405, 10);
			this->saveAs->Name = L"saveAs";
			this->saveAs->Size = System::Drawing::Size(71, 23);
			this->saveAs->TabIndex = 1;
			this->saveAs->Text = L"Save As...";
			this->saveAs->UseVisualStyleBackColor = true;
			this->saveAs->Click += gcnew System::EventHandler(this, &SmoothWindow::saveAs_Click);
			// 
			// logScale
			// 
			this->logScale->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->logScale->AutoSize = true;
			this->logScale->Location = System::Drawing::Point(347, 16);
			this->logScale->Name = L"logScale";
			this->logScale->Size = System::Drawing::Size(52, 17);
			this->logScale->TabIndex = 3;
			this->logScale->Text = L"log (I)";
			this->logScale->UseVisualStyleBackColor = true;
			this->logScale->CheckedChanged += gcnew System::EventHandler(this, &SmoothWindow::logScale_CheckedChanged);
			// 
			// tableLayoutPanel3
			// 
			this->tableLayoutPanel3->ColumnCount = 3;
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				35.8209F)));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent, 
				64.17911F)));
			this->tableLayoutPanel3->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Absolute, 
				55)));
			this->tableLayoutPanel3->Controls->Add(this->label1, 0, 0);
			this->tableLayoutPanel3->Controls->Add(this->label2, 2, 0);
			this->tableLayoutPanel3->Controls->Add(this->trackBar1, 1, 0);
			this->tableLayoutPanel3->Dock = System::Windows::Forms::DockStyle::Fill;
			this->tableLayoutPanel3->Location = System::Drawing::Point(3, 3);
			this->tableLayoutPanel3->Name = L"tableLayoutPanel3";
			this->tableLayoutPanel3->RowCount = 1;
			this->tableLayoutPanel3->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 50)));
			this->tableLayoutPanel3->Size = System::Drawing::Size(268, 30);
			this->tableLayoutPanel3->TabIndex = 4;
			// 
			// label1
			// 
			this->label1->Anchor = System::Windows::Forms::AnchorStyles::Right;
			this->label1->AutoSize = true;
			this->label1->Location = System::Drawing::Point(31, 8);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(42, 13);
			this->label1->TabIndex = 0;
			this->label1->Text = L"Original";
			// 
			// label2
			// 
			this->label2->Anchor = System::Windows::Forms::AnchorStyles::Left;
			this->label2->AutoSize = true;
			this->label2->Location = System::Drawing::Point(215, 8);
			this->label2->Name = L"label2";
			this->label2->Size = System::Drawing::Size(43, 13);
			this->label2->TabIndex = 1;
			this->label2->Text = L"Smooth";
			// 
			// trackBar1
			// 
			this->trackBar1->Dock = System::Windows::Forms::DockStyle::Fill;
			this->trackBar1->Location = System::Drawing::Point(79, 3);
			this->trackBar1->Name = L"trackBar1";
			this->trackBar1->Size = System::Drawing::Size(130, 24);
			this->trackBar1->TabIndex = 2;
			this->trackBar1->Scroll += gcnew System::EventHandler(this, &SmoothWindow::trackBar1_Scroll);
			// 
			// saveFileDialog1
			// 
			this->saveFileDialog1->Filter = L"Data Files (*.dat, *.chi)|*.dat;*.chi|Output Files (*.out)|*.out|All files|*.*";
			this->saveFileDialog1->Title = L"Choose the output data file:";
			// 
			// logscaleX
			// 
			this->logscaleX->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Bottom | System::Windows::Forms::AnchorStyles::Right));
			this->logscaleX->AutoSize = true;
			this->logscaleX->Location = System::Drawing::Point(281, 16);
			this->logscaleX->Name = L"logscaleX";
			this->logscaleX->Size = System::Drawing::Size(55, 17);
			this->logscaleX->TabIndex = 5;
			this->logscaleX->Text = L"log (q)";
			this->logscaleX->UseVisualStyleBackColor = true;
			this->logscaleX->CheckedChanged += gcnew System::EventHandler(this, &SmoothWindow::logscaleX_CheckedChanged);
			// 
			// SmoothWindow
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(568, 444);
			this->Controls->Add(this->tableLayoutPanel1);
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::SizableToolWindow;
			this->MinimumSize = System::Drawing::Size(512, 34);
			this->Name = L"SmoothWindow";
			this->Text = L"Smooth Graph";
			this->Load += gcnew System::EventHandler(this, &SmoothWindow::SmoothWindow_Load);
			this->tableLayoutPanel1->ResumeLayout(false);
			this->tableLayoutPanel2->ResumeLayout(false);
			this->tableLayoutPanel2->PerformLayout();
			this->tableLayoutPanel3->ResumeLayout(false);
			this->tableLayoutPanel3->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^  >(this->trackBar1))->EndInit();
			this->ResumeLayout(false);

		}
#pragma endregion
	private: GUICLR::WGTControl^  wgtGraph;
			 void OpenInitialGraph();
			 void logScale_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
		
	private: System::Void SmoothWindow_Load(System::Object^  sender, System::EventArgs^  e) {
				this->wgtGraph = (gcnew GUICLR::WGTControl());
				this->tableLayoutPanel1->Controls->Add(this->wgtGraph);
				// 
				// wgtGraph
				// 
				this->wgtGraph->Cursor = System::Windows::Forms::Cursors::Cross;
				this->wgtGraph->Dock = System::Windows::Forms::DockStyle::Fill;
				this->wgtGraph->Location = System::Drawing::Point(0, 0);
				this->wgtGraph->Name = L"wgtGraph";
				this->wgtGraph->Size = System::Drawing::Size(343, 349);
				this->wgtGraph->TabIndex = 0;

				// Opening an initial graph
				OpenInitialGraph();

				if(_bOverwrite)
					saveAs->Text = "OK";
			}
	private: System::Void trackBar1_Scroll(System::Object^  sender, System::EventArgs^  e);
	private: System::Void button1_Click(System::Object^  sender, System::EventArgs^  e) {
			 this->Close();
	}
	private: System::Void saveAs_Click(System::Object^  sender, System::EventArgs^  e);
private: System::Void logscaleX_CheckedChanged(System::Object^  sender, System::EventArgs^  e);
};
}
