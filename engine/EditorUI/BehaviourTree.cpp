#include "BehaviourTree.h"


BehaviorTree::BehaviorTree()
    : rootNode{}
    , treeName{ "Unnamed" }
{
}

BehaviorTree::~BehaviorTree()
{
    //delete rootNode;
    //rootNode = nullptr;
    rootNode = nullptr;
    owned.clear();  // destroys all nodes safely

}


void BehaviorTree::InitHardcoded()
{
    //owned.clear();
    //rootNode = nullptr;
    //treeName = "Testing Tree";

    //auto* selector = new ComSelector();
    //auto* failNode = new LeafFailTest();
    //auto* mouseNode = new L_CheckMouseClick();
    //auto* failNode2 = new LeafFailTest();
    //auto* selector2 = new ComSelector();

    //// take ownership first
    //owned.emplace_back(selector);
    //owned.emplace_back(failNode);
    //owned.emplace_back(mouseNode);
    //owned.emplace_back(failNode2);
    //owned.emplace_back(selector2);

    //// then wire
    //selector->AddChild(failNode);
    //selector->AddChild(failNode2);
    //selector->AddChild(selector2);
    //selector2->AddChild(mouseNode);

    //rootNode = selector;
}


void BehaviorTree::Update(float dt)
{
    if (!rootNode)
        return;
    
    rootNode->Tick(dt); // the nodes will handle the logic within

    if (rootNode->IsRunning() == false) {
        rootNode->SetStatus(NODE_STATUS::READY);
    }
}

void BehaviorTree::InitFromAsset(const BehaviorTreeAsset& asset)
{
    //Reset data
    owned.clear();
    rootNode = nullptr;
    treeName = asset.name;

    // Temp table to wire nodes by ID
    std::unordered_map<std::string, BehaviorNode*> created;
    created.reserve(asset.nodes.size());

    // 1) create and take ownership
    for (const auto& nd : asset.nodes) {
        if (BehaviorNode* n = BTFactory::Instance().Create(nd.nodeType)) {
            owned.emplace_back(n);         // takes ownership
            created.emplace(nd.nodeID, n); // map ID to raw pointer for wiring
        }
        else {
            CONSOLE_LOG(LEVEL_ERROR) << "[BT] Unknown nodeType: " << nd.nodeType << "\n";
        }
    }

    // Wire Children ( only for composite nodes)
    for (const auto& nd : asset.nodes) {
        auto itP = created.find(nd.nodeID);
        if (itP == created.end()) {
            continue;
        }
        BehaviorNode* parent = itP->second;

        //ensure is a composite node
        if (auto* comp = dynamic_cast<IComposite*>(parent)) {
            for (const auto& cid : nd.children) {
                //connect each child to the parent
                auto itC = created.find(cid);
                if (itC != created.end()) comp->AddChild(itC->second);
                else CONSOLE_LOG(LEVEL_ERROR) << "[BT] Missing child '" << cid
                    << "' for parent '" << nd.nodeID << "'\n";
            }
        }
        else if (!nd.children.empty()) {
            // Leaf nand Decorator should not declare children
            CONSOLE_LOG(LEVEL_ERROR) << "[BT] Node '" << nd.nodeID << "' (" << nd.nodeType << ") cannot have children\n";
        }
    }

    // Pick the root node for entry
    if (auto itR = created.find(asset.rootID); itR != created.end()) {
        rootNode = itR->second;
    }
    else {
        std::cerr << "[BT] rootID not found: " << asset.rootID << "\n";
    }
}

void BehaviorTree::TestInitFromAsset(){
    // Build an asset representing:
// root: Selector -> [LeafFailTest, LeafFailTest, Selector2]
// Selector2 -> [L_CheckMouseClick]

    //BehaviorTreeAsset asset;
    //asset.name = "TestingTree";
    //asset.rootID = "root";

    //// Root selector
    //BTNodeDesc root;
    //root.nodeID = "root";
    //root.nodeType = "ComSelector";
    //root.children = { "fail1", "fail2", "sel2" };

    //// Two fail leaves
    //BTNodeDesc fail1;
    //fail1.nodeID = "fail1";
    //fail1.nodeType = "LeafFailTest";

    //BTNodeDesc fail2;
    //fail2.nodeID = "fail2";
    //fail2.nodeType = "LeafFailTest";

    //// Second selector with mouse node
    //BTNodeDesc sel2;
    //sel2.nodeID = "sel2";
    //sel2.nodeType = "ComSelector";
    //sel2.children = { "mouse" };

    //BTNodeDesc mouse;
    //mouse.nodeID = "mouse";
    //mouse.nodeType = "L_CheckMouseClick";

    //// Collect them into asset
    //asset.nodes = { root, fail1, fail2, sel2, mouse };

    //// Build runtime tree ON THIS INSTANCE
    //InitFromAsset(asset);

}

BehaviorTreeComp::BehaviorTreeComp()
    : behaviorTree{}
{
}

void BehaviorTreeComp::OnAttached()
{
    behaviorTree.InitHardcoded();
}

void BehaviorTreeComp::OnDetached()
{

}

void BehaviorTreeComp::Update()
{
    behaviorTree.Update((GameTime::IsFixedDtMode() ? GameTime::FixedDt() : GameTime::RealDt()));
}

void BehaviorTreeComp::EditorDraw()
{

}

BehaviorTreeSystem::BehaviorTreeSystem()
    : System_Internal{&BehaviorTreeSystem::UpdateComp}
{
}

void BehaviorTreeSystem::UpdateComp(BehaviorTreeComp& comp)
{
    comp.Update();
}
