/******************************************************************************/
/*!
\file   BehaviourTree.cpp
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
      This is the source file that contains the definition of the BehaviorTree
      class.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "BehaviourTree.h"
#include "BehaviourTreeImguiHelper.h"
#include "BehaviourTreeFactory.h"


BehaviorTree::BehaviorTree()
    : entity{ nullptr }
    , rootNode{ nullptr }
    , btName{}
{
}

BehaviorTree::~BehaviorTree()
{
    rootNode = nullptr;
}

void BehaviorTree::Set(ecs::EntityHandle entityHandle)
{
    Destroy();

    //Set the entity handle
    entity = entityHandle;

    //List of parent nodes.
    std::vector<std::unique_ptr<BehaviorNode*>> parents{};

    //All the data of the tree.
    std::string fileName{ ST<Filepaths>::Get()->behaviourTreeSave + "/" + btName + ".json"};
    BehaviorTreeAsset btAsset{};
    if (!LoadBTAssetFromFile(fileName, &btAsset))
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
            CONSOLE_LOG(LEVEL_ERROR) << "The Level of " << nodeType << " in the behavior tree " << btName << " is Invalid.";
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
    
    rootNode->Tick(entity); // the nodes will handle the logic within
}

void BehaviorTree::Destroy()
{
    if (rootNode)
    {
        rootNode->RemoveChildren();
        delete rootNode;
        rootNode = nullptr;
    }
}

void BehaviorTree::EditorDraw()
{
    std::vector<std::string> btNames{};
    ST<BTFactory>::Get()->GetAllBTNames(&btNames);
    std::string currStr{ std::find(btNames.begin(), btNames.end(), btName) == btNames.end() ? "Invalid" : btName };
    if (ImGui::BeginCombo("##bt_file_combo", currStr.c_str(), ImGuiComboFlags_WidthFitPreview))
    {
        for (std::size_t index{}; index < btNames.size(); ++index) 
        {
            bool sel = (btNames[index] == currStr);
            if (ImGui::Selectable(btNames[index].c_str(), sel))
            {
                btName = btNames[index];
                Set(entity);
            }
            if (sel) 
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
}

BehaviorTreeComp::BehaviorTreeComp()
    : behaviorTree{}
{
}

void BehaviorTreeComp::OnAttached()
{
    IGameComponentCallbacks::OnAttached();
}

void BehaviorTreeComp::OnStart()
{
    behaviorTree.Set(ecs::GetEntity(this));
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
    behaviorTree.EditorDraw();
}

BehaviorTreeSystem::BehaviorTreeSystem()
    : System_Internal{&BehaviorTreeSystem::UpdateComp}
{
}

void BehaviorTreeSystem::UpdateComp(BehaviorTreeComp& comp)
{
    comp.Update();
}
