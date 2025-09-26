/*
NOTE:

To do:
Serialisation (Read,Write) of the files
Figure out how to activate the AI (was thinking more of calling AI >typical major System like physics >etc...
really not sure how to begin coding or testing without c# tho need ask bossman
Blackboard
Integrate with ECS?
Figure out how to register nodes DYNAMICALLY (thinking of making a unique filetype for this but inside is just c#)

Decide if i want fixed decorator and composite nodes so user can only create leaf nodes

imgui:
create interface ( roughly up but all just static and fake)
add others for node and allow to choose which c# file to use
allow load and save of behavior tree files
Check if tree is valid (least priority)



*/



#pragma once
#include <string>
#include <unordered_map>
#include <variant>
#include <iostream>
#include "ECS.h"
#include "BehaviourNode.h"

#include "LeafFailTest.h"
#include "LeafKeyPressed.h"
#include "BehaviourTreeFactory.h"

class BehaviorTree
    : public ISerializeable
{
public:
    BehaviorTree();
    ~BehaviorTree();

    void Set(ecs::EntityHandle entityHandle);
    void Update();
    void Destroy();

    void EditorDraw();
private:
    ecs::EntityHandle entity;
    BehaviorNode* rootNode;
    std::string btName;

public:
    property_vtable()
};
property_begin(BehaviorTree)
{
    property_var(btName)
}
property_vend_h(BehaviorTree)

//For Properties storing of data =======================================
struct BTNodeDesc 
    : public ISerializeable 
{
    std::string nodeType;
    unsigned int nodeLevel;

    property_vtable()
};
property_begin(BTNodeDesc)
{
    property_var(nodeType),
    property_var(nodeLevel),
}
property_vend_h(BTNodeDesc)

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

class BehaviorTreeComp
    : public IRegisteredComponent<BehaviorTreeComp>
#ifdef IMGUI_ENABLED
    , public IEditorComponent<BehaviorTreeComp>
#endif
    , public ecs::IComponentCallbacks
{
public:
    BehaviorTreeComp();

private:
    BehaviorTree behaviorTree;

public:
    void OnAttached() override;
    void OnDetached() override;
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

class BehaviorTreeSystem
    : public ecs::System<BehaviorTreeSystem, BehaviorTreeComp>
{
public:
    BehaviorTreeSystem();

private:
    void UpdateComp(BehaviorTreeComp& comp);
};
//=======================================================================
