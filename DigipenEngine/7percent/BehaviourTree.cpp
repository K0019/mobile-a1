#include "BehaviourTree.h"

BehaviorTree::BehaviorTree()
    : rootNode{}
    , treeName{ "Unnamed" }
{
}

BehaviorTree::~BehaviorTree()
{
    delete rootNode;
}

void BehaviorTree::InitHardcoded()
{
    auto* selector = new ComSelector();

    auto* failNode = new LeafFailTest();
    auto* mouseNode = new L_CheckMouseClick();
    auto* failNode2 = new LeafFailTest();
    auto* selector2 = new ComSelector();
    selector->IsReady();
    selector->AddChild(failNode);
    selector->AddChild(failNode2);
    selector->AddChild(selector2);
    selector2->AddChild(mouseNode);

    rootNode = selector;
    treeName = "Testing Tree";
}

void BehaviorTree::Update(float dt)
{
    if (!rootNode)
        return;
    
    rootNode->Tick(dt); // your nodes’ on_update/on_success/on_failure handle the logic

    if (rootNode->IsRunning() == false)
        rootNode->SetStatus(NODE_STATUS::READY);
}

