#include "BehaviourTreeWindow.h"
#include "Editor.h"
#include "AssetBrowser.h"

namespace editor {

	BehaviourTreeWindow::BehaviourTreeWindow()
		: WindowBase{ ICON_FA_ROBOT" Behaviour Tree", gui::Vec2{ 1000.0f, 768.0f } }
		, modificationsMade{ false }
	{

	}

    void BehaviourTreeWindow::DrawWindow() {
#include <unordered_set>

        enum NodeKind { NK_Selector, NK_Sequence, NK_ParallelAny, NK_Inverter, NK_Succeeder, NK_Repeater, NK_Timer, NK_Condition, NK_Action };
        auto KindName = [](int k) -> const char* {
            switch (k) {
            case NK_Selector:   return "Selector";
            case NK_Sequence:   return "Sequence";
            case NK_ParallelAny:return "ParallelAny";
            case NK_Inverter:   return "Inverter";
            case NK_Succeeder:  return "Succeeder";
            case NK_Repeater:   return "Repeater";
            case NK_Timer:      return "Timer";
            case NK_Condition:  return "Condition";
            case NK_Action:     return "Action";
            } return "?";
            };
        auto IsComposite = [](int k) { return k == NK_Selector || k == NK_Sequence || k == NK_ParallelAny; };
        auto IsDecorator = [](int k) { return k == NK_Inverter || k == NK_Succeeder || k == NK_Repeater || k == NK_Timer; };
        auto IsLeaf = [](int k) { return k == NK_Condition || k == NK_Action; };

        static bool initialized = false;
        static int  nextId = 1;
        static int  rootId = -1;
        static int  selectedId = -1;

        static std::vector<int>                ids;
        static std::vector<int>                kinds;
        static std::vector<std::string>        names;
        static std::vector<std::vector<int>>   childIds;

        static std::vector<int>   repeatCounts;   // NK_Repeater
        static std::vector<float> timerSeconds;   // NK_Timer
        static std::vector<float> posX, posY;

        auto indexOf = [&](int id)->int {
            for (int i = 0; i < (int)ids.size(); ++i) if (ids[i] == id) return i;
            return -1;
            };
        auto addNode = [&](int kind, const char* nm)->int {
            const int id = nextId++;
            ids.push_back(id);
            kinds.push_back(kind);
            names.push_back(nm ? std::string(nm) : std::string(KindName(kind)));
            childIds.emplace_back();               //  can reallocate childIds (outer vector)
            repeatCounts.push_back(-1);
            timerSeconds.push_back(1.0f);
            posX.push_back(0.0f);
            posY.push_back(0.0f);
            return id;
            };

        // SAFE eraseSubtree (recompute idx after recursion)
        std::function<void(int)> eraseSubtree = [&](int id) {
            int idx = indexOf(id); if (idx < 0) return;

            // remove references from all parents
            for (auto& ch : childIds) {
                for (size_t i = 0; i < ch.size();) {
                    if (ch[i] == id) ch.erase(ch.begin() + i);
                    else ++i;
                }
            }

            // copy children, then recurse
            std::vector<int> toDel = childIds[idx];
            for (int cid : toDel) eraseSubtree(cid);

            // re-find idx (vectors may have shifted)
            idx = indexOf(id);
            if (idx < 0) return;

            ids.erase(ids.begin() + idx);
            kinds.erase(kinds.begin() + idx);
            names.erase(names.begin() + idx);
            childIds.erase(childIds.begin() + idx);
            repeatCounts.erase(repeatCounts.begin() + idx);
            timerSeconds.erase(timerSeconds.begin() + idx);
            posX.erase(posX.begin() + idx);
            posY.erase(posY.begin() + idx);

            if (rootId == id) rootId = -1;
            if (selectedId == id) selectedId = -1;
            };

        auto canAdopt = [&](int parentId, int childId)->bool {
            if (parentId < 0 || childId < 0 || parentId == childId) return false;
            int pi = indexOf(parentId), ci = indexOf(childId);
            if (pi < 0 || ci < 0) return false;
            if (IsLeaf(kinds[pi])) return false;
            if (IsDecorator(kinds[pi]) && !childIds[pi].empty()) return false; // decorators: only 1 child

            std::vector<int> stack{ childId };
            std::unordered_set<int> seen;
            while (!stack.empty()) {
                int nid = stack.back(); stack.pop_back();
                if (!seen.insert(nid).second) continue;
                if (nid == parentId) return false; // cycle
                int ni = indexOf(nid); if (ni >= 0) for (int c : childIds[ni]) stack.push_back(c);
            }
            return true;
            };

        // Init sample tree
        if (!initialized) {
            initialized = true;
            rootId = addNode(NK_Selector, "Root");
            int seq = addNode(NK_Sequence, "Chase");
            int cond = addNode(NK_Condition, "CanSeePlayer");
            int act = addNode(NK_Action, "ChasePlayer");
            int idle = addNode(NK_Action, "Patrol");
            int ri = indexOf(rootId);
            childIds[ri].push_back(seq);
            int si = indexOf(seq);
            childIds[si].push_back(cond);
            childIds[si].push_back(act);
            childIds[ri].push_back(idle);
            selectedId = rootId;
        }

        // Toolbar
        if (ImGui::Button("New")) {
            ids.clear(); kinds.clear(); names.clear(); childIds.clear();
            repeatCounts.clear(); timerSeconds.clear(); posX.clear(); posY.clear();
            nextId = 1;
            rootId = addNode(NK_Selector, "Root");
            selectedId = rootId;
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Child to Root")) {
            if (rootId >= 0) {
                int parentIdx = indexOf(rootId);
                int newId = addNode(NK_Sequence, "Sequence");  // STEP 1
                childIds[parentIdx].push_back(newId);          // STEP 2 (after addNode)
            }
        }

        ImGui::Separator();

        ImGui::Columns(2, nullptr, true);

        // LEFT: outline
        ImGui::TextDisabled("Outline");
        if (rootId < 0) {
            ImGui::TextDisabled("<no root>");
        }
        else {
            std::function<void(int)> drawNodeLine = [&](int id) {
                int i = indexOf(id); if (i < 0) return;

                const bool leafish = IsLeaf(kinds[i]) || (IsDecorator(kinds[i]) && childIds[i].empty());
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (leafish) flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (selectedId == id) flags |= ImGuiTreeNodeFlags_Selected;

                bool open = ImGui::TreeNodeEx((void*)(intptr_t)id, flags, "[%s] %s",
                    KindName(kinds[i]), names[i].c_str());
                if (ImGui::IsItemClicked()) selectedId = id;

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::BeginMenu("Add Child")) {
                        bool canAdd = IsComposite(kinds[i]) || (IsDecorator(kinds[i]) && childIds[i].empty());
                        auto addChildIf = [&](int k, const char* n) {
                            if (canAdd && ImGui::MenuItem(n)) {
                                // SPLIT: create then push to parent
                                int parentIdx = indexOf(id);      // re-evaluate after menu action
                                int newId = addNode(k, n);        // may reallocate childIds
                                childIds[parentIdx].push_back(newId);
                            }
                            };
                        addChildIf(NK_Selector, "Selector");
                        addChildIf(NK_Sequence, "Sequence");
                        addChildIf(NK_ParallelAny, "ParallelAny");
                        addChildIf(NK_Inverter, "Inverter");
                        addChildIf(NK_Succeeder, "Succeeder");
                        addChildIf(NK_Repeater, "Repeater");
                        addChildIf(NK_Timer, "Timer");
                        addChildIf(NK_Condition, "Condition");
                        addChildIf(NK_Action, "Action");
                        if (!canAdd) ImGui::TextDisabled("Parent cannot take more children");
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem("Delete")) { int doomed = id; ImGui::EndPopup(); eraseSubtree(doomed); return; }
                    ImGui::EndPopup();
                }

                // Accept drag-drop reparent
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("BTNODE_REPARENT")) {
                        int movingId = *(const int*)p->Data;
                        if (canAdopt(id, movingId)) {
                            for (auto& ch : childIds) {
                                for (size_t k = 0; k < ch.size(); ) { if (ch[k] == movingId) ch.erase(ch.begin() + k); else ++k; }
                            }
                            int parentIdx = indexOf(id);
                            childIds[parentIdx].push_back(movingId);
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Drag source (fetch fresh name each time)
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("BTNODE_REPARENT", &id, sizeof(int));
                    ImGui::Text("Move '%s'", names[i].c_str());
                    ImGui::EndDragDropSource();
                }

                if (!leafish && open) {
                    // children (note: i may change if vectors reallocate, but we only use child IDs)
                    auto kidsCopy = childIds[i]; // copy ids to avoid issues if tree mutates mid-iteration
                    for (int cid : kidsCopy) drawNodeLine(cid);
                    ImGui::TreePop();
                }
                };

