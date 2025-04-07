/******************************************************************************/
/*!
\file   ScriptComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/24/2024

\author Marc Alviz Evangelista (80%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\author Matthew Chan Shao Jie (20%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This file contains the definitions of the class Script Component used for
  the engine's ECS system.
  When an entity has this component, they are able to run C# scripts.

  This also contains the definition of the ScriptSystm class used to update 
  the ScriptComponents that are attached to entities

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "ScriptComponent.h"
#include "Physics.h"
#include "Collision.h"

#include "ResourceManager.h"
#include "EntityUID.h"
#include "NameComponent.h"

/*****************************************************************//*!
\brief
	Struct to hold the necessary information to transfer over to C# side.
\return
	None
*//******************************************************************/
struct DummyCollisionData
{
	Vector2 normal;
	Vector2 point;
	float depth;

	uint64_t eid;
	uint64_t other;

	/*****************************************************************//*!
	\brief
		Non-default constructor for the struct
	\param[in] m
		Mass of the entity to transfer over
	\param[in] fc
		Friction Coefficient of the entity to transfer over
	\param[in] vel
		Velocity of the entity to transfer over
	\param[in] aVel
		Angular velocity to transfer over
	\param[in] id
		Entitle handle of the entity the component is attached to
		to transfer over
	\return
		None
	*//******************************************************************/
	DummyCollisionData(const Physics::CollisionEventData& collisionData)
	{
		normal = collisionData.collisionData->collisionNormal;
		point = collisionData.collisionData->collisionPoint;
		depth = collisionData.collisionData->penetrationDepth;

		ecs::EntityHandle thisEntity = ecs::GetEntity(collisionData.refComp);
		ecs::EntityHandle otherEntity = ecs::GetEntity(collisionData.otherComp);
		// DummyEntity maybe?
		eid = reinterpret_cast<uint64_t>(thisEntity);
		other = reinterpret_cast<uint64_t>(otherEntity);
	}
};


ScriptComponent::ScriptComponent()
#ifdef IMGUI_ENABLED
: REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw)
#endif
{
}

ScriptComponent::ScriptComponent(const ScriptComponent& other) :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
	scriptMap(other.scriptMap)
{
	scriptsToAwaken = other.scriptsToAwaken;
	scriptsToStart = other.scriptsToStart;
	//for (auto const& pair : other.scriptMap)
	//{
	//	AddScript(pair.first);
	//}
}

ScriptComponent::ScriptComponent(ScriptComponent&& other) noexcept :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
	scriptMap(std::move(other.scriptMap))
{
	scriptsToAwaken = std::move(other.scriptsToAwaken);
	scriptsToStart = std::move(other.scriptsToStart);
}

ScriptComponent::~ScriptComponent()
{
	scriptMap.clear();
	openStates.clear();
	scriptsToAwaken.clear();
	scriptsToStart.clear();
}

