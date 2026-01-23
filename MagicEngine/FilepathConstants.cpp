/******************************************************************************/
/*!
\file   FilepathConstants.cpp
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

#include "FilepathConstants.h"

#ifdef _DEBUG
const std::string Filepaths::workingDir = "../../..";
#else
const std::string Filepaths::workingDir = ".";
#endif

const std::string Filepaths::assets				= Filepaths::workingDir + "/Assets";
const std::string Filepaths::imguiJson			= Filepaths::workingDir + "/imgui.json";	//Physical path. outside of assets directory
const std::string Filepaths::gameSettings		= "Settings.json";

const std::string Filepaths::assetsJson			= "assets.json";
const std::string Filepaths::shadersSave		= "shaders";
const std::string Filepaths::fontsSave			= "fonts";
const std::string Filepaths::prefabsSave		= "prefabs";
const std::string Filepaths::scenesSave			= "scenes";
const std::string Filepaths::scriptsSave		= "scripts";
const std::string Filepaths::materialsSave		= "materials";
const std::string Filepaths::behaviourTreeSave	= "behaviourtrees";
const std::string Filepaths::navMeshDataSave	= "navmeshdata";
const std::string Filepaths::soundFolder		= "sounds";

// Editor use only
const std::string Filepaths::compilerExe		= Filepaths::workingDir + "/Tools/Debug/AssetCompiler.exe";
const std::string Filepaths::compileManifest	= Filepaths::workingDir + "/Tools/Debug/CompileResult.json";
const std::string Filepaths::compiledAssets		= Filepaths::assets + "/CompiledAssets";
const std::string Filepaths::compiledAssetsWindows = Filepaths::compiledAssets + "/windows";
const std::string Filepaths::compiledAssetsAndroid = Filepaths::compiledAssets + "/android";

std::filesystem::path Filepaths::GetAssetCompilerPath()
{
#ifdef _DEBUG
	return std::filesystem::path(workingDir) / "Tools" / "Debug" / "AssetCompiler.exe";
#else
	// Try Release first, fall back to Debug
	std::filesystem::path releasePath = std::filesystem::path(workingDir) / "Tools" / "Release" / "AssetCompiler.exe";
	if (std::filesystem::exists(releasePath))
		return releasePath;
	return std::filesystem::path(workingDir) / "Tools" / "Debug" / "AssetCompiler.exe";
#endif
}

const std::string& Filepaths::GetCompiledAssetsPath()
{
#ifdef __ANDROID__
	return compiledAssetsAndroid;
#else
	return compiledAssetsWindows;
#endif
}

const std::string Filepaths::graphicsWindowIcon = Filepaths::assets + "/Icon_game.png";

const std::string Filepaths::scriptsWorkingDir	= Filepaths::assets;
const std::string Filepaths::csproj				= Filepaths::scriptsWorkingDir + "/UserAssembly.csproj";
// Force the dlls to be located at the exe location
const std::string Filepaths::userAssemblyDll	= "./UserAssembly.dll";
const std::string Filepaths::engineScriptingDll = "./EngineScripting.dll";
const std::string Filepaths::glmSharpDll		= "./GlmSharp.dll";

void Filepaths::AddWorkingDirectoryTo(std::string* targetString)
{
	targetString->insert(0, workingDir);
}

std::string Filepaths::AddWorkingDirectoryTo(const std::string& targetString)
{
	return workingDir + targetString;
}

void Filepaths::TrimWorkingDirectoryFrom(std::string* targetString)
{
	if (targetString->rfind(workingDir, 0) == 0)
		targetString->erase(0, workingDir.size());
}

std::string Filepaths::TrimWorkingDirectoryFrom(const std::string& targetString)
{
	if (!(targetString.rfind(workingDir, 0) == 0))
		return targetString;
	return targetString.substr(workingDir.size());
}

std::string Filepaths::MakeRelativeToWorkingDir(const std::filesystem::path& path)
{
	return '/' + std::filesystem::relative(path, workingDir).string();
}

bool Filepaths::IsWithinWorkingDir(const std::filesystem::path& path)
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);
	std::filesystem::path absoluteWorkingDir = std::filesystem::absolute(workingDir);

	auto [begin, end] = std::mismatch(
		absoluteWorkingDir.begin(), absoluteWorkingDir.end(),
		absolutePath.begin(), absolutePath.end()
	);

	return begin == absoluteWorkingDir.end();
}
