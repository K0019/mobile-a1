#include "TextUI.h"
#include "TextComponent.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

ScoreUISystem::ScoreUISystem()
    : System_Internal{ &ScoreUISystem::UpdateComp }
{
}

void ScoreUISystem::UpdateComp(TextUIComponent& scoreComp, TextComponent& textComp)
{
    if (auto ev = EventsReader<Events::Game_ScoreUpdate>{}.ExtractEvent())
    {
        scoreComp.totalScore += ev->score;
        textComp.SetText(std::to_string(static_cast<int>(scoreComp.totalScore)));
    }
}