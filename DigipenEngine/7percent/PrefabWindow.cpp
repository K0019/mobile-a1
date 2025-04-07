/******************************************************************************/
/*!
\file   PrefabWindow.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
Definition of Prefab Window class ans functions

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "PrefabWindow.h"
#include "AssetBrowser.h"
#include "Editor.h"

PrefabWindow::PrefabWindow()
{
}
#ifdef IMGUI_ENABLED
void PrefabWindow::DrawSaveLoadPrompt(bool* p_open)
{
	if (!isLoading && ST<Editor>::Get()->GetSelectedEntity() != nullptr)
	{
		ImGui::Begin("Save Prefab", p_open);
		ImGui::SetWindowSize(ImVec2(500, 100));
		ImGui::InputText("##Username", prefabName, IM_ARRAYSIZE(prefabName));
		ImGui::SetCursorPos(ImVec2(10, 50));
		if (ImGui::Button("Save"))
		{
			*p_open = false;
			PrefabManager::SavePrefab(ST<Editor>::Get()->GetSelectedEntity(), prefabName);
		}
		ImGui::End();
	}
	else
	{
		ImGui::Begin("Load Prefab", p_open);
		ImGui::SetWindowSize(ImVec2(500, 100));
		for (std::string name : PrefabManager::AllPrefabs())
		{
			if (ImGui::Button(name.c_str()))
			{
				*p_open = false;
				PrefabManager::LoadPrefab(name);
			}
		}
		ImGui::End();
	}
}

void PrefabWindow::Open(bool loading)
{
	isOpen = true;
	isLoading = loading;
}

const char* PrefabWindow::PrefabName()
{
	return prefabName;
}

const bool& PrefabWindow::IsOpen() const
{
	return isOpen;
}

bool& PrefabWindow::IsOpen()
{
	return isOpen;
}
#endif