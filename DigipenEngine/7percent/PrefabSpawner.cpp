#include "PrefabSpawner.h"
#include "PrefabManager.h"

PrefabSpawnComponent::PrefabSpawnComponent() :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
	inited{ false },
	PrefabName{ "" }
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

#ifdef IMGUI_ENABLED
void PrefabSpawnComponent::EditorDraw(PrefabSpawnComponent& comp)
{
    // Label
    ImGui::Text("Prefab");
    ImGui::SameLine();

    // Display current prefab or placeholder
    std::string displayName = comp.PrefabName.empty() ? "<None>" : comp.PrefabName;

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
    ImGui::PushStyleColor(ImGuiCol_Text, comp.PrefabName.empty() ?
        ImVec4(0.6f, 0.6f, 0.6f, 1.0f) : // Gray text for empty state
        ImGui::GetStyleColorVec4(ImGuiCol_Text)); // Normal text otherwise

    ImGui::Button(displayName.c_str(), ImVec2(maxWidth, ImGui::GetFrameHeight()));
    ImGui::PopStyleColor(2);

    // Show tooltip with full name on hover if truncated
    if (ImGui::IsItemHovered() && displayName != comp.PrefabName && !comp.PrefabName.empty()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(comp.PrefabName.c_str());
        ImGui::EndTooltip();
    }

    // Handle drag and drop
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB"))
        {
            if (payload->DataSize > 0 && ImGui::IsMouseReleased(0))
            {
                comp.PrefabName = static_cast<const char*>(payload->Data);
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Add clear button if a prefab is selected
    if (!comp.PrefabName.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("X", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
            comp.PrefabName.clear();
        }
    }
}
#endif

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
