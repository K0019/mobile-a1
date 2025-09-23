#include "BehaviourTreeWindow.h"
#include "Editor.h"
#include "AssetBrowser.h"

namespace editor {

	BehaviourTreeWindow::BehaviourTreeWindow()
		: WindowBase{ ICON_FA_ROBOT" Behaviour Tree", gui::Vec2{ 1000.0f, 768.0f } }
		, modificationsMade{ false }
	{

	}

    //static BehaviourTreeEditorModel  gModel;
    //static NodesRegistered           gPalette;
    //static std::string               gDir = ST<Filepaths>::Get() ? ST<Filepaths>::Get()->behaviourTreeSave : "Assets/BehaviorTrees";
    //static std::string               gFileSelected = "";
    //static std::string               gNewFileName = "NewTree.bht";
    //static std::vector<std::string>  gFiles;

    //// helper to rebuild listing
    //static void RefreshFileList() {
    //    gFiles = ScanDirForTrees(gDir);
    //    std::sort(gFiles.begin(), gFiles.end());
    //    if (!gFiles.empty() && gFileSelected.empty())
    //        gFileSelected = gFiles.front();
    //}


    //void BehaviourTreeWindow::DrawWindow() {
    //    // one-time refreshes
    //    static bool first = true;
    //    if (first) {
    //        first = false;
    //        gPalette.Refresh();
    //        RefreshFileList();
    //    }

    //    // --- FILE THINGS ---------------------------------------------------------

    //    //LOADING
    //    ImGui::TextDisabled("Folder: %s", gDir.c_str());
    //    if (ImGui::BeginCombo("Open", gFileSelected.empty() ? "<choose file>" : gFileSelected.c_str())) {
    //        for (auto& f : gFiles) {
    //            bool sel = (f == gFileSelected);
    //            if (ImGui::Selectable(f.c_str(), sel)) gFileSelected = f;
    //            if (sel) ImGui::SetItemDefaultFocus();
    //        }
    //        ImGui::EndCombo();
    //    }
    //    ImGui::SameLine();
    //    if (ImGui::Button("Reload List")) RefreshFileList();

    //    ImGui::SameLine();
    //    if (ImGui::Button("Load")) {
    //        if (!gFileSelected.empty()) {
    //            BehaviorTreeAsset asset;
    //            std::string full = gDir + "/" + gFileSelected;
    //            if (LoadAssetFromFile(full, asset)) {
    //                FillEditorFromAsset(gModel, asset);
    //            }
    //            else {
    //                ImGui::OpenPopup("BT Load Error");
    //            }
    //        }
    //    }
    //    if (ImGui::BeginPopupModal("BT Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    //        ImGui::Text("Failed to load selected file.");
    //        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
    //        ImGui::EndPopup();
    //    }

    //    ImGui::Separator();

    //    //SAVING AND CREATING
    //    ImGui::InputText("New file name", gNewFileName.data(), gNewFileName.size() + 1);
    //    ImGui::SameLine();
    //    if (ImGui::Button("New/Reset")) {
    //        gModel = {};
    //        gModel.rootId = gModel.addNode("ComSelector", "Root");
    //        gModel.selectedId = gModel.rootId;
    //    }
    //    ImGui::SameLine();
    //    if (ImGui::Button("Save")) {
    //        std::string file = gFileSelected.empty() ? gNewFileName : gFileSelected;
    //        if (file.empty()) file = "Untitled.json";
    //        auto asset = MakeAssetFromEditor(gModel, /*assetName*/ file);
    //        std::string full = gDir + "/" + file;
    //        if (SaveAssetToFile(full, asset)) {
    //            gFileSelected = file;
    //            RefreshFileList();
    //        }
    //        else {
    //            ImGui::OpenPopup("BT Save Error");
    //        }
    //    }
    //    if (ImGui::BeginPopupModal("BT Save Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    //        ImGui::Text("Failed to save file.");
    //        if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();
    //        ImGui::EndPopup();
    //    }

    //    ImGui::SameLine();
    //    if (ImGui::Button("Save As")) {
    //        if (!gNewFileName.empty()) {
    //            auto asset = MakeAssetFromEditor(gModel, gNewFileName);
    //            std::string full = gDir + "/" + gNewFileName;
    //            if (SaveAssetToFile(full, asset)) {
    //                gFileSelected = gNewFileName;
    //                RefreshFileList();
    //            }
    //            else {
    //                ImGui::OpenPopup("BT Save Error");
    //            }
    //        }
    //    }

    //    ImGui::Separator();
    //    // ---END  FILE THINGS ---------------------------------------------------------

    //    // ---NODE STUFF ---------------------------------------------------------
    //    ImGui::TextDisabled("Node Things");
    //    ImGui::SameLine();
    //    if (ImGui::SmallButton("Refresh Types")) {
    //        gPalette.Refresh();
    //    }

