#include "NiceThrow.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

NiceThrowSystem::NiceThrowSystem()
	: System_Internal{ &NiceThrowSystem::UpdateComp }
{
}

void NiceThrowSystem::UpdateComp([[maybe_unused]] NiceThrowComponent& niceThrowComp, SpriteComponent& spriteComp)
{
	Vec4 color{ spriteComp.GetColor() };
	if (EventsReader<Events::Game_NiceThrow>{}.ExtractEvent())
		color.w = 1.5f;
	else
		color.w = std::max(color.w - GameTime::Dt(), 0.0f);
	spriteComp.SetColor(color);
}
