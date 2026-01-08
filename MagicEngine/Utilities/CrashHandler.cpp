/******************************************************************************/
/*!
\file   CrashHandler.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/23/2025

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
	Saves fatal exceptions to a crash log.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "CrashHandler.h"

#ifndef __ANDROID__

#include <Windows.h>
#pragma comment(lib, "dbghelp.lib")
#include <DbgHelp.h>

namespace internal {

	/*!
	\brief Dumps the current stack trace to a file.
	\param file The output file stream where the stack trace will be written.
	*/
	/*Why is C-style memory management used here?
	The SYMBOL_INFO structure from the Windows API contains a flexible array member at the end:
	typedef struct _SYMBOL_INFO {
		ULONG   SizeOfStruct;
		ULONG   TypeIndex;  // Type Index of symbol
		ULONG64 Reserved[2];
		ULONG   Index;
		ULONG   Size;
		ULONG64 ModBase;    // Base of module containing this symbol
		ULONG   Flags;
		ULONG64 Value;
		ULONG64 Address;    // Address of symbol including first byte
		ULONG   Register;   // Register holding value or pointer to value
		ULONG   Scope;      // Scope of the symbol
		ULONG   Tag;        // pdb classification
		ULONG   NameLen;    // Actual length of name
		ULONG   MaxNameLen;
		CHAR    Name[1];    // The name starts here, but it's flexible
	}  SYMBOL_INFO;
	The Name[1] field is a flexible array member. This is a technique used in C/C++ where an array with a single element is placed at the end of a structure,
	allowing you to dynamically allocate more space for the array at runtime.
	In this case, the Name field is supposed to hold the name of the symbol being resolved,
	and you need more space than a single character.
	C++'s new operator or std::make_unique does not handle this kind of flexible memory allocation directly.
	new would allocate just enough memory for the SYMBOL_INFO structure without accounting for the additional space required for the Name field,
	leading to memory corruption when the program tries to write to the unallocated memory.*/
	void DumpStackTrace(std::ofstream& file)
	{
		void* stack[62];
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD numberOfFrames = CaptureStackBackTrace(0, 62, stack, NULL);

		constexpr int maxNameLength = 255;
		constexpr int symbolInfoSize = sizeof(SYMBOL_INFO) + (maxNameLength + 1) * sizeof(char);

		auto symbol = reinterpret_cast<SYMBOL_INFO*>(malloc(symbolInfoSize));

		if (symbol) // Check if malloc succeeded
		{
			memset(symbol, 0, symbolInfoSize);  // Zero out the memory for safety

			symbol->MaxNameLen = maxNameLength;
			symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

			for (int i = 0; i < numberOfFrames; i++)
			{
				if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
				{
					file << "[" << std::setw(2) << i << "] "
						<< std::setw(40) << symbol->Name
						<< " - Address: 0x" << std::hex << symbol->Address << std::dec << "\n";
				}
				else
				{
					file << "[" << std::setw(2) << i << "] Frame: [Symbol information unavailable]\n";
				}
			}

			free(symbol);  // Free the allocated memory
		}
		else
		{
			file << "Error: Failed to allocate memory for stack trace symbol info." << std::endl;
		}
	}

	/*!
	\brief Handles unhandled exceptions by invoking a custom handler.
	\param exceptionInfo Pointer to exception information.
	\return A LONG indicating the result of the handler.
	*/
	static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
	{
		CONSOLE_LOG(LEVEL_FATAL) << "Application Crash Detected, Dumping Log to file";
		ST<LoggedMessagesBuffer>::Get()->FlushLogToFile("console_log.txt");

		std::ofstream file("crash_log.txt");
		if (file.is_open())
		{
			file << "==============================\n";
			file << "       APPLICATION CRASH      \n";
			file << "==============================\n\n";

			file << "Crash detected!\n";
			file << "Exception Information:\n";
			file << "-------------------------\n";
			file << "Exception code: 0x" << std::hex << exceptionInfo->ExceptionRecord->ExceptionCode << std::dec << "\n";
			file << "Exception address: 0x" << std::hex << (DWORD64)(exceptionInfo->ExceptionRecord->ExceptionAddress) << std::dec << "\n";
			file << "Number of parameters: " << exceptionInfo->ExceptionRecord->NumberParameters << "\n";

			file << "\nStack Trace:\n";
			file << "-------------------------\n";
			DumpStackTrace(file);

			file << "\n==============================\n";
			file << "       END OF CRASH LOG       \n";
			file << "==============================\n";

			file.close();
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

}

#endif

void CrashHandler::SetupCrashHandler()
{
#ifndef __ANDROID__
	SetUnhandledExceptionFilter(internal::UnhandledExceptionHandler);
#endif
}
