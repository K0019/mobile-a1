#pragma once
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

// Describes each attack/parry/whatever that the weapon provides
struct WeaponMoveInfo : public ISerializeable
{
	// Animation associated with this move
	// For now if the same weapon is used by player and enemy, duplicate the WeaponInfo i guess
	UserResourceHandle<ResourceAnimation> anim = 0;
	// Hitbox details
	Vec3 hitboxOffset; // Position offset from where the entity is located
	Vec3 hitboxExtents;
	// When does the move "hit"
	float hitDelay = 0.1f;
	// note: if certain moves should do more/less damage than others, can add an override value here while leaving the ints in WeaponInfo as the default

	void EditorDraw();

	property_vtable()
};
property_begin(WeaponMoveInfo)
{
	property_var(anim),
	property_var(hitboxOffset),
	property_var(hitboxExtents),
	property_var(hitDelay)
}
property_vend_h(WeaponMoveInfo)

struct WeaponInfo
	: public ISerializeableWithoutJsonObj
	, public ResourceBase
{
	std::vector<WeaponMoveInfo> moves;
	int lightDamage = 0;
	int heavyDamage = 0;
	int parryDamage = 0;

private:
	bool isLoaded = false;
public:
	bool IsLoaded() override;

	void EditorDraw();

	property_vtable()
};

property_begin(WeaponInfo)
{
	property_var(moves),
	property_var(lightDamage),
	property_var(heavyDamage),
	property_var(parryDamage)
}
property_vend_h(WeaponInfo)
