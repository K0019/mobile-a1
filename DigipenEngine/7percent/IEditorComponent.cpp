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

#include "IEditorComponent.h"

std::unordered_map<size_t, void(*)(void*)> editor::ComponentDrawMethods::drawFuncs;

namespace editor {

	void ComponentDrawMethods::Register(size_t compHash, void(*compDrawFunc)(void*))
	{
		drawFuncs.emplace(compHash, compDrawFunc);
	}

	bool ComponentDrawMethods::Draw(size_t compHash, void* compHandle)
	{
		auto drawFuncIter{ drawFuncs.find(compHash) };
		if (drawFuncIter == drawFuncs.end())
			return false;

		drawFuncIter->second(compHandle);
		return true;
	}

}
