#include "BehaviourTreeWindow.h"
#include "Editor.h"
#include "AssetBrowser.h"

namespace editor {

	BehaviourTreeWindow::BehaviourTreeWindow()
		: WindowBase{ ICON_FA_ROBOT" Behaviour Tree", gui::Vec2{ 1000.0f, 768.0f } }
		, modificationsMade{ false }
	{

	}


// Generic walker for a flat BehaviorTreeAsset
template<typename F>
static void WalkBTAssetFlat(const BehaviorTreeAsset& asset, F&& fn)
{
    if (asset.nodes.empty()) {
        ImGui::TextDisabled("<no nodes>");
        return;
    }

    unsigned int prevLevel = 0;

    for (size_t i = 0; i < asset.nodes.size(); ++i) {
        const BTNodeDesc& nd = asset.nodes[i];

        // Close levels as needed
        while (prevLevel > nd.nodeLevel) {
            ImGui::TreePop();
            --prevLevel;
        }

        // Leaf detection (no child if next node isn't deeper)
        bool leaf = !(i + 1 < asset.nodes.size() &&
            asset.nodes[i + 1].nodeLevel > nd.nodeLevel);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth;
        if (leaf) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        bool open = fn(nd, i, flags);  // delegate actual UI work

        if (!leaf && open) {
            ++prevLevel;
        }
    }

    while (prevLevel > 0) {
        ImGui::TreePop();
        --prevLevel;
    }
}

