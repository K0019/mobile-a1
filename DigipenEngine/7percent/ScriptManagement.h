/******************************************************************************/
/*!
\file   ScriptManagement.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/20/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e
\brief
  This file contains the declaration of the class Script Management used for the
  asset browser of the engine and interacts with the user assembly.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once


class ScriptManager
{
public:

	/*****************************************************************//*!
	\brief
		Checks to ensure that the Scripts Folder that the asset browser
		uses exists
	\return
		bool of if it exists
	*//******************************************************************/
	bool EnsureScriptsFoldedrExists();

	/*****************************************************************//*!
	\brief
		Creates a user script and fills it with a template.
	\param[in] scriptName
		Name of the script file to create.
	\return
		int to check the success of the script's creation.
	*//******************************************************************/
	static const int CreateScript(const std::string& scriptName);

	/*****************************************************************//*!
	\brief
		Opens the selected script file inside the asset browser in the
		user assembly .csproj visual studio environment.
	\param[in] scriptName
		Name of the script file to open.
	\return
		bool if the opening of the script file was a success
	*//******************************************************************/
	static bool OpenScript(const std::string& scriptName);

	/*****************************************************************//*!
	\brief
		Gets the absolute path of a file.
	\param[in] relativePath
		Relative path of the file.
	\return
		string of the absolute file within the system.
	*//******************************************************************/
	static std::string GetAbsolutePath(const std::string& relativePath);

private:

	/*****************************************************************//*!
	\brief
		Checks to see if the Scripts folder that the asset browser uses
		exists.
	\return
		bool if it exists.
	*//******************************************************************/
	bool IsScriptsFolderExists();

	/*****************************************************************//*!
	\brief
		Creates the folder to hold scripts that the asset browser uses.
	\return
		bool if the creation fo the file is a success.
	*//******************************************************************/
	bool CreateScriptsFolder();

	/*****************************************************************//*!
	\brief
		Returns the relative path of the Script Folder that the asset
		browser uses.
	\return
		string of the relative path of the script folder.
	*//******************************************************************/
	static const std::string& GetScriptsFolder();
	
	friend ST<ScriptManager>;
};

