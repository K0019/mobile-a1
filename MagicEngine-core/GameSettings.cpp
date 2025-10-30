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
#include "FilepathConstants.h"
#include "Engine/Input.h"
#include "Engine/EntityLayers.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Engine.h"
#include "Managers/AudioManager.h"

GameSettings::GameSettings()
{
}

void GameSettings::Save()
{
#ifdef GLFW
	Serializer{ Filepaths::gameSettings }.Serialize(*this);
#endif
}

void GameSettings::Load()
{
	auto latestVer{ m_settingsversion };
	Deserializer d{ Filepaths::gameSettings };
	if (!d.IsValid() || !d.Deserialize(this) || m_settingsversion != latestVer)
	{
		m_settingsversion = latestVer;
		GameSettings{}.Save();
	}
}

void GameSettings::Apply()
{
	GameTime::SetTargetFps(m_maxFPS);
	GameTime::SetTargetFixedDt(m_targetFixedDt);
	ST<internal::LoggedMessagesBuffer>::Get()->SetLogLevel(static_cast<LogLevel>(m_logLevel));

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

	// In addition to the reflected vars, serialize other systems that are constant across all game builds
	ST<MagicInput>::Get()->Serialize(writer);

	EntityLayerComponent::SerializeLayersMatrix(writer, "collisionLayers");
	ST<ecs::RegisteredSystemsOperatingByLayer>::Get()->SerializeLayerSettings(writer, "systemLayers");
}

void GameSettings::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);

	ST<MagicInput>::Get()->Deserialize(reader);

	EntityLayerComponent::DeserializeLayersMatrix(reader, "collisionLayers");
	ST<ecs::RegisteredSystemsOperatingByLayer>::Get()->DeserializeLayerSettings(reader, "systemLayers");
}

void GameSettings::ApplyFullscreen()
{
	switch (m_fullscreenMode)
	{
	case 0:
		ST<GraphicsWindow>::Get()->SetWindowResolution(m_resolutionX, m_resolutionY);
		break;
	case 1:
		ST<GraphicsWindow>::Get()->SetFullscreen(true);
		break;
	}
}

void GameSettings::ApplyVolumes()
{
	// Set volume for background music
	ST<AudioManager>::Get()->SetGroupVolume(AudioType::BGM, m_volumeBGM);

	// Set volume for sound effects
	ST<AudioManager>::Get()->SetGroupVolume(AudioType::SFX, m_volumeSFX);
}
