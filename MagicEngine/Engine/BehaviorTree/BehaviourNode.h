/******************************************************************************/
/*!
\file   BehaviourNode.h
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
      This header file declares the BehaviorNode base class for all the nodes
      in the behavior tree. It also contains the definition of the status of the
      nodes, declaration of the Decorator base class, and declarations of sequence 
      and selector nodes.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\brief
    Return types of a behavior node.
*//******************************************************************/
enum class NODE_STATUS
{
    READY, // node is enterable
    RUNNING, // node is currently running
    SUCCESS, // return state success
    FAILURE // return state success
};

/*****************************************************************//*!
\brief
    Base class for all the behavior nodes.
*//******************************************************************/
class BehaviorNode {
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    BehaviorNode();

    /*****************************************************************//*!
    \brief
        Destructor
    *//******************************************************************/
    virtual ~BehaviorNode();

private:
    //Status of the node.
    NODE_STATUS status;

public:
    /*****************************************************************//*!
    \brief
        Called when the nodes status is not RUNNING.
    *//******************************************************************/
    virtual void OnInitialize() {}

    /*****************************************************************//*!
    \brief
        Update the child node or itself.
    \param entity 
        entity handle of the entity that has the behavior tree.
    \return
        The status of the node.
    *//******************************************************************/
    virtual NODE_STATUS OnUpdate(ecs::EntityHandle entity) = 0;

    /*****************************************************************//*!
    \brief
        Called when the nodes status is not RUNNING.
    \param stat
        the status the OnUpdate returned
    *//******************************************************************/
    virtual void OnTerminate(NODE_STATUS);

    /*****************************************************************//*!
    \brief
        Check if the status is READY
    \param stat
        true if the status is READY, else false.
    *//******************************************************************/
    bool IsReady() const;

    /*****************************************************************//*!
    \brief
        Check if the status is SUCCESS
    \param stat
        true if the status is SUCCESS, else false.
    *//******************************************************************/
    bool Succeeded() const;

    /*****************************************************************//*!
     \brief
         Check if the status is FAILURE
     \param stat
         true if the status is FAILURE, else false.
     *//******************************************************************/
    bool Failed() const;

    /*****************************************************************//*!
     \brief
         Check if the status is RUNNING
     \param stat
         true if the status is RUNNING, else false.
     *//******************************************************************/
    bool IsRunning() const;

    /*****************************************************************//*!
     \brief
         Get the status value.
     /return
        The valuse of status.
     *//******************************************************************/
    NODE_STATUS GetStatus() const;

    /*****************************************************************//*!
     \brief
         Set the status value.
     /newStatus
         Status to set.
     *//******************************************************************/
    void SetStatus(NODE_STATUS newStatus);

    /*****************************************************************//*!
     \brief
         Call the OnInitialize, OnUpdate, and OnTerminate
     \param entity
         The entity that has the behavior tree.
     \return
         Status of the node ticked.
     *//******************************************************************/
    NODE_STATUS Tick(ecs::EntityHandle entity);

    /*****************************************************************//*!
     \brief
         Add a child to the node.
     \param childPtr
         Pointer of the child to add.
     \return
         true if the child was added, else false.
     *//******************************************************************/
    virtual bool AddChild(BehaviorNode* childPtr);

    /*****************************************************************//*!
     \brief
         Remove all the child node.
     *//******************************************************************/
    virtual void RemoveChildren();

    /*****************************************************************//*!
     \brief
         GUI to adjust the values for each node.
     *//******************************************************************/
    virtual void EditorDraw();
};

/*****************************************************************//*!
 \brief
     Base class for nodes that have multiple child.
 *//******************************************************************/
class CompositeNode
    : public BehaviorNode
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    CompositeNode();

    /*****************************************************************//*!
    \brief
        Destructor
    *//******************************************************************/
    ~CompositeNode();

protected:
    //List of child nodes.
    typedef std::vector<BehaviorNode*> BehaviorNodes;
    BehaviorNodes childrenPtr;

public:
    /*****************************************************************//*!
     \brief
         Add a child to the node.
     \param childPtr
         Pointer of the child to add.
     \return
         true if the child was added, else false.
     *//******************************************************************/
    bool AddChild(BehaviorNode* childPtr) override;

    /*****************************************************************//*!
     \brief
         Remove a given child node.
     \param childPtr
         Pointer to the child to remove.
     *//******************************************************************/
    void RemoveChild(BehaviorNode* childPtr);

    /*****************************************************************//*!
     \brief
         Remove all the child node.
     *//******************************************************************/
    void RemoveChildren() override;

    /*****************************************************************//*!
     \brief
         Remove all the child node.
     *//******************************************************************/
    void CallChildrenEditorDraw();
};

/*****************************************************************//*!
\brief
    Base class for decorator nodes.
*//******************************************************************/
class Decorator
    : public BehaviorNode
{
public:
    /*****************************************************************//*!
    \brief
        Constructor
    *//******************************************************************/
    Decorator();

protected:
    //Pointer to the child node.
    BehaviorNode* childPtr;

public:
    /*****************************************************************//*!
     \brief
         Add a child to the node.
     \param childPtr
         Pointer of the child to add.
     \return
         true if the child was added, else false.
     *//******************************************************************/
    bool AddChild(BehaviorNode* child) override;

    /*****************************************************************//*!
     \brief
         Remove all the child node.
     *//******************************************************************/
    void RemoveChildren() override;
};

class Sequence
    : public CompositeNode
{
public:
    /*****************************************************************//*!
    \brief
        Called when the nodes status is not RUNNING.
    *//******************************************************************/
    void OnInitialize() override;

    /*****************************************************************//*!
    \brief
        Update the child node or itself.
    \param entity
        entity handle of the entity that has the behavior tree.
    \return
        The status of the node.
    *//******************************************************************/
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    BehaviorNodes::iterator currentChildItr;
};

class Selector
    : public CompositeNode
{
public:
    /*****************************************************************//*!
    \brief
        Called when the nodes status is not RUNNING.
    *//******************************************************************/
    void OnInitialize() override;

    /*****************************************************************//*!
    \brief
        Update the child node or itself.
    \param entity
        entity handle of the entity that has the behavior tree.
    \return
        The status of the node.
    *//******************************************************************/
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    BehaviorNodes::iterator currentChildItr;
};


//================================== FOR TESTING =====================================================
class MoveLeft
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    unsigned int count;
    bool reverse_ = false;      // the toggle you care about
    bool prevBtn_ = false;      // previous frame's button state
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

class DetectClickTest
    : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
};