    //    static int paletteIdx = 0;
    //    if (!gPalette.types.empty()) {
    //        if (paletteIdx >= (int)gPalette.types.size()) paletteIdx = 0;
    //        ImGui::SetNextItemWidth(250);
    //        ImGui::Combo("Type", &paletteIdx, [](void* data, int idx, const char** out_text) {
    //            auto* v = (std::vector<std::string>*)data;
    //            *out_text = (*v)[idx].c_str(); return true;
    //            }, (void*)&gPalette.types, (int)gPalette.types.size());

    //        ImGui::SameLine();
    //        if (ImGui::Button("Add Node")) {
    //            std::string type = gPalette.types[paletteIdx];
    //            int id = gModel.addNode(type, type);
    //            if (gModel.rootId < 0) gModel.rootId = id;
    //            gModel.selectedId = id;
    //        }
    //    }
    //    else {
    //        ImGui::TextDisabled("<no registered types>");
    //    }

    //    ImGui::Separator();
    //    // ---END NODE STUFF ---------------------------------------------------------

    //    // --- OUTLINE & INSPECTOR (adapt your existing UI) ---------------------
    //    // Reuse your existing tree outline + inspector, swapping
    //    // ids/kinds/names/childIds -> gModel.nodes[i].id/type/displayName/children
    //    // On �Add Child� for a parent P: create node with palette type and push child id to P.children
    //    // On reparent drag-drop: use gModel.canAdopt and then move child id between parent lists
    //    // On delete: gModel.eraseSubtree(doomed)
    //    // Display type as `nodes[i].type`, editable display as `nodes[i].displayName`
    //    // (Omitted here to keep this message focused.)
    //    // ----------------------------------------------------------------------
    //    ImGui::Separator();
    //    // --- BUILD & RUN PREVIEW ----------------------------------------------
    //    if (ImGui::Button("Build & Push To BTSystem")) {
    //        auto asset = MakeAssetFromEditor(gModel, gFileSelected.empty() ? "EditorPreview" : gFileSelected);
    //        // Option A: temporary tree
    //        // BehaviorTree temp; temp.InitFromAsset(asset); // tick it somewhere

    //        // Option B: push into your system so Engine updates it:
    //        ST<BehaviorTreeSystem>::Get()->CreateFromAsset(asset);
    //    }

    //}

    //void BehaviourTreeWindow::DrawWindow() {

    //    enum NodeKind { NK_Selector, NK_Sequence, NK_ParallelAny, NK_Inverter, NK_Succeeder, NK_Repeater, NK_Timer, NK_Condition, NK_Action };
    //    auto KindName = [](int k) -> const char* {
    //        switch (k) {
    //        case NK_Selector:   return "Selector";
    //        case NK_Sequence:   return "Sequence";
    //        case NK_ParallelAny:return "ParallelAny";
    //        case NK_Inverter:   return "Inverter";
    //        case NK_Succeeder:  return "Succeeder";
    //        case NK_Repeater:   return "Repeater";
    //        case NK_Timer:      return "Timer";
    //        case NK_Condition:  return "Condition";
    //        case NK_Action:     return "Action";
    //        } return "?";
    //        };
    //    auto IsComposite = [](int k) { return k == NK_Selector || k == NK_Sequence || k == NK_ParallelAny; };
    //    auto IsDecorator = [](int k) { return k == NK_Inverter || k == NK_Succeeder || k == NK_Repeater || k == NK_Timer; };
    //    auto IsLeaf = [](int k) { return k == NK_Condition || k == NK_Action; };

    //    static bool initialized = false;
    //    static int  nextId = 1;
    //    static int  rootId = -1;
    //    static int  selectedId = -1;

    //    static std::vector<int>                ids;
    //    static std::vector<int>                kinds;
    //    static std::vector<std::string>        names;
    //    static std::vector<std::vector<int>>   childIds;

    //    static std::vector<int>   repeatCounts;   // NK_Repeater
    //    static std::vector<float> timerSeconds;   // NK_Timer
    //    static std::vector<float> posX, posY;

    //    auto indexOf = [&](int id)->int {
    //        for (int i = 0; i < (int)ids.size(); ++i) if (ids[i] == id) return i;
    //        return -1;
    //        };
    //    auto addNode = [&](int kind, const char* nm)->int {
    //        const int id = nextId++;
    //        ids.push_back(id);
    //        kinds.push_back(kind);
    //        names.push_back(nm ? std::string(nm) : std::string(KindName(kind)));
    //        childIds.emplace_back();               //  can reallocate childIds (outer vector)
    //        repeatCounts.push_back(-1);
    //        timerSeconds.push_back(1.0f);
    //        posX.push_back(0.0f);
    //        posY.push_back(0.0f);
    //        return id;
    //        };

