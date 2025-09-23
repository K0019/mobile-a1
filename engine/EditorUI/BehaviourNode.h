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

    NODE_STATUS Tick();

    virtual bool AddChild(BehaviorNode* childPtr) { return false; }
    virtual void RemoveChildren() {}
private:
    NODE_STATUS status;
};

//Interface to handle multiple child nodes.
class CompositeNode
    : public BehaviorNode
{
public:
    CompositeNode();
    ~CompositeNode();

    bool AddChild(BehaviorNode* childPtr) override;
    void RemoveChild(BehaviorNode* childPtr);
    void RemoveChildren() override;
protected:
    typedef std::vector<BehaviorNode*> BehaviorNodes;
    BehaviorNodes childrenPtr;
};

class Decorator
    : public BehaviorNode
{
public:
    Decorator();
    bool AddChild(BehaviorNode* child) override;
    void RemoveChildren() override;

protected:
    BehaviorNode* childPtr;
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
