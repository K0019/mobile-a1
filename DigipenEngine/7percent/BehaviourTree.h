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
class BehaviorTree {
public:
    BehaviorTree() = default;
    ~BehaviorTree() {
        delete rootNode;
    };

    void update(float dt);

    // Hardcoded init (no file/prototype)
    void initHardcoded();

private:

    BehaviorNode* rootNode = nullptr;
    std::string treeName = "Unnamed";

    // simple ownership bucket (matches raw child pointers on composites)
    std::vector<std::unique_ptr<BehaviorNode>> owned;
};

