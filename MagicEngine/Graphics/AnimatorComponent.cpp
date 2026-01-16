#include "Graphics/AnimatorComponent.h"
#include "Editor/Containers/GUICollection.h"

AnimatorComponent::AnimatorComponent()
	: animStateMachine{ new sm::AnimStateMachine }
{
	
}

AnimatorComponent::AnimatorComponent(sm::AnimStateMachine* inSM)
	: animStateMachine{ inSM }
{

}
AnimatorComponent::AnimatorComponent(const AnimatorComponent& other)
	: animStateMachine{ other.animStateMachine ? new sm::AnimStateMachine(*other.animStateMachine) : nullptr }
{
}

AnimatorComponent::AnimatorComponent(AnimatorComponent&& other) noexcept
	: animStateMachine{ other.animStateMachine }
{
	// Null out the source pointer so its destructor doesn't delete the resource
	other.animStateMachine = nullptr;
}

//void AnimatorComponent::EditorDraw() {
//		// For now, just display a placeholder text
//	gui::TextUnformatted("Animator Component");
//}

AnimatorComponent::~AnimatorComponent()
{
	if (animStateMachine)
	{
		delete animStateMachine;
		animStateMachine = nullptr;
	}
}

void AnimatorComponent::EditorDraw()
{
	gui::TextUnformatted("Animator Component");

	if (animStateMachine)
	{
		gui::TextUnformatted("State Machine: Active");
		// You could add more debug info here, like current state name
	}
	else
	{
		gui::TextUnformatted("State Machine: None");
	}
}