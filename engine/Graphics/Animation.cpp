/******************************************************************************/
/*!
\file   Animation.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the definition for the Animation class.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

/**
 * @brief Constructs an Animation object with a specified number of frames.
 * 
 * @param frames The total number of frames in the animation.
 */
#include "Animation.h"

Animation::Animation(size_t frames)
	: Width{}
	, Height{}
	, totalFrames{ frames }
{
}

Animation::Animation()
	: Width{}
	, Height{}
	, totalFrames{}
{
}

void Animation::Serialize(Serializer& writer) const
{
	writer.Serialize("width", Width);
	writer.Serialize("height", Height);
	writer.Serialize("totalFrames", totalFrames);

	writer.StartArray("frames");
	for (const auto& frame : frames)
	{
		writer.StartObject();
		writer.Serialize("spriteID", frame.spriteID);
		writer.Serialize("duration", frame.duration);
		writer.EndObject();
	}
	writer.EndArray();
}

void Animation::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("width", &Width);
	reader.DeserializeVar("height", &Height);
	reader.DeserializeVar("totalFrames", &totalFrames);

	reader.PushAccess("frames");
	for (size_t i{}; i < totalFrames; ++i)
	{
		frames.push_back({});
		reader.PushArrayElementAccess(i);
		reader.DeserializeVar("spriteID", &frames.back().spriteID);
		reader.DeserializeVar("duration", &frames.back().duration);
		reader.PopAccess();
	}
	reader.PopAccess();
}
