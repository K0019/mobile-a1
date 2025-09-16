#include "BehaviourNode.h"

//agent(nullptr) deleted for now
BehaviorNode::BehaviorNode()
    : status{ NODE_STATUS::READY }
    , result{ NODE_RESULT::IN_PROGRESS }
    , parent{}
{
}

BehaviorNode::~BehaviorNode()
{
    for (auto&& child : children)
    {
        delete child;
    }
}


bool BehaviorNode::IsReady() const
{
    return status == NODE_STATUS::READY;
}

bool BehaviorNode::Succeeded() const
{
    return result == NODE_RESULT::SUCCESS;
}

bool BehaviorNode::Failed() const
{
    return result == NODE_RESULT::FAILURE;
}

bool BehaviorNode::IsRunning() const
{
    return status == NODE_STATUS::RUNNING;
}

bool BehaviorNode::IsSuspended() const
{
    return status == NODE_STATUS::SUSPENDED;
}

void BehaviorNode::SetStatus(NODE_STATUS newStatus)
{
    status = newStatus;
}

void BehaviorNode::SetStatusAll(NODE_STATUS newStatus)
{
    status = newStatus;

    for (auto&& child : children)
    {
        child->SetStatusAll(newStatus);
    }
}

void BehaviorNode::SetStatusForChildren(NODE_STATUS newStatus)
{
    for (auto&& child : children)
    {
        child->SetStatus(newStatus);
    }
}

void BehaviorNode::SetResult(NODE_RESULT r)
{
    result = r;
}

void BehaviorNode::SetResultChildren(NODE_RESULT result)
{
    for (auto&& child : children)
    {
        child->SetResult(result);
    }
}

NODE_STATUS BehaviorNode::GetStatus() const
{
    return status;
}

NODE_RESULT BehaviorNode::GetResult() const
{
    return result;
}

void BehaviorNode::Tick(float dt)
{
    // explicitly check the statuses in order, so a node can transition fully through its states in one frame if needed

    if (status == NODE_STATUS::READY)
    {
        OnEnter();
    }

    if (status == NODE_STATUS::RUNNING)
    {
        OnUpdate(dt);
    }

    if (status == NODE_STATUS::EXITING)
    {
        OnExit();
    }
}

const std::string& BehaviorNode::GetName() const
{
    return name;
}


const std::string& BehaviorNode::GetSummary() const
{
    return summary;
}

void BehaviorNode::OnEnter()
{
    // base logic is to mark as running
    SetStatus(NODE_STATUS::RUNNING);
    SetResult(NODE_RESULT::IN_PROGRESS);

    // and this node's children as ready to run
    SetStatusForChildren(NODE_STATUS::READY);
    SetResultChildren(NODE_RESULT::IN_PROGRESS);
}

void BehaviorNode::OnLeafEnter()
{
    SetStatus(NODE_STATUS::RUNNING);
    SetResult(NODE_RESULT::IN_PROGRESS);
}

void BehaviorNode::OnUpdate(float)
{ // will be created by each node when needed
}

void BehaviorNode::OnExit()
{
    // base logic is to mark the node as done executing
    SetStatus(NODE_STATUS::SUSPENDED);
}

void BehaviorNode::OnSuccess()
{
    SetStatus(NODE_STATUS::EXITING);
    SetResult(NODE_RESULT::SUCCESS);
}

void BehaviorNode::OnFailure()
{
    SetStatus(NODE_STATUS::EXITING);
    SetResult(NODE_RESULT::FAILURE);
}

void BehaviorNode::AddChild(BehaviorNode* child)
{
    children.emplace_back(child);
    child->parent = this;
  //  child->agent = agent;
}