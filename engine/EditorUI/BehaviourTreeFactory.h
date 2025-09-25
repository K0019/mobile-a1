#pragma once
#include <string>
#include <vector>
#include <unordered_map>
class BehaviorNode;

class BTFactory {
public:
    using Maker = BehaviorNode * (*)();

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

 //Registration macro (put this in each node’s .cpp)
#define BT_REGISTER_NODE(NodeType, TypeNameString) \
    namespace { \
        static BehaviorNode* __bt_make_##NodeType() { return new NodeType(); } \
        struct __bt_reg_##NodeType { \
            __bt_reg_##NodeType() { \
                ST<BTFactory>::Get()->Register(TypeNameString, &__bt_make_##NodeType); \
            } \
        } __bt_reg_instance_##NodeType; \
    }