#pragma once
#include "pch.h"
#include "GUIAsECS.h"   // <-- Required so WindowBase is known
#include "BehaviourTree.h"



bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset& out);


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

 bool CreateNewBTFile(const std::string& dir, const std::string& fileStem, std::string& outFullPath);

 bool ValidateLevelOrder(const BehaviorTreeAsset& a, std::string& check);

 std::string EnsureJsonExt(const std::string& s);            // add ".json" if missing
 bool        HasJsonExt(const std::string& s);               // ".json" (case-insensitive)

 void RefreshBTList(const std::string& dir,std::vector<std::string>& files,int& currentIndex,
	 BehaviorTreeAsset& loadedAsset,bool& hasAsset,std::string& lastLoadedPath);

 bool DeleteBTFile(const std::string& dir, const std::vector<std::string>& files,
     int currentIndex,std::string& deletedPath);

 bool LoadSelectedBT(const std::string& dir, const std::vector<std::string>& files, int currentIndex,
     BehaviorTreeAsset& out, bool& hasAsset, std::string& lastLoadedPath, int& selectedNodeIndex);