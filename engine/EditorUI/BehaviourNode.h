#pragma once

enum class NODE_STATUS
{
    READY, // node is enterable
    RUNNING, // node is currently running
    SUCCESS,
    FAILURE
    //EXITING, // node has succeeded or failed
    //SUSPENDED // node won't exceute anything
};

//enum class NODE_RESULT
//{
//    IN_PROGRESS, // still being run 
//    SUCCESS, // node succeeded
//    FAILURE // node failed
//};

class BehaviorNode {
public:
    BehaviorNode();
    virtual ~BehaviorNode();

    virtual void OnInitialize() {}
    virtual NODE_STATUS OnUpdate() = 0;
    virtual void OnTerminate(NODE_STATUS) {}

    //getters setters etc
    bool IsReady() const;
    bool Succeeded() const;
    bool Failed() const;
    bool IsRunning() const;

    NODE_STATUS GetStatus() const;
    void SetStatus(NODE_STATUS newStatus);

    virtual BehaviorNode* Clone();

    NODE_STATUS Tick();

    //virtual BehaviorNode* clone() = 0;
private:
    NODE_STATUS status;
};

//Interface to handle multiple child nodes.
class CompositeNode
    : public BehaviorNode
{
public:
    void AddChild(BehaviorNode* childPtr);
    void RemoveChild(BehaviorNode* childPtr);
    void ClearChildren();
protected:
    typedef std::vector<BehaviorNode*> BehaviorNodes;
    BehaviorNodes childrenPtr;
};

class Decorator
    : public BehaviorNode
{
protected:
    BehaviorNode* childPtr;
public:
    Decorator(BehaviorNode* child);
};

class Sequence
    : public CompositeNode
{
protected:
    void OnInitialize() override;
    NODE_STATUS OnUpdate() override;
private:
    BehaviorNodes::iterator currentChildItr;
};