    //    // SAFE eraseSubtree (recompute idx after recursion)
    //    std::function<void(int)> eraseSubtree = [&](int id) {
    //        int idx = indexOf(id); if (idx < 0) return;

    //        // remove references from all parents
    //        for (auto& ch : childIds) {
    //            for (size_t i = 0; i < ch.size();) {
    //                if (ch[i] == id) ch.erase(ch.begin() + i);
    //                else ++i;
    //            }
    //        }

    //        // copy children, then recurse
    //        std::vector<int> toDel = childIds[idx];
    //        for (int cid : toDel) eraseSubtree(cid);

    //        // re-find idx (vectors may have shifted)
    //        idx = indexOf(id);
    //        if (idx < 0) return;

    //        ids.erase(ids.begin() + idx);
    //        kinds.erase(kinds.begin() + idx);
    //        names.erase(names.begin() + idx);
    //        childIds.erase(childIds.begin() + idx);
    //        repeatCounts.erase(repeatCounts.begin() + idx);
    //        timerSeconds.erase(timerSeconds.begin() + idx);
    //        posX.erase(posX.begin() + idx);
    //        posY.erase(posY.begin() + idx);

    //        if (rootId == id) rootId = -1;
    //        if (selectedId == id) selectedId = -1;
    //        };

    //    auto canAdopt = [&](int parentId, int childId)->bool {
    //        if (parentId < 0 || childId < 0 || parentId == childId) return false;
    //        int pi = indexOf(parentId), ci = indexOf(childId);
    //        if (pi < 0 || ci < 0) return false;
    //        if (IsLeaf(kinds[pi])) return false;
    //        if (IsDecorator(kinds[pi]) && !childIds[pi].empty()) return false; // decorators: only 1 child

    //        std::vector<int> stack{ childId };
    //        std::unordered_set<int> seen;
    //        while (!stack.empty()) {
    //            int nid = stack.back(); stack.pop_back();
    //            if (!seen.insert(nid).second) continue;
    //            if (nid == parentId) return false; // cycle
    //            int ni = indexOf(nid); if (ni >= 0) for (int c : childIds[ni]) stack.push_back(c);
    //        }
    //        return true;
    //        };

    //    // Init sample tree
    //    if (!initialized) {
    //        initialized = true;
    //        rootId = addNode(NK_Selector, "Root");
    //        int seq = addNode(NK_Sequence, "Chase");
    //        int cond = addNode(NK_Condition, "CanSeePlayer");
    //        int act = addNode(NK_Action, "ChasePlayer");
    //        int idle = addNode(NK_Action, "Patrol");
    //        int ri = indexOf(rootId);
    //        childIds[ri].push_back(seq);
    //        int si = indexOf(seq);
    //        childIds[si].push_back(cond);
    //        childIds[si].push_back(act);
    //        childIds[ri].push_back(idle);
    //        selectedId = rootId;
    //    }

    //    // Toolbar
    //    if (ImGui::Button("New")) {
    //        ids.clear(); kinds.clear(); names.clear(); childIds.clear();
    //        repeatCounts.clear(); timerSeconds.clear(); posX.clear(); posY.clear();
    //        nextId = 1;
    //        rootId = addNode(NK_Selector, "Root");
    //        selectedId = rootId;
    //    }
    //    ImGui::SameLine();
    //    if (ImGui::Button("Add Child to Root")) {
    //        if (rootId >= 0) {
    //            int parentIdx = indexOf(rootId);
    //            int newId = addNode(NK_Sequence, "Sequence");  // STEP 1
    //            childIds[parentIdx].push_back(newId);          // STEP 2 (after addNode)
    //        }
    //    }

    //    ImGui::Separator();

    //    ImGui::Columns(2, nullptr, true);

    //    // LEFT: outline
    //    ImGui::TextDisabled("Outline");
    //    if (rootId < 0) {
    //        ImGui::TextDisabled("<no root>");
    //    }
    //    else {
    //        std::function<void(int)> drawNodeLine = [&](int id) {
    //            int i = indexOf(id); if (i < 0) return;

    //            const bool leafish = IsLeaf(kinds[i]) || (IsDecorator(kinds[i]) && childIds[i].empty());
    //            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    //            if (leafish) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    //            if (selectedId == id) flags |= ImGuiTreeNodeFlags_Selected;

    //            bool open = ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "[%s] %s",
    //                KindName(kinds[i]), names[i].c_str());
    //            if (ImGui::IsItemClicked()) selectedId = id;

