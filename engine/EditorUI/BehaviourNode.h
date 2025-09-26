

#pragma once

enum class NODE_STATUS
{
    READY, // node is enterable
    RUNNING, // node is currently running
    SUCCESS, // return state success
    FAILURE // return state success
};

class BehaviorNode {
public:

    BehaviorNode();
    virtual ~BehaviorNode();

    virtual void OnInitialize() {}
    virtual NODE_STATUS OnUpdate(ecs::EntityHandle entity) = 0;
    virtual void OnTerminate(NODE_STATUS) {}

    //getters setters etc
    bool IsReady() const;
    bool Succeeded() const;
    bool Failed() const;
    bool IsRunning() const;

    NODE_STATUS GetStatus() const;
    void SetStatus(NODE_STATUS newStatus);

    NODE_STATUS Tick(ecs::EntityHandle entity);

    virtual bool AddChild([[maybe_unused]] BehaviorNode* childPtr) { return false; }
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
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    BehaviorNodes::iterator currentChildItr;
};

class Selector
    : public CompositeNode
{
protected:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    BehaviorNodes::iterator currentChildItr;
};


//For testing

class MoveLeft
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    unsigned int count;
};

class MoveDown
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    unsigned int count;
};

class MoveRight
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    unsigned int count;
};

class MoveUp
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    unsigned int count;
};
