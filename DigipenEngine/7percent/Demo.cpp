#include "pch.h"
#include "Demo.h"
#include "NameComponent.h"

ExampleComponent::ExampleComponent()
	: x{ 24.0f, -10.0f }
{
	// IT IS NOT SAFE TO GET THIS COMPONENT'S ENTITY IN THE CONSTRUCTOR OR DESTRUCTOR! THE COMPONENT IS DEFINITELY NOT ATTACHED TO AN ENTITY HERE YET!
	// Only do so in OnAttached(), OnStart() and OnDetached().
	// ecs::GetEntity(this)->GetComp<NameComponent>(); // WILL CRASH!
}

void ExampleComponent::OnAttached()
{
	// Logging to console is like std::cout syntax.
	// You can get the entity this component is attached to via ecs::GetEntity() and passing in the component's address.
	CONSOLE_LOG(LEVEL_INFO) << "Example component attached to entity " << ecs::GetEntity(this)
		// There are some components that are guaranteed to exist on all entities (see class EntitySpawnEvents)
		<< " of name " << ecs::GetEntity(this)->GetComp<NameComponent>()->GetName();

	// If inheriting IGameComponentCallbacks, this base function MUST be called, and at the end (unless you're ok with OnStart() potentially being processed first).
	IGameComponentCallbacks::OnAttached();
}

void ExampleComponent::OnStart()
{
	CONSOLE_LOG(LEVEL_INFO) << "Simulation has started with example component attached to entity " << ecs::GetEntity(this);
}

void ExampleComponent::OnDetached()
{
	// Be careful in OnDetached(), guaranteed-to-exist components may not exist anymore if the entity is being deleted outright
	ecs::CompHandle<NameComponent> nameComp{ ecs::GetEntity(this)->GetComp<NameComponent>() };
	CONSOLE_LOG(LEVEL_INFO) << "Example component detached(removed) from entity " << ecs::GetEntity(this)
		<< " of name " << (nameComp ? nameComp->GetName() : "[unknown]");
}

void ExampleComponent::EditorDraw()
{
	// If using ImGui functions directly, be sure to put them within #ifdef IMGUI_ENABLED blocks.
//#ifdef IMGUI_ENABLED
//	ImGui::DragFloat2("x", &x.x);
//#endif

	// namespace "gui" contains many functions and constructs that remove the need to "end" your "begin" ImGui calls, provides a more C++ style interface,
	// convenience functions that reduce the amount of code you need to write, and removes the need to use #ifdef IMGUI_ENABLED. However, it is
	// incomplete, so you may need to fall back to direct ImGui calls for some things until I get around to adding them.

	{
		// Sets the color of text until this goes out of scope
		gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 1.0f, 0.0f, 1.0f, 1.0f } };
		gui::TextUnformatted("Example pink text!");
	}
	gui::TextFormatted("Example normal text");

	gui::VarDrag("x", &x); // ps. notice the single "x" instead of the multiple "x.x" in the ImGui call at the top of this function, and how the format is a bit nicer when displayed in the ImGui window.

	// Buttons (and many other gui classes) can be tested like a bool to check whether they were clicked (or open, depending on the class) this frame.
	if (gui::Button button{ "Log a message to console" })
		CONSOLE_LOG(LEVEL_INFO) << "Logging a message to console!";
}

ExampleSystem::ExampleSystem()
// The function that you want to be called per entity processed must be specified here.
	: System_Internal{ &ExampleSystem::UpdateComp }
{
}

void ExampleSystem::UpdateComp(ExampleComponent& comp)
{
	// Do whatever you want in here with each component!

	// IMPORTANT!!! Storing the address of components is unsafe as they may change at any time!
	// However, ecs::EntityHandle will never change and are thus safe to store until the entity is deleted.

	// The entity can be obtained via ecs::GetEntity().
	ecs::EntityHandle entity{ ecs::GetEntity(&comp) };

	// An ecs::EntityHandle's validity can be checked via this function
	if (ecs::IsEntityHandleValid(entity))
		// ecs::EntityHandle and even iterators have some helpful functions!
		for (auto compIter{ entity->Comps_Begin() }, endIter{ entity->Comps_End() }; compIter != endIter; ++compIter)
			if (compIter.GetCompHash() == ecs::GetCompHash<ExampleComponent>())
				break;

	// Transform are built into entities
	Transform& transform{ entity->GetTransform() }; // ecs::GetEntityTransform(&comp) is a slight shortcut to get an entity's transform from a component address
	// You can get the entity directly from transform as well
	assert(transform.GetEntity() == entity);

	// Parent/child relationships are obtained via Transform.
	if (std::distance(transform.GetChildren().begin(), transform.GetChildren().end()) > 1)
		CONSOLE_LOG(LEVEL_INFO) << "This entity has 2 or more children!";
	if (transform.GetParent())
		CONSOLE_LOG(LEVEL_INFO) << "This entity has a parent!";

	// Let's move the entity to the right by 10 units each time this system runs, to demonstrate that this system works.
	transform.AddLocalPosition(Vector2{ 10.0f, 0.0f });
}

bool ExampleSystem::PreRun()
{
	// Let's only run this system every 5 times
	static int counter{};
	return !(++counter % 5);
}

void ExampleSystem::PostRun()
{
}

bool ExampleSystem_NoComps::PreRun()
{
	// You can also get components from anywhere in the code with these iterators (either active only or both active/inactive)
	for (auto compIter{ ecs::GetCompsActiveBegin<ExampleComponent>() }, endIter{ ecs::GetCompsEnd<ExampleComponent>() }; compIter != endIter; ++compIter)
		// There are many other functions and classes that I can't possibly introduce in this demo, so do take a look around,
		// and if you find implementing something to be unreasonably difficult, maybe there's an easier way that exists within this project
		// that you just don't know about yet, so don't be afraid to ask for help!
		assert(ecs::IsEntityHandleValid(compIter.GetEntity()));

	return false;
}
