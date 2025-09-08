#include "MiscAssetTabs.h"
#include "ResourceManager.h"
#include "PrefabWindow.h"
#include "Import.h"
#include "EditorGuiUtils.h"

///////////////
/// Prefabs ///
///////////////
const char* PrefabTab::GetName() const
{
    return "Prefabs";
}

const char* PrefabTab::GetIdentifier() const
{
    return ICON_FA_CUBE" Prefabs";
}

void PrefabTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    std::string prefabName = "";
    for (size_t i = 0; i < PrefabManager::AllPrefabs().size(); ++i)
    {
        prefabName = PrefabManager::AllPrefabs()[i];
        if (!MatchesFilter(prefabName))
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        {
            gui::Group group;

            bool clicked = ImGui::Button(ICON_FA_CUBE,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE + 10));

            gui::ShowSimpleHoverTooltip(prefabName);
            gui::PayloadSource<std::string>("PREFAB", prefabName.c_str(), std::string{ ICON_FA_CUBE + prefabName}.c_str());

            if (clicked)
            {
                ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(prefabName) };
                ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
            }

            // Name label
            gui::ThumbnailLabel(prefabName, THUMBNAIL_SIZE);
        }

        ImGui::PopID();

        if (ImGui::BeginPopupContextItem(prefabName.c_str()))
        {
            if (ImGui::MenuItem(ICON_FA_TRASH" Delete"))
            {
                CONSOLE_LOG_EXPLICIT("DELETE " + prefabName, LogLevel::LEVEL_DEBUG);
                PrefabManager::DeletePrefab(prefabName);
            }
            if (ImGui::MenuItem(ICON_FA_CLONE" Duplicate"))
            {
                CONSOLE_LOG_EXPLICIT("CLONE " + prefabName, LogLevel::LEVEL_DEBUG);
            }
            ImGui::EndPopup();
        }

        grid.NextItem();
    }
}

///////////////
///  Fonts  ///
///////////////
const char* FontTab::GetName() const
{
    return "Fonts";
}

const char* FontTab::GetIdentifier() const
{
    return ICON_FA_FONT" Fonts";
}

void FontTab::Render()
{
    const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();

    ImGui::BeginChild("FontTable", ImVec2(0.0f, 0.0f), true);
    ImGui::Text("Loaded Fonts");
    ImGui::Separator();

    for (const auto& [atlasName, atlas] : fontAtlases)
    {
        ImGui::Text("%s", atlasName.c_str());
    }

    ImGui::EndChild();
}

///////////////
/// Shaders ///
///////////////
const char* ShaderTab::GetName() const
{
    return "Shaders";
}

const char* ShaderTab::GetIdentifier() const
{
    return ICON_FA_FILE_CODE" Shaders";
}

void ShaderTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

    static std::vector<std::string> shaderNames;
    if (shaderNames.empty())
    {
        for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->shadersSave })
            shaderNames.push_back(entry.path().filename().string());
    }

    int count{};
    for (const auto& shaderName : shaderNames)
    {
        gui::SetID id{ count++ };

        ImGui::BeginGroup();

        ImGui::Button("##shader", gui::Vec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE });

        // Name label
        gui::ThumbnailLabel(shaderName, THUMBNAIL_SIZE);
        ImGui::EndGroup();

        grid.NextItem();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}
