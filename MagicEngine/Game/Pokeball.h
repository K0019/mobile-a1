#pragma once
#include "ECS/IRegisteredComponent.h"

class PokeballComponent
	: public IRegisteredComponent<PokeballComponent>
	, public ecs::IComponentCallbacks
{
public:
	PokeballComponent();

	bool GetIsThrown() const;
	void SetThrown();
	float GetTimeInAir() const;

	void OnTargetHit(ecs::EntityHandle targetEntity);

private:
	bool isThrown;
	float launchTime;
};

class PokeballThrowSystem : public ecs::System<PokeballThrowSystem, PokeballComponent>
{
public:
	PokeballThrowSystem();

	bool PreRun() override;

private:
	void UpdateComp(PokeballComponent& comp);

private:
	bool pressed, released;
	Vec2 downPos, pos;
	int m_activePid = -1;
};

class PokeballRespawnSystem : public ecs::System<PokeballRespawnSystem, PokeballComponent>
{
public:
	PokeballRespawnSystem();

private:
	void UpdateComp(PokeballComponent& comp);

};

class PokeballTargetHitSystem : public ecs::System<PokeballTargetHitSystem>
{
public:
	bool PreRun() override;
};