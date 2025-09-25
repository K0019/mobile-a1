#include "BehaviourNode.h"
#include "BehaviourTreeFactory.h"

BT_REGISTER_NODE(Sequence, "Sequence")
BT_REGISTER_NODE(Selector, "Selector")
BT_REGISTER_NODE(MoveLeft, "MoveLeft")
BT_REGISTER_NODE(MoveRight, "MoveRight")
BT_REGISTER_NODE(MoveUp, "MoveUp")
BT_REGISTER_NODE(MoveDown, "MoveDown")

//agent(nullptr) deleted for now
BehaviorNode::BehaviorNode()
    : status{ NODE_STATUS::READY }
{
}

BehaviorNode::~BehaviorNode()
{
}


bool BehaviorNode::IsReady() const
{
    return status == NODE_STATUS::READY;
}

bool BehaviorNode::Succeeded() const
{
    return status == NODE_STATUS::SUCCESS;
}

bool BehaviorNode::Failed() const
{
    return status == NODE_STATUS::FAILURE;
}

bool BehaviorNode::IsRunning() const
{
    return status == NODE_STATUS::RUNNING;
}

void BehaviorNode::SetStatus(NODE_STATUS newStatus)
{
    status = newStatus;
}

NODE_STATUS BehaviorNode::GetStatus() const
{
    return status;
}

NODE_STATUS BehaviorNode::Tick(ecs::EntityHandle entity)
{
    if (status != NODE_STATUS::RUNNING)
        OnInitialize();

    status = OnUpdate(entity);

    if (status != NODE_STATUS::RUNNING)
        OnTerminate(status);

    return status;
}

CompositeNode::CompositeNode()
    : BehaviorNode{}
    , childrenPtr{}
{
}

CompositeNode::~CompositeNode()
{
    RemoveChildren();
}

bool CompositeNode::AddChild(BehaviorNode* childPtr)
{
    childrenPtr.push_back(childPtr);
    return true;
}

void CompositeNode::RemoveChild(BehaviorNode* childPtr)
{
    childrenPtr.erase(std::remove(childrenPtr.begin(), childrenPtr.end(), childPtr), childrenPtr.end());
    delete childPtr;
}

void CompositeNode::RemoveChildren()
{
    for (BehaviorNode* child : childrenPtr)
    {
        child->RemoveChildren();
        delete child;
    }
    childrenPtr.clear();
}

Decorator::Decorator()
    : childPtr{ nullptr }
{
}

bool Decorator::AddChild(BehaviorNode* child)
{
    if (!childPtr)
        childPtr = child;
    else
    {
        delete childPtr;
        childPtr = child;
    }
    return true;
}

void Decorator::RemoveChildren()
{
    delete childPtr;
    childPtr = nullptr;
}

void Sequence::OnInitialize()
{
    currentChildItr = childrenPtr.begin();
}

NODE_STATUS Sequence::OnUpdate(ecs::EntityHandle entity)
{
    while (currentChildItr != childrenPtr.end())
    {
        //Run the child node.
        NODE_STATUS stat{ (*currentChildItr)->Tick(entity) };
        
        //If the child returns failure or running, return the same thing.
        if (stat != NODE_STATUS::SUCCESS)
            return stat;

        //If the child succeeded, go to the next node.
        ++currentChildItr;
    }
    //If all the node succeeded, return success.
    return NODE_STATUS::SUCCESS;
}

void Selector::OnInitialize()
{
    currentChildItr = childrenPtr.begin();
}

NODE_STATUS Selector::OnUpdate(ecs::EntityHandle entity)
{
    while (currentChildItr != childrenPtr.end())
    {
        NODE_STATUS stat{ (*currentChildItr)->Tick(entity) };
        if (stat != NODE_STATUS::FAILURE)
            return stat;
        ++currentChildItr;
    }
    return NODE_STATUS::FAILURE;
}

void MoveLeft::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveLeft::OnUpdate(ecs::EntityHandle entity)
{
    entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ -0.5f, 0.f, 0.f });
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveRight::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveRight::OnUpdate(ecs::EntityHandle entity)
{
    entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.5f, 0.f, 0.f });
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveUp::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveUp::OnUpdate(ecs::EntityHandle entity)
{
    entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, 0.5f, 0.f });
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveDown::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveDown::OnUpdate(ecs::EntityHandle entity)
{
    entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, -0.5f, 0.f });
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}