/******************************************************************************/
/*!
\file   GameSettings.cpp
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
	Implementation of functions related to saving, loading Game Settings.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "GameSettings.h"
#include "EntityLayers.h"
#include "Engine.h"

GameSettings::GameSettings()
{
}

void GameSettings::Save()
{
	Serializer{ ST<Filepaths>::Get()->gameSettings }.Serialize(*this);
}

void GameSettings::Load()
{
	auto latestVer{ m_settingsversion };
	Deserializer d{ ST<Filepaths>::Get()->gameSettings };
	if (!d.IsValid() || !d.Deserialize(this) || m_settingsversion != latestVer)
	{
		m_settingsversion = latestVer;
		GameSettings{}.Save();
	}
	ST<Filepaths>::Get()->UpdateFilepaths();
}

void GameSettings::Apply()
{
	ST<Engine>::Get()->setFPS(m_maxFPS);
	GameTime::SetTargetFixedDt(m_targetFixedDt);
	ST<Console>::Get()->SetLogLevel(static_cast<LogLevel>(m_logLevel));

	ApplyVolumes();
	ApplyFullscreen();

	// Set everything else here!
}

void GameSettings::LoadAndApply()
{
	Load();
	Apply();
}

void GameSettings::Serialize(Serializer& writer) const
{
	ISerializeable::Serialize(writer);

	// In addition to the reflected vars, serialize the layers here.
	EntityLayerComponent::SerializeLayersMatrix(writer, "collisionLayers");
	ST<ecs::RegisteredSystemsOperatingByLayer>::Get()->SerializeLayerSettings(writer, "systemLayers");
}

void GameSettings::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);

	EntityLayerComponent::DeserializeLayersMatrix(reader, "collisionLayers");
	ST<ecs::RegisteredSystemsOperatingByLayer>::Get()->DeserializeLayerSettings(reader, "systemLayers");
}

void GameSettings::ApplyFullscreen()
{
	switch (m_fullscreenMode)
	{
	case 0:
		ST<Engine>::Get()->onResolutionChanged(m_resolutionX, m_resolutionY);
		break;
	case 1:
		ST<Engine>::Get()->onFullscreen();
		break;
	}
}

void GameSettings::ApplyVolumes()
{
	// Set volume for background music
	ST<AudioManager>::Get()->SetBaseVolume("BGM", m_volumeBGM);

	// Set volume for sound effects
	ST<AudioManager>::Get()->SetBaseVolume("SFX", m_volumeSFX);
}

Filepaths::Filepaths()
{
	UpdateFilepaths();
}

void Filepaths::UpdateFilepaths()
{
	workingDir = ".";
#ifdef _DEBUG
	// Use the project's actual assets folder so changes in editor do not have to be manually copied out to the project root so changes reflect on git
	if (std::filesystem::is_directory("../../Assets/"))
		workingDir = "../..";
#endif

	// Reset all filepaths to the root first. NOTE: FILEPATH CLASS MUST NOT HAVE ANY OTHER VARIABLE OTHER THAN STRINGS! This is technically undefined behavior but works for our purposes here.
	for (std::string* filepathPtr{ reinterpret_cast<std::string*>(this) }, *endPtr{ reinterpret_cast<std::string*>(this + 1) }; filepathPtr < endPtr; ++filepathPtr)
		*filepathPtr = workingDir;

	gameSettings += "/Assets/Settings.json";

	// Append the relative filepaths to each full filepath string
	const GameSettings& settings{ *ST<GameSettings>::Get() };
	assets += settings.m_assetsRelativeFilepath;
	assetsJson += settings.m_assetsJsonRelativeFilepath;

	shadersSave += settings.m_shadersSaveLocation;
	fontsSave += settings.m_fontsSaveLocation;
	prefabsSave += settings.m_prefabsSaveLocation;
	scenesSave += settings.m_scenesSaveLocation;
	scriptsSave += settings.m_scriptsSaveLocation;
	materialsSave += settings.m_materialSaveLocation;

	soundSingleFolder = assets + "/Sounds/SingleSounds/";
	soundGroupedFolder = assets + "/Sounds/GroupedSounds/";

	scriptsWorkingDir += settings.m_scriptsTargetLocation;
	csproj = scriptsWorkingDir + "/UserAssembly.csproj";
	// Force the dlls to be located at the exe location
	userAssemblyDll = "./UserAssembly.dll";
	engineScriptingDll = "./EngineScripting.dll";
}

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
	if (!targetString.rfind(workingDir, 0) == 0)
		return targetString;
	return targetString.substr(workingDir.size());
}

std::string Filepaths::MakeRelativeToWorkingDir(const std::filesystem::path& path) const
{
	return '/' + std::filesystem::relative(path, workingDir).string();
}

bool Filepaths::IsWithinWorkingDir(const std::filesystem::path& path) const
{
	std::filesystem::path absolutePath = std::filesystem::absolute(path);
	std::filesystem::path absoluteWorkingDir = std::filesystem::absolute(workingDir);

	auto [begin, end] = std::mismatch(
		absoluteWorkingDir.begin(), absoluteWorkingDir.end(),
		absolutePath.begin(), absolutePath.end()
	);

	return begin == absoluteWorkingDir.end();
}
