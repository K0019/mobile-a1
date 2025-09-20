#pragma once

enum class NODE_STATUS
{
    READY, // node is enterable
    RUNNING, // node is currently running
    EXITING, // node has succeeded or failed
    SUSPENDED // node won't exceute anything
};

enum class NODE_RESULT
{
    IN_PROGRESS, // still being run 
    SUCCESS, // node succeeded
    FAILURE // node failed
};

class BehaviorNode {
public:
    BehaviorNode();
    virtual ~BehaviorNode();

    //getters setters etc
    bool IsReady() const;
    bool Succeeded() const;
    bool Failed() const;
    bool IsRunning() const;
    bool IsSuspended() const;

    void SetStatus(NODE_STATUS newStatus);

    void SetStatusAll(NODE_STATUS newStatus); // set this node and all childrens' status, recursively

    void SetStatusForChildren(NODE_STATUS newStatus);     // set only the direct children's status

    void SetResult(NODE_RESULT result);

    void SetResultChildren(NODE_RESULT result);

    NODE_STATUS GetStatus() const;

    NODE_RESULT GetResult() const;

    void Tick(float dt);

    const std::string& GetName() const;
    const std::string& GetSummary() const;

    //virtual BehaviorNode* clone() = 0;
protected:
   // BehaviorAgent* agent;
    NODE_STATUS status;
    NODE_RESULT result;
    BehaviorNode* parent;
    std::vector<BehaviorNode*> children;

    void OnLeafEnter();

    // override for any non-generic logic
    virtual void OnEnter();
    virtual void OnUpdate(float dt);
    virtual void OnExit();

    // convenience functions for setting status and result
    void OnSuccess();
    void OnFailure();
    void AddChild(BehaviorNode* child);

    //void displayLeafText(); //may remove later

private:
    std::string name;
    std::string summary;

   // void setDebugInfo(std::string name, std::string summary);

};

template <typename T>
class BaseNode : public BehaviorNode
{
public:
    virtual BehaviorNode* Clone();
};

// A small interface so the builder can acknowledge this type of nodes can have children
class IComposite {
public:
    virtual ~IComposite() = default;
    virtual void AddChild(BehaviorNode* child) = 0;
};

template <typename T>
class CompositeNode : public BaseNode<T>, public IComposite {
public:
    void AddChild(BehaviorNode* child) override {
        BehaviorNode::AddChild(child);   
    }
};
#pragma region Definition

template<typename T>
inline BehaviorNode* BaseNode<T>::Clone()
{
    T& self = *static_cast<T*>(this);
    return new T(self);
}

#pragma endregion // Definition
