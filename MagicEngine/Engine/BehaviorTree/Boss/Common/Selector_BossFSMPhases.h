/******************************************************************************/
/*!
\file   Selector_BossFSMPhases.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
    A behavior tree selector node that uses an internal FSM to manage boss phases.
    Each phase can access moves from current + all previous phases (cumulative).
    Animations are triggered through the FSM on phase transitions.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "../../BehaviourNode.h"
#include <vector>
#include <cstdlib>

/*****************************************************************//*!
\class S_Boss_FSMPhases
\brief
    A selector that uses an internal FSM to manage boss phases.
    
    Key Features:
    - Phases are cumulative: Phase 2 can use Phase 1 + Phase 2 moves
    - Randomly selects from available moves based on current phase
    - Triggers animations on phase transitions via FSM
    - Health-based phase transitions
    
    Children should be ordered: [Phase1 moves..., Phase2 moves..., Phase3 moves...]
    
    Example with movesPerPhase = {3, 2}:
    - Phase 1 (health > 0.5): randomly picks from children [0, 1, 2]
    - Phase 2 (health <= 0.5): randomly picks from children [0, 1, 2, 3, 4]
*//******************************************************************/
class S_Boss_FSMPhases : public CompositeNode
{
public:
    /*****************************************************************//*!
    \brief
        Boss phase enumeration
    *//******************************************************************/
    enum class BossPhase
    {
        Phase1,
        Phase2,
        Phase3,
        PhaseCount
    };

    /*****************************************************************//*!
    \brief
        Constructor - initializes FSM state
    *//******************************************************************/
    S_Boss_FSMPhases();

    /*****************************************************************//*!
    \brief
        Called when the node is first entered or re-entered
    *//******************************************************************/
    void OnInitialize() override;

    /*****************************************************************//*!
    \brief
        Main update - manages FSM transitions and child selection
    \param entity
        The entity this behavior tree is attached to
    \return
        Node status (RUNNING, SUCCESS, or FAILURE)
    *//******************************************************************/
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

    /*****************************************************************//*!
    \brief
        Configure how many children belong to each phase.
        e.g., {3, 2, 1} means:
        - Children 0-2 are Phase 1 moves
        - Children 3-4 are Phase 2 moves  
        - Child 5 is Phase 3 move
    \param counts
        Vector of move counts per phase
    *//******************************************************************/
    void SetMovesPerPhase(const std::vector<size_t>& counts);

    /*****************************************************************//*!
    \brief
        Set the animation ID to play when transitioning to a specific phase
    \param phase
        The phase to set the animation for
    \param animationId
        The animation hash/ID to play
    *//******************************************************************/
    void SetPhaseTransitionAnimation(BossPhase phase, size_t animationId);

    /*****************************************************************//*!
    \brief
        Set the health threshold for transitioning to a phase.
        Transitions when health drops BELOW this threshold.
    \param phase
        The phase to set threshold for
    \param threshold
        Health threshold (0.0 to 1.0, normalized)
    *//******************************************************************/
    void SetPhaseHealthThreshold(BossPhase phase, float threshold);

private:
    // FSM State
    BossPhase currentPhase;
    BossPhase previousPhase;
    
    // Child execution tracking
    size_t currentChildIndex;
    bool isChildRunning;
    bool justTransitioned;  // Flag for when we just changed phases
    
    // Phase configuration
    std::vector<size_t> movesPerPhase;          // How many moves each phase adds
    std::vector<float> phaseHealthThresholds;   // Health thresholds for each phase
    std::vector<size_t> phaseTransitionAnims;   // Animation IDs for phase transitions
    
    /*****************************************************************//*!
    \brief
        Determine which phase the boss should be in based on health
    \param healthNormalized
        Current health as a value from 0.0 to 1.0
    \return
        The phase the boss should be in
    *//******************************************************************/
    BossPhase DeterminePhaseFromHealth(float healthNormalized) const;

    /*****************************************************************//*!
    \brief
        Get the maximum child index accessible in the current phase.
        This is cumulative - Phase 2 includes all Phase 1 moves.
    \return
        The highest child index that can be selected
    *//******************************************************************/
    size_t GetMaxChildIndexForPhase() const;

    /*****************************************************************//*!
    \brief
        Randomly pick a child from the available moves for current phase
    \return
        Index of the selected child
    *//******************************************************************/
    size_t PickRandomChild() const;

    /*****************************************************************//*!
    \brief
        Handle FSM state transition - triggers animations
    \param entity
        The entity to play animations on
    \param newPhase
        The phase we're transitioning to
    *//******************************************************************/
    void TransitionToPhase(ecs::EntityHandle entity, BossPhase newPhase);

    /*****************************************************************//*!
    \brief
        Play the transition animation for a phase
    \param entity
        The entity to play animation on
    \param phase
        The phase whose transition animation to play
    *//******************************************************************/
    void PlayPhaseTransitionAnimation(ecs::EntityHandle entity, BossPhase phase);
};