    //            // Context menu
    //            if (ImGui::BeginPopupContextItem()) {
    //                if (ImGui::BeginMenu("Add Child")) {
    //                    bool canAdd = IsComposite(kinds[i]) || (IsDecorator(kinds[i]) && childIds[i].empty());
    //                    auto addChildIf = [&](int k, const char* n) {
    //                        if (canAdd && ImGui::MenuItem(n)) {
    //                            // SPLIT: create then push to parent
    //                            int parentIdx = indexOf(id);      // re-evaluate after menu action
    //                            int newId = addNode(k, n);        // may reallocate childIds
    //                            childIds[parentIdx].push_back(newId);
    //                        }
    //                        };
    //                    addChildIf(NK_Selector, "Selector");
    //                    addChildIf(NK_Sequence, "Sequence");
    //                    addChildIf(NK_ParallelAny, "ParallelAny");
    //                    addChildIf(NK_Inverter, "Inverter");
    //                    addChildIf(NK_Succeeder, "Succeeder");
    //                    addChildIf(NK_Repeater, "Repeater");
    //                    addChildIf(NK_Timer, "Timer");
    //                    addChildIf(NK_Condition, "Condition");
    //                    addChildIf(NK_Action, "Action");
    //                    if (!canAdd) ImGui::TextDisabled("Parent cannot take more children");
    //                    ImGui::EndMenu();
    //                }
    //                if (ImGui::MenuItem("Delete")) { int doomed = id; ImGui::EndPopup(); eraseSubtree(doomed); return; }
    //                ImGui::EndPopup();
    //            }

    //            // Accept drag-drop reparent
    //            if (ImGui::BeginDragDropTarget()) {
    //                if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("BTNODE_REPARENT")) {
    //                    int movingId = *(const int*)p->Data;
    //                    if (canAdopt(id, movingId)) {
    //                        for (auto& ch : childIds) {
    //                            for (size_t k = 0; k < ch.size(); ) { if (ch[k] == movingId) ch.erase(ch.begin() + k); else ++k; }
    //                        }
    //                        int parentIdx = indexOf(id);
    //                        childIds[parentIdx].push_back(movingId);
    //                    }
    //                }
    //                ImGui::EndDragDropTarget();
    //            }

    //            // Drag source (fetch fresh name each time)
    //            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
    //                ImGui::SetDragDropPayload("BTNODE_REPARENT", &id, sizeof(int));
    //                ImGui::Text("Move '%s'", names[i].c_str());
    //                ImGui::EndDragDropSource();
    //            }

    //            if (!leafish && open) {
    //                // children (note: i may change if vectors reallocate, but we only use child IDs)
    //                auto kidsCopy = childIds[i]; // copy ids to avoid issues if tree mutates mid-iteration
    //                for (int cid : kidsCopy) drawNodeLine(cid);
    //                ImGui::TreePop();
    //            }
    //            };

    //        drawNodeLine(rootId);
    //    }

    //    ImGui::NextColumn();

    //    // RIGHT: inspector
    //    ImGui::TextDisabled("Inspector");
    //    if (selectedId < 0) {
    //        ImGui::TextDisabled("<no selection>");
    //    }
    //    else {
    //        int si = indexOf(selectedId);
    //        if (si >= 0) {
    //            ImGui::Text("ID: %d", ids[si]);
    //            ImGui::Text("Type: %s", KindName(kinds[si]));

    //            char nameBuf[128]; std::snprintf(nameBuf, sizeof(nameBuf), "%s", names[si].c_str());
    //            if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) names[si] = nameBuf;

    //            if (kinds[si] == NK_Repeater) {
    //                ImGui::DragInt("Repeat (-1=inf)", &repeatCounts[si], 1, -1, 100000);
    //            }
    //            if (kinds[si] == NK_Timer) {
    //                ImGui::DragFloat("Timer (s)", &timerSeconds[si], 0.01f, 0.0f, 3600.0f, "%.2f");
    //            }

    //            if (IsComposite(kinds[si]) || IsDecorator(kinds[si])) {
    //                ImGui::SeparatorText("Children");
    //                auto& kids = childIds[si];
    //                for (int idx = 0; idx < (int)kids.size(); ++idx) {
    //                    int ci = indexOf(kids[idx]); if (ci < 0) continue;
    //                    ImGui::BulletText("%d. %s (%s)", idx + 1, names[ci].c_str(), KindName(kinds[ci]));
    //                    ImGui::SameLine();
    //                    ImGui::PushID(idx);
    //                    if (ImGui::SmallButton("Up") && idx > 0) std::swap(kids[idx - 1], kids[idx]);
    //                    ImGui::SameLine();
    //                    if (ImGui::SmallButton("Down") && idx + 1 < (int)kids.size()) std::swap(kids[idx], kids[idx + 1]);
    //                    ImGui::SameLine();
    //                    if (ImGui::SmallButton("Detach")) { kids.erase(kids.begin() + idx); ImGui::PopID(); break; }
    //                    ImGui::PopID();
    //                }
    //            }

