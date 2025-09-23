#pragma once
#include "pch.h"
#include "GUIAsECS.h"   // <-- Required so WindowBase is known
#include "BehaviourTree.h"
//
//struct BTEditorNode {
//    int id;                           // editor ID
//    std::string type;                 // factory type string e.g. "ComSelector", "LeafFailTest"
//    std::string displayName;          // nice name shown in UI (editable)
//    std::vector<int> children;        // editor IDs of children
//    // later: params as key/value string pairs if you want
//    // std::vector<std::pair<std::string,std::string>> params;
//};
//
//struct BehaviourTreeEditorModel {
//    int nextId = 1;
//    int rootId = -1;
//    int selectedId = -1;
//
//    std::vector<BTEditorNode> nodes;
//
//    int indexOf(int id) const;
//    int addNode(const std::string& type, const std::string& displayName);
//
//    void eraseSubtree(int id);
//    bool canAdopt(int parentId, int childId) const;
//};
//
//BehaviorTreeAsset MakeAssetFromEditor(const BehaviourTreeEditorModel& m, const std::string& assetName);
//
//void FillEditorFromAsset(BehaviourTreeEditorModel& m, const BehaviorTreeAsset& a);
//
//
////For FILE IO
//
//std::vector<std::string> ScanDirForTrees(const std::string& dir);
//bool LoadAssetFromFile(const std::string& fullPath, BehaviorTreeAsset& out);
//
//bool SaveAssetToFile(const std::string& fullPath, const BehaviorTreeAsset& asset);
//
//
//// Node list to see registered nodes
//struct NodesRegistered {
//    std::vector<std::string> types; // e.g. "ComSelector", "LeafFailTest", ...
//    void Refresh();
//};


bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset& out);

 //int FindNodeIndexById(const BehaviorTreeAsset& a, const std::string& id);

 //std::unordered_set<std::string> CollectAllIds(const BehaviorTreeAsset& a);

 //std::string MakeUniqueId(const BehaviorTreeAsset& a, const std::string& base);

// Find indices of nodes at a specific level (since asset is level-based now)
std::vector<int> FindNodesAtLevel(const BehaviorTreeAsset& a, unsigned int level);

// Make a readable label for UI since nodeID is gone (type + index)
std::string MakeNodeLabel(const BehaviorTreeAsset& a, int index);

 bool IsCompositeType(const std::string& typeName);

 bool IsDecoratorType(const std::string& typeName);

 bool HasAllowedExt(const std::string& s);

 std::string EnsureAllowedExt(const std::string& s, const std::string& fallbackExt);
// Windows filename scrub 
 std::string SanitizeFilename(std::string s);

 bool SaveBTAssetToFile(const std::string& path, const BehaviorTreeAsset& asset);

 bool ValidateLevelOrder(const BehaviorTreeAsset& a, std::string& check);