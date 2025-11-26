/******************************************************************************/
/*!
\file   EntityReferenceHolderComponent.cpp
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

#include "Components/EntityReferenceHolder.h"
#include "Editor/Containers/GUICollection.h"

EntityReferenceHolderComponent::EntityReferenceHolderComponent()
{
}


void EntityReferenceHolderComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("entityReferences", entityReferences);
}

void EntityReferenceHolderComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("entityReferences", &entityReferences);
}

EntityReference EntityReferenceHolderComponent::GetEntity(int index)
{
	if (index < 0 || index >= entityReferences.size())
	{
		return nullptr;
	}
	return entityReferences[index];
}
void EntityReferenceHolderComponent::SetEntity(int index, EntityReference entity)
{
	if (index < 0)
		return;

	while (index >= entityReferences.size())
		entityReferences.push_back(nullptr);

	entityReferences[index] = entity;
}

void EntityReferenceHolderComponent::EditorDraw()
{
	// Workaround for now due to custom UI
#ifdef IMGUI_ENABLED
	/*		if (gui::Table table{ "", 4, true })
		{
			table.AddColumnHeader("Input Sets");
			table.AddColumnHeader("Actions");
			table.AddColumnHeader("Bindings");
			table.AddColumnHeader("Settings");
			table.SubmitColumnHeaders();

			DrawInputSetsColumn();
			table.NextColumn();
			const std::string* actionName{ DrawActionsColumn() };
			table.NextColumn();
			DrawBindingsColumn();
			table.NextColumn();
			DrawInspector(actionName);
		}
*/
// Add/Remove buttons
	if (gui::Table table{ "Values", 2,false })
	{
		if (gui::Button button{ "Add Reference" })
		{
			entityReferences.push_back(nullptr);
		}
		table.NextColumn();
		if (gui::Button button{ "Remove Reference" })
		{
			if (entityReferences.size() > 0)
			{
				entityReferences.pop_back();
			}
		}
	}
	int index = 0;
	for (auto& iterator : entityReferences)
	{
		iterator.EditorDraw(std::string("##"+std::to_string(index++)).c_str());
	}
#endif
}


