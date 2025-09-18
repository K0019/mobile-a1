/******************************************************************************/
/*!
\file   main.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (70%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (30%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	This is the source file containing the entry point of the program.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine.h"
#include <crtdbg.h>


/*****************************************************************//*!
\brief
	The entry point of the program. The exact parameters differ depending on the project configuration, but they are all currently unused.
*//******************************************************************/
#if defined(DEBUG) || defined(_DEBUG)
int main(int argc, char* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

#else // Release build
#include <Windows.h>
int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	UNREFERENCED_PARAMETER(hInst);
	UNREFERENCED_PARAMETER(hInstPrev);
	UNREFERENCED_PARAMETER(cmdline);
	UNREFERENCED_PARAMETER(cmdshow);
#endif

	int returnVal{ EXIT_SUCCESS };
	try {
		Engine* app{ ST<Engine>::Get() };
		app->init();
		app->run();
		app->shutdown();
	}
	catch(const std::exception& e) {
		CONSOLE_LOG(LEVEL_FATAL) << "UNHANDLED EXCEPTION: " << e.what();
		returnVal = EXIT_FAILURE;
	}
	catch(...) {
		CONSOLE_LOG(LEVEL_FATAL) << "UNHANDLED EXCEPTION: Unknown exception occurred!";
		returnVal = EXIT_FAILURE;
	}

	ST<Engine>::Destroy();
	return returnVal;
}
