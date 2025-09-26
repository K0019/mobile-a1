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

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "BehaviourNode.h"
#include "IGameComponentCallbacks.h"

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
        Cleans up and destroys the behavior tree’s resources.
    *//******************************************************************/
    void Destroy();

    /*****************************************************************//*!
    \brief
        Renders the behavior tree in the editor UI
    *//******************************************************************/
    void EditorDraw();

private:
    ecs::EntityHandle entity;   //entity the tree is bound to
    BehaviorNode* rootNode;     //starting node
    std::string btName;         //tree name

public:
    property_vtable()
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
#ifdef IMGUI_ENABLED
    , public IEditorComponent<BehaviorTreeComp>
#endif
    , public IGameComponentCallbacks<BehaviorTreeComp>
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    BehaviorTreeComp();

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
