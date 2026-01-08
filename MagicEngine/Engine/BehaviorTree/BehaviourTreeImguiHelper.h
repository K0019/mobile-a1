/******************************************************************************/
/*!
\file   BehaviorTreeImguiHelper.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    This header file declares helper functions for the behavior tree imgui.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "pch.h"
#include "Editor/Containers/GUIAsECS.h"   // <-- Required so WindowBase is known
#include "BehaviourTree.h"

namespace bt {
	/*****************************************************************//*!
	\brief
		Basic types of nodes.
	*//******************************************************************/
	enum class NODE_KIND
	{
		LEAF,
		DECORATOR,
		COMPOSITE
	};

	/*****************************************************************//*!
	\brief
		Load node information from file.
	\param path
		Path of the file.
	\param out
		Reference to the BT Asset to store the information from the file.
	\return
		true if the information was successfuly recieved, else false.
	*//******************************************************************/
	bool LoadBTAssetFromFile(const std::string& path, BehaviorTreeAsset* out);

	/*****************************************************************//*!
	\brief
		Get the index of a specific level.
	\param a
		The tree asset.
	\param level
		The level to check.
	\return
		List of index of the nodes that have the level value.
	*//******************************************************************/
	std::vector<int> FindNodesAtLevel(const BehaviorTreeAsset& a, unsigned int level);

	/*****************************************************************//*!
	\brief
		Make a readable label for UI
	\param a
		The tree asset.
	\param index
		Index of the node to make a lable
	\return
		Lable for the specific node.
	*//******************************************************************/
	std::string MakeNodeLabel(const BehaviorTreeAsset& a, int index);

	/*****************************************************************//*!
	\brief
		Check if the node is a composite type node.
	\param typename
		Name of the type to check.
	\return
		true if it's a composite type node, else false.
	*//******************************************************************/
	bool IsCompositeType(const std::string& typeName);

	/*****************************************************************//*!
	\brief
		Check if the node is a decorator type node.
	\param typename
		Name of the type to check.
	\return
		true if it's a decorator type node, else false.
	*//******************************************************************/
	bool IsDecoratorType(const std::string& typeName);

	/*****************************************************************//*!
	\brief
		check if the path name has the extention .json or .bht
	\param s
		path string.
	\return
		true if it has the correct extention, else false.
	*//******************************************************************/
	bool HasAllowedExt(const std::string& s);

	/*****************************************************************//*!
	\brief
		check if the path name has the extention .json or .bht
	\param s
		path string.
	\param fallbackExt
		string to add to the file path.
	\return
		string with s and fallbackExt merged.
	*//******************************************************************/
	std::string EnsureAllowedExt(const std::string& s, const std::string& fallbackExt);
	// Windows filename scrub 
	std::string SanitizeFilename(std::string s);

	/*****************************************************************//*!
	\brief
		save a behavior tree file.
	\param path
		path of the file to save
	\param asset
		The asset to save
	\return
		true if the bt was saved successfully.
	*//******************************************************************/
	bool SaveBTAssetToFile(const std::string& path, const BehaviorTreeAsset& asset);

	/*****************************************************************//*!
	\brief
		reload a behavior tree file.
	\param dir
		path to the directory of the behavior tree file.
	\param fileStem
		stem of the file to create.
	\param outFullPath
		variable to store the full path.
	\return
		true if the bt was created successfully.
	*//******************************************************************/
	bool CreateNewBTFile(const std::string& dir, const std::string& fileStem, std::string& outFullPath);

	/*****************************************************************//*!
	\brief
		check if the level order is valid
	\param a
		asset of the behavior tree
	\param check
		The node to check.
	\return
		true if the level order is valid, else false.
	*//******************************************************************/
	bool ValidateLevelOrder(const BehaviorTreeAsset& a, std::string& check);

	/*****************************************************************//*!
	\brief
		put the json extention
	\param s
		path string.
	\return
		the string that merged s and .json
	*//******************************************************************/
	std::string EnsureJsonExt(const std::string& s);            // add ".json" if missing

	/*****************************************************************//*!
	\brief
		check if the path name has the extention .json
	\param s
		path string.
	\return
		true if it has the correct extention, else false.
	*//******************************************************************/
	bool HasJsonExt(const std::string& s);               // ".json" (case-insensitive)

	/*****************************************************************//*!
	\brief
		reload a behavior tree file.
	\param dir
		path to the directory of the behavior tree file.
	\param files
		name of all the files in the directory.
	\param currentIndex
		index of the current file.
	\param loadedAsset
		variable to put the behavior tree asset loaded.
	\param hasAsset
		variable to put the result of if the bt has assets.
	\param lastLoadedPath
		variable to put the result of the file name of the previously loaded path
	\return
		true if the bt was loaded successfully.
	*//******************************************************************/
	void RefreshBTList(const std::string& dir, std::vector<std::string>& files, int& currentIndex,
		BehaviorTreeAsset& loadedAsset, bool& hasAsset, std::string& lastLoadedPath);

	/*****************************************************************//*!
	\brief
		Delete a behavior tree file.
	\param dir
		path to the directory of the behavior tree file.
	\param files
		name of all the files in the directory.
	\param currentIndex
		index of the current file.
	\param deletedPath
		variable to store the deleted path.
	\return
		true if the bt was deleted successfully.
	*//******************************************************************/
	bool DeleteBTFile(const std::string& dir, const std::vector<std::string>& files,
		int currentIndex, std::string& deletedPath);

	/*****************************************************************//*!
	\brief
		Load a behavior tree file.
	\param dir
		path to the directory of the behavior tree file.
	\param files
		name of all the files in the directory.
	\param currentIndex
		index of the current file.
	\param out
		variable to put the behavior tree asset loaded.
	\param hasAsset
		variable to put the result of if the bt has assets.
	\param lastLoadedPath
		variable to put the result of the file name of the previously loaded path
	\param selectedNodeIndex
		variable to put the result of the selected node index.
	\return
		true if the bt was loaded successfully.
	*//******************************************************************/
	bool LoadSelectedBT(const std::string& dir, const std::vector<std::string>& files, int currentIndex,
		BehaviorTreeAsset& out, bool& hasAsset, std::string& lastLoadedPath, int& selectedNodeIndex);

	/*****************************************************************//*!
	\brief
		Get the basic node type
	\param typename
		Name of the type to check.
	\return
		Basic node type of the specific node.
	*//******************************************************************/
	NODE_KIND ClassifyNodeType(const std::string& typeName);

	/*****************************************************************//*!
	\brief
		Get the list of index of the children.
	\param nodes
		Nodes of the behavior tree.
	\param parentIdx
		index of the parent node.
	\param out
		list to put the result.
	*//******************************************************************/
	void ListDirectChildren(const std::vector<BTNodeDesc>& nodes, int parentIdx, std::vector<int>& out);

	/*****************************************************************//*!
	\brief
		Get the index of the end of the subtree.
	\param nodes
		Nodes of the behavior tree.
	\param startIdx
		start index of the subtree.
	\return
		index of the end of subtree.
	*//******************************************************************/
	int SubtreeEnd(const std::vector<BTNodeDesc>& nodes, int startIdx);

	/*****************************************************************//*!
	\brief
		Check if the node can add a child.
	\param nodes
		Nodes of the behavior tree.
	\param parentIdx
		index of the parent node.
	\return
		true if the parent can add a child node, else false.
	*//******************************************************************/
	bool CanParentAddChild(const std::vector<BTNodeDesc>& nodes, int parentIdx);

	/*****************************************************************//*!
	\brief
		Delete a specific node and the subtree of the node.
	\param nodes
		Nodes of the behavior tree.
	\param startIdx
		index of the node to delete.
	\param selectedIndex
		index of the parent node.
	\return
		true if it's a successfuly deleted, else false.
	*//******************************************************************/
	bool DeleteNodeAndSubtree(std::vector<BTNodeDesc>& nodes, int startIdx, int& selectedIndex);

	/*****************************************************************//*!
	\brief
		Count the number of root nodes.
	\param nodes
		Nodes of the behavior tree.
	\return
		number of root nodes in the behavior tree.
	*//******************************************************************/
	int CountRootNodes(const std::vector<BTNodeDesc>& nodes);
}
