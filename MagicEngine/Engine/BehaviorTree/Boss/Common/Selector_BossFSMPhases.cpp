/******************************************************************************/
/*!
\file   Selector_BossFSMPhases.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
    Implementation of the FSM-based boss phase selector node.
    Manages phase transitions and cumulative move lists.
    
    Supports two usage patterns:
    1. Flat children (individual attack leaves) - randomly picks from available
    2. Sequencer children (each sequencer = one phase) - randomly picks a sequencer,
       Phase 2 can pick from Phase 1 OR Phase 2 sequencers

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Selector_BossFSMPhases.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Graphics/AnimationComponent.h"
#include "Utilities/GameTime.h"

// Register this node type with the behavior tree factory
BT_REGISTER_NODE(S_Boss_FSMPhases, "S_Boss_FSMPhases")

S_Boss_FSMPhases::S_Boss_FSMPhases()
    : CompositeNode()
    , currentPhase(BossPhase::Phase1)
    , previousPhase(BossPhase::Phase1)
    , currentChildIndex(0)
    , isChildRunning(false)
    , justTransitioned(false)
    , movesPerPhase()
    , phaseHealthThresholds()
    , phaseTransitionAnims()
{
    // Default configuration for 2 phases
    // Phase 1: health > 0.5
    // Phase 2: health <= 0.5
    phaseHealthThresholds.resize(static_cast<size_t>(BossPhase::PhaseCount));
    phaseHealthThresholds[static_cast<size_t>(BossPhase::Phase1)] = 1.0f;   // Always in Phase1 if health > 0.5
    phaseHealthThresholds[static_cast<size_t>(BossPhase::Phase2)] = 0.5f;   // Enter Phase2 at 50% health
    phaseHealthThresholds[static_cast<size_t>(BossPhase::Phase3)] = 0.25f;  // Enter Phase3 at 25% health (if used)

    // Default: no transition animations
    phaseTransitionAnims.resize(static_cast<size_t>(BossPhase::PhaseCount), 0);
}

void S_Boss_FSMPhases::OnInitialize()
{
    currentChildIndex = 0;
    isChildRunning = false;
    justTransitioned = false;
    
    // If movesPerPhase wasn't explicitly set, default based on child count
    if (movesPerPhase.empty() && !childrenPtr.empty())
    {
        // Default: assume each child is one phase (for sequencer-based setup)
        // e.g., 2 children = {1, 1} meaning 1 sequencer per phase
        for (size_t i = 0; i < childrenPtr.size(); ++i)
        {
            movesPerPhase.push_back(1);
        }
        
        CONSOLE_LOG(LEVEL_DEBUG) << "S_Boss_FSMPhases: Auto-configured " 
                                  << childrenPtr.size() << " phases with 1 move each";
    }
}

NODE_STATUS S_Boss_FSMPhases::OnUpdate(ecs::EntityHandle entity)
{
    if (!entity || childrenPtr.empty())
        return NODE_STATUS::FAILURE;

    // Get health component to determine phase
    auto healthComp = entity->GetComp<HealthComponent>();
    if (!healthComp)
        return NODE_STATUS::FAILURE;

    // === FSM UPDATE: Check for phase transitions ===
    float currentHealth = healthComp->GetCurrHealthNormalized();
    BossPhase targetPhase = DeterminePhaseFromHealth(currentHealth);

    // Handle phase transition
    if (targetPhase != currentPhase)
    {
        TransitionToPhase(entity, targetPhase);
    }

    // === If we just transitioned, we might want to play a transition animation ===
    // The animation is handled in TransitionToPhase, but we can add delay logic here if needed
    if (justTransitioned)
    {
        justTransitioned = false;
        // Optionally: return RUNNING here to give time for transition animation
        // For now, we continue to picking the next move
    }

    // === BEHAVIOR TREE LOGIC: Select and run a child ===
    
    // If a child is currently running, continue it
    if (isChildRunning && currentChildIndex < childrenPtr.size())
    {
        NODE_STATUS childStatus = childrenPtr[currentChildIndex]->Tick(entity);
        
        if (childStatus == NODE_STATUS::RUNNING)
        {
            return NODE_STATUS::RUNNING;
        }
        else
        {
            // Child finished (success or failure), pick a new one next tick
            isChildRunning = false;
            childrenPtr[currentChildIndex]->SetStatus(NODE_STATUS::READY);
        }
    }

    // Pick a new random child from available moves
    currentChildIndex = PickRandomChild();
    
    if (currentChildIndex < childrenPtr.size())
    {
        isChildRunning = true;
        NODE_STATUS childStatus = childrenPtr[currentChildIndex]->Tick(entity);
        
        if (childStatus != NODE_STATUS::RUNNING)
        {
            isChildRunning = false;
            childrenPtr[currentChildIndex]->SetStatus(NODE_STATUS::READY);
        }
    }

    // This node keeps running indefinitely (boss fight continues)
    return NODE_STATUS::RUNNING;
}

void S_Boss_FSMPhases::SetMovesPerPhase(const std::vector<size_t>& counts)
{
    movesPerPhase = counts;
}

void S_Boss_FSMPhases::SetPhaseTransitionAnimation(BossPhase phase, size_t animationId)
{
    size_t phaseIndex = static_cast<size_t>(phase);
    if (phaseIndex < phaseTransitionAnims.size())
    {
        phaseTransitionAnims[phaseIndex] = animationId;
    }
}

void S_Boss_FSMPhases::SetPhaseHealthThreshold(BossPhase phase, float threshold)
{
    size_t phaseIndex = static_cast<size_t>(phase);
    if (phaseIndex < phaseHealthThresholds.size())
    {
        phaseHealthThresholds[phaseIndex] = threshold;
    }
}

S_Boss_FSMPhases::BossPhase S_Boss_FSMPhases::DeterminePhaseFromHealth(float healthNormalized) const
{
    // Check phases from highest (Phase3) to lowest (Phase1)
    // Return the first phase whose threshold the health is at or below
    
    if (phaseHealthThresholds.size() > static_cast<size_t>(BossPhase::Phase3) &&
        healthNormalized <= phaseHealthThresholds[static_cast<size_t>(BossPhase::Phase3)])
    {
        return BossPhase::Phase3;
    }
    
    if (phaseHealthThresholds.size() > static_cast<size_t>(BossPhase::Phase2) &&
        healthNormalized <= phaseHealthThresholds[static_cast<size_t>(BossPhase::Phase2)])
    {
        return BossPhase::Phase2;
    }
    
    return BossPhase::Phase1;
}

size_t S_Boss_FSMPhases::GetMaxChildIndexForPhase() const
{
    if (movesPerPhase.empty() || childrenPtr.empty())
        return 0;

    // Cumulative: sum up moves from Phase 1 to current phase
    size_t maxIndex = 0;
    size_t phaseIndex = static_cast<size_t>(currentPhase);
    
    for (size_t i = 0; i <= phaseIndex && i < movesPerPhase.size(); ++i)
    {
        maxIndex += movesPerPhase[i];
    }
    
    // Clamp to actual children count
    if (maxIndex > childrenPtr.size())
        maxIndex = childrenPtr.size();
    
    return maxIndex;
}

size_t S_Boss_FSMPhases::PickRandomChild() const
{
    size_t maxIndex = GetMaxChildIndexForPhase();
    
    if (maxIndex == 0)
        return 0;
    
    // Random selection from [0, maxIndex)
    return static_cast<size_t>(std::rand()) % maxIndex;
}

void S_Boss_FSMPhases::TransitionToPhase(ecs::EntityHandle entity, BossPhase newPhase)
{
    previousPhase = currentPhase;
    currentPhase = newPhase;
    justTransitioned = true;
    
    // Reset current child execution since we're changing phases
    if (isChildRunning && currentChildIndex < childrenPtr.size())
    {
        childrenPtr[currentChildIndex]->SetStatus(NODE_STATUS::READY);
    }
    isChildRunning = false;
    
    // Play the phase transition animation
    PlayPhaseTransitionAnimation(entity, newPhase);
    
    // Log the transition for debugging
    CONSOLE_LOG(LEVEL_DEBUG) << "Boss FSM: Transitioning from Phase " 
                              << (static_cast<int>(previousPhase) + 1) 
                              << " to Phase " 
                              << (static_cast<int>(newPhase) + 1);
}

void S_Boss_FSMPhases::PlayPhaseTransitionAnimation(ecs::EntityHandle entity, BossPhase phase)
{
    size_t phaseIndex = static_cast<size_t>(phase);
    
    // Check if we have a transition animation for this phase
    if (phaseIndex >= phaseTransitionAnims.size() || phaseTransitionAnims[phaseIndex] == 0)
        return;
    
    // Get the animation component and play the transition animation
    auto animComp = entity->GetComp<AnimationComponent>();
    if (animComp)
    {
        animComp->TransitionTo(phaseTransitionAnims[phaseIndex], 0.1f);
        animComp->timeA = 0.0f;
    }
}