#ifdef IMGUI_ENABLED
void ScriptComponent::EditorDraw(ScriptComponent& comp)
{
	std::unordered_map<std::string, CSharpScripts::ScriptInstance>& map = comp.scriptMap;
	std::unordered_map<std::string, bool>& states = comp.openStates;
	ImGui::Text("List of Scripts:");
	// for each script attached to entity do this
	//ImGui::BeginChild("list", ImVec2(0, 50), true);
	for (const auto& pair : map)
	{
		if (ImGui::Button(pair.first.c_str()))
		{
			states[pair.first] = !states[pair.first];
		}
		ImGui::SameLine();
		ImGui::PushID(pair.first.c_str());
		if (ImGui::Button("Remove")) 
		{
			comp.RemoveScript(pair.first);
			ImGui::PopID();
			break;
		}
		ImGui::PopID();

		// Show the public variables inside the script here
		if (states[pair.first])
		{
			auto pVars = comp.scriptMap[pair.first].GetPublicVars();

			// Define the height of the child rect based on the number of items
			const float itemHeight = ImGui::GetTextLineHeightWithSpacing(); // Height of each item
			const float padding = 45.0f; // Extra padding for aesthetics
			float totalHeight = (pVars.size() * itemHeight) + padding;

			// Begin the child rectangle with dynamic height
			ImGui::BeginChild(("Child Rect" + pair.first).c_str(), ImVec2(0, totalHeight), true);

			ImGui::Text("Details for %s", pair.first.c_str());

			for(auto& [name, field] : pVars)
			{
				auto& value = field.GetValue();
				ImGui::Text("%s:", name.c_str()); // Display the variable name

				ImGui::SameLine();
				// Determine the type of variable and create the appropriate input field
				std::visit([&](auto&& arg) 
				{
					using T = std::decay_t<decltype(arg)>;
					ImGui::PushID(name.c_str());
					if constexpr (std::is_same_v<T, int>) 
					{
						// Use a reference to update the original value directly
						if (ImGui::InputInt("", &std::get<int>(value))) 
						{
							// If needed, additional logic for change notification can be added here

							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
					}
					else if constexpr (std::is_same_v<T, bool>)
					{
						// Use a reference to update the original value directly
						if (ImGui::Checkbox("", &std::get<bool>(value)))
						{
							// If needed, additional logic for change notification can be added here
							comp.scriptMap[pair.first].SetPublicVar(name, value);

						}
					}
					else if constexpr (std::is_same_v<T, float>) 
					{
						// Use a reference to update the original value directly
						if (ImGui::InputFloat("", &std::get<float>(value)))
						{
							// If needed, additional logic for change notification can be added here
							comp.scriptMap[pair.first].SetPublicVar(name, value);

						}
					}
					else if constexpr (std::is_same_v<T, double>) 
					{
						// Use a reference to update the original value directly
						if (ImGui::InputDouble("", &std::get<double>(value)))
						{
							// If needed, additional logic for change notification can be added here
							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
					}
					else if constexpr (std::is_same_v<T, Vector3>)
					{
						// For Vector3, create individual input fields for each component (x, y, z)
						Vector3& vectorValue = std::get<Vector3>(value);

						ImGui::Text(name.c_str());
						if (ImGui::InputFloat("X", &vectorValue.x))
						{
							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
						if (ImGui::InputFloat("Y", &vectorValue.y))
						{
							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
						if (ImGui::InputFloat("Z", &vectorValue.z))
						{
							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
					}
					else if constexpr (std::is_same_v<T, size_t>)
					{
						ImGui::PushID(name.c_str());

						std::string entityName = "-";
						ecs::EntityHandle ent = EntityUIDLookup::GetEntity(std::get<size_t>(value));

						if (ent != nullptr)
						{
							entityName = ent->GetComp<NameComponent>()->GetName();
						}

						// Display the name in a text input field, but don't allow editing
						bool isHovered = ImGui::IsItemHovered(); // Check if the input field is being hovered

						// Make the field read-only, so no manual editing is allowed
						if (ImGui::InputText("", &entityName[0], entityName.size() + 1, ImGuiInputTextFlags_ReadOnly))
						{
						}

						// Add a visual cue to show that it's a drag-and-drop field
						if (isHovered)
						{
							ImGui::SetTooltip("Drop an entity here to assign its ID");
						}

						// Now, accept drag-and-drop on this InputText
						if (ImGui::BeginDragDropTarget())
						{
							// Check if the dropped payload is the correct one (ENTITY)
							const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY");

							// If the payload is valid
							if (payload != nullptr)
							{
								// Extract the dropped entity's UID (assumed to be a uint64_t)
								ecs::EntityHandle droppedHandle = *reinterpret_cast<ecs::EntityHandle*>(payload->Data);
								uint64_t droppedEntityId = droppedHandle->GetComp<EntityUIDComponent>()->GetUID();

								// Update the uint64_t field with the dropped entity's ID
								std::get<size_t>(value) = droppedEntityId;

								// Update the corresponding variable in the script
								comp.scriptMap[pair.first].SetPublicVar(name, value);
								
								if (droppedHandle != nullptr)
								{
									entityName = droppedHandle->GetComp<NameComponent>()->GetName();
								}

								CONSOLE_LOG_EXPLICIT("paylod dropped", LogLevel::LEVEL_DEBUG);
								CONSOLE_LOG_EXPLICIT("value is " + std::to_string(std::get<size_t>(value)), LogLevel::LEVEL_DEBUG);
								CONSOLE_LOG_EXPLICIT("name of value is: " + entityName, LogLevel::LEVEL_DEBUG);
							}

							ImGui::EndDragDropTarget();
						}

						ImGui::PopID();
					}
					else if constexpr (std::is_same_v<T, std::string>) {
						char buffer[256]; // Buffer for string input
						strncpy_s(buffer, std::get<std::string>(value).c_str(), sizeof(buffer));
						if (ImGui::InputText("", buffer, sizeof(buffer)))
						{
							// Update the original string if edited
							pVars.at(name).GetValue() = std::string(buffer); // Set the updated value back to the map
							comp.scriptMap[pair.first].SetPublicVar(name, value);
						}
						//if (ImGui::BeginDragDropTarget()) {
						//	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PREFAB");

						//	// If the payload is valid (e.g., an entity or prefab string)
						//	if (payload != nullptr && payload->DataSize > 0) {
						//		const char* droppedPrefab = (const char*)payload->Data;

						//		// Update the buffer with the dropped string
						//		strncpy(buffer, droppedPrefab, sizeof(buffer) - 1);
						//		buffer[sizeof(buffer) - 1] = '\0';  // Null-terminate buffer

						//		// Update the value (underlying data) with the dropped prefab name
						//		std::get<std::string>(value) = std::string(droppedPrefab);

						//		// Optionally update the script or other variables
						//		comp.scriptMap[pair.first].SetPublicVar(name, value);

						//		// Update the map with the dropped value
						//		pVars[name] = std::string(droppedPrefab);
						//	}
						//	ImGui::EndDragDropTarget();
						//}
					}
					ImGui::PopID();
				}, value);
			}

			// Add more details or controls related to obj here
			ImGui::EndChild();
		}
	}
	//ImGui::EndChild();

	ImGui::Text("Add New Script:");

	if (ImGui::BeginCombo("", "Select Script"))
	{
		std::unordered_map<std::string, CSharpScripts::ScriptClass>& coreMap = CSharpScripts::CSScripting::GetCoreClassMap();

		for (const auto& pair : coreMap)
		{
			if (comp.scriptMap.find(pair.first) == comp.scriptMap.end())
			{
				if (ImGui::Selectable(pair.first.c_str()))
				{
					comp.AddScript(pair.first);
				}
			}
		}
		ImGui::EndCombo();
	}
}
#endif

void ScriptComponent::AddScript(const std::string sName)
{
	auto it = scriptMap.find(sName);
	if (it == scriptMap.end())
	{
		scriptMap[sName] = std::move(CSharpScripts::ScriptInstance(CSharpScripts::CSScripting::GetClassFromData(sName)));
		openStates[sName] = false;
		// set the e_id of the script here
		scriptMap[sName].SetHandleInvoke(*this);
		std::string msg = std::to_string(reinterpret_cast<unsigned long long>(this));
		CONSOLE_LOG_EXPLICIT(msg, LogLevel::LEVEL_DEBUG);
		scriptMap[sName].OnCreateInvoke();
		scriptMap[sName].RetrievePublicVariables();
		scriptsToAwaken.push_back(sName);
	}
}

void ScriptComponent::RemoveScript(const std::string sName)
{
	auto it = scriptMap.find(sName);
	if (it != scriptMap.end())
	{
		scriptMap.erase(sName);
		openStates.erase(sName);
		auto ite = std::find(scriptsToAwaken.begin(), scriptsToAwaken.end(), sName);
		if (ite != scriptsToAwaken.end())
		{
			scriptsToAwaken.erase(ite);
		}

		ite = std::find(scriptsToStart.begin(), scriptsToStart.end(), sName);
		if (ite != scriptsToStart.end())
		{
			scriptsToStart.erase(ite);
		}
	}
}

void ScriptComponent::InvokeOnUpdate()
{
	for (auto& pair : scriptMap)
	{
		std::string name = pair.first;
		pair.second.OnUpdateInvoke(GameTime::FixedDt());

	}
}

void ScriptComponent::InvokeAwake()
{
	if (scriptsToAwaken.empty())
	{
		return;
	}

	std::vector<std::string> namesToRemove;
	// Awaken each script then add it to remove
	for (std::string name : scriptsToAwaken)
	{
		scriptMap[name].AwakeInvoke();
		namesToRemove.push_back(name);
	}

	if (!namesToRemove.empty())
	{
		// for each script to remove add it to scripts to start
		for (const std::string& name : namesToRemove)
		{
			scriptsToAwaken.erase(std::remove(scriptsToAwaken.begin(), scriptsToAwaken.end(), name), scriptsToAwaken.end());
			scriptsToStart.push_back(name);
		}
	}
}

void ScriptComponent::InvokeOnStart()
{
	if (scriptsToStart.empty())
	{
		return;
	}

	std::vector<std::string> namesToRemove;
	// Awaken each script then add it to remove
	for (std::string name : scriptsToStart)
	{
		scriptMap[name].OnStartInvoke();
		namesToRemove.push_back(name);
	}

	if (!namesToRemove.empty())
	{
		// for each script to remove add it to scripts to start
		for (const std::string& name : namesToRemove)
		{
			scriptsToStart.erase(std::remove(scriptsToStart.begin(), scriptsToStart.end(), name), scriptsToStart.end());
		}
	}
}

void ScriptComponent::InvokeLateUpdate()
{
	for (const auto& pair : scriptMap)
	{
		scriptMap[pair.first].LateUpdateInvoke(GameTime::FixedDt());
	}
}

void ScriptComponent::SaveVariables()
{
	// Clear all these first

	// Save everything first
	pvs.clear();
	names.clear();
	scriptsToAwaken.clear();
	scriptsToStart.clear();

	for (auto& pair : scriptMap)
	{
		names.push_back(pair.first);
		pvs.push_back(pair.second.GetPublicVars());
	}
}

void ScriptComponent::InvokeSetHandle()
{
	for (auto& pair : scriptMap)
	{
		pair.second.SetHandleInvoke(*this);
	}
}

MonoObject* ScriptComponent::FindScriptInstance(std::string name)
{
	std::string s = ".";
	s += name;

	std::unordered_map<std::string, CSharpScripts::ScriptInstance>::iterator it = scriptMap.find(s);

	if (it != scriptMap.end())
	{
		//std::cout << "found the instance";
		return 	it->second.GetInstance();
	}
	return nullptr;
}

void ScriptComponent::Serialize(Serializer& writer) const
{
	//Serialize scriptMap
	//Maybe serailize openStates as well? Could be convenient
	writer.Serialize("scriptMap", scriptMap);
	writer.Serialize("openStates", openStates);
}

void ScriptComponent::Deserialize(Deserializer& entry)
{
	entry.DeserializeVar("scriptMap",&scriptMap);
	entry.DeserializeVar("openStates",&openStates);

	for (auto& script : scriptMap)
	{
		script.second.SetHandleInvoke(*this);
		script.second.OnCreateInvoke();
		auto pVars = script.second.GetPublicVars();
		for (auto var : pVars)
		{
			script.second.SetPublicVar(var.first, var.second.GetValue());
		}
		scriptsToAwaken.push_back(script.first);
	}
}

void ScriptComponent::RemoveAllScripts()
{
	for (std::string n : names)
	{
		RemoveScript(n);
	}
}

void ScriptComponent::ReattachAllScripts()
{
	// After all the scripts
	for (std::string n : names)
	{
		AddScript(n);
		scriptMap[n].RetrievePublicVariables();
	}
}

void ScriptComponent::LoadVariables()
{
	// Set the public vars
	// for each script inside the map
	// set the public vars from inside pvs
	for (size_t i = 0; i < names.size(); ++i)
	{
		// CONSOLE_LOG_EXPLICIT("Script: " + names[i], LogLevel::LEVEL_DEBUG);
		for (auto& pair : pvs[i])
		{
			scriptMap[names[i]].SetPublicVar(pair.first, pair.second.GetValue());
			//CONSOLE_LOG_EXPLICIT("Var"+ std::to_string(x++) +": " + pair.first, LogLevel::LEVEL_DEBUG);
		}
	}
	pvs.clear();
	names.clear();
}

void ScriptComponent::OnAttached()
{
	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->Subscribe("OnCollision", this, &ScriptComponent::OnCollision);
}

void ScriptComponent::OnDetached()
{
	if (auto eventsComp{ ecs::GetEntity(this)->GetComp<EntityEventsComponent>() })
		eventsComp->Unsubscribe("OnCollision", this, &ScriptComponent::OnCollision);
}

void ScriptComponent::OnCollision(const Physics::CollisionEventData& collisionData)
{
	// Iterate through all scripts and call function "OnCoillisionEnter(Collision collision)"
	// We probably want OnTriggerEnter as well
	// TEST FIRST FFS

	for (auto &scriptPair : scriptMap)
	{
		CSharpScripts::ScriptInstance& scriptInstance = scriptPair.second;
		DummyCollisionData d(collisionData);
		scriptInstance.OnCollisionInvoke((Physics::CollisionEventData &)collisionData);
	}
}


ScriptSystem::ScriptSystem()
	: System_Internal{ &ScriptSystem::UpdateScriptComp }
{
}

void ScriptSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptSystem::UpdateScriptComp(ScriptComponent& comp)
{
	comp.InvokeOnUpdate();
}

ScriptAwakeSystem::ScriptAwakeSystem()
	: System_Internal{ &ScriptAwakeSystem::AwakenScriptComp }

{
}

void ScriptAwakeSystem::AwakenScriptComp(ScriptComponent& comp)
{
	comp.InvokeAwake();
}

ScriptStartSystem::ScriptStartSystem()
	: System_Internal{ &ScriptStartSystem::StartScriptComp }

{
}

void ScriptStartSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptStartSystem::StartScriptComp(ScriptComponent& comp)
{
	comp.InvokeOnStart();
}

ScriptLateUpdateSystem::ScriptLateUpdateSystem()
	: System_Internal{ &ScriptLateUpdateSystem::LateUpdateScriptComp }

{
}

void ScriptLateUpdateSystem::PostRun()
{
	ecs::FlushChanges();
}

void ScriptLateUpdateSystem::LateUpdateScriptComp(ScriptComponent& comp)
{
	comp.InvokeLateUpdate();
}

ScriptPreAwakeSystem::ScriptPreAwakeSystem()
	: System_Internal{ &ScriptPreAwakeSystem::PreAwakeScriptComp }
{
}

void ScriptPreAwakeSystem::PreAwakeScriptComp(ScriptComponent& comp)
{
	comp.InvokeSetHandle();
}