    //            ImGui::Separator();
    //            if (ImGui::Button("Delete Node")) { int doomed = ids[si]; eraseSubtree(doomed); }
    //        }
    //        else {
    //            ImGui::TextDisabled("<dangling selection>");
    //        }
    //    }

    //    ImGui::Columns(1);
    //}

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
//
//// Draw a BehaviorTreeAsset as an outline using nodeLevel for hierarchy
//void DrawBTAssetOutline(const BehaviorTreeAsset& asset)
//{
//    ImGui::SeparatorText(asset.name.c_str());
//
//    if (asset.nodes.empty()) {
//        ImGui::TextDisabled("<no nodes>");
//        return;
//    }
//
//    // Track curr depth
//    unsigned int prevLevel = 0;
//
//    for (size_t i = 0; i < asset.nodes.size(); ++i) {
//        const BTNodeDesc& nd = asset.nodes[i];
//
//        // Close prev nodes as gg back up the lvl
//        while (prevLevel > nd.nodeLevel) {
//            ImGui::TreePop();
//            --prevLevel;
//        }
//
//        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
//        bool leaf = true;
//
//        // Check if children exist
//        if (i + 1 < asset.nodes.size() && asset.nodes[i + 1].nodeLevel > nd.nodeLevel) {
//            leaf = false;
//        }
//
//        if (leaf) {
//            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
//            ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)", nd.nodeType.c_str(), nd.nodeLevel);
//        }
//        else {
//            bool open = ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)", nd.nodeType.c_str(), nd.nodeLevel);
//            if (open) {
//                ++prevLevel; // keep it open until a higher level node closes it
//            }
//        }
//    }
//    // Close  remaining open levels
//    while (prevLevel > 0) {
//        ImGui::TreePop();
//        --prevLevel;
//    }
//}
//
//    namespace fs = std::filesystem;
//    static void RefreshBTList(std::string& dir,
//        std::vector<std::string>& files,
//        int& currentIndex,
//        BehaviorTreeAsset& loadedAsset,
//        bool& hasAsset,
//        std::string& lastLoadedPath)
//    {
//        namespace fs = std::filesystem;
//        files.clear();
//
//        std::error_code ec;
//        if (dir.empty()) return;
//        if (!fs::exists(dir, ec)) return;
//
//        for (auto& e : fs::directory_iterator(dir, ec)) {
//            if (!e.is_regular_file(ec)) continue;
//            const auto ext = e.path().extension().string();
//            if (ext == ".json" || ext == ".bht")
//                files.emplace_back(e.path().filename().string());
//        }
//        std::sort(files.begin(), files.end());
//
//        // keep selection valid
//        if (currentIndex < 0 || currentIndex >= (int)files.size()) {
//            currentIndex = files.empty() ? -1 : 0;
//        }
//
//        // auto-load first if valid
//        if (currentIndex >= 0 && currentIndex < (int)files.size()) {
//            const std::string full = dir + "/" + files[currentIndex];
//            if (LoadBTAssetFromFile(full, loadedAsset)) {
//                hasAsset = true;
//                lastLoadedPath = full;
//            }
//            else {
//                hasAsset = false;
//            }
//        }
//        else {
//            hasAsset = false;
//        }
//    }

