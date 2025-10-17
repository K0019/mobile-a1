/******************************************************************************/
/*!
\file   ScriptManagement.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e
\brief
  This file contains the definitions of the class Script Management used for the
  asset browser of the engine and interacts with the user assembly.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Scripting/ScriptManagement.h"
#include "Scripting/CSScripting.h"
#include "GameSettings.h"

bool ScriptManager::IsScriptsFolderExists()
{
	return std::filesystem::exists(GetScriptsFolder());
}

bool ScriptManager::CreateScriptsFolder()
{
	return std::filesystem::create_directory(GetScriptsFolder());
}

bool ScriptManager::EnsureScriptsFolderExists()
{
	if (!IsScriptsFolderExists() && !CreateScriptsFolder())
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to create Scripts directory!";
		return false;
	}
	return true;
}

const std::string& ScriptManager::GetScriptsFolder()
{
	return ST<Filepaths>::Get()->scriptsSave;
}

bool ScriptManager::OpenScript(const std::string& scriptName)
{
	std::string filePath = ST<Filepaths>::Get()->scriptsSave + "/" + scriptName;
	std::string command = "start devenv \"" + GetAbsolutePath(ST<Filepaths>::Get()->csproj) + "\" \"" + GetAbsolutePath(filePath) + "\"";

	// Use std::system to execute the command
	int result = std::system(command.c_str());
	if (result != 0)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Script " << scriptName << " was created but is unable to be opened in Visual Studio";
		return false;
	}
	
	return true;
}

std::string ScriptManager::GetAbsolutePath(const std::string& relativePath)
{
	return std::filesystem::absolute(relativePath).string();
}

bool ScriptManager::CreateScript(const std::string& scriptName)
{
	std::string filePath = ST<Filepaths>::Get()->scriptsSave + "/" + scriptName;
	std::ofstream s(filePath);
	/*std::ofstream("./Assets/Scripts/" + name);*/
	if (!s.is_open())
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to create " << scriptName << "!";
		return 0;
	}

	s <<
R"(using GlmSharp;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using EngineScripting;
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Sequential)]
public class )" + scriptName.substr(0, scriptName.find_last_of('.')) + R"( : ComponentBase
{
	)" + scriptName.substr(0, scriptName.find_last_of('.')) + R"(()
	{
	}

    // This method is called once on the frame when the script is initialized
    void Start()
    {
       
    }

    // This method is called once per frame
    void Update(float dt)
    {
        // Update logic here
    }
}
)";
	s.close();

	CSharpScripts::CSScripting::ReloadAssembly();

	OpenScript(scriptName);

	return true;
}
