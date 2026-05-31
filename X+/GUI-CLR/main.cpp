#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <windows.h>
#include <commctrl.h>

#include "OpeningWindow.h"

using GUICLR::OpeningWindow;

[STAThreadAttribute]
int WINAPI WinMain(      
				   HINSTANCE,
				   HINSTANCE,
				   LPSTR,
				   int
				   ) {
	//Initialize common controls
	InitCommonControls();

	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 
	
	//code = InitializeOpeningWindow();

	Application::Run(gcnew OpeningWindow());
	
	_CrtDumpMemoryLeaks();

	return 0;
}
