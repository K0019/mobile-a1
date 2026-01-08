#pragma once
#include "../../BehaviourNode.h"

// Selects which of its children to run based on the health of the entity it's attached to.
// If 2 children, switches to secong child at under .5 of hp
// If 3 children, phase 2 at .66666 and phase 3 at .33333
// etc
class S_Boss_HealthPhases : public CompositeNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};