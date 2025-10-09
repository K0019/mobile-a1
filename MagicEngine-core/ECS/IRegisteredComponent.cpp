/******************************************************************************/
/*!
\file   IRegisteredComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements components that are registered to a global list of components.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "IRegisteredComponent.h"

const RegisteredComponentData* RegisteredComponents::GetData(ecs::CompHash compHash)
{
	auto dataIter{ ST<RegisteredComponents>::Get()->dataMap.find(compHash) };
	if (dataIter == ST<RegisteredComponents>::Get()->dataMap.end())
		return nullptr;
	return &dataIter->second;
}

RegisteredComponents::ConstIterator RegisteredComponents::Begin()
{
	return ST<RegisteredComponents>::Get()->dataMap.begin();
}

RegisteredComponents::ConstIterator RegisteredComponents::End()
{
	return ST<RegisteredComponents>::Get()->dataMap.end();
}

void RegisteredComponents::RegisterComponent(ecs::CompHash compHash, RegisteredComponentData&& data)
{
	ST<RegisteredComponents>::Get()->dataMap.emplace(compHash, std::forward<RegisteredComponentData>(data));
}
