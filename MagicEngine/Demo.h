#pragma once
#include "ECS/IRegisteredComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "ECS/IEditorComponent.h"

// This is an example component.
class ExampleComponent
	// Note that all of these below base classes are optional, and adding this component to ecs will still work without them.
	: public IRegisteredComponent<ExampleComponent> // For serialization and ability to add this comp via the inspector ImGui window
	, public IEditorComponent<ExampleComponent> // For drawing this component to the inspector ImGui window
	, public IGameComponentCallbacks<ExampleComponent> // For OnAttached() / OnDetached() / OnStart() callbacks (for more info, see overrides below, or their function documentation)
	// , public ecs::IComponentCallbacks // Use this instead of IGameComponentCallbacks if you only need OnAttached() / OnDetached()
	// , public IHiddenComponent<DemoComponent> // For hiding this component from the inspector ImGui window (exists so you can still have serialization but not show this component to the user)
{
public:
	ExampleComponent();

private:
	// Your data goes here
	Vec2 x;

public:
	// OnAttached() is called when the component is attached to an entity. Note that this is called even within the editor!
	void OnAttached() override;
	// OnStart() is called only when the simulation is started, or immediately once the component is attached to an entity if the simulation is already ongoing.
	void OnStart() override;
	// OnDetached() is called right before the component is about to be deleted.
	void OnDetached() override;

private:
	// Draws this component to the inspector window.
	virtual void EditorDraw() override;

	// Serialization defaults to the properties implementation. (the variables marked by the macros below) (iirc arrays and vectors are supported)
	// If your variables are non-trivial (such as a map), you can override these with your own implementation.
	// You can also use both the properties auto serialization and your own serialization by calling the base function (which is the properties auto serialization).
	// void Serialize(Serializer& writer) const override;
	// void Deserialize(Deserializer& reader) override;

	// Finally, here we list the variables that we want to include in serialization.
public:
	property_vtable()
};
property_begin(ExampleComponent)
{
	property_var(x)
}
property_vend_h(ExampleComponent)

// Now we'll create a system that runs on this ExampleComponent
// The first template argument must always be the system. Each template argument afterwards are the components that this system processes.
class ExampleSystem : public ecs::System<ExampleSystem, ExampleComponent>
	// For 2 or more components, simply add them into the template parameters.
	// e.g. ecs::System<ExampleSystem, ExampleComponent, Example2Component, etc...>
{
public:
	ExampleSystem();

private:
	// This function will be called for each entity that has the components required by the system.
	// The type of each argument must be l-value reference as per the below declaration.
	// For 2 or more components, simply add them into the function's arguments.
	// e.g. void UpdateComp(ExampleComponent& comp1, Example2Component& comp2);
	void UpdateComp(ExampleComponent& comp);

public:
	// In addition, there are 2 functions that run only once each time the system is executed.
	// PreRun() is called before processing components, and the bool return is so you can skip processing components this frame by returning false.
	bool PreRun() override;
	// PostRun() is called after processing all components, and is only called if PreRun() returned true.
	void PostRun() override;
};

// Fun tip, you can also have systems that process no components.
class ExampleSystem_NoComps : public ecs::System<ExampleSystem_NoComps>
{
public:
	// Systems processing no components do not require a constructor specifying a component update function (as seen in the previous system).
	// ExampleSystem_NoComps();

	bool PreRun() override;
};

// Remember to add your systems to GameSystems.cpp so that they will run in the first place!
