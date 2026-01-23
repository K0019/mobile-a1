/******************************************************************************/
/*!
\file   BehaviourTree.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Takumi Shibamoto (60%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\author Hong Tze Keat (40%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This is the header file that contains the declaration of the BehaviorTree
      class.

All content  2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Engine/BehaviorTree/BehaviourNode.h"
#include "Utilities/Serializer.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "Game/IGameComponentCallbacks.h"


class BlackBoard;

 /*****************************************************************//*!
     \brief
         Represents a behavior tree instance bound to an entity.
         Provides lifecycle functions for setup, update, and destruct.
 *//******************************************************************/
class BehaviorTree
    : public ISerializeable
{
public:

    /*****************************************************************//*!
    \brief
        Constructor and Destructor of trees
    *//******************************************************************/
    BehaviorTree();
    ~BehaviorTree();

    /*****************************************************************//*!
    \brief
        Assigns the behavior tree to a given entity.
    \param
        entityHandle The entity handle to bind the tree to
    *//******************************************************************/
    void Set(ecs::EntityHandle entityHandle);

    /*****************************************************************//*!
    \brief
        Executes an update tick on the behavior tree.
    *//******************************************************************/
    void Update();

    /*****************************************************************//*!
    \brief
        Cleans up and destroys the behavior trees resources.
    *//******************************************************************/
    void Destroy();

    /*****************************************************************//*!
    \brief
        Renders the behavior tree in the editor UI
    *//******************************************************************/
    void EditorDraw();

    void SetName(std::string n) { btName = std::move(n); }
    const std::string& GetName() const { return btName; }

    static BlackBoard globalBlackBoard;
private:
    ecs::EntityHandle entity;   //entity the tree is bound to
    BehaviorNode* rootNode;     //starting node
    std::string btName;         //tree name


public:
    property_vtable()
};

class BlackBoard
{
public:
    BlackBoard();

    template <typename T>
    void SetValue(const std::string& key, const T& value)
    {
        data[key] = value;
    }

    template <typename T>
    bool GetValue(const std::string& key, T& outValue)
    {
        auto it = data.find(key);
        if (it != data.end())
        {
            try
            {
                outValue = std::any_cast<T>(it->second);
                return true;
            }
            catch (const std::bad_any_cast& e)
            {
                return false;
            }
        }
        return false;
    }

    bool HasKey(const std::string& key);

    void Clear();

private:
    std::map<std::string, std::any> data;
};

//=======================================================================
// For Properties storing of data
//=======================================================================

property_begin(BehaviorTree)
{
    property_var(btName)
}
property_vend_h(BehaviorTree)

struct BTNodeDesc 
    : public ISerializeable 
{
    std::string nodeType;
    unsigned int nodeLevel; //depth lvl starting from 0

    property_vtable()
};
property_begin(BTNodeDesc)
{
    property_var(nodeType),
    property_var(nodeLevel),
}
property_vend_h(BTNodeDesc)

/*****************************************************************//*!
    \brief
        Serializable asset representing a behavior tree 
        definition with its nodes.
*//******************************************************************/
struct BehaviorTreeAsset 
    : public ISerializeable 
{
    std::string name;
    std::vector<BTNodeDesc> nodes;
    property_vtable()
};
property_begin(BehaviorTreeAsset)
{
    property_var(name),
    property_var(nodes)
}
property_vend_h(BehaviorTreeAsset)
//=======================================================================



//FOR TESTING
/*****************************************************************//*!
    \brief
        ECS component wrapper for a BehaviorTree instance
        Provides lifecycle callbacks and editor drawing integration
*//******************************************************************/
class BehaviorTreeComp
    : public IRegisteredComponent<BehaviorTreeComp>
    , public IEditorComponent<BehaviorTreeComp>
    , public IGameComponentCallbacks<BehaviorTreeComp>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    BehaviorTreeComp();

	BehaviorTree& GetBehaviorTree() { return behaviorTree; }

private:
    BehaviorTree behaviorTree;

public:
    /*****************************************************************//*!
    \brief
        Called when the component is attached to an entity.
    *//******************************************************************/
    void OnAttached() override;

    void OnStart() override;

    /*****************************************************************//*!
    \brief
        Called when the component is detached from an entity.
    *//******************************************************************/
    void OnDetached() override;
    /*****************************************************************//*!
    \brief
         Updates the behavior tree each tick.
    *//******************************************************************/
    void Update();

private:
    virtual void EditorDraw() override;

public:
    property_vtable()
};
property_begin(BehaviorTreeComp)
{
    property_var(behaviorTree)
}
property_vend_h(BehaviorTreeComp)


/*****************************************************************//*!
\brief
      ECS system responsible for updating all BehaviorTreeComp
*//******************************************************************/
class BehaviorTreeSystem
    : public ecs::System<BehaviorTreeSystem, BehaviorTreeComp>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    BehaviorTreeSystem();

    void OnAdded() override;

private:
    /*****************************************************************//*!
    \brief
        Updates a single BehaviorTreeComp instance.
    \param
        The component to update
    *//******************************************************************/
    void UpdateComp(BehaviorTreeComp& comp);
};
//=======================================================================
