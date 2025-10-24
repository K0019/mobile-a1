/******************************************************************************/
/*!
\file   FilepathConstants.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Contains string constants defining filepaths.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\class Filepaths
\brief
	Provides standardized filepaths interface to certain locations.
*//******************************************************************/
class Filepaths
{
private:
	Filepaths() = delete;

public:
	static const std::string workingDir;
	static const std::string gameSettings;
	static const std::string virtualGameSettings;

	// Assets folder
	static const std::string assets;
	static const std::string assetsJson;

	// Assets subfolders
	static const std::string shadersSave;
	static const std::string fontsSave;
	static const std::string prefabsSave;
	static const std::string virtualPrefabsSave;
	static const std::string scenesSave;
	static const std::string virtualScenesSave;
	static const std::string scriptsSave;
	static const std::string materialsSave;
	static const std::string behaviourTreeSave;
	static const std::string virtualBehaviourTreeSave;

	// Graphics
	static const std::string graphicsWindowIcon;

	// Sound
	static const std::string soundFolder;

	// Scripting
	static const std::string scriptsWorkingDir;
	static const std::string csproj;
	static const std::string userAssemblyDll;
	static const std::string engineScriptingDll;
	static const std::string glmSharpDll;

	/*****************************************************************//*!
	\brief
		Adds the working directory to the target string.
	\param targetString
		The target string.
	*//******************************************************************/
	static void AddWorkingDirectoryTo(std::string* targetString);
	/*****************************************************************//*!
	\brief
		Adds the working directory to the target string.
	\param targetString
		The target string.
	\return
		The modified target string.
	*//******************************************************************/
	static std::string AddWorkingDirectoryTo(const std::string& targetString);

	/*****************************************************************//*!
	\brief
		Trims the working directory from the target string.
	\param targetString
		The target string.
	*//******************************************************************/
	static void TrimWorkingDirectoryFrom(std::string* targetString);
	/*****************************************************************//*!
	\brief
		Trims the working directory from the target string.
	\param targetString
		The target string.
	\return
		The modified target string.
	*//******************************************************************/
	static std::string TrimWorkingDirectoryFrom(const std::string& targetString);

	/*****************************************************************//*!
	\brief
		Converts an absolute filepath to a relative filepath.
	\param path
		The absolute filepath.
	\return
		The relative filepath.
	*//******************************************************************/
	static std::string MakeRelativeToWorkingDir(const std::filesystem::path& path);

	/*****************************************************************//*!
	\brief
		Checks if a directory points to a location within the working directory.
	\param path
		The filepath.
	\return
		True if path points to a location within the working directory.
	*//******************************************************************/
	static bool IsWithinWorkingDir(const std::filesystem::path& path);

};
