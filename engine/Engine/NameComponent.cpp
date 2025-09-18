/******************************************************************************/
/*!
\file   NameComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an source file implementing the name component and an attacher class that
  automatically attaches name components to new entities created.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "NameComponent.h"

NameComponent::NameComponent()
	: name{ "New Entity" }
{
}

NameComponent::NameComponent(const std::string& name)
	: name{ name }
{
}

const std::string& NameComponent::GetName() const
{
	return name;
}

void NameComponent::SetName(const std::string& newName)
{
	name = newName;
}

namespace util { namespace name {

	ecs::EntityHandle GetEntityWithNameInChildren(ecs::EntityHandle parent, const std::string& name)
	{
		for (Transform* child : parent->GetTransform().GetChildren())
		{
			if (child->GetEntity()->GetComp<NameComponent>()->GetName() == name)
				return child->GetEntity();
			if (ecs::EntityHandle entity{ GetEntityWithNameInChildren(child->GetEntity(), name) })
				return entity;
		}
		return nullptr;
	}
	ecs::ConstEntityHandle GetEntityWithNameInChildren(ecs::ConstEntityHandle parent, const std::string& name)
	{
		for (const Transform* child : parent->GetTransform().GetChildren())
		{
			if (child->GetEntity()->GetComp<NameComponent>()->GetName() == name)
				return child->GetEntity();
			if (ecs::ConstEntityHandle entity{ GetEntityWithNameInChildren(child->GetEntity(), name) })
				return entity;
		}
		return nullptr;
	}

} }
