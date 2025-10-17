#include "Game/PrefabSpawner.h"
#include "Engine/PrefabManager.h"
#include "Editor/Containers/GUICollection.h"

PrefabSpawnComponent::PrefabSpawnComponent()
    : inited{ false }
	, PrefabName{ "" }
{
}
void PrefabSpawnComponent::OnStart()
{
    // Deprecated
}

void PrefabSpawnComponent::Init()
{
    if (PrefabName.empty())
    {
        return;
    }
    ecs::EntityHandle enemyEntity{ PrefabManager::LoadPrefab(PrefabName) };
    enemyEntity->GetTransform().SetWorldPosition(ecs::GetEntityTransform(this).GetWorldPosition());
    ecs::DeleteEntity(ecs::GetEntity(this));
    inited = true;
}

void PrefabSpawnComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    // Label
    ImGui::Text("Prefab");
    ImGui::SameLine();

    // Display current prefab or placeholder
    std::string displayName = PrefabName.empty() ? "<None>" : PrefabName;

    // Truncate long names
    float maxWidth = 150.0f;
    ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
    if (textSize.x > maxWidth) {
        while (textSize.x > maxWidth && displayName.length() > 3) {
            displayName = displayName.substr(0, displayName.length() - 4) + "...";
            textSize = ImGui::CalcTextSize(displayName.c_str());
        }
    }

    // Simple text display with background to indicate it's a drop target
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleColor(ImGuiCol_Text, PrefabName.empty() ?
        ImVec4(0.6f, 0.6f, 0.6f, 1.0f) : // Gray text for empty state
        ImGui::GetStyleColorVec4(ImGuiCol_Text)); // Normal text otherwise

    ImGui::Button(displayName.c_str(), ImVec2(maxWidth, ImGui::GetFrameHeight()));
    ImGui::PopStyleColor(2);

    // Show tooltip with full name on hover if truncated
    if (ImGui::IsItemHovered() && displayName != PrefabName && !PrefabName.empty()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(PrefabName.c_str());
        ImGui::EndTooltip();
    }

    // Handle drag and drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB"))
        {
            if (payload->DataSize > 0 && ImGui::IsMouseReleased(0))
            {
                PrefabName = static_cast<const char*>(payload->Data);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Add clear button if a prefab is selected
    if (!PrefabName.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
            PrefabName.clear();
        }
    }
#endif
}

PrefabSpawnSystem::PrefabSpawnSystem()
    : System_Internal{ &PrefabSpawnSystem::UpdatePrefabSpawn }
{
}

void PrefabSpawnSystem::UpdatePrefabSpawn(PrefabSpawnComponent& comp)
{
    if (!comp.inited)
    {
        comp.Init();
    }
}
