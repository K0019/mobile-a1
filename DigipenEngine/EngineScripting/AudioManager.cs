/******************************************************************************/
/*!
\file   AudioManager.cs
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	Allows C# to call AudioManager functions.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;

namespace EngineScripting
{
    public class AudioManager
    {
		/*****************************************************************//*!
		\brief
			Starts playing a single sound.
		\param name
			Name of the sound, excluding extensions and trailing numbers.
		\param loop
			Whether to loop this sound.
		\param position
			World position. If provided, spatial audio will be applied.
		*//******************************************************************/
		public static void StartSingleSound(float volume, string name, bool loop = false, Vector2? position = null)
        {
			if (position.HasValue)
			{
				InternalCalls.StartSingleSoundWithPosition(volume, name, loop, position.Value);
			}
			else
			{
				InternalCalls.StartSingleSound(volume, name, loop);
			}
        }

		/*****************************************************************//*!
		\brief
			Starts playing a random sound from a sound group.
		\param baseName
			Name of the sound group, excluding extensions and trailing numbers.
		\param loop
			Whether to loop this sound.
		\param position
			World position. If provided, spatial audio will be applied.
		*//******************************************************************/
		public static void StartGroupedSound(float volume, string name, bool loop = false, Vector2? position = null)
		{
			if (position.HasValue)
			{
				InternalCalls.StartGroupedSoundWithPosition(volume, name, loop, position.Value);
			}
			else
			{
				InternalCalls.StartGroupedSound(volume, name, loop);
			}
		}

		/*****************************************************************//*!
		\brief
			Stops a sound that is currently playing.
		\param name
			Name of the sound you want to stop.
		*//******************************************************************/
		public static void StopSound(string name)
		{
			InternalCalls.StopSound(name);
		}

		/*****************************************************************//*!
		\brief
			Stops all currently playing sounds.
		*//******************************************************************/
		public static void StopAllSounds()
		{
			InternalCalls.StopAllSounds();
		}

		/*****************************************************************//*!
		\brief
			Sets the channel group of a certain sound.
		\param soundName
			Name of the sound, excluding extensions and trailing numbers.
		\param channelName
			Name of the channel group.
			"SFX" is available by default.
		*//******************************************************************/
		public static void SetChannelGroup(string soundName, string channelName)
		{
			InternalCalls.SetChannelGroup(soundName, channelName);
		}

		/*****************************************************************//*!
		\brief
			Sets the volume of a soundgroup.
		\param volume
			float value representing new volume.
		\param group
			Name of the sound group.
		*//******************************************************************/
		public static void SetGroupVolume(float volume, string group)
		{
			InternalCalls.SetGroupVolume(volume, group);
		}
	}
}
