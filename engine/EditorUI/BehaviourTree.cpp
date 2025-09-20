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

void BehaviorTree::InitFromAsset(const BehaviorTreeAsset& asset)
{
    owned.clear();
    rootNode = nullptr;
    treeName = asset.name;

    std::unordered_map<std::string, BehaviorNode*> created;
    created.reserve(asset.nodes.size());
    owned.reserve(asset.nodes.size());

    // 1) Create every node by type
    for (const auto& nd : asset.nodes)
    {
        if (created.count(nd.nodeID)) {
            std::cerr << "[BT] Duplicate nodeID: " << nd.nodeID << "\n";
            continue;
        }

        BehaviorNode* n = BTFactory::Instance().Create(nd.nodeType);
        if (!n) {
            std::cerr << "[BT] Unknown nodeType: " << nd.nodeType
                << " (missing BT_REGISTER_NODE in that node’s .cpp?)\n";
            continue;
        }

        owned.emplace_back(n);              // take ownership
        created.emplace(nd.nodeID, n);      // map id -> instance

        // (optional) apply params (keys/values) to the node here
        // for (size_t i = 0; i < nd.keys.size() && i < nd.values.size(); ++i) {
        //     applyParamToNode(n, nd.keys[i], nd.values[i]);
        // }
    }

    // 2) Wire up children (only composites can accept children)
    for (const auto& nd : asset.nodes)
    {
        auto itP = created.find(nd.nodeID);
        if (itP == created.end()) continue;      // was unknown/duplicate

        BehaviorNode* parent = itP->second;

        // can this node have children?
        if (auto* comp = dynamic_cast<IComposite*>(parent)) {
            for (const auto& cid : nd.children) {
                auto itC = created.find(cid);
                if (itC != created.end()) {
                    comp->AddChild(itC->second); // public composite API
                }
                else {
                    std::cerr << "[BT] Missing child '" << cid
                        << "' for parent '" << nd.nodeID << "'\n";
                }
            }
        }
        else {
            if (!nd.children.empty()) {
                std::cerr << "[BT] Node '" << nd.nodeID << "' (" << nd.nodeType
                    << ") is not composite but has children in asset\n";
            }
        }
    }

    // 3) Set root
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

    BehaviorTreeAsset asset;
    asset.name = "TestingTree";
    asset.rootID = "root";

    // Root selector
    BTNodeDesc root;
    root.nodeID = "root";
    root.nodeType = "ComSelector";
    root.children = { "fail1", "fail2", "sel2" };

    // Two fail leaves
    BTNodeDesc fail1;
    fail1.nodeID = "fail1";
    fail1.nodeType = "LeafFailTest";

    BTNodeDesc fail2;
    fail2.nodeID = "fail2";
    fail2.nodeType = "LeafFailTest";

    // Second selector with mouse node
    BTNodeDesc sel2;
    sel2.nodeID = "sel2";
    sel2.nodeType = "ComSelector";
    sel2.children = { "mouse" };

    BTNodeDesc mouse;
    mouse.nodeID = "mouse";
    mouse.nodeType = "L_CheckMouseClick";

    // Collect them into asset
    asset.nodes = { root, fail1, fail2, sel2, mouse };

    // Build runtime tree
    BehaviorTree tree;
    tree.InitFromAsset(asset);

}