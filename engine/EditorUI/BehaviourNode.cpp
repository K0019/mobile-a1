#include "BehaviourNode.h"
#include "BehaviourTreeFactory.h"

BT_REGISTER_NODE(Sequence, "Sequence")

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

NODE_STATUS BehaviorNode::Tick()
{
    if (status != NODE_STATUS::RUNNING)
        OnInitialize();

    status = OnUpdate();

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

Decorator::Decorator(BehaviorNode* child)
    : BehaviorNode{}
    , childPtr{ child }
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

NODE_STATUS Sequence::OnUpdate()
{
    while (currentChildItr != childrenPtr.end())
    {
        //Run the child node.
        NODE_STATUS status{ (*currentChildItr)->Tick() };
        
        //If the child returns failure or running, return the same thing.
        if (status != NODE_STATUS::SUCCESS)
            return status;

        //If the child succeeded, go to the next node.
        ++currentChildItr;
    }
    //If all the node succeeded, return success.
    return NODE_STATUS::SUCCESS;
}