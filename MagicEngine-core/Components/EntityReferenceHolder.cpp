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
EntityReferenceHolderComponent::~EntityReferenceHolderComponent()
{
	for (int i = 0; i < entityNames.size(); ++i)
		delete entityNames[i];
}

EntityReferenceHolderComponent::EntityReferenceHolderComponent(const std::string& name)
{
}

void EntityReferenceHolderComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("referencedComponents", referencedComponents);
}

void EntityReferenceHolderComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("referencedComponents", &referencedComponents);
	UpdateVectors();
}

void EntityReferenceHolderComponent::UpdateVectors()
{
	for (int i = 0; i < entityNames.size(); ++i)
		delete entityNames[i];

	entityNames.resize(0);
	entityReferences.resize(0);

	for (auto iterator : referencedComponents)
	{
		char* name = new char[16];
		strcpy_s(name, 16, iterator.first.c_str());
		entityNames.push_back(name);

		entityReferences.push_back(iterator.second);
	}

}

void EntityReferenceHolderComponent::UpdateMap()
{
	referencedComponents.clear();

	for (int i = 0;i<entityNames.size();++i)
	{
		referencedComponents[std::string{ entityNames[i] }] = entityReferences[i];
	}

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
			auto name = new char[16] {};
			strcpy(name, (std::to_string(referencedComponents.size())).c_str());
			name[15] = '\0';
			referencedComponents[name] = nullptr;
			UpdateVectors();
		}
		table.NextColumn();
		if (gui::Button button{ "Remove Reference" })
		{
			if (referencedComponents.size() > 0)
			{
				auto lastItem{ referencedComponents.end() };
				--lastItem;
				referencedComponents.erase(lastItem);
				UpdateVectors();
			}
		}

		for (int i = 0; i < referencedComponents.size(); ++i)
		{
			table.NextColumn();

			if (gui::VarDefault(std::string("-##" + std::to_string(i)).c_str(), entityNames[i]))
			{
				UpdateMap();
			}

			//gui::TextBox(std::string("-##" + std::to_string(i)).c_str(), entityNames[i], 16);
			table.NextColumn();
			if (entityReferences[i].EditorDraw(std::string("##" + std::to_string(i)).c_str()))
			{
				UpdateMap();
			}
		}
	}
#endif
}


