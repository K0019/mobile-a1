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

//#include "Engine/Engine.h"
//#ifdef DEBUG
//#include <crtdbg.h>
//#endif
//
//
///*****************************************************************//*!
//\brief
//	The entry point of the program. The exact parameters differ depending on the project configuration, but they are all currently unused.
//*//******************************************************************/
//#if defined(DEBUG) || defined(_DEBUG)
//int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
//{
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//
//#else // Release build
//#include <Windows.h>
//int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInst, [[maybe_unused]] HINSTANCE hInstPrev, [[maybe_unused]] PSTR cmdline, [[maybe_unused]] int cmdshow)
//{
//#endif
//
//	int returnVal{ EXIT_SUCCESS };
//	try {
//		MagicEngine* app{ ST<MagicEngine>::Get() };
//		app->init();
//		app->run();
//		app->shutdown();
//	}
//	catch(const std::exception& e) {
//		CONSOLE_LOG(LEVEL_FATAL) << "UNHANDLED EXCEPTION: " << e.what();
//		returnVal = EXIT_FAILURE;
//	}
//	catch(...) {
//		CONSOLE_LOG(LEVEL_FATAL) << "UNHANDLED EXCEPTION: Unknown exception occurred!";
//		returnVal = EXIT_FAILURE;
//	}
//
//	ST<MagicEngine>::Destroy();
//	return returnVal;
//}
