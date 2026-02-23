#pragma once

#include "ECS/ECS.h"
#include "ECS/IRegisteredComponent.h"

class TextComponent;

class TextUIComponent : public IRegisteredComponent<TextUIComponent>
{
public:
    float totalScore = 0.0f;
};

class ScoreUISystem : public ecs::System<ScoreUISystem, TextUIComponent, TextComponent>
{
public:
    ScoreUISystem();

private:
    void UpdateComp(TextUIComponent& scoreComp, TextComponent& textComp);
};