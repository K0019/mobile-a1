#include "BehaviourTree.h"
#include "BehaviourTreeImguiHelper.h"
#include "BehaviourTreeFactory.h"


BehaviorTree::BehaviorTree()
    : rootNode{}
    , entity{nullptr}
{
}

BehaviorTree::~BehaviorTree()
{
    rootNode = nullptr;
}

void BehaviorTree::Set(std::string treeName, ecs::EntityHandle entityHandle)
{
    //Set the entity handle
    entity = entityHandle;

    //List of parent nodes.
    std::vector<std::unique_ptr<BehaviorNode*>> parents{};

    //All the data of the tree.
    std::string fileName{ ST<Filepaths>::Get()->behaviourTreeSave + "/" + treeName + ".json"};
    BehaviorTreeAsset btAsset{};
    if (!LoadBTAssetFromFile(fileName, btAsset))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "behavior tree file could not be loaded.";
        return;
    }

    //Create and Set the root node.
    BehaviorNode* root{ ST<BTFactory>::Get()->Create(btAsset.nodes[0].nodeType) };
    parents.push_back(std::make_unique<BehaviorNode*>(root));
    rootNode = root;

    //Iterate through all the node in the tree.
    for (std::size_t index{1}; index < btAsset.nodes.size(); ++index)
    {
        //Create the node.
        std::string nodeType{ btAsset.nodes[index].nodeType };
        unsigned int nodeLevel{ btAsset.nodes[index].nodeLevel };
        BehaviorNode* node{ ST<BTFactory>::Get()->Create(nodeType) };

        //If the level is invalid, print an error message.
        if (parents.size() < nodeLevel)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "The Level of " << nodeType << " in the behavior tree " << treeName << " is Invalid.";
            return;
        }

        //Set the node as a child.
        //If the child couldn't be added, print an error message.
        if ((*(parents[nodeLevel - 1]))->AddChild(node) == false)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Unable to add a child " << nodeType;
            return;
        }

        //Set the node as a parent.
        if (parents.size() == nodeLevel)
            parents.push_back(std::make_unique<BehaviorNode*>(node));
        else
            parents[nodeLevel] = std::make_unique<BehaviorNode*>(node);
    }
}

void BehaviorTree::Update()
{
    if (!rootNode)
        return;
    
    rootNode->Tick(); // the nodes will handle the logic within
}

void BehaviorTree::Destroy()
{
    rootNode->RemoveChildren();
    delete rootNode;
    rootNode = nullptr;
}

BehaviorTreeComp::BehaviorTreeComp()
    : behaviorTree{}
{
}

void BehaviorTreeComp::OnAttached()
{
    behaviorTree.Set("testNestedTree", ecs::GetEntity(this));
}

void BehaviorTreeComp::OnDetached()
{
    behaviorTree.Destroy();
}

void BehaviorTreeComp::Update()
{
    behaviorTree.Update();
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