            drawNodeLine(rootId);
        }

        ImGui::NextColumn();

        // RIGHT: inspector
        ImGui::TextDisabled("Inspector");
        if (selectedId < 0) {
            ImGui::TextDisabled("<no selection>");
        }
        else {
            int si = indexOf(selectedId);
            if (si >= 0) {
                ImGui::Text("ID: %d", ids[si]);
                ImGui::Text("Type: %s", KindName(kinds[si]));

                char nameBuf[128]; std::snprintf(nameBuf, sizeof(nameBuf), "%s", names[si].c_str());
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) names[si] = nameBuf;

                if (kinds[si] == NK_Repeater) {
                    ImGui::DragInt("Repeat (-1=inf)", &repeatCounts[si], 1, -1, 100000);
                }
                if (kinds[si] == NK_Timer) {
                    ImGui::DragFloat("Timer (s)", &timerSeconds[si], 0.01f, 0.0f, 3600.0f, "%.2f");
                }

                if (IsComposite(kinds[si]) || IsDecorator(kinds[si])) {
                    ImGui::SeparatorText("Children");
                    auto& kids = childIds[si];
                    for (int idx = 0; idx < (int)kids.size(); ++idx) {
                        int ci = indexOf(kids[idx]); if (ci < 0) continue;
                        ImGui::BulletText("%d. %s (%s)", idx + 1, names[ci].c_str(), KindName(kinds[ci]));
                        ImGui::SameLine();
                        ImGui::PushID(idx);
                        if (ImGui::SmallButton("Up") && idx > 0) std::swap(kids[idx - 1], kids[idx]);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Down") && idx + 1 < (int)kids.size()) std::swap(kids[idx], kids[idx + 1]);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Detach")) { kids.erase(kids.begin() + idx); ImGui::PopID(); break; }
                        ImGui::PopID();
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Delete Node")) { int doomed = ids[si]; eraseSubtree(doomed); }
            }
            else {
                ImGui::TextDisabled("<dangling selection>");
            }
        }

        ImGui::Columns(1);
    }



}