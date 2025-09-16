#include "BehaviourTree.h"

void BehaviorTree::initHardcoded() {
    auto* selector = new ComSelector();

    auto* failNode = new LeafFailTest();
    auto* mouseNode = new L_CheckMouseClick();
    auto* failNode2 = new LeafFailTest();
    auto* selector2 = new ComSelector();
    selector->isReady();
    selector->AddChild(failNode);
    selector->AddChild(failNode2);
    selector->AddChild(selector2);
    selector2->AddChild(mouseNode);

    rootNode = selector;
    treeName = "Testing Tree";
}

void BehaviorTree::update(float dt) {
    if (!rootNode) {
        return;
    }
    rootNode->tick(dt); // your nodes’ on_update/on_success/on_failure handle the logic

    if (rootNode->isRunning() == false)
    {
        rootNode->setStatus(NodeStatus::READY);
    }
}