    void DrawBTAssetOutline(const BehaviorTreeAsset& asset)
    {
        ImGui::SeparatorText(asset.name.c_str());

        WalkBTAssetFlat(asset, [&](const BTNodeDesc& nd, size_t, ImGuiTreeNodeFlags flags) {
            bool open = ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
                nd.nodeType.c_str(), nd.nodeLevel);
            return open;
            });
    }


    static void DrawSelectableTree(const BehaviorTreeAsset& asset, int& selectedIndex)
    {
        WalkBTAssetFlat(asset, [&](const BTNodeDesc& nd, size_t idx, ImGuiTreeNodeFlags flags) {
            if ((int)idx == selectedIndex) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            bool open = ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
                nd.nodeType.c_str(), nd.nodeLevel);

            if (ImGui::IsItemClicked()) {
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

        if (currentIndex < 0 || currentIndex >= (int)files.size()) {
            ImGui::TextDisabled("<no file selected>");
            return;
        }

        // Keep an editable buffer per selected item
        static int  bufForIndex = -1;
        static char nameBuf[256] = { 0 };

        if (bufForIndex != currentIndex) {
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
            bufForIndex = currentIndex;
        }

        ImGui::TextDisabled("Rename:");
        ImGui::SameLine();
        if (ImGui::InputTextWithHint("##bt_rename", "new_name.json", nameBuf, IM_ARRAYSIZE(nameBuf))) {
            // typing only; we don't apply here
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply")) {
            std::string proposed = SanitizeFilename(std::string(nameBuf));

            // default to .json if user removed extension
            proposed = EnsureAllowedExt(proposed, std::string(".json"));

            // Avoid no-op
            if (proposed == files[currentIndex]) {
                // nothing to do
            }
            else {
                const fs::path oldPath = fs::path(dir) / files[currentIndex];
                const fs::path newPath = fs::path(dir) / proposed;

                //check for clash
                if (fs::exists(newPath)) {
                    lastErrorOut = "A file with that name already exists.";
                    ImGui::OpenPopup("BT Rename Error");
                }
                else {
                    //======================================== ADD IN ECS LOGIC HERE ======================

                    //when file name change ADD IN THE CODE TO UPDATE ALL CURRENT COMPONENT THAT USES THIS FILE NAME

                    //======================================== endADD IN ECS LOGIC HERE ======================

                    std::error_code ec;
                    fs::rename(oldPath, newPath, ec);
                    if (ec) {
                        lastErrorOut = ec.message();
                        ImGui::OpenPopup("BT Rename Error");
                    }
                    else {
                        // Update in-memory list and buffer
                        files[currentIndex] = proposed;
                        std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
                        bufForIndex = currentIndex;
                    }
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            std::snprintf(nameBuf, sizeof(nameBuf), "%s", files[currentIndex].c_str());
            bufForIndex = currentIndex;
        }

        // Error popup (optional; remove if you show it elsewhere)
        if (ImGui::BeginPopupModal("BT Rename Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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

        const bool badSel = (currentIndex < 0 || currentIndex >= (int)files.size());
        if (badSel) ImGui::BeginDisabled();

        if (ImGui::Button("Save")) {
            try {
                const std::string filename = files.at(currentIndex); // throws if badSel
                if (filename.empty()) {
                    ImGui::OpenPopup("BT Save Error");
                }
                else {
                    std::filesystem::path p = std::filesystem::path(dir) / filename;
                    lastPath = p.string();

                    // keep asset.name == filename stem
                    loadedAsset.name = p.stem().string();

                    // optional: validate before saving
                    std::string why;
                    if (!ValidateLevelOrder(loadedAsset, why)) {
                        CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] invalid: " << why;
                        ImGui::OpenPopup("BT Save Error");
                    }
                    else if (SaveBTAssetToFile(lastPath, loadedAsset)) {
                        ImGui::OpenPopup("BT Save Success");
                    }
                    else {
                        ImGui::OpenPopup("BT Save Error");
                    }
                }
            }
            catch (...) {
                ImGui::OpenPopup("BT Save Error");
            }
        }

        if (badSel) ImGui::EndDisabled();

        if (ImGui::BeginPopupModal("BT Save Success", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Saved:\n%s", lastPath.c_str());
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
        if (ImGui::BeginPopupModal("BT Save Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
        if (ImGui::Button("New")) {
            ImGui::OpenPopup("New Behaviour Tree");
        }

        // Creation modal
        if (ImGui::BeginPopupModal("New Behaviour Tree", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char newNameBuf[128] = "";
            ImGui::Text("Enter file name (no extension):");
            ImGui::InputText("##bt_new_name", newNameBuf, IM_ARRAYSIZE(newNameBuf));

            ImGui::Spacing();
            ImGui::Separator();

            const bool canCreate = (newNameBuf[0] != '\0');

            // --- Create ---
            if (!canCreate) ImGui::BeginDisabled();
            if (ImGui::Button("Create", ImVec2(120, 0))) {
                std::string createdPath;
                std::string stem = newNameBuf;                    // user input, e.g. "MyTree"

                // Create file on disk (your helper should add .json, sanitize, and write a minimal asset)
                if (CreateNewBTFile(dir, stem, createdPath)) {
                    // Refresh the list (also maintains/auto-loads selection per your helper)
                    RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

                    // Select the newly created file & load it explicitly
                    const std::string target = std::filesystem::path(createdPath).filename().string();
                    auto it = std::find(files.begin(), files.end(), target);
                    if (it != files.end()) {
                        currentIndex = static_cast<int>(std::distance(files.begin(), it));

                        const std::string full = (std::filesystem::path(dir) / files[currentIndex]).string();
                        BehaviorTreeAsset tmp;
                        if (LoadBTAssetFromFile(full, tmp)) {
                            loadedAsset = std::move(tmp);
                            hasAsset = true;
                            lastLoadedPath = full;
                        }
                        else {
                            hasAsset = false;
                        }
                    }

                    // Reset buffer & close
                    newNameBuf[0] = '\0';
                    ImGui::CloseCurrentPopup();
                }
                else {
                    ImGui::OpenPopup("BT Create Error");
                }
            }
            if (!canCreate) ImGui::EndDisabled();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            // Error popup (nested)
            if (ImGui::BeginPopupModal("BT Create Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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
        if (currentIndex < 0 || currentIndex >= (int)files.size()) {
            ImGui::BeginDisabled();
            ImGui::Button("Delete");
            ImGui::EndDisabled();
            return;
        }

        if (ImGui::Button("Delete")) {
            ImGui::OpenPopup("Confirm Delete BT File");
        }

        if (ImGui::BeginPopupModal("Confirm Delete BT File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            const std::string target = files[currentIndex];
            ImGui::Text("Are you sure you want to delete:\n%s ?", target.c_str());

            ImGui::Spacing();

            if (ImGui::Button("Yes", ImVec2(120, 0))) {
                std::string deletedPath;
                if (DeleteBTFile(dir, files, currentIndex, deletedPath)) {
                    // Refresh file list after deletion
                    RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

                    hasAsset = false;
                    lastLoadedPath.clear();
                    loadedAsset = BehaviorTreeAsset{};
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    //void BehaviourTreeWindow::DrawWindow()
    //{

    //    // ===== persistent UI state =====
    //    static std::string dir;                        // BT folder
    //    static std::vector<std::string> files;              // file names (no path)
    //    static int currentIndex = -1;             // index into files
    //    static BehaviorTreeAsset loadedAsset;     // currently loaded asset
    //    static bool hasAsset = false;
    //    static std::string lastLoadedPath;             // for error popup
    //    static int selectedNodeIndex;             // selection inside loadedAsset
    //    static int nodeTypeComboIdx = 0;          // "Add Child" combo index
    //    
    //    // ===== small helpers kept local to this function =====
    //    auto loadAssetFromSelected = [&]() -> bool {
    //        if (currentIndex < 0 || currentIndex >= (int)files.size())
    //            return false;

    //        const std::string full = dir + "/" + files[currentIndex];

    //        BehaviorTreeAsset tmp;
    //        if (!LoadBTAssetFromFile(full, tmp)) {
    //            hasAsset = false;
    //            lastLoadedPath = full;
    //            selectedNodeIndex = -1;
    //            return false;
    //        }

    //        loadedAsset =tmp;
    //        hasAsset = true;
    //        lastLoadedPath = full;

    //        // pick a reasonable default selection:
    //        // prefer the first node at level 0, else just index 0, else none
    //        if (!loadedAsset.nodes.empty()) {
    //            int idx = 0;
    //            for (int i = 0; i < (int)loadedAsset.nodes.size(); ++i) {
    //                if (loadedAsset.nodes[i].nodeLevel == 0) { idx = i; break; }
    //            }
    //            selectedNodeIndex = idx;
    //        }
    //        else {
    //            selectedNodeIndex = -1;
    //        }

    //        return true;
    //        };

    //    auto refreshList = [&]() {
    //        files.clear();
    //        std::error_code ec;
    //        if (dir.empty()) return;
    //        if (!std::filesystem::exists(dir, ec)) return;
    //        for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
    //            if (!e.is_regular_file(ec)) continue;
    //            const auto ext = e.path().extension().string();
    //            if (ext == ".json" || ext == ".bht")
    //                files.emplace_back(e.path().filename().string());
    //        }
    //        std::sort(files.begin(), files.end());
    //        // fix selection
    //        if (currentIndex < 0 || currentIndex >= (int)files.size())
    //            currentIndex = files.empty() ? -1 : 0;
    //        };

    //    // ===== initialize dir once =====
    //    if (dir.empty()) {
    //        if (auto* fp = ST<Filepaths>::Get()) dir = fp->behaviourTreeSave;
    //        else                                 dir = "Assets/BehaviourTrees";
    //        refreshList();
    //        (void)loadAssetFromSelected(); // auto-load first if any
    //    }

    //    // ===== top bar =====
    //    ImGui::Text("Folder: %s", dir.c_str());
    //    ImGui::SameLine();
    //    if (ImGui::Button("Reload List")) {
    //        refreshList();
    //        if (!loadAssetFromSelected()) ImGui::OpenPopup("BT Load Error");
    //    }

    //    // file combo (auto-load on change)
    //    ImGui::SameLine();
    //    ImGui::TextDisabled("  File:");
    //    ImGui::SameLine();
    //    if (files.empty()) {
    //        ImGui::TextDisabled("<no .json/.bht files>");
    //    }
    //    else {
    //        const char* preview = files[currentIndex].c_str();
    //        if (ImGui::BeginCombo("##bt_file_combo", preview, ImGuiComboFlags_WidthFitPreview)) {
    //            for (int i = 0; i < (int)files.size(); ++i) {
    //                bool sel = (i == currentIndex);
    //                if (ImGui::Selectable(files[i].c_str(), sel)) {
    //                    currentIndex = i;
    //                    if (!loadAssetFromSelected()) ImGui::OpenPopup("BT Load Error");
    //                }
    //                if (sel) ImGui::SetItemDefaultFocus();
    //            }
    //            ImGui::EndCombo();
    //        }
    //    }

    //    // error popup
    //    if (ImGui::BeginPopupModal("BT Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    //        ImGui::Text("Failed to load:\n%s", lastLoadedPath.c_str());
    //        ImGui::Spacing();
    //        if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
    //        ImGui::EndPopup();
    //    }

    //    //FILE SAVE AND DELETE
    //    ImGui::SameLine();
    //    DrawCreateBTFileUI(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
    //    ImGui::SameLine();
    //    DrawDeleteBTFileUI(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);

    //    ImGui::Separator();


    //    //FILE RENAMING
    //    DrawBTRenameUI(dir, files, currentIndex, lastLoadedPath);

    //    //SAVE CURR FILE BUTTON
    //    DrawBTSaveUI(dir, files, currentIndex, loadedAsset, lastLoadedPath, hasAsset);

    //    
    //    // ===== editor split: left outline, right palette/inspector =====
    //    ImGui::BeginChild("##bt_editor_root", ImVec2(0, 0), true);
    //    ImGui::Columns(2, nullptr, true);

    //    // LEFT: hierarchy
    //    ImGui::TextDisabled("Hierarchy");
    //    ImGui::BeginChild("##bt_hierarchy", ImVec2(0, 0), true);
    //    if (!hasAsset || loadedAsset.nodes.empty()) {
    //        ImGui::TextDisabled("<nothing loaded>");
    //    }
    //    else {
    //        for (int i = 0; i < (int)loadedAsset.nodes.size(); ++i) {
    //            const auto& nd = loadedAsset.nodes[i];

    //            // indent by level
    //            ImGui::Indent(nd.nodeLevel * 20.0f);

    //            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
    //                | ImGuiTreeNodeFlags_NoTreePushOnOpen
    //                | ImGuiTreeNodeFlags_SpanAvailWidth;
    //            if (i == selectedNodeIndex) flags |= ImGuiTreeNodeFlags_Selected;

    //            ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s (level %u)", nd.nodeType.c_str(), nd.nodeLevel);
    //            if (ImGui::IsItemClicked()) selectedNodeIndex = i;

    //            // unindent back
    //            ImGui::Unindent(nd.nodeLevel * 20.0f);
    //        }
    //    }
    //    ImGui::EndChild();

    //    ImGui::NextColumn();

    //    // ------------------------------------
    //    // RIGHT: node palette + inspector
    //    // ------------------------------------
    //    ImGui::TextDisabled("Nodes Palette");
    //    ImGui::BeginChild("##bt_palette", ImVec2(0, 160), true);
    //    auto regTypes = ST<BTFactory>::Get()->RegisteredTypes();
    //    std::sort(regTypes.begin(), regTypes.end());
    //    if (regTypes.empty()) {
    //        ImGui::TextDisabled("<no registered node types>");
    //    }
    //    else {
    //        for (const auto& t : regTypes) {
    //            ImGui::BulletText("%s", t.c_str());
    //        }
    //    }
    //    ImGui::EndChild();

    //    ImGui::Separator();
    //    ImGui::TextDisabled("Inspector");

    //    // Guard selection
    //    if (!hasAsset || selectedNodeIndex < 0 || selectedNodeIndex >= (int)loadedAsset.nodes.size()) {
    //        ImGui::TextDisabled("<select a node>");
    //    }
    //    else {
    //        auto& nodes = loadedAsset.nodes;
    //        const int parentIdx = selectedNodeIndex;
    //        const unsigned pLevel = nodes[parentIdx].nodeLevel;

    //        ImGui::Text("Type:  %s", nodes[parentIdx].nodeType.c_str());
    //        ImGui::Text("Level: %u", nodes[parentIdx].nodeLevel);

    //        // ---------- level-based helpers ----------
    //        auto subtreeEnd = [&](int startIdx) -> int {
    //            const unsigned base = nodes[startIdx].nodeLevel;
    //            int i = startIdx + 1;
    //            while (i < (int)nodes.size() && nodes[i].nodeLevel > base) ++i;
    //            return i; // one past end of subtree
    //            };

    //        auto childrenRange = [&](int pIdx) -> std::pair<int, int> {
    //            // direct children are in [first, end), but only those with level = pLevel+1 are *direct*
    //            int first = pIdx + 1;
    //            int end = subtreeEnd(pIdx);
    //            return { first, end };
    //            };

    //        auto listDirectChildren = [&](int pIdx, std::vector<int>& out) {
    //            out.clear();
    //            auto [first, end] = childrenRange(pIdx);
    //            const unsigned want = nodes[pIdx].nodeLevel + 1;
    //            for (int i = first; i < end; ++i) {
    //                if (nodes[i].nodeLevel == want) out.push_back(i);
    //            }
    //            };

    //        auto adjustSubtreeLevel = [&](int startIdx, int delta) {
    //            int end = subtreeEnd(startIdx);
    //            for (int i = startIdx; i < end; ++i) {
    //                int nl = (int)nodes[i].nodeLevel + delta;
    //                if (nl < 0) nl = 0;
    //                nodes[i].nodeLevel = (unsigned)nl;
    //            }
    //            };

    //        auto moveSubtreeBlock = [&](int startA, int startB) {
    //            // Move subtree block A to just before subtree block B by rotating ranges.
    //            // Precondition: startA < startB.
    //            const int endA = subtreeEnd(startA);
    //            const int endB = subtreeEnd(startB);
    //            std::rotate(nodes.begin() + startA, nodes.begin() + startB, nodes.begin() + endB);
    //            };

    //        // ---------- Children UI ----------
    //        ImGui::SeparatorText("Children");

    //        std::vector<int> childIdxs;
    //        listDirectChildren(parentIdx, childIdxs);

    //        if (childIdxs.empty()) {
    //            ImGui::TextDisabled("<no children>");
    //        }
    //        else {
    //            for (int c = 0; c < (int)childIdxs.size(); ++c) {
    //                const int ci = childIdxs[c];
    //                ImGui::BulletText("%d. %s (level %u)", c + 1, nodes[ci].nodeType.c_str(), nodes[ci].nodeLevel);
    //                ImGui::SameLine();

    //                ImGui::PushID(ci);

    //                // Up: swap this child subtree with previous sibling subtree
    //                if (ImGui::SmallButton("Up") && c > 0) {
    //                    int prevStart = childIdxs[c - 1];
    //                    int thisStart = childIdxs[c];
    //                    if (prevStart > thisStart) std::swap(prevStart, thisStart);
    //                    moveSubtreeBlock(prevStart, thisStart);
    //                }
    //                ImGui::SameLine();

    //                // Down: swap with next sibling subtree
    //                if (ImGui::SmallButton("Down") && c + 1 < (int)childIdxs.size()) {
    //                    int thisStart = childIdxs[c];
    //                    int nextStart = childIdxs[c + 1];
    //                    if (thisStart > nextStart) std::swap(thisStart, nextStart);
    //                    moveSubtreeBlock(thisStart, nextStart);
    //                }
    //                ImGui::SameLine();

    //                // Detach: promote whole subtree by one level (becomes sibling of parent)
    //                if (ImGui::SmallButton("Detach")) {
    //                    if (nodes[ci].nodeLevel > 0) {
    //                        adjustSubtreeLevel(ci, -1);
    //                    }
    //                }

    //                ImGui::PopID();
    //            }
    //        }

    //        // ---------- Add Child ----------
    //        ImGui::SeparatorText("Add Child");

    //        // Get the registered node types once per frame
    //        std::vector<std::string> regTypes = ST<BTFactory>::Get()->RegisteredTypes();
    //        std::sort(regTypes.begin(), regTypes.end());

    //        if (regTypes.empty()) {
    //            ImGui::TextDisabled("<no registered node types>");
    //        }
    //        else {
    //            // A compact list: [TypeName]  [Add]
    //            for (int i = 0; i < (int)regTypes.size(); ++i) {
    //                ImGui::PushID(i); // ensure unique IDs per row

    //                ImGui::TextUnformatted(regTypes[i].c_str());
    //                ImGui::SameLine();

    //                if (ImGui::SmallButton("Add")) {
    //                    // Build the new node right under this parent
    //                    BTNodeDesc newNd;
    //                    newNd.nodeType = regTypes[i];
    //                    newNd.nodeLevel = pLevel + 1;

    //                    // Insert after the last direct child subtree (or right after the parent if no children)
    //                    int insertPos = parentIdx + 1;
    //                    {
    //                        std::vector<int> directChildren;
    //                        listDirectChildren(parentIdx, directChildren);     // fills indices of direct children (same level+1)
    //                        if (!directChildren.empty()) {
    //                            const int lastChildStart = directChildren.back();
    //                            insertPos = subtreeEnd(lastChildStart);         // position *after* last child's subtree
    //                        }
    //                    }

    //                    nodes.insert(nodes.begin() + insertPos, newNd);

    //                    // pick the newly inserted node in the UI
    //                    selectedNodeIndex = insertPos;
    //                }

    //                ImGui::PopID();
    //            }
    //        }

    //    }

    //    ImGui::Columns(1);
    //    ImGui::EndChild();
    //}

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
        if (ImGui::Button("Reload List")) {
            RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
            if (!LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex))
                ImGui::OpenPopup("BT Load Error");
        }

        // File combo (auto-load)
        ImGui::SameLine();
        ImGui::TextDisabled("  File:");
        ImGui::SameLine();

        if (files.empty()) {
            ImGui::TextDisabled("<no .json/.bht files>");
        }
        else {
            const char* preview = files[currentIndex].c_str();
            if (ImGui::BeginCombo("##bt_file_combo", preview, ImGuiComboFlags_WidthFitPreview)) {
                for (int i = 0; i < (int)files.size(); ++i) {
                    bool sel = (i == currentIndex);
                    if (ImGui::Selectable(files[i].c_str(), sel)) {
                        currentIndex = i;
                        if (!LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex))
                            ImGui::OpenPopup("BT Load Error");
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // Load error popup
        if (ImGui::BeginPopupModal("BT Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
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

        if (!hasAsset || loadedAsset.nodes.empty()) {
            ImGui::TextDisabled("<nothing loaded>");
        }
        else {
            for (int i = 0; i < (int)loadedAsset.nodes.size(); ++i) {
                const auto& nd = loadedAsset.nodes[i];

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
        if (regTypes.empty()) {
            ImGui::TextDisabled("<no registered node types>");
        }
        else {
            for (const auto& t : regTypes) {
                ImGui::BulletText("%s", t.c_str());
            }
        }
        ImGui::EndChild();
    }

    void DrawInspectorPanel(BehaviorTreeAsset& loadedAsset,
        bool hasAsset,
        int& selectedNodeIndex)
    {
        ImGui::Separator();
        ImGui::TextDisabled("Inspector");

        if (!hasAsset || selectedNodeIndex < 0 || selectedNodeIndex >= (int)loadedAsset.nodes.size()) {
            ImGui::TextDisabled("<select a node>");
            return;
        }

        auto& nodes = loadedAsset.nodes;
        const int parentIdx = selectedNodeIndex;
        const unsigned pLevel = nodes[parentIdx].nodeLevel;

        ImGui::Text("Type:  %s", nodes[parentIdx].nodeType.c_str());
        ImGui::Text("Level: %u", nodes[parentIdx].nodeLevel);

        // helpers
        auto subtreeEnd = [&](int startIdx) -> int {
            const unsigned base = nodes[startIdx].nodeLevel;
            int i = startIdx + 1;
            while (i < (int)nodes.size() && nodes[i].nodeLevel > base) ++i;
            return i;
            };
        auto childrenRange = [&](int pIdx) -> std::pair<int, int> {
            int first = pIdx + 1;
            int end = subtreeEnd(pIdx);
            return { first, end };
            };
        auto listDirectChildren = [&](int pIdx, std::vector<int>& out) {
            out.clear();
            auto [first, end] = childrenRange(pIdx);
            const unsigned want = nodes[pIdx].nodeLevel + 1;
            for (int i = first; i < end; ++i)
                if (nodes[i].nodeLevel == want) out.push_back(i);
            };
        auto adjustSubtreeLevel = [&](int startIdx, int delta) {
            int end = subtreeEnd(startIdx);
            for (int i = startIdx; i < end; ++i) {
                int nl = (int)nodes[i].nodeLevel + delta;
                if (nl < 0) nl = 0;
                nodes[i].nodeLevel = (unsigned)nl;
            }
            };
        auto moveSubtreeBlock = [&](int startA, int startB) {
            // pre: startA < startB
            const int endA = subtreeEnd(startA);
            const int endB = subtreeEnd(startB);
            std::rotate(nodes.begin() + startA, nodes.begin() + startB, nodes.begin() + endB);
            };

        // Children list
        ImGui::SeparatorText("Children");
        std::vector<int> childIdxs;
        listDirectChildren(parentIdx, childIdxs);

        if (childIdxs.empty()) {
            ImGui::TextDisabled("<no children>");
        }
        else {
            for (int c = 0; c < (int)childIdxs.size(); ++c) {
                const int ci = childIdxs[c];
                ImGui::BulletText("%d. %s (level %u)", c + 1, nodes[ci].nodeType.c_str(), nodes[ci].nodeLevel);
                ImGui::SameLine();

                ImGui::PushID(ci);

                if (ImGui::SmallButton("Up") && c > 0) {
                    int prevStart = childIdxs[c - 1], thisStart = childIdxs[c];
                    if (prevStart > thisStart) std::swap(prevStart, thisStart);
                    moveSubtreeBlock(prevStart, thisStart);
                }
                ImGui::SameLine();

                if (ImGui::SmallButton("Down") && c + 1 < (int)childIdxs.size()) {
                    int thisStart = childIdxs[c], nextStart = childIdxs[c + 1];
                    if (thisStart > nextStart) std::swap(thisStart, nextStart);
                    moveSubtreeBlock(thisStart, nextStart);
                }
                ImGui::SameLine();

                if (ImGui::SmallButton("Detach")) {
                    if (nodes[ci].nodeLevel > 0) adjustSubtreeLevel(ci, -1);
                }

                ImGui::PopID();
            }
        }

        // Add Child (dropdown)
        ImGui::SeparatorText("Add Child");

        std::vector<std::string> regTypes = ST<BTFactory>::Get()->RegisteredTypes();
        std::sort(regTypes.begin(), regTypes.end());

        static int addTypeIdx = 0;
        if (regTypes.empty()) {
            ImGui::TextDisabled("<no registered node types>");
        }
        else {
            if (addTypeIdx >= (int)regTypes.size()) addTypeIdx = 0;
            if (ImGui::BeginCombo("Type", regTypes[addTypeIdx].c_str())) {
                for (int i = 0; i < (int)regTypes.size(); ++i) {
                    bool sel = (i == addTypeIdx);
                    if (ImGui::Selectable(regTypes[i].c_str(), sel)) addTypeIdx = i;
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::SmallButton("Add")) {
                BTNodeDesc nd;
                nd.nodeType = regTypes[addTypeIdx];
                nd.nodeLevel = pLevel + 1;

                // insert after last direct child's subtree (or right after parent if none)
                int insertPos = parentIdx + 1;
                std::vector<int> directChildren;
                listDirectChildren(parentIdx, directChildren);
                if (!directChildren.empty()) {
                    const int lastChildStart = directChildren.back();
                    insertPos = subtreeEnd(lastChildStart);
                }
                nodes.insert(nodes.begin() + insertPos, nd);
                selectedNodeIndex = insertPos;
            }
        }
    }


    void BehaviourTreeWindow::DrawWindow()
    {
        // Persistent UI state
        static std::string dir;
        static std::vector<std::string> files;
        static int currentIndex = -1;
        static BehaviorTreeAsset loadedAsset;
        static bool hasAsset = false;
        static std::string lastLoadedPath;
        static int selectedNodeIndex = -1;

        // Init dir + list + first load
        if (dir.empty()) {
            if (auto* fp = ST<Filepaths>::Get()) dir = fp->behaviourTreeSave;
            else                                 dir = "Assets/BehaviourTrees";

            RefreshBTList(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath);
            LoadSelectedBT(dir, files, currentIndex, loadedAsset, hasAsset, lastLoadedPath, selectedNodeIndex);
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
    }


}
