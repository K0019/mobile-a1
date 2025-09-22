#include "BehaviourTreeImguiHelper.h"

//int BehaviourTreeEditorModel::indexOf(int id) const {
//    for (int i = 0; i < (int)nodes.size(); ++i)
//        if (nodes[i].id == id) return i;
//    return -1;
//}
//int BehaviourTreeEditorModel::addNode(const std::string& type, const std::string& displayName) {
//    BTEditorNode n;
//    n.id = nextId++;
//    n.type = type;
//    n.displayName = displayName.empty() ? type : displayName;
//    nodes.push_back(std::move(n));
//    return nodes.back().id;
//
//}
//
//void BehaviourTreeEditorModel::eraseSubtree(int id) {
//    int idx = indexOf(id); if (idx < 0) return;
//
//    // detach from any parent lists
//    for (auto& n : nodes) {
//        auto& ch = n.children;
//        ch.erase(std::remove(ch.begin(), ch.end(), id), ch.end());
//    }
//
//    // copy children, then recurse
//    auto toDel = nodes[idx].children;
//    for (int cid : toDel) {
//        eraseSubtree(cid);
//    }
//    // refetch index after mutations
//    idx = indexOf(id); if (idx < 0) return;
//
//    if (rootId == id) rootId = -1;
//    if (selectedId == id) selectedId = -1;
//
//    nodes.erase(nodes.begin() + idx);
//}
//
//bool BehaviourTreeEditorModel::canAdopt(int parentId, int childId) const {
//    if (parentId < 0 || childId < 0 || parentId == childId) return false;
//    int pi = indexOf(parentId), ci = indexOf(childId);
//    if (pi < 0 || ci < 0) return false;
//
//    // prevent cycles
//    std::vector<int> stack{ childId };
//    std::unordered_set<int> seen;
//    while (!stack.empty()) {
//        int nid = stack.back(); stack.pop_back();
//        if (!seen.insert(nid).second) continue;
//        if (nid == parentId) return false;
//        int ni = indexOf(nid);
//        if (ni >= 0) for (int c : nodes[ni].children) stack.push_back(c);
//    }
//    return true;
//}
//
//
////Convert Editor to Behaviour Tree and vice versa
//
//BehaviorTreeAsset MakeAssetFromEditor(const BehaviourTreeEditorModel& m, const std::string& assetName) {
//    BehaviorTreeAsset out;
//    out.name = assetName;
//    out.rootID = std::to_string(m.rootId);
//    out.nodes.reserve(m.nodes.size());
//
//    for (const auto& e : m.nodes) {
//        BTNodeDesc nd;
//        nd.nodeID = std::to_string(e.id);
//        nd.nodeType = e.type;
//        // store children as string IDs
//        for (int cid : e.children) nd.children.push_back(std::to_string(cid));
//
//        // If/when you add params in the editor, push into nd.keys / nd.values here.
//
//        out.nodes.push_back(std::move(nd));
//    }
//    return out;
//}
//
//
//void FillEditorFromAsset(BehaviourTreeEditorModel& m, const BehaviorTreeAsset& a) {
//    // Reset model (use your model's reset if it has one)
//    m = {};
//    m.nextId = 1;
//    m.rootId = -1;
//    m.selectedId = -1;
//    m.nodes.clear();
//
//    // 1) create all nodes, let editor assign IDs
//    std::unordered_map<std::string, int> sid2eid; // serialized-id -> editor-id
//    sid2eid.reserve(a.nodes.size());
//
//    for (const auto& nd : a.nodes) {
//        const std::string display = nd.nodeType;               // or pull from a 'name' key if you have one
//        const int eid = m.addNode(nd.nodeType, display);       // addNode returns a *new* editor ID
//        sid2eid.emplace(nd.nodeID, eid);
//    }
//
//    // 2) wire children using the map
//    for (const auto& nd : a.nodes) {
//        auto pit = sid2eid.find(nd.nodeID);
//        if (pit == sid2eid.end()) continue;
//
//        const int parentEid = pit->second;
//        const int pi = m.indexOf(parentEid);
//        if (pi < 0) continue;
//
//        for (const auto& scid : nd.children) {
//            auto cit = sid2eid.find(scid);
//            if (cit != sid2eid.end()) {
//                m.nodes[pi].children.push_back(cit->second);
//            }
//        }
//    }
//
//    // 3) root & selection via the map
//    if (!a.rootID.empty()) {
//        auto ir = sid2eid.find(a.rootID);
//        if (ir != sid2eid.end()) m.rootId = ir->second;
//    }
//    if (m.rootId < 0 && !m.nodes.empty()) m.rootId = m.nodes.front().id;
//
//    m.selectedId = m.rootId;
//}
//
//
////================================== FILE IO ============================================
//
//std::vector<std::string> ScanDirForTrees(const std::string& dir) {
//    std::vector<std::string> out;
//    if (!std::filesystem::exists(dir)) return out;
//
//    for (auto& entry : std::filesystem::directory_iterator(dir)) {
//        if (!entry.is_regular_file()) continue;
//        auto ext = entry.path().extension().string();
//        if (ext == ".bht" || ext == ".json") {
//            out.push_back(entry.path().filename().string()); // file only
//        }
//    }
//    return out;
//}
//
//bool LoadAssetFromFile(const std::string& fullPath, BehaviorTreeAsset& out) {
//    Deserializer r(fullPath.c_str());
//    if (!r.IsValid()) return false;
//    return r.Deserialize(&out);
//}
//
//bool SaveAssetToFile(const std::string& fullPath, const BehaviorTreeAsset& asset) {
//    Serializer w(fullPath.c_str());
//    if (!w.IsOpen()) return false;
//
//    // Same as Property style ( WILL NEED MODIFY TO USE PROEPERTY FR LATER)
//    w.Serialize("name", asset.name);
//    w.Serialize("rootID", asset.rootID);
//
//    w.StartArray("nodes");
//    for (auto& nd : asset.nodes) {
//        w.StartObject();
//        w.Serialize("nodeID", nd.nodeID);
//        w.Serialize("nodeType", nd.nodeType);
//        w.Serialize("children", nd.children);
//        w.Serialize("keys", nd.keys);
//        w.Serialize("values", nd.values);
//        w.EndObject();
//    }
//    w.EndArray();
//
//    return w.SaveAndClose();
//}
//
//void NodesRegistered::Refresh() {
//    types = BTFactory::Instance().RegisteredTypes();
//    std::sort(types.begin(), types.end());
//}

bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset& out)
{
    Deserializer r(path.c_str());
    if (!r.IsValid()) return false;
    return r.Deserialize(&out);
}

 int FindNodeIndexById(const BehaviorTreeAsset& a, const std::string& id) {
    for (int i = 0; i < (int)a.nodes.size(); ++i)
        if (a.nodes[i].nodeID == id) return i;
    return -1;
}

 std::unordered_set<std::string> CollectAllIds(const BehaviorTreeAsset& a) {
    std::unordered_set<std::string> s;
    for (auto& n : a.nodes) s.insert(n.nodeID);
    return s;
}

 std::string MakeUniqueId(const BehaviorTreeAsset& a, const std::string& base) {
    auto used = CollectAllIds(a);
    std::string id = base;
    int i = 1;
    while (used.count(id)) id = base + "_" + std::to_string(i++);
    return id;
}

// ---- “is composite?” (uses your factory & IComposite) ----
 bool IsCompositeType(const std::string& typeName) {
    if (auto* n = BTFactory::Instance().Create(typeName)) {
        bool result = (dynamic_cast<IComposite*>(n) != nullptr);
        delete n;
        return result;
    }
    return false;
}

 bool HasAllowedExt(const std::string& s) {
    auto ext = std::filesystem::path(s).extension().string();
    return (ext == ".json" || ext == ".bht");
}

 std::string EnsureAllowedExt(const std::string& s, const std::string& fallbackExt){
    if (HasAllowedExt(s)) return s;
    return s + fallbackExt;
}

// Windows-safe-ish filename scrub (keeps it simple)
 std::string SanitizeFilename(std::string s) {
    static const char* bad = "\\/:*?\"<>|";
    for (char& c : s) {
        if (std::strchr(bad, c) || (unsigned char)c < 32) c = '_';
    }
    // trim spaces
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    return s;
}


 // ---- SAVE ----
 bool SaveBTAssetToFile(const std::string& fullPath, const BehaviorTreeAsset& asset)
 {
     try {
         Serializer writer(fullPath.c_str());
         if (!writer.IsOpen()) {
             CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] cannot open: " << fullPath;
             return false;
         }

         // Let the property system write everything (including arrays) correctly
         asset.Serialize(writer);

         if (!writer.SaveAndClose()) {
             CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] SaveAndClose failed: " << fullPath;
             return false;
         }
         return true;
     }
     catch (const std::exception& e) {
         CONSOLE_LOG(LEVEL_ERROR) << "[BT Save] exception: " << e.what();
         return false;
     }
 }