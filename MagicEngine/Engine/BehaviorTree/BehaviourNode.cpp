/******************************************************************************/
/*!
\file   BehaviorNode.cpp
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
      This header file defines the BehaviorNode base class for all the nodes
      in the behavior tree. It also contains the definition of the Decorator 
      base class, and definition of sequence and selector nodes.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/BehaviorTree/BehaviourNode.h"
#include "LeafMoveTowardsPlayer.h"
#include "FollowPlayerUsingNavMesh.h"
#include "LeafLookForPlayer.h"
#include "LeafAttackPlayer.h"
#include "DecoWantToMove.h"
#include "DecoAttackCoolDown.h"
#include "LeafWindUp.h"
#include "LeafMeleeAttack.h"
#include "LeafFollowThrough.h"
#include "LeafRotateTowardsTarget.h"
#include "LeafThrowAttack.h"
#include "BehaviourTreeFactory.h"
#include "Engine/Input.h"

#include "Melee/Melee_LeafAttack.h"
#include "Melee/Melee_LeafCheckAttack.h"
#include "Melee/Melee_LeafDodge.h"
#include "Melee/Melee_DecoIsInCombatRange.h"
#include "Melee/Melee_DecoIsInAttackRange.h"
#include "Melee/Melee_DecoIsNotCurrentAttack.h"

#include "Range/Range_LeafCheckAttack.h"
#include "Range/Range_LeafThrow.h"
#include "Range/Range_DecoHasWeapon.h"
#include "Range/Range_LeafMoveToWeapon.h"

#include "Boss/Boss_Prefect/Boss_LeafDetention.h"
#include "Boss/Boss_Prefect/Boss_LeafBookingSlips.h"
#include "Boss/Boss_Prefect/Boss_LeafInvincibility.h"
#include "Boss/Boss_Prefect/Boss_LeafLashOut.h"
#include "Boss/Boss_Prefect/Boss_LeafDontRun.h"
#include "Boss/Boss_Prefect/Boss_LeafDisciplinaryAction.h"

#include "Boss/Common/Selector_BossHealthPhases.h"
 

BT_REGISTER_NODE(Sequence, "Sequence")
BT_REGISTER_NODE(Selector, "Selector")

// For testing
BT_REGISTER_NODE(MoveLeft, "MoveLeft")
BT_REGISTER_NODE(MoveRight, "MoveRight")
BT_REGISTER_NODE(MoveUp, "MoveUp")
BT_REGISTER_NODE(MoveDown, "MoveDown")
BT_REGISTER_NODE(DetectClickTest, "DetectClickTest")

BT_REGISTER_NODE(L_MoveTowardsPlayer, "L_MoveTowardsPlayer")
BT_REGISTER_NODE(L_LookForPlayer, "L_LookForPlayer")
BT_REGISTER_NODE(L_AttackPlayer, "L_AttackPlayer")
BT_REGISTER_NODE(L_FollowPlayerUsingNavMesh, "L_FollowPlayerUsingNavMesh")
BT_REGISTER_NODE(D_WantToMove, "D_WantToMove")
BT_REGISTER_NODE(D_AttackCoolDown, "D_AttactCoolDown")
BT_REGISTER_NODE(L_WindUp, "L_WindUp")
BT_REGISTER_NODE(L_MeleeAttack, "L_MeleeAttack")
BT_REGISTER_NODE(L_ThrowAttack, "L_ThrowAttack")
BT_REGISTER_NODE(L_RotateTowardsTarget, "L_RotateTowardsTarget")
BT_REGISTER_NODE(L_FollowThrough, "L_FollowThrough")

BT_REGISTER_NODE(L_Melee_Dodge, "L_Melee_Dodge")
BT_REGISTER_NODE(L_Melee_Attack, "L_Melee_Attack")
BT_REGISTER_NODE(L_Melee_CheckAttack, "L_Melee_CheckAttack")
BT_REGISTER_NODE(D_Melee_IsInCombatRange, "D_Melee_IsInCombatRange")
BT_REGISTER_NODE(D_Melee_IsNotCurrentAttack, "D_Melee_IsNotCurrentAttack")
BT_REGISTER_NODE(D_Melee_IsInAttackRange, "D_Melee_IsInAttackRange")

BT_REGISTER_NODE(L_Range_CheckAttack, "L_Range_CheckAttack")
BT_REGISTER_NODE(L_Range_Throw, "L_Range_Throw")
BT_REGISTER_NODE(L_Range_MoveToWeapon, "L_Range_MoveToWeapon")
BT_REGISTER_NODE(D_Range_HasWeapon, "D_Range_HasWeapon")

BT_REGISTER_NODE(L_Boss_Prefect_Detention, "L_Boss_Prefect_Detention")
BT_REGISTER_NODE(L_Boss_Prefect_BookingSlips, "L_Boss_Prefect_BookingSlips")
BT_REGISTER_NODE(L_Boss_Prefect_Invincibility, "L_Boss_Prefect_Invincibility")
BT_REGISTER_NODE(L_Boss_Prefect_LashOut, "L_Boss_Prefect_LashOut")
BT_REGISTER_NODE(L_Boss_Prefect_DontRun, "L_Boss_Prefect_DontRun")
BT_REGISTER_NODE(L_Boss_Prefect_DisciplinaryAction, "L_Boss_Prefect_DisciplinaryAction")
BT_REGISTER_NODE(S_Boss_HealthPhases, "S_Boss_HealthPhases")


BehaviorNode::BehaviorNode()
    : status{ NODE_STATUS::READY }
{
}

BehaviorNode::~BehaviorNode()
{
}

void BehaviorNode::OnTerminate(NODE_STATUS)
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

bool BehaviorNode::AddChild([[maybe_unused]] BehaviorNode* childPtr)
{
    return false;
}

void BehaviorNode::RemoveChildren()
{
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



//FOR TESTING 
static  bool  reverse = 0;

void MoveLeft::OnInitialize()
{
    count = 0;

}

NODE_STATUS MoveLeft::OnUpdate(ecs::EntityHandle entity)
{
    //FOR DEMO
    static bool prev = false; // remembers last frame state

    auto input = ST<MagicInput>::Get();
    if (input)
    {
        bool now = false;
        if (auto act = input->GetAction<bool>("ReverseDir"))
            now = act->GetValue();   // true while the button is held

        if (now && !prev)            // toggle only on the rising edge
            reverse = !reverse;

        prev = now;
    }

    if (reverse == 0) {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ -0.2f, 0.f, 0.f });
    }
    else {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.2f, 0.f, 0.f });

    }
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveRight::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveRight::OnUpdate(ecs::EntityHandle entity)
{
    //FOR DEMO
    static bool prev = false; // remembers last frame state

    auto input = ST<MagicInput>::Get();
    if (input)
    {
        bool now = false;
        if (auto act = input->GetAction<bool>("ReverseDir"))
            now = act->GetValue();   // true while the button is held

        if (now && !prev)            // toggle only on the rising edge
            reverse = !reverse;

        prev = now;
    }

    if (reverse == 0) {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.2f, 0.f, 0.f });
    }
    else {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ -0.2f, 0.f, 0.f });

    }
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveUp::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveUp::OnUpdate(ecs::EntityHandle entity)
{
    //FOR DEMO
    static bool prev = false; // remembers last frame state

    auto input = ST<MagicInput>::Get();
    if (input)
    {
        bool now = false;
        if (auto act = input->GetAction<bool>("ReverseDir"))
            now = act->GetValue();   // true while the button is held

        if (now && !prev)            // toggle only on the rising edge
            reverse = !reverse;

        prev = now;
    }

    if (reverse == 0) {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, 0.15f, 0.f });
    }
    else {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, -0.15f, 0.f });

    }
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void MoveDown::OnInitialize()
{
    count = 0;
}

NODE_STATUS MoveDown::OnUpdate(ecs::EntityHandle entity)
{
    //FOR DEMO
    static bool prev = false; // remembers last frame state

    auto input = ST<MagicInput>::Get();
    if (input)
    {
        bool now = false;
        if (auto act = input->GetAction<bool>("ReverseDir"))
            now = act->GetValue();   // true while the button is held

        if (now && !prev)            // toggle only on the rising edge
            reverse = !reverse;

        prev = now;
    }

    if (reverse == 0) {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, -0.15f, 0.f });
    }
    else {
        entity->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition() + Vec3{ 0.f, 0.15f, 0.f });

    }
    return (++count) == 100 ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}

void DetectClickTest::OnInitialize()
{ 
}
NODE_STATUS DetectClickTest::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    //FOR DEMO
    static bool prev = false; // remembers last frame state

    auto input = ST<MagicInput>::Get();
    if (input)
    {
        bool now = false;
        if (auto act = input->GetAction<bool>("ClickTest")) {
            now = act->GetValue();   // true while the button is held
        }

        if (now && !prev)            // toggle only on the rising edge
            CONSOLE_LOG(LEVEL_DEBUG) << "Can click anywhere";

        prev = now;
    }
    return NODE_STATUS::SUCCESS;
}




