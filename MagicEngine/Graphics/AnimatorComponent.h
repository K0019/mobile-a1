#pragma once
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"

namespace sm {
	class AnimStateMachine {};
}

class AnimatorComponent : public IRegisteredComponent<AnimatorComponent>, public IEditorComponent<AnimatorComponent>
{
public:
	AnimatorComponent();
	AnimatorComponent(sm::AnimStateMachine* inSM);
	// Copy Constructor
	AnimatorComponent(const AnimatorComponent& other);
	// Move Constructor
	AnimatorComponent(AnimatorComponent&& other) noexcept;
	~AnimatorComponent();
	// Getter for the state machine
	sm::AnimStateMachine* GetStateMachine() const { return animStateMachine; }

private:
	/*****************************************************************//*!
	\brief
		Draws a physics component to the ImGui editor window.
	*//******************************************************************/
	virtual void EditorDraw() override;
private:
	sm::AnimStateMachine* animStateMachine{ nullptr };
public:

};



