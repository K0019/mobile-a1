#include "Editor/AnimationsTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Assets/Types/AssetTypes.h"

namespace editor
{
    const char* AnimationsTab::GetName() const
    {
        return "Animations";
    }

    const char* AnimationsTab::GetIdentifier() const
    {
        return ICON_FA_PERSON_RUNNING" Animations";
    }

    void AnimationsTab::Render(const gui::TextBoxWithFilter& filter)
    {
        float availableHeight = gui::GetAvailableContentRegion().y;
        float detailPanelHeight = selectedAnimationHash.has_value() ? 150.0f : 0.0f;
        float gridHeight = availableHeight - detailPanelHeight;

        RenderGridView(filter, gridHeight);

        if (selectedAnimationHash.has_value())
        {
            gui::Separator();
            RenderDetailPanel();
        }
    }

    void AnimationsTab::RenderGridView(const gui::TextBoxWithFilter& filter, float height)
    {
        gui::Child gridChild{ "AnimationGrid", gui::Vec2{ 0.0f, height } };

        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (const auto& [hash, animRef] : ST<AssetManager>::Get()->Editor_GetContainer<ResourceAnimation>().Editor_GetAllResources())
        {
            const std::string& animName{ *ST<AssetManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(animName))
                continue;

            gui::GridItem item{ grid.Item(animName) };

            bool isSelected{ selectedAnimationHash.has_value() && selectedAnimationHash.value() == hash.get() };
            gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.6f }, isSelected };

            if (gui::Button{ ICON_FA_PERSON_RUNNING, gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE} })
                if (isSelected)
                    selectedAnimationHash.reset();
                else
                    selectedAnimationHash = hash.get();

            gui::PayloadSource{ "ANIMATION_HASH", hash.get() };
        }
    }

    void AnimationsTab::RenderDetailPanel()
    {
        if (!selectedAnimationHash.has_value())
            return;

        gui::Child detailsChild{ "AnimationDetails", gui::Vec2{}, gui::FLAG_CHILD::BORDERS };

        size_t hash{ selectedAnimationHash.value() };
        const std::string* animName{ ST<AssetManager>::Get()->Editor_GetName(hash) };
        if (!animName)
        {
            gui::TextColored(gui::Vec4{ 1.0f, 0.4f, 0.4f, 1.0f }, "Animation not found!");
            return;
        }

        // Header with close button
        gui::TextFormatted(ICON_FA_PERSON_RUNNING " %s", animName->c_str());
        gui::SameLine(gui::GetAvailableContentRegion().x - 20.0f);
        if (gui::SmallButton{ ICON_FA_XMARK })
        {
            selectedAnimationHash.reset();
            return;
        }

        gui::Separator();

        gui::Table table{ "AnimationDetailsTable", 2, true, gui::FLAG_TABLE::NO_BORDERS_IN_BODY };
        table.AddColumnHeader(ICON_FA_CUBE " Related meshes:", gui::FLAG_TABLE_COLUMN::WIDTH_FIXED, 300.0f);
        table.AddColumnHeader("Actions:");
        table.SubmitColumnHeaders();

        {
            // Find meshes with similar folder path (likely compatible)
            gui::Child relatedMeshListChild{ "RelatedMeshList", gui::Vec2{ 0.0f, 80.0f }, gui::FLAG_CHILD::BORDERS };

            // Extract folder hint from animation name
            std::string animFolder;
            if (animName)
            {
                // Try to find a common prefix pattern
                // e.g., "mc_animatedIdle" -> look for "mc_animated" meshes
                const std::string& name = *animName;
                // Find where the animation action name starts (usually uppercase or after known prefixes)
                for (size_t i = 3; i < name.length(); ++i)
                {
                    if (std::isupper(name[i]) || name[i] == '_')
                    {
                        animFolder = name.substr(0, i);
                        break;
                    }
                }
            }

            int relatedCount = 0;
            if (!animFolder.empty())
                for (const auto& [meshHash, meshRef] : ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
                {
                    const std::string* meshName{ ST<AssetManager>::Get()->Editor_GetName(meshHash) };
                    if (meshName && meshName->find(animFolder) != std::string::npos)
                    {
                        gui::Selectable(meshName->c_str(), false);
                        gui::PayloadSource{ "MESH_HASH", meshHash.get(), std::string{"Mesh: " + *meshName}.c_str(), gui::FLAG_PAYLOAD_SOURCE::ALLOW_NULL_ID};
                        ++relatedCount;
                    }
                }
            if (relatedCount == 0)
                gui::TextDisabled("No related meshes found");
        }
        table.NextColumn();

        gui::Spacing();

        gui::Button{ ICON_FA_HAND " Drag to assign", gui::Vec2{ -1.0f, 0.0f } };
        gui::PayloadSource{ "ANIMATION_HASH", hash, std::string{"Animation: " + *animName}.c_str(), gui::FLAG_PAYLOAD_SOURCE::ALLOW_NULL_ID };
    }
}