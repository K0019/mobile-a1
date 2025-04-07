#pragma once
#include "EntityUID.h"

class ArmPivotIKComponent
	: public IRegisteredComponent<ArmPivotIKComponent>
	, public IEditorComponent<ArmPivotIKComponent>
{
	public:

	ArmPivotIKComponent();

	Vector2 shoulderOffset;
	EntityReference upperArmEntity;
	EntityReference lowerArmEntity;
	EntityReference handEntity;
	float maxExtensionAngle;
	float preferredElbowAngle;
	float smoothingFactor;

	float currentShoulderAngle;
	float currentElbowAngle;
	Vector2 elbowPositionOffset;
	float upperArmRotation;
	float forearmRotation;

#ifdef IMGUI_ENABLED
	bool debugDraw = false;
#endif
	virtual void EditorDraw() override;

	property_vtable()
};

property_begin(ArmPivotIKComponent)
{
	property_var(shoulderOffset),
	property_var(upperArmEntity),
	property_var(lowerArmEntity),
	property_var(handEntity),
	property_var(maxExtensionAngle),
	property_var(preferredElbowAngle),
	property_var(smoothingFactor),
}
property_vend_h(ArmPivotIKComponent)

class ArmPivotIKSystem : public ecs::System<ArmPivotIKSystem, ArmPivotIKComponent>
{
	public:
	explicit ArmPivotIKSystem();
	private:
	void UpdateArmPivotIKComp(ArmPivotIKComponent& comp);
};

