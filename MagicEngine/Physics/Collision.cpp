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
#include "Engine/EntityEvents.h"

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


	const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		if (inLayer == BroadPhaseLayers::NON_MOVING)
			return "NON_MOVING";
		if (inLayer == BroadPhaseLayers::MOVING)
			return "MOVING";
		if (inLayer == BroadPhaseLayers::NON_COLLIDABLE)
			return "NON_COLLIDABLE";
		return "UNKNOWN";
	}

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
	std::vector<std::pair<std::pair<ecs::EntityHash, JPH::BodyID>, ContactTiming>> MyCharacterContactListener::characterBodyContactPair{};
	std::vector<std::pair<std::pair<ecs::EntityHash, ecs::EntityHash>, ContactTiming>> MyCharacterContactListener::charCharContactPair{};


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

	std::string ContactFuncName(bool isTrigger, ContactTiming timing)
	{
		if (isTrigger)
		{
			switch (timing)
			{
			case ContactTiming::ENTER:
				return "OnTriggerEnter";
			case ContactTiming::STAY:
				return "OnTriggerStay";
			case ContactTiming::EXIT:
				return "OnTriggerExit";
			}
		}
		else
		{
			switch (timing)
			{
			case ContactTiming::ENTER:
				return "OnCollisionEnter";
			case ContactTiming::STAY:
				return "OnCollisionStay";
			case ContactTiming::EXIT:
				return "OnCollisionExit";
			}
		}
		return "";
	}

	void MyContactListener::CallContactFunc()
	{
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
			ecs::EntityHandle entity1{ ecs::GetEntity(entityHash1) };
			ecs::EntityHandle entity2{ ecs::GetEntity(entityHash2) };
			if (!ecs::IsEntityHandleValid(entity1) || !ecs::IsEntityHandleValid(entity2))
				continue;

			auto scriptComp1{ entity1->GetComp<ScriptComponent>() };
			auto colliderComp1{ entity1->GetComp<BoxColliderComp>() };
			if (scriptComp1 && colliderComp1 && colliderComp1->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				scriptComp1->CallScriptFunction(ContactFuncName(colliderComp1->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER), contactBodyPair.second), entity2);

			auto scriptComp2{ ecs::GetEntity(entityHash2)->GetComp<ScriptComponent>() };
			auto colliderComp2{ ecs::GetEntity(entityHash2)->GetComp<BoxColliderComp>() };
			if (scriptComp2 && colliderComp2 && colliderComp2->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				scriptComp2->CallScriptFunction(ContactFuncName(colliderComp2->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER), contactBodyPair.second), entity1);
		}
	}

	void MyCharacterContactListener::OnContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		auto pair{ std::make_pair(inCharacter->GetUserData(), inBodyID2)};
		characterBodyContactPair.push_back(std::make_pair(pair, ContactTiming::ENTER));
	}

	void MyCharacterContactListener::OnContactPersisted(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		auto pair{ std::make_pair(inCharacter->GetUserData(), inBodyID2) };
		characterBodyContactPair.push_back(std::make_pair(pair, ContactTiming::STAY));
	}

	void MyCharacterContactListener::OnContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::BodyID& inBodyID2, const JPH::SubShapeID& inSubShapeID2)
	{
		auto pair{ std::make_pair(inCharacter->GetUserData(), inBodyID2) };
		characterBodyContactPair.push_back(std::make_pair(pair, ContactTiming::EXIT));
	}

	void MyCharacterContactListener::OnCharacterContactAdded(const JPH::CharacterVirtual* inCharacter, const JPH::CharacterVirtual* inOtherCharacter, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		auto pair{ std::make_pair(inCharacter->GetUserData(), inOtherCharacter->GetUserData()) };
		charCharContactPair.push_back(std::make_pair(pair, ContactTiming::ENTER));
	}

	void MyCharacterContactListener::OnCharacterContactPersisted(const JPH::CharacterVirtual* inCharacter, const JPH::CharacterVirtual* inOtherCharacter, const JPH::SubShapeID& inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::CharacterContactSettings& ioSettings)
	{
		auto pair{ std::make_pair(inCharacter->GetUserData(), inOtherCharacter->GetUserData()) };
		charCharContactPair.push_back(std::make_pair(pair, ContactTiming::STAY));
	}

	//void MyCharacterContactListener::OnCharacterContactRemoved(const JPH::CharacterVirtual* inCharacter, const JPH::CharacterID& inOtherCharacterID, const JPH::SubShapeID& inSubShapeID2)
	//{
	//	auto pair{ std::make_pair(inCharacter->GetUserData(), inOtherCharacter->GetUserData()) };
	//	charCharContactPair.push_back(std::make_pair(pair, ContactTiming::EXIT));
	//}

	void MyCharacterContactListener::ClearContactPair()
	{
		characterBodyContactPair.clear();
		charCharContactPair.clear();
	}

	void MyCharacterContactListener::CallContactFunc()
	{
		//Get the body interface.
		JPH::BodyInterface& bodyInterface{ ST<JoltPhysics>::Get()->GetBodyInterface() };

		//Loop through all the character vs body contact occured.
		for (auto& contactBodyPair : characterBodyContactPair)
		{
			//Get the entity hash of the contact objects.
			ecs::EntityHash entityHash1{ contactBodyPair.first.first };
			ecs::EntityHash entityHash2{ bodyInterface.GetUserData(contactBodyPair.first.second) };
			if (!entityHash1 || !entityHash2)
				continue;
			ecs::EntityHandle entity1{ ecs::GetEntity(entityHash1) };
			ecs::EntityHandle entity2{ ecs::GetEntity(entityHash2) };
			if (!ecs::IsEntityHandleValid(entity1) || !ecs::IsEntityHandleValid(entity2))
				continue;

			auto scriptComp1{ entity1->GetComp<ScriptComponent>() };
			if (scriptComp1)
				scriptComp1->CallScriptFunction(ContactFuncName(false, contactBodyPair.second), entity2);

			auto scriptComp2{ ecs::GetEntity(entityHash2)->GetComp<ScriptComponent>() };
			auto colliderComp2{ ecs::GetEntity(entityHash2)->GetComp<BoxColliderComp>() };
			if (scriptComp2 && colliderComp2 && colliderComp2->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				scriptComp2->CallScriptFunction(ContactFuncName(colliderComp2->GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER), contactBodyPair.second), entity1);
		}

		//Loop through all the character vs character contact occured.
		for (auto& contactPair : charCharContactPair)
		{
			//Get the entity hash of the contact objects.
			ecs::EntityHash entityHash1{ contactPair.first.first };
			ecs::EntityHash entityHash2{ contactPair.first.second };
			if (!entityHash1 || !entityHash2)
				continue;
			ecs::EntityHandle entity1{ ecs::GetEntity(entityHash1) };
			ecs::EntityHandle entity2{ ecs::GetEntity(entityHash2) };
			if (!ecs::IsEntityHandleValid(entity1) || !ecs::IsEntityHandleValid(entity2))
				continue;

			auto scriptComp1{ entity1->GetComp<ScriptComponent>() };
			if (scriptComp1)
				scriptComp1->CallScriptFunction(ContactFuncName(false, contactPair.second), entity2);
		}
	}

	BoxColliderComp::BoxColliderComp()
		: flags{COLLIDER_COMP_FLAG::ENABLED}
		, center{}
		, size{1.f, 1.f, 1.f}
	{
	}

	void BoxColliderComp::OnCreation()
	{
		auto entity{ ecs::GetEntity(this) };
		auto physCompPtr{ entity->GetComp<PhysicsComp>() };

		// Note: If JoltBodyComp is crashing due to invalid body ID, try using this bool and reattaching the comp
		// bodyCompPtr->GetBodyID().IsInvalid()

		// If JoltBodyComp is missing, attach it.
		auto bodyCompPtr{ entity->GetComp<JoltBodyComp>() };
		if (!bodyCompPtr)
		{
			assert(!physCompPtr); // JoltBodyComp would exist if PhysicsComp exists. If not, something's wrong with PhysicsComp or OnCreation()...
			entity->AddComp(JoltBodyComp{ JPH::EMotionType::Static, ShapeType::BOX, Layers::NON_MOVING });
		}
		else
		{
			// JoltBodyComp already exists, perhaps because a PhysicsComp was attached to this entity earlier.
			// Update JoltBodyComp parameters to that where a BoxColliderComp is attached to the entity.
			bodyCompPtr->SetShapeType(ShapeType::BOX);
		}
	}

	void BoxColliderComp::OnAttached()
	{
		auto entity{ ecs::GetEntity(this) };
		auto bodyCompPtr{ entity->GetComp<JoltBodyComp>() };
		assert(bodyCompPtr); // OnCreation() should've been called before OnAttached(). Something's very wrong within ecs if this assert fails.

		// Initialize JoltBodyComp
		bodyCompPtr->SetPosition(entity->GetTransform().GetWorldPosition() + GetCenter());
		bodyCompPtr->SetScale(entity->GetTransform().GetWorldScale() * GetSize());
		Layers layer{};
		if (!GetFlag(COLLIDER_COMP_FLAG::ENABLED))
			layer = Layers::NON_COLLIDABLE;
		else if (entity->HasComp<PhysicsComp>())
			layer = Layers::MOVING;
		else
			layer = Layers::NON_MOVING;
		bodyCompPtr->SetCollisionLayer(layer);

		bodyCompPtr->SetIsTrigger(GetFlag(COLLIDER_COMP_FLAG::IS_TRIGGER));
	}

	void BoxColliderComp::OnDetached()
	{
		auto entity{ ecs::GetEntity(this) };
		if (!entity->HasComp<JoltBodyComp>())
			return;

		if (!entity->HasComp<PhysicsComp>())
			entity->RemoveComp<JoltBodyComp>();
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
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetIsTrigger(IsTrigger());
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

		if (auto bodyComp = ecs::GetEntity(this)->GetComp<JoltBodyComp>())
			if (bodyComp->GetPrevTrans() != TransformValues{ ecs::GetEntityTransform(this) })
				bodyComp->UpdateBody();
	}

}