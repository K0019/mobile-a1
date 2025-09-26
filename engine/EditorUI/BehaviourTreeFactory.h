/******************************************************************************/
/*!
\file   BehaviourTreeFactory.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Takumi Shibamoto (30%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\author Hong Tze Keat (70%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This is the header file that Defines the BTFactory class 
      for registering and creating behavior tree nodes at runtime.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/


#pragma once
#include <string>
#include <vector>
#include <unordered_map>
class BehaviorNode;


/*****************************************************************//*!
\brief
     Factory for registering and creating behavior tree nodes
*//******************************************************************/
class BTFactory {
public:
    using Maker = BehaviorNode * (*)();

    /*****************************************************************//*!
    \brief
        Constructor and Destructor
    *//******************************************************************/
    BTFactory();
    ~BTFactory();

    /*****************************************************************//*!
    \brief
        Register the node types.
    \param type
        The name of the node type to create.
    \param fn
        The function pointer to create the node with that type.
    \return
        True if successfully inserted, false if key already existed.
    *//******************************************************************/
    bool Register(const std::string& type, Maker fn);   // for both C++ + C# in the future

    /*****************************************************************//*!
    \brief
        Create a node of the given type name.
    \param type
        The name of the node type to create.
    \return
        A node pointer of the given type.
    *//******************************************************************/
    BehaviorNode* Create(const std::string& type) const;

    /*****************************************************************//*!
    \brief
        Get a list of all registered type names
    \return
        A vector that contains multiple strings that represent the names
        of the registered type.
    *//******************************************************************/
    std::vector<std::string> RegisteredTypes() const;

    /*****************************************************************//*!
    \brief
        Set a behavior tree name and corresponding file name.
    *//******************************************************************/
    void SetAllFilePath();

    /*****************************************************************//*!
    \brief
        Get the file name of the tree name.
    \param btName
        name of the behavior tree.
    \return
        file name of the behavior tree if it exist and empty string if not.
    *//******************************************************************/
    const std::string& GetFilePath(const std::string& btName) const;

    /*****************************************************************//*!
    \brief
        Get a list of behavior tree names.
    \param out
        vector to put all the names.
    *//******************************************************************/
    void GetAllBTNames(std::vector<std::string>& out) const;

    /*****************************************************************//*!
    \brief
        Clear all registered types (empties the registry)
        After this, no nodes can be created until they are registered again
    *//******************************************************************/
    void Clear();   
private:
    using NodeTypes = std::unordered_map<std::string, Maker>;
    NodeTypes nodeTypes;
    std::unordered_map<std::string, std::string> filePaths;
};

/*****************************************************************//*!
\brief
    Macro to register a node type with the BTFactory.
    Place this macro in the .cpp file of each node to register it.
\param
    NodeType        C++ class type of the node.
\param
    TypeNameString  String identifier to refer to the node type.

*//******************************************************************/
#define BT_REGISTER_NODE(NodeType, TypeNameString) \
    namespace { \
        static BehaviorNode* __bt_make_##NodeType() { return new NodeType(); } \
        struct __bt_reg_##NodeType { \
            __bt_reg_##NodeType() { \
                ST<BTFactory>::Get()->Register(TypeNameString, &__bt_make_##NodeType); \
            } \
        } __bt_reg_instance_##NodeType; \
    }