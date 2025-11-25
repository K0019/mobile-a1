/******************************************************************************/
/*!
\file   Collision.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains definition of member functions for collision layers and broad phase
	collision layers. It also contains the definition of the collision listener
	of Jolt Physics and the collision component member functions.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Editor/Containers/GUICollection.h"
#include "Scripting/ScriptComponent.h"

#define X(name, str) str,
static const char* colliderFlagNames[]{
	D_COLLIDER_COMP_FLAG
};
#undef X

namespace physics {
	bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
	{
		switch (inObject1)
		{
		case +Layers::NON_MOVING:
			return +inObject2 == +Layers::MOVING; // Non moving only collides with moving
		case +Layers::MOVING:
			return +inObject2 != +Layers::NON_COLLIDABLE; // Moving collides with everything except non collidable
		case +Layers::NON_COLLIDABLE:
			return false; // Can't collide with anything.
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	BPLayerInterfaceImpl::BPLayerInterfaceImpl()
	{
		mObjectToBroadPhase[+Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[+Layers::MOVING] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[+Layers::NON_COLLIDABLE] = BroadPhaseLayers::NON_COLLIDABLE;
	}

	JPH::uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		JPH_ASSERT(+inLayer < +Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}


	// TODO could not get this to compile. need to revisit later.
	/*const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer))
		{
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
			return "NON_MOVING";
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
			return "MOVING";
		default:
			JPH_ASSERT(false);
			return "INVALID";
		}
	}*/

	bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
		case +Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case +Layers::MOVING:
			return inLayer2 != BroadPhaseLayers::NON_COLLIDABLE;
		case +Layers::NON_COLLIDABLE:
			return false;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	std::vector<std::pair<std::pair<JPH::BodyID, JPH::BodyID>, ContactTiming>> MyContactListener::contactPair{};

	void MyContactListener::Init()
	{
		contactPair.reserve(1000);
	}

	void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		auto bodyIDPair{ std::make_pair(inBody1.GetID(), inBody2.GetID()) };
		contactPair.push_back(std::make_pair(bodyIDPair, ContactTiming::ENTER));
	}

	void MyContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		auto bodyIDPair{ std::make_pair(inBody1.GetID(), inBody2.GetID()) };
		contactPair.push_back(std::make_pair(bodyIDPair, ContactTiming::STAY));
	}

	void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		auto bodyIDPair{ std::make_pair(inSubShapePair.GetBody1ID(), inSubShapePair.GetBody2ID())};
		contactPair.push_back(std::make_pair(bodyIDPair, ContactTiming::EXIT));
	}

	void MyContactListener::ClearContactPair()
	{
		contactPair.clear();
	}

	void MyContactListener::CallContactFunc()
	{
		ST<Scheduler>::Get()->Add([]() {
			//Get the body interface.
			JPH::BodyInterface& bodyInterface{ ST<JoltPhysics>::Get()->GetBodyInterface() };

			//Loop through all the contact occured.
			for (auto& contactBodyPair : contactPair)
			{
				//Get the entity hash of the contact objects.
				ecs::EntityHash entityHash1{ bodyInterface.GetUserData(contactBodyPair.first.first) };
				ecs::EntityHash entityHash2{ bodyInterface.GetUserData(contactBodyPair.first.second) };
				if (!entityHash1 || !entityHash2)
					continue;
				auto scriptComp1{ ecs::GetEntity(entityHash1)->GetComp<ScriptComponent>() };
				auto colliderComp1{ ecs::GetEntity(entityHash1)->GetComp<BoxColliderComp>() };
				if (scriptComp1 && colliderComp1 && colliderComp1->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				{
					std::string funcName{};
					ecs::EntityHandle entity2 = ecs::GetEntity(entityHash2);
					if (ecs::IsEntityHandleValid(entity2))
					{
						if (colliderComp1->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER))
						{
							switch (contactBodyPair.second)
							{
							case ContactTiming::ENTER:
								funcName = "OnTriggerEnter";
								break;
							case ContactTiming::STAY:
								funcName = "OnTriggerStay";
								break;
							case ContactTiming::EXIT:
								funcName = "OnTriggerExit";
								break;
							}
							scriptComp1->CallScriptFunction(funcName, entity2);
						}
						else
						{
							switch (contactBodyPair.second)
							{
							case ContactTiming::ENTER:
								funcName = "OnCollisionEnter";
								break;
							case ContactTiming::STAY:
								funcName = "OnCollisionStay";
								break;
							case ContactTiming::EXIT:
								funcName = "OnCollisionExit";
								break;
							}
							scriptComp1->CallScriptFunction(funcName, entity2);
						}
					}
				}

				auto scriptComp2{ ecs::GetEntity(entityHash2)->GetComp<ScriptComponent>() };
				auto colliderComp2{ ecs::GetEntity(entityHash2)->GetComp<BoxColliderComp>() };
				if (scriptComp2 && colliderComp2 && colliderComp2->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				{
					std::string funcName{};
					ecs::EntityHandle entity1 = ecs::GetEntity(entityHash1);
					if (ecs::IsEntityHandleValid(entity1))
					{
						if (colliderComp2->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER))
						{
							switch (contactBodyPair.second)
							{
							case ContactTiming::ENTER:
								funcName = "OnTriggerEnter";
								break;
							case ContactTiming::STAY:
								funcName = "OnTriggerStay";
								break;
							case ContactTiming::EXIT:
								funcName = "OnTriggerExit";
								break;
							}
							scriptComp2->CallScriptFunction(funcName, entity1);
						}
						else
						{
							switch (contactBodyPair.second)
							{
							case ContactTiming::ENTER:
								funcName = "OnCollisionEnter";
								break;
							case ContactTiming::STAY:
								funcName = "OnCollisionStay";
								break;
							case ContactTiming::EXIT:
								funcName = "OnCollisionExit";
								break;
							}
							scriptComp2->CallScriptFunction(funcName, entity1);
						}
					}
				}
			}
			});
	}

	BoxColliderComp::BoxColliderComp()
		: flags{COLLIDER_COMP_FLAG::ENABLED}
		, center{}
		, size{1.f, 1.f, 1.f}
	{
	}

	void BoxColliderComp::OnAttached()
	{
		//If the entity has the physics component, get the body pointer of the physics component.
		//If not, create a body.
		ST<Scheduler>::Get()->Add(0.0f, [entity = ecs::GetEntity(this)]() {
			if (!ecs::IsEntityHandleValid(entity))
				return;
			auto compPtr{ entity->GetComp<BoxColliderComp>() };
			if (!compPtr)
				return;

			if (entity->HasComp<PhysicsComp>())
			{
				auto bodyCompPtr{ entity->GetComp<JoltBodyComp>() };
				if (!bodyCompPtr || bodyCompPtr->GetBodyID().IsInvalid())
					entity->AddCompNow<JoltBodyComp>(JoltBodyComp{ JPH::EMotionType::Dynamic, ShapeType::BOX, Layers::MOVING });
				else
					bodyCompPtr->SetShapeType(ShapeType::BOX);
			}
			else
			{
				entity->AddCompNow<JoltBodyComp>(JoltBodyComp{ JPH::EMotionType::Static, ShapeType::BOX, Layers::NON_MOVING });
			}

			entity->GetComp<JoltBodyComp>()->SetPosition(entity->GetTransform().GetWorldPosition() + compPtr->GetCenter());
			entity->GetComp<JoltBodyComp>()->SetScale(entity->GetTransform().GetWorldScale() * compPtr->GetSize());
			Layers layer{};
			if (!compPtr->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				layer = Layers::NON_COLLIDABLE;
			else if (entity->HasComp<PhysicsComp>())
				layer = Layers::MOVING;
			else
				layer = Layers::NON_MOVING;
			entity->GetComp<JoltBodyComp>()->SetCollisionLayer(layer);

			entity->GetComp<JoltBodyComp>()->SetIsTrigger(compPtr->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER));

		});
	}

	void BoxColliderComp::OnDetached()
	{
		if (!ecs::GetEntity(this)->HasComp<JoltBodyComp>())
			return;

		if (!ecs::GetEntity(this)->HasComp<PhysicsComp>())
			ecs::GetEntity(this)->RemoveComp<JoltBodyComp>();
		else
		{
			JoltBodyComp* bodyCompPtr{ ecs::GetEntity(this)->GetComp<JoltBodyComp>() };
			bodyCompPtr->SetCollisionLayer(Layers::NON_COLLIDABLE);
			bodyCompPtr->SetShapeType(ShapeType::EMPTY);
			bodyCompPtr->SetPosition(ecs::GetEntityTransform(this).GetWorldPosition());
			bodyCompPtr->SetScale(ecs::GetEntityTransform(this).GetWorldScale());
		}
	}

	bool BoxColliderComp::GetFlag(COLLIDER_COMP_FLAG flag) const
	{
		return flags.TestMask(flag);
	}

	void BoxColliderComp::SetFlag(COLLIDER_COMP_FLAG flag, bool val)
	{
		flags.SetMask(flag, val);

		Layers layer{};
		switch (flag)
		{
		case COLLIDER_COMP_FLAG::ENABLED:
			if (!val)
				layer = Layers::NON_COLLIDABLE;
			else if (ecs::GetEntity(this)->HasComp<PhysicsComp>())
				layer = Layers::MOVING;
			else
				layer = Layers::NON_MOVING;
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetCollisionLayer(layer);
			break;
		case COLLIDER_COMP_FLAG::IS_TRIGGER:
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetIsTrigger(val);
			break;
		}
	}

	Vec3 const& BoxColliderComp::GetCenter() const
	{
		return center;
	}

	void BoxColliderComp::SetCenter(const Vec3& val)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetPosition(ecs::GetEntityTransform(this).GetWorldPosition() + val);
		center = val;
	}

	Vec3 const& BoxColliderComp::GetSize() const
	{
		return size;
	}

	void BoxColliderComp::SetSize(const Vec3& val)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetScale(ecs::GetEntityTransform(this).GetWorldScale() * val);
		size = val;
	}

	void BoxColliderComp::Serialize(Serializer& writer) const
	{
		ISerializeable::Serialize(writer);
		flags.MaskSerialize(writer, "flags", colliderFlagNames);
	}

	void BoxColliderComp::Deserialize(Deserializer& reader)
	{
		ISerializeable::Deserialize(reader);
		flags.MaskDeserialize(reader, "flags", colliderFlagNames);
	}

	void BoxColliderComp::EditorDraw()
	{
		bool enabled{ GetFlag(COLLIDER_COMP_FLAG::ENABLED) };
		if (gui::Checkbox(colliderFlagNames[+COLLIDER_COMP_FLAG::ENABLED], &enabled))
		{
			SetFlag(COLLIDER_COMP_FLAG::ENABLED, enabled);
		}
		bool trigger{ GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER) };
		if (gui::Checkbox(colliderFlagNames[+COLLIDER_COMP_FLAG::IS_TRIGGER], &trigger))
		{
			SetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER, trigger);
		}

		gui::SetStyleVar styleFramePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 4.0f, 2.0f } };
		gui::SetStyleVar styleItemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 4.0f, 2.0f } };
		gui::Indent indent{ 4.0f };

		// TODO: This is copied from Transform.cpp. Should unify both implementations in GUICollection in the future.
		// Helper function for drawing the controls
		const auto DrawVec3Control = [](const char* label, Vec3* values, float columnWidth, float speed, const char* format) -> bool {
			bool modified = false;
			if (gui::Table table{ label, 4, true, gui::FLAG_TABLE::HIDE_HEADER })
			{
				table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_FIXED, columnWidth); // Set first column as fixed width. The rest will be stretch columns.
				table.SubmitColumnHeaders();

				gui::TextUnformatted(label);
				table.NextColumn();

				const auto DrawFloatComponent{ [&modified, speed, format](const char* text, const char* label, float* value, const gui::Vec4& textColor) -> void {
					{
						gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, textColor };
						gui::TextUnformatted(text);
					}
					gui::SameLine();
					gui::SetNextItemWidth(gui::GetAvailableContentRegion().x);
					modified |= gui::VarDrag(label, value, speed, 0.0f, 0.0f, format);
				} };

				DrawFloatComponent("X", "##X", &values->x, gui::Vec4{ 1.0f, 0.4f, 0.4f, 1.0f });
				table.NextColumn();
				DrawFloatComponent("Y", "##Y", &values->y, gui::Vec4{ 0.4f, 1.0f, 0.4f, 1.0f });
				table.NextColumn();
				DrawFloatComponent("Z", "##Z", &values->z, gui::Vec4{ 0.4f, 0.4f, 1.0f, 1.0f });
			}
			return modified;
			};

		//Center of collider control
		Vec3 tempVec{ center };
		if (DrawVec3Control("Center", &tempVec, 60.f, 1.f, "%.1f"))
		{
			SetCenter(tempVec);
		}

		//Size of collider control
		tempVec = size;
		if (DrawVec3Control("Size", &tempVec, 60.f, 0.02f, "%.01f"))
		{
			SetSize(tempVec);
		}
	}

}