/******************************************************************************/
/*!
\file   IEditorComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for IEditorComponent, which registers a function for
  each type that inherits from it to draw a component to the ImGui window of Editor.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ECS/IEditorComponent.h"

void editor::ComponentDrawMethods::Register(size_t compHash, size_t offset)
{
	byteOffsets.try_emplace(compHash, offset);
}

bool editor::ComponentDrawMethods::Draw(size_t compHash, void* compHandle)
{
	auto byteOffsetIter{ byteOffsets.find(compHash) };
	if (byteOffsetIter == byteOffsets.end())
		return false;

	reinterpret_cast<internal::IEditorComponentBase*>(reinterpret_cast<uint8_t*>(compHandle) + byteOffsetIter->second)->EditorDraw();
	return true;
}