    void DrawBTAssetOutline(const BehaviorTreeAsset& asset)
    {
        ImGui::SeparatorText(asset.name.c_str());

        WalkBTAssetFlat(asset, [&](const BTNodeDesc& nd, size_t, ImGuiTreeNodeFlags flags) {
            bool open = ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
                nd.nodeType.c_str(), nd.nodeLevel);
            return open;
            });
    }



    // ---- draw tree where you can select a node ----
    //static void DrawSelectableTree(
    //    const BehaviorTreeAsset& asset,
    //    int& selectedIndex // index in asset.nodes
    //)
    //{
    //    if (asset.nodes.empty()) {
    //        ImGui::TextDisabled("<no nodes>");
    //        return;
    //    }

    //    unsigned int prevLevel = 0;

    //    for (size_t i = 0; i < asset.nodes.size(); ++i) {
    //        const BTNodeDesc& nd = asset.nodes[i];

    //        // Close open nodes when moving up
    //        while (prevLevel > nd.nodeLevel) {
    //            ImGui::TreePop();
    //            --prevLevel;
    //        }

    //        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    //        // Leaf detection: if next node is deeper, this one has children
    //        bool leaf = !(i + 1 < asset.nodes.size() && asset.nodes[i + 1].nodeLevel > nd.nodeLevel);

    //        if (leaf) {
    //            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    //        }

    //        if ((int)i == selectedIndex) {
    //            flags |= ImGuiTreeNodeFlags_Selected;
    //        }

    //        bool open = ImGui::TreeNodeEx((void*)&nd, flags, "%s (level %u)",
    //            nd.nodeType.c_str(), nd.nodeLevel);

    //        // Handle click
    //        if (ImGui::IsItemClicked()) {
    //            selectedIndex = static_cast<int>(i);
    //        }

    //        if (!leaf && open) {
    //            ++prevLevel; // Keep open until we find a shallower node
    //        }
    //    }

    //    // Close any still-open tree nodes
    //    while (prevLevel > 0) {
    //        ImGui::TreePop();
    //        --prevLevel;
    //    }
    //}

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

    void BehaviourTreeWindow::DrawWindow()
    {

        // ===== persistent UI state =====
        static std::string dir;                        // BT folder
        static std::vector<std::string> files;              // file names (no path)
        static int currentIndex = -1;             // index into files
        static BehaviorTreeAsset loadedAsset;     // currently loaded asset
        static bool hasAsset = false;
        static std::string lastLoadedPath;             // for error popup
        static int selectedNodeIndex;             // selection inside loadedAsset
        static int nodeTypeComboIdx = 0;          // "Add Child" combo index
        
        // ===== small helpers kept local to this function =====
        auto loadAssetFromSelected = [&]() -> bool {
            if (currentIndex < 0 || currentIndex >= (int)files.size())
                return false;

            const std::string full = dir + "/" + files[currentIndex];

            BehaviorTreeAsset tmp;
            if (!LoadBTAssetFromFile(full, tmp)) {
                hasAsset = false;
                lastLoadedPath = full;
                selectedNodeIndex = -1;
                return false;
            }

            loadedAsset =tmp;
            hasAsset = true;
            lastLoadedPath = full;

            // pick a reasonable default selection:
            // prefer the first node at level 0, else just index 0, else none
            if (!loadedAsset.nodes.empty()) {
                int idx = 0;
                for (int i = 0; i < (int)loadedAsset.nodes.size(); ++i) {
                    if (loadedAsset.nodes[i].nodeLevel == 0) { idx = i; break; }
                }
                selectedNodeIndex = idx;
            }
            else {
                selectedNodeIndex = -1;
            }

            return true;
            };

        auto refreshList = [&]() {
            files.clear();
            std::error_code ec;
            if (dir.empty()) return;
            if (!std::filesystem::exists(dir, ec)) return;
            for (auto& e : std::filesystem::directory_iterator(dir, ec)) {
                if (!e.is_regular_file(ec)) continue;
                const auto ext = e.path().extension().string();
                if (ext == ".json" || ext == ".bht")
                    files.emplace_back(e.path().filename().string());
            }
            std::sort(files.begin(), files.end());
            // fix selection
            if (currentIndex < 0 || currentIndex >= (int)files.size())
                currentIndex = files.empty() ? -1 : 0;
            };


        // selectable tree outline (left)
        //std::function<void(const std::string&)> drawTreeRecursive =
        //    [&](const std::string& nodeID)
        //    {
        //        // build quick index map once for speed
        //        static std::unordered_map<std::string, BTNodeDesc*> byId;
        //        byId.clear();
        //        byId.reserve(loadedAsset.nodes.size());
        //        for (auto& n : loadedAsset.nodes)
        //            byId[n.nodeID] = &n;

        //        auto it = byId.find(nodeID);
        //        if (it == byId.end()) { ImGui::BulletText("[missing] %s", nodeID.c_str()); return; }
        //        BTNodeDesc* nd = it->second;

        //        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        //        const bool leaf = nd->children.empty();
        //        if (leaf) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        //        if (selectedNodeId == nd->nodeID) flags |= ImGuiTreeNodeFlags_Selected;

        //        bool open = ImGui::TreeNodeEx((void*)nd, flags, "%s  (%s)",
        //            nd->nodeID.c_str(), nd->nodeType.c_str());
        //        if (ImGui::IsItemClicked()) selectedNodeId = nd->nodeID;

        //        if (!leaf && open) {
        //            // draw children
        //            for (const auto& cid : nd->children) {
        //                // recurse via direct call to preserve byId scope
        //                auto itC = byId.find(cid);
        //                if (itC == byId.end()) { ImGui::BulletText("[missing] %s", cid.c_str()); continue; }
        //                BTNodeDesc* child = itC->second;

        //                ImGuiTreeNodeFlags cflags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        //                const bool cleaf = child->children.empty();
        //                if (cleaf) cflags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        //                if (selectedNodeId == child->nodeID) cflags |= ImGuiTreeNodeFlags_Selected;

        //                bool copen = ImGui::TreeNodeEx((void*)child, cflags, "%s  (%s)",
        //                    child->nodeID.c_str(), child->nodeType.c_str());
        //                if (ImGui::IsItemClicked()) selectedNodeId = child->nodeID;

        //                if (!cleaf && copen) {
        //                    // second level recursion
        //                    for (const auto& gcid : child->children) {
        //                        auto itG = byId.find(gcid);
        //                        if (itG == byId.end()) { ImGui::BulletText("[missing] %s", gcid.c_str()); continue; }
        //                        BTNodeDesc* g = itG->second;

        //                        ImGuiTreeNodeFlags gflags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        //                        const bool gleaf = g->children.empty();
        //                        if (gleaf) gflags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        //                        if (selectedNodeId == g->nodeID) gflags |= ImGuiTreeNodeFlags_Selected;

        //                        bool gopen = ImGui::TreeNodeEx((void*)g, gflags, "%s  (%s)",
        //                            g->nodeID.c_str(), g->nodeType.c_str());
        //                        if (ImGui::IsItemClicked()) selectedNodeId = g->nodeID;
        //                        if (gopen && !gleaf) {
        //                            for (const auto& hc : g->children) {
        //                                auto itH = byId.find(hc);
        //                                if (itH == byId.end()) { ImGui::BulletText("[missing] %s", hc.c_str()); continue; }
        //                                BTNodeDesc* h = itH->second;

        //                                ImGuiTreeNodeFlags hflags = ImGuiTreeNodeFlags_Leaf |
        //                                    ImGuiTreeNodeFlags_NoTreePushOnOpen |
        //                                    ImGuiTreeNodeFlags_SpanAvailWidth;
        //                                if (selectedNodeId == h->nodeID) hflags |= ImGuiTreeNodeFlags_Selected;
        //                                ImGui::TreeNodeEx((void*)h, hflags, "%s  (%s)",
        //                                    h->nodeID.c_str(), h->nodeType.c_str());
        //                                if (ImGui::IsItemClicked()) selectedNodeId = h->nodeID;
        //                                // leaf: no push/pop
        //                            }
        //                            ImGui::TreePop();
        //                        }
        //                    }
        //                    ImGui::TreePop();
        //                }
        //            }
        //            ImGui::TreePop();
        //        }
        //    };

        // ===== initialize dir once =====
        if (dir.empty()) {
            if (auto* fp = ST<Filepaths>::Get()) dir = fp->behaviourTreeSave;
            else                                 dir = "Assets/BehaviourTrees";
            refreshList();
            (void)loadAssetFromSelected(); // auto-load first if any
        }

        // ===== top bar =====
        ImGui::Text("Folder: %s", dir.c_str());
        ImGui::SameLine();
        if (ImGui::Button("Reload List")) {
            refreshList();
            if (!loadAssetFromSelected()) ImGui::OpenPopup("BT Load Error");
        }

        // file combo (auto-load on change)
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
                        if (!loadAssetFromSelected()) ImGui::OpenPopup("BT Load Error");
                    }
                    if (sel) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }

        // error popup
        if (ImGui::BeginPopupModal("BT Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to load:\n%s", lastLoadedPath.c_str());
            ImGui::Spacing();
            if (ImGui::Button("OK", ImVec2(120, 0))) ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        ImGui::Separator();


        //FILE RENAMING
        DrawBTRenameUI(dir, files, currentIndex, lastLoadedPath);

        //SAVE BUTTON
        DrawBTSaveUI(dir, files, currentIndex, loadedAsset, lastLoadedPath, hasAsset);

        
        // ===== editor split: left outline, right palette/inspector =====
        ImGui::BeginChild("##bt_editor_root", ImVec2(0, 0), true);
        ImGui::Columns(2, nullptr, true);

        // LEFT: hierarchy
        ImGui::TextDisabled("Hierarchy");
        ImGui::BeginChild("##bt_hierarchy", ImVec2(0, 0), true);
        if (!hasAsset || loadedAsset.nodes.empty()) {
            ImGui::TextDisabled("<nothing loaded>");
        }
        else {
            for (int i = 0; i < (int)loadedAsset.nodes.size(); ++i) {
                const auto& nd = loadedAsset.nodes[i];

                // indent by level
                ImGui::Indent(nd.nodeLevel * 20.0f);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (i == selectedNodeIndex) flags |= ImGuiTreeNodeFlags_Selected;

                ImGui::TreeNodeEx((void*)(intptr_t)i, flags, "%s (level %u)", nd.nodeType.c_str(), nd.nodeLevel);
                if (ImGui::IsItemClicked()) selectedNodeIndex = i;

                // unindent back
                ImGui::Unindent(nd.nodeLevel * 20.0f);
            }
        }
        ImGui::EndChild();

        ImGui::NextColumn();

        // ------------------------------------
        // RIGHT: node palette + inspector
        // ------------------------------------
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

        ImGui::Separator();
        ImGui::TextDisabled("Inspector");

        // Guard selection
        if (!hasAsset || selectedNodeIndex < 0 || selectedNodeIndex >= (int)loadedAsset.nodes.size()) {
            ImGui::TextDisabled("<select a node>");
        }
        else {
            auto& nodes = loadedAsset.nodes;
            const int parentIdx = selectedNodeIndex;
            const unsigned pLevel = nodes[parentIdx].nodeLevel;

            ImGui::Text("Type:  %s", nodes[parentIdx].nodeType.c_str());
            ImGui::Text("Level: %u", nodes[parentIdx].nodeLevel);

            // ---------- level-based helpers ----------
            auto subtreeEnd = [&](int startIdx) -> int {
                const unsigned base = nodes[startIdx].nodeLevel;
                int i = startIdx + 1;
                while (i < (int)nodes.size() && nodes[i].nodeLevel > base) ++i;
                return i; // one past end of subtree
                };

            auto childrenRange = [&](int pIdx) -> std::pair<int, int> {
                // direct children are in [first, end), but only those with level = pLevel+1 are *direct*
                int first = pIdx + 1;
                int end = subtreeEnd(pIdx);
                return { first, end };
                };

            auto listDirectChildren = [&](int pIdx, std::vector<int>& out) {
                out.clear();
                auto [first, end] = childrenRange(pIdx);
                const unsigned want = nodes[pIdx].nodeLevel + 1;
                for (int i = first; i < end; ++i) {
                    if (nodes[i].nodeLevel == want) out.push_back(i);
                }
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
                // Move subtree block A to just before subtree block B by rotating ranges.
                // Precondition: startA < startB.
                const int endA = subtreeEnd(startA);
                const int endB = subtreeEnd(startB);
                std::rotate(nodes.begin() + startA, nodes.begin() + startB, nodes.begin() + endB);
                };

            // ---------- Children UI ----------
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

                    // Up: swap this child subtree with previous sibling subtree
                    if (ImGui::SmallButton("Up") && c > 0) {
                        int prevStart = childIdxs[c - 1];
                        int thisStart = childIdxs[c];
                        if (prevStart > thisStart) std::swap(prevStart, thisStart);
                        moveSubtreeBlock(prevStart, thisStart);
                    }
                    ImGui::SameLine();

                    // Down: swap with next sibling subtree
                    if (ImGui::SmallButton("Down") && c + 1 < (int)childIdxs.size()) {
                        int thisStart = childIdxs[c];
                        int nextStart = childIdxs[c + 1];
                        if (thisStart > nextStart) std::swap(thisStart, nextStart);
                        moveSubtreeBlock(thisStart, nextStart);
                    }
                    ImGui::SameLine();

                    // Detach: promote whole subtree by one level (becomes sibling of parent)
                    if (ImGui::SmallButton("Detach")) {
                        if (nodes[ci].nodeLevel > 0) {
                            adjustSubtreeLevel(ci, -1);
                        }
                    }

                    ImGui::PopID();
                }
            }

            // ---------- Add Child ----------
            ImGui::SeparatorText("Add Child");

            // Get the registered node types once per frame
            std::vector<std::string> regTypes = ST<BTFactory>::Get()->RegisteredTypes();
            std::sort(regTypes.begin(), regTypes.end());

            if (regTypes.empty()) {
                ImGui::TextDisabled("<no registered node types>");
            }
            else {
                // A compact list: [TypeName]  [Add]
                for (int i = 0; i < (int)regTypes.size(); ++i) {
                    ImGui::PushID(i); // ensure unique IDs per row

                    ImGui::TextUnformatted(regTypes[i].c_str());
                    ImGui::SameLine();

                    if (ImGui::SmallButton("Add")) {
                        // Build the new node right under this parent
                        BTNodeDesc newNd;
                        newNd.nodeType = regTypes[i];
                        newNd.nodeLevel = pLevel + 1;

                        // Insert after the last direct child subtree (or right after the parent if no children)
                        int insertPos = parentIdx + 1;
                        {
                            std::vector<int> directChildren;
                            listDirectChildren(parentIdx, directChildren);     // fills indices of direct children (same level+1)
                            if (!directChildren.empty()) {
                                const int lastChildStart = directChildren.back();
                                insertPos = subtreeEnd(lastChildStart);         // position *after* last child's subtree
                            }
                        }

                        nodes.insert(nodes.begin() + insertPos, newNd);

                        // pick the newly inserted node in the UI
                        selectedNodeIndex = insertPos;
                    }

                    ImGui::PopID();
                }
            }

        }

        ImGui::Columns(1);
        ImGui::EndChild();
    }


}
