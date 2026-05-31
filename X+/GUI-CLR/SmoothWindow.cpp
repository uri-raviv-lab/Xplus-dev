#include "SmoothWindow.h"
#include "clrfunctionality.h"

#include <ctime>

#include <complex>
#include "../fftw3.h"
#pragma comment (lib, "libfftw3-3.lib")

namespace GUICLR {

	void SmoothWindow::OpenInitialGraph() {
		std::vector<double> dx, dy;
		RECT area;
		
		ReadCLRFile(_dataFile, dx, dy);

		// TODO REMOVE
		srand(time(NULL));
		for(int i = 0; i < dx.size(); i++) {
			// [0, 1]
			double rnd = (double)rand() / (double)RAND_MAX;
			// [-0.5, 0.5]
			rnd -= 0.5;
			// [-0.1, 0.1]
			rnd *= 0.2;

			if((rand() % 100) >= 20)
				rnd = 0.0;

			dy[i] = sin(dx[i]) + rnd;
		}

		area.top = 0;
		area.left = 0;
		area.right = wgtGraph->Size.Width + area.left;
		area.bottom = wgtGraph->Size.Height + area.top;
		wgtGraph->graph = gcnew Graph(
						area, 
						RGB(255, 0, 0), 
						DRAW_LINES, dx, dy, 
						logscaleX->Checked,	logScale->Checked);


		origY->x = dx;
		origY->y = dy;
	}

	void SmoothWindow::logScale_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(wgtGraph->graph) {
			wgtGraph->graph->SetScale(0, (logScale->Checked) ? 
				SCALE_LOG : SCALE_LIN);
		}
		wgtGraph->Invalidate();
	}

	void SmoothWindow::logscaleX_CheckedChanged(System::Object^  sender, System::EventArgs^  e) {
		if(wgtGraph->graph) {
			wgtGraph->graph->SetScale(1, (logscaleX->Checked) ? 
				SCALE_LOG : SCALE_LIN);
		}
		wgtGraph->Invalidate();
	}

	void SmoothWindow::saveAs_Click(System::Object^  sender, System::EventArgs^  e) {
		// Save As...
		std::wstring savefile;

		if(_bOverwrite)
			savefile = clrToWstring(_dataFile);
		else {
			if(saveFileDialog1->ShowDialog() != Windows::Forms::DialogResult::OK)
				return;

			clrToString(saveFileDialog1->FileName, savefile);
		}

		WriteDataFile(savefile.c_str(), wgtGraph->graph->x[0], 
					  wgtGraph->graph->y[0]);
		this->Close();
	}

	void SmoothWindow::trackBar1_Scroll(System::Object^  sender, System::EventArgs^  e) {
			vector<double> cury = origY->y,
				           x = origY->x, newy, newx;

			int pos = trackBar1->Value;
			int N = cury.size();
			/*
			if(pos > 0) {
				double rPos = double(pos) / 100.0;
				// rPos is 0.0->0.99
				smoothVector(int(rPos * 80.0), cury);
			}*/

			// FT-based derivative:
			// 1. I^ = FFT(I)
			// 2. D^(q) = I^(q) * 2*pi*i*q
			// 3. D = IFFT(D^)

			/*int N = cury.size();
			fftw_complex *out;
			double *rout, *final;
			fftw_plan p, pr;
			//in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
			out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
			rout = (double*) fftw_malloc(sizeof(double) * N);
			p = fftw_plan_dft_r2c_1d(N, &cury[0], out, FFTW_ESTIMATE);
			pr = fftw_plan_dft_c2r_1d(N, out, rout, FFTW_ESTIMATE);
			
			fftw_execute(p); // repeat as needed

			// Perform derivation
			std::complex<double> im (0.0, 1.0);
			for(int i = 0; i < N; i++) {
				std::complex<double> val (out[i][0], out[i][1]);

				// 2*pi*i*q
				val *= 2.0 * 3.1415926 * im * x[i];

				out[i][0] = val.real();
				out[i][1] = val.imag();
			}

			fftw_execute(pr); // repeat as needed
			
			fftw_destroy_plan(p);
			fftw_destroy_plan(pr);

			if((pos % 2) == 1) {
				for(int i = 0; i < N; i++)
					cury[i] = rout[i];
			} else {
				//for(int i = 0; i < N; i++)
				//	cury[i] = out[i][0];
			}

			fftw_free(rout); fftw_free(out);*/

			// Interpolate the data to produce quadruple amount of points
			newx.clear();
			newy.clear();
			newy.push_back(cury[0]);
			for(int i = 1; i < N; i++) {
				double dx = (1.0 * x[i - 1] + 0.0 * x[i]);

				newx.push_back(dx);
				// TODO
				//newy.push_back();
			}

			cury = newy;
			x = newx;
			N = cury.size();

			newy.clear();
			for(int i = 0; i < N; i++) {
				double finalVal = pos == 0 ? cury[i] : 0.0;

				for(int k = 1; k <= pos; k++) {
					double avg = 0.0;

					if(i - k >= 0)
						avg = (cury[i - k] - cury[i]) / (x[i - k] - x[i]);
						
					if(i + k < N) {
						if(i - k < 0)
							avg = (cury[i] - cury[i + k]) / (x[i] - x[i + k]);
						else {
							avg += (cury[i] - cury[i + k]) / (x[i] - x[i + k]);
							avg /= 2;
						}
					}

					finalVal += avg;
				}
				if(pos > 0)
					finalVal /= (double)pos;

				newy.push_back(finalVal);
			}
			cury = newy;


			/*
			for(int k = 0; k < pos; k++) {
				newy.clear();

				newy.push_back(0.0); // Final point

				for(int i = 1; i < cury.size() - 1; i++) {
					newy.push_back((cury[i+1] - cury[i-1]) / (x[i+1] - x[i-1]));


				}
				
				newy.push_back(0.0); // Final point



				cury = newy;
			}*/

			wgtGraph->graph->Modify(0, x, cury);
			wgtGraph->Invalidate();
	}
};
