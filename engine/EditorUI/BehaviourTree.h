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
#include "BehaviourNode.h"

#include "LeafFailTest.h"
#include "LeafKeyPressed.h"
#include "ComSelector.h"
#include "BehaviourTreeFactory.h"

class BehaviorTree {
public:
    BehaviorTree();
    ~BehaviorTree();

    void Update(float dt);

    // Hardcoded init (no file/prototype)
    void InitHardcoded();

    void InitFromAsset(const struct BehaviorTreeAsset& asset); // build runtime

    void TestInitFromAsset();

private:

    BehaviorNode* rootNode = nullptr;
    std::string treeName;

    // simple ownership bucket (matches raw child pointers on composites)
    std::vector<std::unique_ptr<BehaviorNode>> owned;
};

//For Properties storing of data =======================================
struct BTNodeDesc : public ISerializeable {
    std::string nodeID;
    std::string nodeType;
    std::vector<std::string> children; // references to children IDs
    std::vector<std::string> keys;
    std::vector<std::string> values;

    property_vtable()
};

property_begin(BTNodeDesc)
{
    property_var(nodeID),
    property_var(nodeType),
    property_var(children),
    property_var(keys),
    property_var(values),

}
property_vend_h(BTNodeDesc)

struct BehaviorTreeAsset : public ISerializeable {
    std::string name;
    std::string rootID;
    std::vector<BTNodeDesc> nodes;
    property_vtable()
};
property_begin(BehaviorTreeAsset)
{
        property_var(name),
        property_var(rootID),
        property_var(nodes)
}
property_vend_h(BehaviorTreeAsset)
//=======================================================================

//FOR TESTING

