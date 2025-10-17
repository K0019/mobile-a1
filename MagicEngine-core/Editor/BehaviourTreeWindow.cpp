/******************************************************************************/
/*!
\file   BehaviourTreeFactory.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    This source file defines the object that creates the imgui window for the
    behavior tree editing.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Editor/BehaviourTreeWindow.h"
#include "Engine/BehaviorTree/BehaviourTree.h"
#include "Engine/BehaviorTree/BehaviourTreeFactory.h"
#include "Editor/BehaviourTreeImguiHelper.h"
#include "GameSettings.h"

namespace editor {

	BehaviourTreeWindow::BehaviourTreeWindow()
		: WindowBase{ ICON_FA_ROBOT" Behaviour Tree", gui::Vec2{ 1000.0f, 768.0f } }
		, modificationsMade{ false }
	{

	}

#ifdef IMGUI_ENABLED

// Generic walker for a flat BehaviorTreeAsset
template<typename F>
static void WalkBTAssetFlat(const BehaviorTreeAsset& asset, F&& fn)
{
    if (asset.nodes.empty()) 
    {
        gui::TextDisabled("<no nodes>");
        return;
    }

    unsigned int prevLevel{ 0 };

    for (size_t i{ 0 }; i < asset.nodes.size(); ++i)
    {
        const BTNodeDesc& nd{ asset.nodes[i] };

        // Close levels as needed
        while (prevLevel > nd.nodeLevel) 
        {
            ImGui::TreePop();
            --prevLevel;
        }

        // Leaf detection (no child if next node isn't deeper)
        bool leaf{ !(i + 1 < asset.nodes.size() &&
            asset.nodes[i + 1].nodeLevel > nd.nodeLevel) };

        ImGuiTreeNodeFlags flags{ ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth };
        if (leaf) 
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        bool open{ fn(nd, i, flags) };  // delegate actual UI work

        if (!leaf && open) 
        {
            ++prevLevel;
        }
    }

    while (prevLevel > 0) 
    {
        ImGui::TreePop();
        --prevLevel;
    }
}

    void DrawBTAssetOutline(const BehaviorTreeAsset& asset)
    {
        ImGui::SeparatorText(asset.name.c_str());

        WalkBTAssetFlat(asset, [&](const BTNodeDesc& nd, size_t, ImGuiTreeNodeFlags flags) 
        {
            bool open{ ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
                nd.nodeType.c_str(), nd.nodeLevel) };
            return open;
        });
    }


    static void DrawSelectableTree(const BehaviorTreeAsset& asset, int& selectedIndex)
    {
        WalkBTAssetFlat(asset, [&](const BTNodeDesc& nd, size_t idx, ImGuiTreeNodeFlags flags) 
        {
            if ((int)idx == selectedIndex) 
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool open{ ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
                nd.nodeType.c_str(), nd.nodeLevel) };

            if (ImGui::IsItemClicked()) 
            {
                selectedIndex = static_cast<int>(idx);
            }

            return open;
        });
    }

    // ---- outline wrapper that supports selection ----
    void DrawBTAssetOutlineSelectable(const BehaviorTreeAsset& asset, int& selectedID)
    {
        DrawSelectableTree(asset, selectedID);
    }

    void DrawBTRenameUI(const std::string& dir,
        std::vector<std::string>& files,
        int& currentIndex,
        std::string& lastErrorOut)
    {
        namespace fs = std::filesystem;

        if (currentIndex < 0 || currentIndex >= (int)files.size())
        {
            ImGui::TextDisabled("<no file selected>");
            return;
        }

        // Keep an editable buffer per selected item
        static int  bufForIndex{ -1 };
        static char nameBuf[256]{ 0 };

        if (bufForIndex != currentIndex) 
        {
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
            bufForIndex = currentIndex;
        }

        ImGui::TextDisabled("Rename:");
        ImGui::SameLine();
        if (ImGui::InputTextWithHint("##bt_rename", "new_name.json", nameBuf, IM_ARRAYSIZE(nameBuf))) 
        {
            // typing only; we don't apply here
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) 
        {
            std::string proposed{ bt::SanitizeFilename(std::string(nameBuf)) };

            // default to .json if user removed extension
            proposed = bt::EnsureAllowedExt(proposed, std::string(".json"));

            // Avoid no-op
            if (proposed == files[currentIndex]) 
            {
                // nothing to do
            }
            else 
            {
                const fs::path oldPath = fs::path(dir) / files[currentIndex];
                const fs::path newPath = fs::path(dir) / proposed;

                //check for clash
                if (fs::exists(newPath)) 
                {
                    lastErrorOut = "A file with that name already exists.";
                    ImGui::OpenPopup("BT Rename Error");
                }
                else 
                {
                    //======================================== ADD IN ECS LOGIC HERE ======================

                    //when file name change ADD IN THE CODE TO UPDATE ALL CURRENT COMPONENT THAT USES THIS FILE NAME

                    //======================================== endADD IN ECS LOGIC HERE ======================

                    std::error_code ec;
                    fs::rename(oldPath, newPath, ec);
                    if (ec) 
                    {
                        lastErrorOut = ec.message();
                        ImGui::OpenPopup("BT Rename Error");
                    }
                    else 
                    {
                        // Update in-memory list and buffer
                        files[currentIndex] = proposed;
                        std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
                        bufForIndex = currentIndex;
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) 
        {
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
            bufForIndex = currentIndex;
        }

        // Error popup (optional; remove if you show it elsewhere)
        if (ImGui::BeginPopupModal("BT Rename Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            ImGui::TextWrapped("%s", lastErrorOut.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }


    void DrawBTSaveUI(const std::string& dir,
        const std::vector<std::string>& files,
        int currentIndex,
        BehaviorTreeAsset& loadedAsset,
        std::string& lastPath,
        bool hasAsset)
    {
        if (!hasAsset) return;

        const bool badSel{ (currentIndex < 0 || currentIndex >= (int)files.size()) };
        if (badSel) ImGui::BeginDisabled();

        if (ImGui::Button("Save")) 
        {
            try 
            {
                const std::string filename{ files.at(currentIndex) }; // throws if badSel
                if (filename.empty()) 
                {
                    ImGui::OpenPopup("BT Save Error");
                }
                else 
                {
                    std::filesystem::path p{ std::filesystem::path(dir) / filename };
                    lastPath = p.string();

                    // keep asset.name == filename stem
                    loadedAsset.name = p.stem().string();

                    // optional: validate before saving
                    std::string why;
                    if (!bt::ValidateLevelOrder(loadedAsset, why)) 
                    {
                        CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] invalid: " << why;
                        ImGui::OpenPopup("BT Save Error");
                    }
                    else if (bt::SaveBTAssetToFile(lastPath, loadedAsset)) 
                    {
                        ImGui::OpenPopup("BT Save Success");
                    }
                    else 
                    {
                        ImGui::OpenPopup("BT Save Error");
                    }
                }
            }
            catch (...) 
            {
                ImGui::OpenPopup("BT Save Error");
            }
        }

        if (badSel) ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("BT Save Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            ImGui::Text("Saved:\n%s", lastPath.c_str());
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("BT Save Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            ImGui::Text("Failed to save:\n%s", lastPath.c_str());
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    void DrawCreateBTFileUI(const std::string& dir,
        std::vector<std::string>& files,
        int& currentIndex,
        BehaviorTreeAsset& loadedAsset,
        bool& hasAsset,
        std::string& lastLoadedPath)
    {
        // Launch the modal
        if (ImGui::Button("New")) 
        {
            ImGui::OpenPopup("New Behaviour Tree");
        }

        // Creation modal
        if (ImGui::BeginPopupModal("New Behaviour Tree", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            static char newNameBuf[128]{ "" };
            ImGui::Text("Enter file name (no extension):");
            ImGui::InputText("##bt_new_name", newNameBuf, IM_ARRAYSIZE(newNameBuf));

            ImGui::Spacing();
            ImGui::Separator();

            const bool canCreate = (newNameBuf[0] != '\0');

            // --- Create ---
            if (!canCreate) ImGui::BeginDisabled();
            if (ImGui::Button("Create", ImVec2(120, 0))) 
            {
                std::string createdPath;
                std::string stem = newNameBuf;                    // user input, e.g. "MyTree"

                // Create file on disk (your helper should add .json, sanitize, and write a minimal asset)
                if (bt::CreateNewBTFile(dir, stem, createdPath)) 
                {
                    // Refresh the list (also maintains/auto-loads selection per your helper)
                    bt::RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

                    // Select the newly created file & load it explicitly
                    const std::string target{ std::filesystem::path(createdPath).filename().string() };
                    auto it{ std::find(files.begin(), files.end(), target) };
                    if (it != files.end()) 
                    {
                        currentIndex = static_cast<int>(std::distance(files.begin(), it));

                        const std::string full{ (std::filesystem::path(dir) / files[currentIndex]).string() };
                        BehaviorTreeAsset tmp;
                        if (bt::LoadBTAssetFromFile(full, &tmp)) 
                        {
                            loadedAsset = std::move(tmp);
                            hasAsset = true;
                            lastLoadedPath = full;
                        }
                        else 
                        {
                            hasAsset = false;
                        }
                    }

                    // Reset buffer & close
                    newNameBuf[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
                else 
                {
                    ImGui::OpenPopup("BT Create Error");
                }
            }
            if (!canCreate) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) 
            {
                ImGui::CloseCurrentPopup();
            }

            // Error popup (nested)
            if (ImGui::BeginPopupModal("BT Create Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
            {
                ImGui::Text("Failed to create the file.\nCheck write permissions and the filename.");
                if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            ImGui::EndPopup();
        }
    }

    void DrawDeleteBTFileUI(const std::string& dir,
        std::vector<std::string>& files,
        int& currentIndex,
        BehaviorTreeAsset& loadedAsset,
        bool& hasAsset,
        std::string& lastLoadedPath)
    {
        if (currentIndex < 0 || currentIndex >= (int)files.size()) 
        {
            ImGui::BeginDisabled();
            ImGui::Button("Delete");
            ImGui::EndDisabled();
            return;
        }

        if (ImGui::Button("Delete")) 
        {
            ImGui::OpenPopup("Confirm Delete BT File");
        }

        if (ImGui::BeginPopupModal("Confirm Delete BT File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            const std::string target{ files[currentIndex] };
            ImGui::Text("Are you sure you want to delete:\n%s ?", target.c_str());

            ImGui::Spacing();

            if (ImGui::Button("Yes", ImVec2(120, 0))) 
            {
                std::string deletedPath;
                if (bt::DeleteBTFile(dir, files, currentIndex, deletedPath)) 
                {
                    // Refresh file list after deletion
                    bt::RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

                    hasAsset = false;
                    lastLoadedPath.clear();
                    loadedAsset = BehaviorTreeAsset{};
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0))) 
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    void DrawTopBar(std::string& dir,
        std::vector<std::string>& files,
        int& currentIndex,
        BehaviorTreeAsset& loadedAsset,
        bool& hasAsset,
        std::string& lastLoadedPath,
        int& selectedNodeIndex)
    {
        ImGui::Text("Folder: %s", dir.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Reload List")) 
        {
            bt::RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
            if (!bt::LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex))
                ImGui::OpenPopup("BT Load Error");

            ST<BTFactory>::Get()->SetAllFilePath();
        }

        // File combo (auto-load)
        ImGui::SameLine();
        ImGui::TextDisabled("  File:");
        ImGui::SameLine();

        if (files.empty()) 
        {
            ImGui::TextDisabled("<no .json/.bht files>");
        }
        else 
        {
            const char* preview = files[currentIndex].c_str();
            if (ImGui::BeginCombo("##bt_file_combo", preview, ImGuiComboFlags_WidthFitPreview)) 
            {
                for (int i{ 0 }; i < (int)files.size(); ++i)
                {
                    bool sel = (i == currentIndex);
                    if (ImGui::Selectable(files[i].c_str(), sel)) 
                    {
                        currentIndex = i;
                        if (!bt::LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex))
                            ImGui::OpenPopup("BT Load Error");
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Load error popup
        if (ImGui::BeginPopupModal("BT Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) 
        {
            ImGui::Text("Failed to load:\n%s", lastLoadedPath.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        // File ops
        ImGui::SameLine();
        DrawCreateBTFileUI(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
        ImGui::SameLine();
        DrawDeleteBTFileUI(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

        ImGui::Separator();

        // Rename + Save
        DrawBTRenameUI(dir, files, currentIndex, lastLoadedPath);
        DrawBTSaveUI(dir, files, currentIndex, loadedAsset, lastLoadedPath, hasAsset);
    }

    void DrawHierarchyPanel(const BehaviorTreeAsset& loadedAsset,
        bool hasAsset,
        int& selectedNodeIndex)
    {
        ImGui::TextDisabled("Hierarchy");
        ImGui::BeginChild("##bt_hierarchy", ImVec2(0, 0), true);

        if (!hasAsset || loadedAsset.nodes.empty()) 
        {
            ImGui::TextDisabled("<nothing loaded>");
        }
        else 
        {
            for (int i{ 0 }; i < (int)loadedAsset.nodes.size(); ++i) 
            {
                const auto& nd{ loadedAsset.nodes[i] };

                ImGui::Indent(nd.nodeLevel * 20.0f);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (i == selectedNodeIndex) flags |= ImGuiTreeNodeFlags_Selected;

                ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s (level %u)", nd.nodeType.c_str(), nd.nodeLevel);
                if (ImGui::IsItemClicked()) selectedNodeIndex = i;

                ImGui::Unindent(nd.nodeLevel * 20.0f);
            }
        }

        ImGui::EndChild();
    }

    void DrawNodesPalettePanel()
    {
        ImGui::TextDisabled("Nodes Palette");
        ImGui::BeginChild("##bt_palette", ImVec2(0, 160), true);

        auto regTypes = ST<BTFactory>::Get()->RegisteredTypes();
        std::sort(regTypes.begin(), regTypes.end());
        if (regTypes.empty()) 
        {
            ImGui::TextDisabled("<no registered node types>");
        }
        else 
        {
            for (const auto& t : regTypes) 
            {
                ImGui::BulletText("%s", t.c_str());
            }
        }
        ImGui::EndChild();
    }

    void DrawDeleteCurrentNodeButton(BehaviorTreeAsset& asset, int& selectedIndex)
    {
        auto& nodes = asset.nodes;
        if (selectedIndex < 0 || selectedIndex >= (int)nodes.size()) return;

        // Only allow this for root-level nodes
        if (nodes[selectedIndex].nodeLevel != 0) return;

        const bool lastRoot = (bt::CountRootNodes(nodes) <= 1);

        if (lastRoot) ImGui::BeginDisabled();
        const bool clicked = ImGui::SmallButton("Delete This Root Node");
        if (lastRoot) 
        {
            ImGui::EndDisabled();
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Cannot delete the only root node. Add another root first.");
        }

        if (clicked) 
        {
            bt::DeleteNodeAndSubtree(nodes, selectedIndex, selectedIndex);
        }
    }

    void DrawInspectorPanel(BehaviorTreeAsset& loadedAsset,
        bool hasAsset,
        int& selectedNodeIndex)
    {
        ImGui::Separator();
        ImGui::TextDisabled("Inspector");

        if (!hasAsset || selectedNodeIndex < 0 || selectedNodeIndex >= (int)loadedAsset.nodes.size()) 
        {
            ImGui::TextDisabled("<select a node>");
            return;
        }

        auto& nodes = loadedAsset.nodes;
        const int parentIdx = selectedNodeIndex;
        const unsigned pLevel = nodes[parentIdx].nodeLevel;

        ImGui::Text("Type:  %s", nodes[parentIdx].nodeType.c_str());
        ImGui::Text("Level: %u", nodes[parentIdx].nodeLevel);

        DrawDeleteCurrentNodeButton(loadedAsset, selectedNodeIndex);


        // helpers
        auto subtreeEnd = [&](int startIdx) -> int {
            const unsigned base{ nodes[startIdx].nodeLevel };
            int i{ startIdx + 1 };
            while (i < (int)nodes.size() && nodes[i].nodeLevel > base) ++i;
            return i;
        };
        auto childrenRange = [&](int pIdx) -> std::pair<int, int> {
            int first{ pIdx + 1 };
            int end{ subtreeEnd(pIdx) };
            return { first, end };
        };
        auto listDirectChildren = [&](int pIdx, std::vector<int>& out) {
            out.clear();
            auto [first, end]{ childrenRange(pIdx) };
            const unsigned want{ nodes[pIdx].nodeLevel + 1 };
            for (int i{ first }; i < end; ++i)
                if (nodes[i].nodeLevel == want) out.push_back(i);
        };
        auto adjustSubtreeLevel = [&](int startIdx, int delta) {
            int end{ subtreeEnd(startIdx) };
            for (int i{ startIdx }; i < end; ++i) {
                int nl{ static_cast<int>(nodes[i].nodeLevel) + delta };
                if (nl < 0) nl = 0;
                nodes[i].nodeLevel = static_cast<unsigned>(nl);
            }
        };
        auto moveSubtreeBlock = [&](int startA, int startB) {
            // pre: startA < startB
            const int endB{ subtreeEnd(startB) };
            std::rotate(nodes.begin() + startA, nodes.begin() + startB, nodes.begin() + endB);
        };

        // Children list
        ImGui::SeparatorText("Children");
        std::vector<int> childIdxs;
        std::pair<bool, int> deletedCi{std::make_pair<bool, int>(false, -1)};
        listDirectChildren(parentIdx, childIdxs);

        if (childIdxs.empty()) 
        {
            ImGui::TextDisabled("<no children>");
        }
        else 
        {
            for (int c{ 0 }; c < (int)childIdxs.size(); ++c)
            {
                const int ci{ childIdxs[c] };
                ImGui::BulletText("%d. %s (level %u)", c + 1, nodes[ci].nodeType.c_str(), nodes[ci].nodeLevel);
                ImGui::SameLine();

                ImGui::PushID(ci);

                // ----- Up -----
                if (ImGui::SmallButton("Up") && c > 0) 
                {
                    int prevStart{ childIdxs[c - 1] }, thisStart{ childIdxs[c] };
                    if (prevStart > thisStart) std::swap(prevStart, thisStart);
                    moveSubtreeBlock(prevStart, thisStart);
                }
                ImGui::SameLine();

                // ----- Down -----
                if (ImGui::SmallButton("Down") && c + 1 < (int)childIdxs.size()) 
                {
                    int thisStart{ childIdxs[c] }, nextStart{ childIdxs[c + 1] };
                    if (thisStart > nextStart) std::swap(thisStart, nextStart);
                    moveSubtreeBlock(thisStart, nextStart);
                }
                ImGui::SameLine();

                // ----- Detach (prevent non-composite becoming level 0) -----
                {
                    const bool wouldBecomeRoot{ (nodes[ci].nodeLevel == 1) };
                    const bt::NODE_KIND childKind{ bt::ClassifyNodeType(nodes[ci].nodeType) };
                    const bool detachAllowed{ !wouldBecomeRoot || (childKind == bt::NODE_KIND::COMPOSITE) };

                    if (!detachAllowed) ImGui::BeginDisabled();
                    if (ImGui::SmallButton("Detach")) 
                    {
                        // Promote entire subtree by one level
                        if (nodes[ci].nodeLevel > 0) adjustSubtreeLevel(ci, -1);
                    }
                    if (!detachAllowed) 
                    {
                        ImGui::EndDisabled();
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Only Composite nodes can be at level 0.");
                    }
                }

                ImGui::SameLine();

                // ----- Delete (disallow deleting the last root) -----
                {
                    const bool isRoot{ (nodes[ci].nodeLevel == 0) };
                    const bool isLastRoot{ isRoot && (bt::CountRootNodes(nodes) <= 1) };

                    if (isLastRoot) ImGui::BeginDisabled();
                    const bool wantDelete{ ImGui::SmallButton("Delete") };
                    if (isLastRoot) 
                    {
                        ImGui::EndDisabled();
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Cannot delete the only root node.");
                    }

                    if (wantDelete) {
                        deletedCi = { true, ci };   // defer actual erase until after the loop
                    }
                }

                ImGui::PopID();
            }

            // Perform the actual deletion after the UI loop (safe for indices)
            if (deletedCi.first) 
            {
                bt::DeleteNodeAndSubtree(nodes, deletedCi.second, selectedNodeIndex);
            }

        }

        // Add Child (dropdown)
        ImGui::SeparatorText("Add Child");

        // Get the available node types once per frame
        std::vector<std::string> regTypes{ ST<BTFactory>::Get()->RegisteredTypes() };
        std::sort(regTypes.begin(), regTypes.end());

        if (regTypes.empty())
        {
            ImGui::TextDisabled("<no registered node types>");
        }
        else
        {
            // Decide if we even allow adding for this parent
            const bt::NODE_KIND parentKind{ bt::ClassifyNodeType(nodes[parentIdx].nodeType) };
            const bool canAddNow{ bt::CanParentAddChild(nodes, parentIdx) };

            if (parentKind == bt::NODE_KIND::LEAF)
            {
                ImGui::TextDisabled("This is a Leaf node. It cannot have children.");
            }
            else if (parentKind == bt::NODE_KIND::DECORATOR && !canAddNow)
            {
                ImGui::TextDisabled("Decorator already has 1 child.");
            }

            // Node type dropdown (works for Composite and Decorator when allowed)
            static int nodeTypeComboIdx{ 0 };
            if (nodeTypeComboIdx >= (int)regTypes.size()) nodeTypeComboIdx = 0;

            if (ImGui::BeginCombo("Type", regTypes[nodeTypeComboIdx].c_str()))
            {
                for (int i{ 0 }; i < (int)regTypes.size(); ++i)
                {
                    const bool sel{ (i == nodeTypeComboIdx) };
                    if (ImGui::Selectable(regTypes[i].c_str(), sel)) nodeTypeComboIdx = i;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Disable the button if parent can't accept a child (leaf or full decorator)
            if (!canAddNow) ImGui::BeginDisabled();

            if (ImGui::Button("Add Child"))
            {
                BTNodeDesc newNd;
                newNd.nodeType = regTypes[nodeTypeComboIdx];
                newNd.nodeLevel = pLevel + 1;

                // Insert after the last existing direct child subtree (or right after parent if none)
                int insertPos{ parentIdx + 1 };
                {
                    std::vector<int> directChildren;
                    bt::ListDirectChildren(nodes, parentIdx, directChildren);
                    if (!directChildren.empty())
                    {
                        const int lastChildStart{ directChildren.back() };
                        insertPos = bt::SubtreeEnd(nodes, lastChildStart);
                    }
                }

                nodes.insert(nodes.begin() + insertPos, newNd);
                selectedNodeIndex = insertPos; // select the newly added node
            }

            if (!canAddNow) ImGui::EndDisabled();
        }
        
    }

#endif

    void BehaviourTreeWindow::DrawWindow()
    {
#ifdef IMGUI_ENABLED
        // Persistent UI state
        static std::string dir;
        static std::vector<std::string> files;
        static int currentIndex{ -1 };
        static BehaviorTreeAsset loadedAsset;
        static bool hasAsset{ false };
        static std::string lastLoadedPath;
        static int selectedNodeIndex{ -1 };

        // Init dir + list + first load
        if (dir.empty()) 
        {
            if (auto* fp = ST<Filepaths>::Get()) dir = fp->behaviourTreeSave;
            else                                 dir = "Assets/BehaviourTrees";

            bt::RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
            bt::LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex);
        }

        // Top bar
        DrawTopBar(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex);

        // Split: left hierarchy / right palette + inspector
        ImGui::BeginChild("##bt_editor_root", ImVec2(0, 0), true);
        ImGui::Columns(2, nullptr, true);

        DrawHierarchyPanel(loadedAsset, hasAsset, selectedNodeIndex);

        ImGui::NextColumn();
        DrawNodesPalettePanel();
        DrawInspectorPanel(loadedAsset, hasAsset, selectedNodeIndex);

        ImGui::Columns(1);
        ImGui::EndChild();
#endif
    }


}
