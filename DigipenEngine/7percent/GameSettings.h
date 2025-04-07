/******************************************************************************/
/*!
\file   GameSettings.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/02/2024

\author Kendrick Sim Hean Guan (70%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Matthew Chan Shao Jie (30%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	This is a file to handle Serialized game settings.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "AudioManager.h"

/*****************************************************************//*!
\class GameSettings
\brief
	User settings for various aspects of this program.
*//******************************************************************/
struct GameSettings : public ISerializeable
{
public:
	GameSettings();

	/*****************************************************************//*!
	\brief
		Saves settings to file.
	*//******************************************************************/
	void Save();

	/*****************************************************************//*!
	\brief
		Loads settings from file. THIS SIMPLY READS VALUES - DOES NOT APPLY SETTINGS TO THE PROGRAM.
	*//******************************************************************/
	void Load();

	/*****************************************************************//*!
	\brief
		Applies current settings. THIS SIMPLY APPLY SETTINGS TO THE PROGRAM - DOES NOT UPDATE FROM FILE.
	*//******************************************************************/
	void Apply();

	/*****************************************************************//*!
	\brief
		Loads settings from file and applies them.
	*//******************************************************************/
	void LoadAndApply();

	/*****************************************************************//*!
	\brief
		Serializes game settings.
	*//******************************************************************/
	void Serialize(Serializer& writer) const override;

	/*****************************************************************//*!
	\brief
		Deserializes game settings.
	*//******************************************************************/
	void Deserialize(Deserializer& reader) override;

	void ApplyFullscreen();

	void ApplyVolumes();

	int m_settingsversion = 12;	//Increment this every time something is added

	// The size of the physics simulation.
	// The physics simulation will be centered around the player, and only updates objects within this range.
	float m_physicsSimulationSize = 1200.0f;
	// The size of the collision simulation.
	// Collision will be checked against all colliders within this range.
	float m_collisionSimulationSize = 1700.0f;

	float m_volumeBGM = 1.0f; // 0.0f - 1.0f
	float m_volumeSFX = 1.0f; // 0.0f - 1.0f

	int m_logLevel = 0;			// 0=DEBUG 1=INFO 2=WARNING 3=ERROR 4=FATAL
	int m_maxFPS = 0;			//<=0 is infinite, else max is this
	float m_targetFixedDt = 0.01666666667f;	// Fixed dt of the application. If 0 or less, fixed delta time is equal to delta time.
	int m_fullscreenMode = 1;	//0 is windowed, 1 is Fullscreen, 2 is borderless

	int m_resolutionX = 1920;
	int m_resolutionY = 1080;

	float m_editorZoomSensitivity = 0.2f; // The sensitivity of zooming the editor
	float m_editorZoomLerpFactor = 12.0f; // How fast the editor camera lerps to the target zoom
	int m_editorHistoryMax = 127; // The maximum number of events to save in the history log (for undo).

	std::string m_assetsRelativeFilepath = "/Assets"; // Relative filepath to the assets folder
	std::string m_assetsJsonRelativeFilepath = "/Assets/assets.json"; // Relative filepath to assets.json
	std::string m_shadersSaveLocation = "/Assets/Shaders"; // Relative filepath of shaders
	std::string m_fontsSaveLocation = "/Assets/Fonts"; // Relative filepath of fonts
	std::string m_prefabsSaveLocation = "/Assets/Prefab"; // Relative filepath of prefabs
	std::string m_scenesSaveLocation = "/Assets/Scenes"; // Relative filepath of scene saves
	std::string m_scriptsSaveLocation = "/Assets/Scripts"; // Relative filepath of scripts
	std::string m_scriptsTargetLocation = "/Assets"; // Relative filepath of the location where scripts will be scanned for compilation
	std::string m_materialSaveLocation = "/Assets/Materials"; // Relative filepath of materials

	property_vtable()
};
property_begin(GameSettings)
{
	property_var(m_settingsversion),

		property_var(m_physicsSimulationSize),
		property_var(m_collisionSimulationSize),

		property_var(m_volumeBGM),
		property_var(m_volumeSFX),

		property_var(m_logLevel),
		property_var(m_maxFPS),
		property_var(m_targetFixedDt),
		property_var(m_fullscreenMode),

		property_var(m_resolutionX),
		property_var(m_resolutionY),

		property_var(m_editorZoomSensitivity),
		property_var(m_editorZoomLerpFactor),
		property_var(m_editorHistoryMax),

		property_var(m_fontsSaveLocation),
		property_var(m_prefabsSaveLocation),
		property_var(m_scenesSaveLocation),
		property_var(m_scriptsSaveLocation),
		property_var(m_scriptsTargetLocation),
		property_var(m_materialSaveLocation)
}
property_vend_h(GameSettings)

/*****************************************************************//*!
\class Filepaths
\brief
	Provides standardized filepaths interface to certain locations.
*//******************************************************************/
class Filepaths
{
public:
	Filepaths();

	std::string workingDir;
	std::string gameSettings;

	// Assets folder
	std::string assets;
	std::string assetsJson;

	// Assets subfolders
	std::string shadersSave;
	std::string fontsSave;
	std::string prefabsSave;
	std::string scenesSave;
	std::string scriptsSave;
	std::string materialsSave;

	// Sound
	std::string soundSingleFolder;
	std::string soundGroupedFolder;

	// Scripting
	std::string scriptsWorkingDir;
	std::string csproj;
	std::string userAssemblyDll;
	std::string engineScriptingDll;

	/*****************************************************************//*!
	\brief
		Updates the full filepaths from GameSettings.
	*//******************************************************************/
	void UpdateFilepaths();

	/*****************************************************************//*!
	\brief
		Adds the working directory to the target string.
	\param targetString
		The target string.
	*//******************************************************************/
	void AddWorkingDirectoryTo(std::string* targetString);
	/*****************************************************************//*!
	\brief
		Adds the working directory to the target string.
	\param targetString
		The target string.
	\return
		The modified target string.
	*//******************************************************************/
	std::string AddWorkingDirectoryTo(const std::string& targetString);

	/*****************************************************************//*!
	\brief
		Trims the working directory from the target string.
	\param targetString
		The target string.
	*//******************************************************************/
	void TrimWorkingDirectoryFrom(std::string* targetString);
	/*****************************************************************//*!
	\brief
		Trims the working directory from the target string.
	\param targetString
		The target string.
	\return
		The modified target string.
	*//******************************************************************/
	std::string TrimWorkingDirectoryFrom(const std::string& targetString);

	/*****************************************************************//*!
	\brief
		Converts an absolute filepath to a relative filepath.
	\param path
		The absolute filepath.
	\return
		The relative filepath.
	*//******************************************************************/
	std::string MakeRelativeToWorkingDir(const std::filesystem::path& path) const;

	/*****************************************************************//*!
	\brief
		Checks if a directory points to a location within the working directory.
	\param path
		The filepath.
	\return
		True if path points to a location within the working directory.
	*//******************************************************************/
	bool IsWithinWorkingDir(const std::filesystem::path& path) const;

};
