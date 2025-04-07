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
#include "ScriptManagement.h"
#include "CSScripting.h"
#include "GameSettings.h"

bool ScriptManager::IsScriptsFolderExists()
{
	return std::filesystem::exists(GetScriptsFolder());
}

bool ScriptManager::CreateScriptsFolder()
{
	return std::filesystem::create_directory(GetScriptsFolder());
}

bool ScriptManager::EnsureScriptsFoldedrExists()
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
	// add to the thing
	// Now, open the script in Visual Studio (Windows-specific)
	std::string command = "start devenv \"" + GetAbsolutePath(ST<Filepaths>::Get()->csproj) + "\" \"" + GetAbsolutePath(filePath) + "\"";

	// Use std::system to execute the command
	int result = std::system(command.c_str());

	if (result == 0)
	{
		std::cout << "Successfully opened " << scriptName << " in Visual Studio." << std::endl;
		return true;
	}
	else
	{
		std::cerr << "Failed to open " << scriptName << " in Visual Studio." << std::endl;
		return false;
	}
	return false;
}

std::string ScriptManager::GetAbsolutePath(const std::string& relativePath)
{
	std::filesystem::path p(relativePath);
	return std::filesystem::absolute(p).string();
}

const int ScriptManager::CreateScript(const std::string& scriptName)
{

	std::string filePath = ST<Filepaths>::Get()->scriptsSave + "/" + scriptName;
	std::ofstream s(filePath);
	/*std::ofstream("./Assets/Scripts/" + name);*/
	if (!s.is_open())
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Failed to create " << scriptName << "!";

		return 0;
	}

	std::string defaultContent = 
		R"(using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using EngineScripting;

public class )" + scriptName.substr(0, scriptName.find_last_of('.')) + R"( : EID
{
	)" + scriptName.substr(0, scriptName.find_last_of('.')) + R"(()
	{
	}

    // This method is called once when the script is first initialized
    public void OnCreate()
    {
       
    }

    // This method is called once per frame
    void OnUpdate(float dt)
    {
        // Update logic here
    }
}
)";

	s << defaultContent;
	s.close();

	CSharpScripts::CSScripting::ReloadAssembly();

	// add to the thing
	// Now, open the script in Visual Studio (Windows-specific)
	std::string command = "start devenv \"" + GetAbsolutePath(ST<Filepaths>::Get()->csproj) + "\" \"" + GetAbsolutePath(filePath) + "\"";


	// Use std::system to execute the command
	int result = std::system(command.c_str());

	if (result == 0)
	{
		std::cout << "Successfully opened " << scriptName << " in Visual Studio." << std::endl;
	}
	else
	{
		std::cerr << "Failed to open " << scriptName << " in Visual Studio." << std::endl;
	}
	return 1;
}
