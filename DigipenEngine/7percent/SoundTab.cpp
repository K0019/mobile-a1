#include "SoundTab.h"


const char* SoundTab::GetName() const
{
    return "Sounds";
}

const char* SoundTab::GetIdentifier() const
{
    return ICON_FA_MICROPHONE" Sounds";
}

void SoundTab::Render()
{
    ImGui::BeginChild("SoundTable", ImVec2(0.0f, 0.0f), true);

    std::set<std::string> singleSoundNames = ST<AudioManager>::Get()->GetSingleSoundNames();
    std::map<std::string, std::set<std::string>> groupedSoundNames = ST<AudioManager>::Get()->GetGroupedSoundNames();

    ImGui::Columns(2, nullptr, true);

    // Left column for single sounds
    ImGui::Text("Single Sounds");

    // Iterate all single sound names
    for (std::string const& name : singleSoundNames)
    {
        // Create Button
        if (ImGui::Selectable(name.c_str()))
        {
            if (ST<AudioManager>::Get()->IsSoundPlaying(name))
            {
                ST<AudioManager>::Get()->StopSound(name);
            }
            else
            {
                ST<AudioManager>::Get()->StartSingleSound(name, false);
            }
        }

        // Drag-drop source
        gui::PayloadSource("SOUND", name, name.c_str());

        // Context menu
        RenderSoundContextMenu(name, false);
    }

    // Right column for grouped sounds
    ImGui::NextColumn();
    ImGui::Text("Grouped Sounds");
    ImGui::Separator();

    // Iterate all grouped sound names
    for (std::pair<std::string, std::set<std::string>> const& group : groupedSoundNames)
    {
        if (ImGui::TreeNode(group.first.c_str()))
        {
            ImGui::Indent(55.0f);
            for (std::string const& name : group.second)
            {
                // Create Button
                if (ImGui::Selectable(name.c_str()))
                {
                    if (ST<AudioManager>::Get()->IsSoundPlaying(name))
                    {
                        ST<AudioManager>::Get()->StopSound(name);
                    }
                    else
                    {
                        ST<AudioManager>::Get()->StartSpecificGroupedSound(name, false);
                    }
                }

                // Single sound drag-drop source
                gui::PayloadSource("SOUND", name, name.c_str());

                // Context menu
                RenderSoundContextMenu(name, true);
            }
            ImGui::Unindent(55.0f);
            ImGui::TreePop();
        }

        // Grouped sound drag-drop source
        gui::PayloadSource("SOUND", group.first, group.first.c_str());
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void SoundTab::RenderSoundContextMenu(std::string const& name, bool isGrouped)
{
    if (ImGui::BeginPopupContextItem(("Delete##" + name).c_str()))
    {
        if (ImGui::MenuItem("Delete"))
        {
            ST<AudioManager>::Get()->DeleteSound(name, isGrouped);
        }
        ImGui::EndPopup();
    }
}
