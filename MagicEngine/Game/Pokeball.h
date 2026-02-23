#pragma once
#include "ECS/IRegisteredComponent.h"
#include "ECS/EntityUID.h"

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

	Vec3 GetSpinAxis() const;
	float GetSpinSpeed() const;
	float GetSpinAngle() const;
	void AccumulateSpin(float dt);
	void RandomizeIdleSpin();

	// Optional audio source entity to play sound when a target is hit
	EntityReference hitSoundSource;
	EntityReference scoreBoard;

private:
	bool isThrown;
	float launchTime;
	Vec3 spinAxis;
	float spinSpeed;
	float spinAngle;
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

class PokeballKeepFrontSystem : public ecs::System<PokeballKeepFrontSystem, PokeballComponent>
{
public:
	PokeballKeepFrontSystem();
	bool PreRun() override;

private:
	void UpdateComp(PokeballComponent& comp);

private:
	float yaw;
};