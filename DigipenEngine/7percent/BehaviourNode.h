#pragma once

enum class NodeStatus
{
    READY, // node is enterable
    RUNNING, // node is currently running
    EXITING, // node has succeeded or failed
    SUSPENDED // node won't exceute anything
};

enum class NodeResult
{
    IN_PROGRESS, // still being run 
    SUCCESS, // node succeeded
    FAILURE // node failed
};

class BehaviorNode {
public:
    BehaviorNode();
    ~BehaviorNode();

    //getters setters etc
    bool isReady() const;
    bool succeeded() const;
    bool failed() const;
    bool isRunning() const;
    bool isSuspended() const;

    void setStatus(NodeStatus newStatus);

    void setStatusAll(NodeStatus newStatus); // set this node and all childrens' status, recursively

    void setStatusForChildren(NodeStatus newStatus);     // set only the direct children's status

    void setResult(NodeResult result);

    void setResultChildren(NodeResult result);

    NodeStatus getStatus() const;

    NodeResult getResult() const;

    void tick(float dt);

    std::string getName() const;
    std::string getSummary() const;

    //virtual BehaviorNode* clone() = 0;
protected:
   // BehaviorAgent* agent;
    NodeStatus status;
    NodeResult result;
    BehaviorNode* parent;
    std::vector<BehaviorNode*> children;

    void onLeafEnter();

    // override for any non-generic logic
    virtual void onEnter();
    virtual void onUpdate(float dt);
    virtual void onExit();

    // convenience functions for setting status and result
    void onSuccess();
    void onFailure();
    void addChild(BehaviorNode* child);

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
    virtual BehaviorNode* clone()
    {
        T& self = *static_cast<T*>(this);
        return new T(self);
    }
};