#include "BehaviourNode.h"

//agent(nullptr) deleted for now
BehaviorNode::BehaviorNode() :  status(NodeStatus::READY), result(NodeResult::IN_PROGRESS), parent(nullptr)
{
}

BehaviorNode::~BehaviorNode()
{
    for (auto&& child : children)
    {
        delete child;
    }
}


bool BehaviorNode::isReady() const
{
    return status == NodeStatus::READY;
}

bool BehaviorNode::succeeded() const
{
    return result == NodeResult::SUCCESS;
}

bool BehaviorNode::failed() const
{
    return result == NodeResult::FAILURE;
}

bool BehaviorNode::isRunning() const
{
    return status == NodeStatus::RUNNING;
}

bool BehaviorNode::isSuspended() const
{
    return status == NodeStatus::SUSPENDED;
}

void BehaviorNode::setStatus(NodeStatus newStatus)
{
    status = newStatus;
}

void BehaviorNode::setStatusAll(NodeStatus newStatus)
{
    status = newStatus;

    for (auto&& child : children)
    {
        child->setStatusAll(newStatus);
    }
}

void BehaviorNode::setStatusForChildren(NodeStatus newStatus)
{
    for (auto&& child : children)
    {
        child->setStatus(newStatus);
    }
}

void BehaviorNode::setResult(NodeResult r)
{
    result = r;
}

void BehaviorNode::setResultChildren(NodeResult result)
{
    for (auto&& child : children)
    {
        child->setResult(result);
    }
}

NodeStatus BehaviorNode::getStatus() const
{
    return status;
}

NodeResult BehaviorNode::getResult() const
{
    return result;
}

void BehaviorNode::tick(float dt)
{
    // explicitly check the statuses in order, so a node can transition fully through its states in one frame if needed

    if (status == NodeStatus::READY)
    {
        onEnter();
    }

    if (status == NodeStatus::RUNNING)
    {
        onUpdate(dt);
    }

    if (status == NodeStatus::EXITING)
    {
        onExit();
    }
}

std::string BehaviorNode::getName() const
{
    return name;
}


std::string BehaviorNode::getSummary() const
{
    return summary;
}

void BehaviorNode::onEnter()
{
    // base logic is to mark as running
    setStatus(NodeStatus::RUNNING);
    setResult(NodeResult::IN_PROGRESS);

    // and this node's children as ready to run
    setStatusForChildren(NodeStatus::READY);
    setResultChildren(NodeResult::IN_PROGRESS);
}

void BehaviorNode::onLeafEnter()
{
    setStatus(NodeStatus::RUNNING);
    setResult(NodeResult::IN_PROGRESS);
}

void BehaviorNode::onUpdate(float)
{ // will be created by each node when needed
}

void BehaviorNode::onExit()
{
    // base logic is to mark the node as done executing
    setStatus(NodeStatus::SUSPENDED);
}

void BehaviorNode::onSuccess()
{
    setStatus(NodeStatus::EXITING);
    setResult(NodeResult::SUCCESS);
}

void BehaviorNode::onFailure()
{
    setStatus(NodeStatus::EXITING);
    setResult(NodeResult::FAILURE);
}

void BehaviorNode::addChild(BehaviorNode* child)
{
    children.emplace_back(child);
    child->parent = this;
  //  child->agent = agent;
}