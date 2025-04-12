/******************************************************************************/
/*!
\file   Collision.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements the Collision system, QuadTree, collision component,
  and various collider definitions such as AABB, that implement a structure that can detect
  and separate collisions.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Collision.h"
#include "Physics.h"
#include "GameSettings.h"
#include "Editor.h"

namespace Physics {

#define X(name, str) str,
	static const char* const collisionCompFlagNames[]{
		D_COLLISION_COMP_FLAG
	};
	static const char* const collisionColliderTypeNames[]{
		D_COLLIDER_TYPE
	};
#undef X

#pragma region QuadTree

	BoxWrapper::BoxWrapper(const Vector2& center, const Vector2& halfLengths, ecs::CompHandle<ColliderComp> comp)
		: center{ center }
		, halfLengths{ halfLengths }
		, compHandle{ comp }
	{
	}

	const Vector2& BoxWrapper::GetCenter() const
	{
		return center;
	}

	const Vector2& BoxWrapper::GetHalfLengths() const
	{
		return halfLengths;
	}

	ecs::CompHandle<ColliderComp> BoxWrapper::GetCompHandle() const
	{
		return compHandle;
	}

	Node::Node()
		: remainingSplits{ 0 }
		, isRoot{ false }
		, center{}
		, halfLength{ 0.0f }
		, children{ nullptr }
	{
	}

	Node::~Node()
	{
		if (children)
			delete[] children;
	}

	void Node::Setup(int newRemainingSplits, const Vector2& newCenter, float newHalfLength, bool newIsRoot)
	{
		remainingSplits = newRemainingSplits;
		isRoot = newIsRoot;
		center = newCenter;
		halfLength = newHalfLength;

		SetupChildren();
	}

	bool Node::Insert(BoxWrapper& box)
	{
		// Nodes should only hold boxes that are fully contained within the node
		// except the root node which should hold boxes that are at least partially overlapping
		//	This variable is a pointer to a const member function of Node that takes a BoxWrapper and returns bool.
		bool (Node::*ContainmentTest)(const BoxWrapper&) const { (isRoot ? &Node::IsOverlapping : &Node::FullyContains) };
		if (!(this->*ContainmentTest)(box))
			return false;

		// Attempt to insert to children. If the children accepts the box, that means the box is fully contained within the child.
		if (TryInsertToChildren(box))
			return true;

		// Failed to insert to any children, so this box should be inserted into ourselves
		colliders.emplace_back(std::move(box));

		// Check whether the threshold for further splitting is reached.
		if (remainingSplits && colliders.size() > static_cast<size_t>(QuadTree::MAX_COLLIDERS) && !children)
		{
			children = new Node[4]{};
			SetupChildren();

			for (auto iter{ colliders.rbegin() }; iter != colliders.rend(); )
			{
				if (TryInsertToChildren(*iter))
					// erase() is not defined for reverse iterators, so we kinda have to hack this a bit to make it work.
					// We're converting the reverse iterator into a normal iterator to erase.
					// erase() then gives back a normal iterator, which we use to construct a new reverse iterator pointing to the next collider in line.
					iter = decltype(iter){ colliders.erase(std::next(iter).base()) };
				else
					++iter;
			}
		}

		return true;
	}

	bool Node::Prune()
	{
		// Whether all our children are safe to be deleted
		bool childrenSafeToDelete{ static_cast<bool>(children) };

		// Need to tell our children to try to prune
		if (children)
		{
			for (int i = 0; i < 4; ++i)
				// If a child says that it can't be prune, we can't delete any of our children
				if (!children[i].Prune())
					childrenSafeToDelete = false;
		}

		// Delete children if they are empty
		if (childrenSafeToDelete)
		{
			delete[] children;
			children = nullptr;
		}

		// We are only safe to be deleted when we have no children and no colliders
		return !children && colliders.empty();
	}

	void Node::Clear()
	{
		colliders.clear();
		if (children)
			for (int i = 0; i < 4; ++i)
				children[i].Clear();
	}

	void Node::CheckCollisionPairs(CollisionSystem& collisionSystem) const
	{
		// Check colliders on this node
		for (auto thisCollider{ colliders.begin() }, endCollider{ colliders.end() }; thisCollider != endCollider; ++thisCollider)
		{
			// Check collisions between each collider within this node
			for (auto otherCollider{ thisCollider + 1 }; otherCollider != endCollider; ++otherCollider)
				collisionSystem.CheckCollision(*thisCollider->GetCompHandle(), *otherCollider->GetCompHandle());

			// Check for collisions between this collider and all colliders in children
			if (children)
				for (int i = 0; i < 4; ++i)
					children[i].CheckCollisionPairs(*thisCollider, collisionSystem);
		}

		// Check colliders in children
		if (children)
			for (int i = 0; i < 4; ++i)
				children[i].CheckCollisionPairs(collisionSystem);
	}

	Vector2 Node::GetCenter() const
	{
		return center;
	}

	void Node::SetupChildren()
	{
		if (!children)
			return;

		float quarterLength{ halfLength * 0.5f };
		children[0].Setup(remainingSplits - 1, { center.x - quarterLength, center.y + quarterLength }, quarterLength); // Top left
		children[1].Setup(remainingSplits - 1, { center.x + quarterLength, center.y + quarterLength }, quarterLength); // Top right
		children[2].Setup(remainingSplits - 1, { center.x + quarterLength, center.y - quarterLength }, quarterLength); // Bot right
		children[3].Setup(remainingSplits - 1, { center.x - quarterLength, center.y - quarterLength }, quarterLength); // Bot left
	}

	bool Node::FullyContains(const BoxWrapper& box) const
	{
		return QuadTree::CheckIsContained(center, halfLength, box.GetCenter(), box.GetHalfLengths());
	}

	bool Node::IsOverlapping(const BoxWrapper& box) const
	{
		return QuadTree::CheckIsOverlapping(center, halfLength, box.GetCenter(), box.GetHalfLengths());
	}

	bool Node::TryInsertToChildren(BoxWrapper& box)
	{
		if (children)
			for (int i = 0; i < 4; ++i)
				if (children[i].Insert(box))
					return true;
		return false;
	}

	void Node::CheckCollisionPairs(const BoxWrapper& box, CollisionSystem& collisionSystem) const
	{
		// This function is called from nodes above us. This collider may not collide with us.
		if (!IsOverlapping(box))
			return;

		// Check collision between all colliders in this node.
		for (const auto& collider : colliders)
			collisionSystem.CheckCollision(*box.GetCompHandle(), *collider.GetCompHandle());

		// Check collision in all children.
		if (children)
			for (int i = 0; i < 4; ++i)
				children[i].CheckCollisionPairs(box, collisionSystem);
	}

	void Node::ExecuteFuncPerNode(void(*func)(const Vector2& nodeCenter, float nodeHalfLength)) const
	{
		func(center, halfLength);

		if (children)
			for (int i = 0; i < 4; ++i)
				children[i].ExecuteFuncPerNode(func);
	}

	void QuadTree::Set(const Vector2& center, float length)
	{
		root.Setup(MAX_DEPTH - 1, center, length * 0.5f, true);
	}

	void QuadTree::Insert(ecs::CompHandle<ColliderComp> comp)
	{
		BoxWrapper box{ comp->CreateBoxWrapper() };
		root.Insert(box);
	}

	void QuadTree::Prune()
	{
		root.Prune();
	}

	void QuadTree::Clear()
	{
		root.Clear();
	}

	void QuadTree::CheckCollisionPairs(CollisionSystem& collisionSystem) const
	{
		root.CheckCollisionPairs(collisionSystem);
	}

	bool QuadTree::CheckIsContained(const Vector2& nodeCenter, float nodeHalfLength, const Vector2& objCenter, const Vector2& objHalfLength)
	{
		return nodeCenter.x - nodeHalfLength <= objCenter.x - objHalfLength.x &&
			nodeCenter.x + nodeHalfLength >= objCenter.x + objHalfLength.x &&
			nodeCenter.y - nodeHalfLength <= objCenter.y - objHalfLength.y &&
			nodeCenter.y + nodeHalfLength >= objCenter.y + objHalfLength.y;
	}

	bool QuadTree::CheckIsOverlapping(const Vector2& nodeCenter, float nodeHalfLength, const Vector2& objCenter, const Vector2& objHalfLength)
	{
		return nodeCenter.x - nodeHalfLength <= objCenter.x + objHalfLength.x &&
			nodeCenter.x + nodeHalfLength >= objCenter.x - objHalfLength.x &&
			nodeCenter.y - nodeHalfLength <= objCenter.y + objHalfLength.y &&
			nodeCenter.y + nodeHalfLength >= objCenter.y - objHalfLength.y;
	}

	void QuadTree::ExecuteFuncPerNode(void(*func)(const Vector2& nodeCenter, float nodeHalfLength)) const
	{
		root.ExecuteFuncPerNode(func);
	}

	Vector2 QuadTree::GetCenter() const
	{
		return root.GetCenter();
	}

#pragma endregion // QuadTree

#pragma region Collision

	bool ColliderBase::CalculateRay_CollisionOnAxis(const Vector2& axis, const Vector2& posOffset, float halfLength, const Vector2& rayDir, float* outTMin, float* outTMax)
	{
		float rayDirProjAxis = rayDir.Dot(axis);
		float offsetProjAxis = posOffset.Dot(axis);

		float tMin, tMax;

		// If the ray is parallel to the planes, its only colliding if it goes through the bounding box.
		// Defer to testing the other axis to find the distance.
		if (std::abs(rayDirProjAxis) <= std::numeric_limits<float>::epsilon())
			return (-halfLength <= offsetProjAxis && offsetProjAxis <= halfLength);

		// Find the distances to the bounding box's lines/planes from the ray origin
		tMin = (offsetProjAxis - halfLength) / rayDirProjAxis;
		tMax = (offsetProjAxis + halfLength) / rayDirProjAxis;

		if (tMin > tMax)
			std::swap(tMin, tMax);

		if (*outTMin < tMin)
			*outTMin = tMin;
		if (*outTMax > tMax)
			*outTMax = tMax;
		return true;
	}

	Vector2 ColliderBase::GetAABBHalfLengths() const
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return reinterpret_cast<const AABB*>(this)->GetHalfLengths();
		case COLLIDER_TYPE::TYPE_POINT:
			return Vector2{};
		}

		// Unimplemented collider type!
		assert(false);
		return {};
	}

	ColliderBase::ColliderBase(COLLIDER_TYPE type)
		: type{ type }
	{
	}

	bool ColliderBase::TestCollision(const Vector2& thisPos, ColliderBase& other, const Vector2& otherPos, CollisionData* outCollisionData)
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
		{
			AABB& aabb{ reinterpret_cast<AABB&>(*this) };
			switch (other.type)
			{
			case COLLIDER_TYPE::TYPE_AABB:
				return aabb.TestCollision_AABB(thisPos, reinterpret_cast<AABB&>(other), otherPos, outCollisionData);
			case COLLIDER_TYPE::TYPE_POINT:
				return aabb.TestCollision_Point(thisPos, reinterpret_cast<Point&>(other), otherPos, outCollisionData);
			}
			break;
		}
		case COLLIDER_TYPE::TYPE_POINT:
		{
			switch (other.type)
			{
			case COLLIDER_TYPE::TYPE_AABB:
				return reinterpret_cast<AABB&>(other).TestCollision_Point(thisPos, reinterpret_cast<Point&>(*this), otherPos, outCollisionData);
			}
			break;
		}
		}

		// Unimplemented collider combination!
		assert(false);
		return false;
	}

	RAYCAST_OUTCOME ColliderBase::TestRaycast(const Vector2& thisPos, const Ray& ray, RaycastResult* outResult)
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return reinterpret_cast<AABB*>(this)->TestRaycast_AABB(thisPos, ray, outResult);
		case COLLIDER_TYPE::TYPE_POINT:
			return RAYCAST_OUTCOME::MISS;
		}

		// Unimplemented collider!
		assert(false);
		return RAYCAST_OUTCOME::MISS;
	}

	COLLIDER_TYPE ColliderBase::GetType() const
	{
		return type;
	}

	AABB::AABB(const Vector2& halfLengths)
		: ColliderBase{ COLLIDER_TYPE::TYPE_AABB }
		, halfLengths{ halfLengths }
	{
	}

	bool AABB::TestCollision_AABB(const Vector2& thisPos, AABB& other, const Vector2& otherPos, CollisionData* outCollisionData)
	{
		Vector2 halfLengthsTotal = halfLengths + other.halfLengths;
		Vector2 positionDiff = otherPos - thisPos;
		Vector2 positivePositionDiff = Abs(positionDiff);

		// Colliders are only colliding when position is less than sum of half lengths
		Vector2 penetrationDist = halfLengthsTotal - positivePositionDiff;
		if (penetrationDist.x < 0.0f || penetrationDist.y < 0.0f)
			return false;

		// Let's take this collider as the main collider
		outCollisionData->referenceCollider = this;
		outCollisionData->otherCollider = &other;

		// If x axis has smaller penetration than y axis, shortest separation vector is along x axis
		if (penetrationDist.x < penetrationDist.y)
		{
			outCollisionData->collisionNormal = Vector2{ (positionDiff.x >= 0.0f ? -1.0f : 1.0f), 0.0f };
			outCollisionData->penetrationDepth = penetrationDist.x;
		}
		// Else shortest separation vector is along y axis
		else
		{
			outCollisionData->collisionNormal = Vector2{ 0.0f, (positionDiff.y >= 0.0f ? -1.0f : 1.0f) };
			outCollisionData->penetrationDepth = penetrationDist.y;
		}

		// Get the extents of the colliders along the orthogonal normal, separating between the shorter and longer collider
		Vector2 right{ outCollisionData->collisionNormal.Rotate90() };
		Vector2 penetratingExtent{ halfLengths.Proj(right) }, receivingExtent{ other.halfLengths.Proj(right) };
		Vector2 receivingHalfLengths{ other.halfLengths }, receivingPos{ otherPos };
		if (penetratingExtent.LengthSquared() > receivingExtent.LengthSquared())
		{
			std::swap(penetratingExtent, receivingExtent);
			receivingHalfLengths = halfLengths;
			receivingPos = thisPos;
		}
		else
			positionDiff = -positionDiff; // Position difference should now be from the perspective of the receiver
		// Make sure the extents are pointing in the direction of the position difference
		if (positionDiff.Dot(penetratingExtent) < 0.0f)
		{
			penetratingExtent = -penetratingExtent;
			receivingExtent = -receivingExtent;
		}

		// First move the collision point to the receiver's edge's center
		Vector2 thisHalfLengthUp{ receivingHalfLengths.Proj(outCollisionData->collisionNormal) };
		if (thisHalfLengthUp.Dot(positionDiff) < 0.0f)
			thisHalfLengthUp = -thisHalfLengthUp;
		outCollisionData->collisionPoint = receivingPos + thisHalfLengthUp;

		// If the penetrating collider's center of mass is within the receiving extents, the collision point is simply the position diff projected onto the orthogonal normal.
		// Otherwise, the collision point is simply the receiving's extent.
		Vector2 centerOfMassExtent{ positionDiff.Proj(right) };
		Vector2 penetratingFurthestExtent{ centerOfMassExtent + penetratingExtent };
		if (centerOfMassExtent.LengthSquared() <= receivingExtent.LengthSquared())
			outCollisionData->collisionPoint += centerOfMassExtent;
		else
			outCollisionData->collisionPoint += receivingExtent;

		return true;
	}

	bool AABB::TestCollision_Point(const Vector2& thisPos, Point& other, const Vector2& otherPos, CollisionData* outCollisionData)
	{
		Vector2 posOffset{ otherPos - thisPos };
		Vector2 positivePosOffset{ Abs(posOffset) };

		// We're only colliding if we're within the half lengths
		if (positivePosOffset.x > halfLengths.x || positivePosOffset.y > halfLengths.y)
			return false;

		Vector2 penetration{ halfLengths - positivePosOffset };
		if (penetration.x < penetration.y)
		{
			outCollisionData->penetrationDepth = penetration.x;
			outCollisionData->collisionNormal = (posOffset.x >= 0.0f ? Vector2{ 1.0f, 0.0f } : Vector2{ -1.0f, 0.0f });
		}
		else
		{
			outCollisionData->penetrationDepth = penetration.y;
			outCollisionData->collisionNormal = (posOffset.y >= 0.0f ? Vector2{ 0.0f, 1.0f } : Vector2{ 0.0f, -1.0f });
		}
		outCollisionData->collisionPoint = otherPos + outCollisionData->collisionNormal * outCollisionData->penetrationDepth;

		outCollisionData->referenceCollider = &other;
		outCollisionData->otherCollider = this;

		return true;
	}

	RAYCAST_OUTCOME AABB::TestRaycast_AABB(const Vector2& thisPos, const Ray& ray, RaycastResult* outResult)
	{
		Vector2 rayToThis{ thisPos - ray.originPoint };

		// There is no collision if we're behind the ray.
		if (rayToThis.Dot(ray.direction) < 0.0f &&						// Angle check (pointing away from us)
			rayToThis.LengthSquared() > halfLengths.LengthSquared())	// Distance check (beyond the maximum bounds of this collider)
			return RAYCAST_OUTCOME::MISS;

		// Find the distances at which the ray intersects the orthogonal planes of the bounding box
		float tMin = 0.0f, tMax = std::numeric_limits<float>::max();
		if (!(CalculateRay_CollisionOnAxis({ 1.0f, 0.0f }, rayToThis, halfLengths.x, ray.direction, &tMin, &tMax) &&
			CalculateRay_CollisionOnAxis({ 0.0f, 1.0f }, rayToThis, halfLengths.y, ray.direction, &tMin, &tMax)))
			return RAYCAST_OUTCOME::MISS;

		// If the ray exits a far plane on 1 axis before entering the close plane on the other axis, there is no collision
		if (tMax < tMin)
			return RAYCAST_OUTCOME::MISS;

		// If this collision occured further than the previously recorded closest collision
		if (tMin >= outResult->distance)
			return RAYCAST_OUTCOME::NON_CLOSEST_HIT;

		// Collision occured
		outResult->distance = tMin;
		outResult->collisionPoint = ray.originPoint + ray.direction * outResult->distance;

		outResult->collisionNormal = outResult->collisionPoint - thisPos;
		if (std::fabs(outResult->collisionNormal.x / halfLengths.x) >= std::fabs(outResult->collisionNormal.y / halfLengths.y))
		{
			outResult->collisionNormal.x = (outResult->collisionNormal.x > 0.0f ? 1.0f : -1.0f);
			outResult->collisionNormal.y = 0.0f;
		}
		else
		{
			outResult->collisionNormal.x = 0.0f;
			outResult->collisionNormal.y = (outResult->collisionNormal.y > 0.0f ? 1.0f : -1.0f);
		}

		return RAYCAST_OUTCOME::CLOSEST_HIT;
	}

	float AABB::GetMomentOfInertia() const
	{
		// By right, the formula for moment of inertia changes depending on the axis.
		// That's a bit too complicated for this system, so we'll use an estimation instead.

		float smallerSide, greaterSide;
		if (halfLengths.x < halfLengths.y)
		{
			smallerSide = halfLengths.x;
			greaterSide = halfLengths.y;
		}
		else
		{
			smallerSide = halfLengths.y;
			greaterSide = halfLengths.x;
		}
		return (smallerSide * greaterSide * greaterSide) * 0.1f;
	}

	const Vector2& AABB::GetHalfLengths() const
	{
		return halfLengths;
	}

	void AABB::SetHalfLengths(const Vector2& newHalfLengths)
	{
		halfLengths = newHalfLengths;
	}

	Point::Point()
		: ColliderBase{ COLLIDER_TYPE::TYPE_POINT }
	{
	}

	CollisionData::CollisionData()
		: CollisionData{ nullptr, nullptr, 0.0f, Vector2{} }
	{
	}
	CollisionData::CollisionData(ColliderBase* referenceCollider, ColliderBase* otherCollider, float penetrationDepth, const Vector2& collisionNormal)
		: referenceCollider(referenceCollider)
		, otherCollider{ otherCollider }
		, penetrationDepth(penetrationDepth)
		, collisionNormal(collisionNormal)
		, collisionPoint(Vector2{})
	{
	}

	bool CollisionData::operator<(const CollisionData& other) const
	{
		return penetrationDepth < other.penetrationDepth;
	}

	ColliderComp::ColliderComp()
		: ColliderComp{ COLLIDER_TYPE::TYPE_AABB }
	{
	}

	ColliderComp::ColliderComp(COLLIDER_TYPE type, const Vector2& scale)
		: flags{ 0 }
		, type{ type }
		, collider{ AABB{ scale * 0.5f } }
		, scale{ scale }
	{
	}

	void ColliderComp::UpdateColliderDimensions()
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
		{
			Vector2 effectiveHalfLengths{ Abs(ecs::GetEntityTransform(this).GetWorldScale() * scale * 0.5f) };
			collider.aabb.SetHalfLengths(effectiveHalfLengths);
			break;
		}
		case COLLIDER_TYPE::TYPE_POINT:
			break;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "UpdateColliderDimensions() - Unsupported collider type: " << static_cast<int>(type);
		}
	}

	BoxWrapper ColliderComp::CreateBoxWrapper()
	{
		return BoxWrapper{
			ecs::GetEntityTransform(this).GetWorldPosition(),
			GetActiveCollider().GetAABBHalfLengths(),
			this
		};
	}

	bool ColliderComp::CheckCollision(ColliderComp& otherCollider, CollisionData* outCollisionData)
	{
		// Don't need to check collision against colliders that don't collide with our masks
		if (!GetMask().TestMask(otherCollider.GetMask()))
			return false;

		return GetActiveCollider().TestCollision(
			ecs::GetEntityTransform(this).GetWorldPosition(),
			otherCollider.GetActiveCollider(),
			ecs::GetEntityTransform(&otherCollider).GetWorldPosition(),
			outCollisionData
		);
	}

	bool ColliderComp::CheckRaycast(const Ray& ray, RaycastResult* outResult)
	{
		switch (GetActiveCollider().TestRaycast(ecs::GetEntityTransform(this).GetWorldPosition(), ray, outResult))
		{
		case RAYCAST_OUTCOME::CLOSEST_HIT:
			outResult->hitComp = this;
			return true;
		case RAYCAST_OUTCOME::NON_CLOSEST_HIT:
			return true;
		case RAYCAST_OUTCOME::MISS:
			return false;
		default:
			return false;
		}
	}

	bool ColliderComp::IsReferenceCollider(const CollisionData& collisionData) const
	{
		// Check if the referenceCollider pointer points to our collider's address
		return collisionData.referenceCollider == reinterpret_cast<const void*>(&collider);
	}

	float ColliderComp::GetMomentOfInertia() const
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return collider.aabb.GetMomentOfInertia();
		case COLLIDER_TYPE::TYPE_POINT:
			return 1.0f;
		default:
			// Unsupported collider type!
			assert(false);
			return 0.0f;
		}
	}

	float ColliderComp::GetMomentOfInertiaReciprocal() const
	{
		return 1.0f / GetMomentOfInertia();
	}

	const Vector2& ColliderComp::GetScale() const
	{
		return scale;
	}

	void ColliderComp::SetScale(const Vector2& newScale)
	{
		scale = newScale;
	}

	bool ColliderComp::IsTrigger() const
	{
		return flags.TestMask(COLLISION_COMP_FLAG::IS_TRIGGER);
	}

	bool ColliderComp::SupportsRotation() const
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return false;
		case COLLIDER_TYPE::TYPE_POINT:
			return true;
		default:
			CONSOLE_LOG(LEVEL_ERROR) << "Unimplemented: ColliderComp::SupportsRotation() for type " << static_cast<int>(type);
			return false;
		}
	}

	ColliderBase& ColliderComp::GetActiveCollider()
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return collider.aabb;
		case COLLIDER_TYPE::TYPE_POINT:
			return collider.point;
		default:
			// Unsupported collider type!
			assert(false);
			return collider.aabb;
		}
	}
	const ColliderBase& ColliderComp::GetActiveCollider() const
	{
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			return collider.aabb;
		case COLLIDER_TYPE::TYPE_POINT:
			return collider.point;
		default:
			// Unsupported collider type!
			assert(false);
			return collider.aabb;
		}
	}

	EntityLayersMask ColliderComp::GetMask() const
	{
		return EntityLayersMask{ { ecs::GetEntity(this)->GetComp<EntityLayerComponent>()->GetLayer() } };
	}

	COLLIDER_TYPE ColliderComp::GetColliderType() const
	{
		return type;
	}

	void ColliderComp::SetColliderType(COLLIDER_TYPE newType)
	{
		type = newType;
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			collider.aabb = AABB{ ecs::GetEntityTransform(this).GetWorldScale() * scale * 0.5f };
			break;
		case COLLIDER_TYPE::TYPE_POINT:
			collider.point = Point{};
			break;
		}
	}

	void ColliderComp::EditorDraw()
	{
		int currType{ +type };
		if (gui::Combo typeCombo{ "Type", collisionColliderTypeNames, std::size(collisionColliderTypeNames), &currType })
			SetColliderType(static_cast<COLLIDER_TYPE>(currType));
		flags.MaskEditorDraw(collisionCompFlagNames);
		gui::VarDrag("Scale", &scale, 0.1f, Vector2{ -100.0f, -100.0f }, Vector2{ 100.0f, 100.0f }, "%.1f");
	}

	void ColliderComp::Serialize(Serializer& writer) const
	{
		writer.Serialize("type", static_cast<char>(type));
		writer.Serialize("scale", scale);
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
			writer.Serialize("halfLengths", collider.aabb.GetHalfLengths());
			break;
		default:
			CONSOLE_LOG(LEVEL_WARNING) << "Unimplemented collider type for serialization.";
		}

		flags.MaskSerialize(writer, "flags", collisionCompFlagNames);
	}

	void ColliderComp::Deserialize(Deserializer& reader)
	{
		reader.DeserializeVar("type", reinterpret_cast<char*>(&type));
		reader.DeserializeVar("scale", &scale);
		switch (type)
		{
		case COLLIDER_TYPE::TYPE_AABB:
		{
			Vector2 vec2{};
			reader.DeserializeVar("halfLengths", &vec2);
			collider.aabb.SetHalfLengths(vec2);
			break;
		}
		case COLLIDER_TYPE::TYPE_POINT:
		{
			collider.point = Point{};
			break;
		}
		default:
			CONSOLE_LOG(LEVEL_WARNING) << "Unimplemented collider type for deserialization.";
		}

		flags.MaskDeserialize(reader, "flags", collisionCompFlagNames);
	}

	CollisionSystem::CollisionSystem()
		: SystemOperatingByLayer{ &CollisionSystem::AddCompToQuadTree }
		, physSystemHandle{ ecs::GetSystem<PhysicsSystem>() }
	{
		quadTree.Set({}, ST<GameSettings>::Get()->m_collisionSimulationSize);
	}

	bool CollisionSystem::PreRun()
	{
		// Center the quadtree on the player
		/*auto playerCompsIter{ ecs::GetCompsBegin<PlayerComponent>() };
		if (playerCompsIter != ecs::GetCompsEnd<PlayerComponent>())
			quadTree.Set(playerCompsIter.GetEntity()->GetTransform().GetWorldPosition(), ST<GameSettings>::Get()->m_collisionSimulationSize);*/

		// QuadTree will be populated at Run().
		// Finally collisions are checked at PostRun().

		return true;
	}

	void CollisionSystem::PostRun()
	{
		// Prune the tree to keep it as small as required
		quadTree.Prune();
		// Query quadtree to check every possible collision
		quadTree.CheckCollisionPairs(*this);
		quadTree.Clear();
	}

	void CollisionSystem::CheckCollision(ColliderComp& comp1, ColliderComp& comp2)
	{
		// Only check for collisions if either components have physics comp
		ecs::CompHandle<PhysicsComp> physComp1{ ecs::GetEntity(&comp1)->GetComp<PhysicsComp>() };
		ecs::CompHandle<PhysicsComp> physComp2{ ecs::GetEntity(&comp2)->GetComp<PhysicsComp>() };
		if (!(physComp1 || physComp2))
			return;

		if (comp1.CheckCollision(comp2, &collisionData))
		{
			if (comp1.IsReferenceCollider(collisionData))
				ResolveCollision(comp1, physComp1, comp2, physComp2, &collisionData);
			else
				ResolveCollision(comp2, physComp2, comp1, physComp1, &collisionData);
		}
	}

	const QuadTree& CollisionSystem::GetQuadTree() const
	{
		return quadTree;
	}

	void CollisionSystem::AddCompToQuadTree(ColliderComp& comp)
	{
		comp.UpdateColliderDimensions();
		quadTree.Insert(&comp);
	}

	void CollisionSystem::ResolveCollision(ColliderComp& refComp, ecs::CompHandle<PhysicsComp> refPhysComp,
		ColliderComp& otherComp, ecs::CompHandle<PhysicsComp> otherPhysComp, CollisionData* inCollisionData)
	{
		// If neither component is a trigger, resolve the intersection
		bool isTriggerCollision{ refComp.IsTrigger() || otherComp.IsTrigger() };
		if (!isTriggerCollision)
			ResolveIntersection(refComp, IsPhysCompDynamic(refPhysComp), otherComp, IsPhysCompDynamic(otherPhysComp), inCollisionData);

		// Inform messaging system about this collision so physics can update values
		CollisionEventData collisionEventData{ isTriggerCollision, &refComp, refPhysComp, &otherComp, otherPhysComp, inCollisionData };
		Messaging::BroadcastAll("OnCollision", collisionEventData);

		// Skip informing entities about this collision if we have a point collider collision
		if (refComp.GetColliderType() == COLLIDER_TYPE::TYPE_POINT || otherComp.GetColliderType() == COLLIDER_TYPE::TYPE_POINT)
			return;

		// Inform entities about this collision
		ecs::GetEntity(&refComp)->GetComp<EntityEventsComponent>()->BroadcastAll("OnCollision", collisionEventData);
		std::swap(collisionEventData.refComp, collisionEventData.otherComp);
		std::swap(collisionEventData.refPhysComp, collisionEventData.otherPhysComp);
		std::swap(collisionEventData.collisionData->referenceCollider, collisionEventData.collisionData->otherCollider);
		collisionEventData.collisionData->collisionNormal = -collisionEventData.collisionData->collisionNormal;
		ecs::GetEntity(&otherComp)->GetComp<EntityEventsComponent>()->BroadcastAll("OnCollision", collisionEventData);
	}

	void CollisionSystem::ResolveIntersection(ColliderComp& refComp, bool isRefPhysCompDynamic, ColliderComp& otherComp, bool isOtherPhysCompDynamic, const CollisionData* inCollisionData)
	{
		Transform& refTransform{ ecs::GetEntityTransform(&refComp) };
		Transform& otherTransform{ ecs::GetEntityTransform(&otherComp) };

		if (isRefPhysCompDynamic && isOtherPhysCompDynamic)
		{
			// Push both colliders away from each other equally
			// NOTE: If these transforms are parented to each other, this resolution may not fully push them out of each other.
			//		 If we want to fully fix this, we'll have to check if these components have a parent child relationship, then
			//		 calculate the correct push vector relative to their spaces...
			Vector2 pushVec{ inCollisionData->collisionNormal * inCollisionData->penetrationDepth * 0.5f };
			refTransform.AddWorldPosition(pushVec);
			otherTransform.AddWorldPosition(-pushVec);
		}
		else
			// Push dynamic collider away from static
			if (isRefPhysCompDynamic)
				refTransform.AddWorldPosition(inCollisionData->collisionNormal * inCollisionData->penetrationDepth);
			else // Other physComp is dynamic
				otherTransform.AddWorldPosition(inCollisionData->collisionNormal * -inCollisionData->penetrationDepth);
	}

	bool CollisionSystem::IsPhysCompDynamic(ecs::CompHandle<const PhysicsComp> physComp)
	{
		return physComp && physComp->IsDynamic();
	}

	ColliderBorderSystem::ColliderBorderSystem()
		: System_Internal{ &ColliderBorderSystem::RenderComp }
	{
	}

	bool ColliderBorderSystem::PreRun()
	{
#ifdef IMGUI_ENABLED
		return ST<Inspector>::Get()->m_drawPhysicsBoxes;
#else
		return false;
#endif
	}

	void ColliderBorderSystem::RenderComp(ColliderComp& comp)
	{
		const Transform& transform{ ecs::GetEntity(&comp)->GetTransform() };
		util::DrawBoundingBox(transform.GetWorldPosition(), transform.GetWorldScale() * comp.GetScale(), { 1.0f, 165.0f / 255.0f, 0 }); // orange
	}

	bool QuadtreeRenderSystem::PreRun()
	{
#ifdef IMGUI_ENABLED
		if (ST<Inspector>::Get()->m_drawPhysicsBoxes)
		{
			if (ecs::SysHandle<CollisionSystem> collisionSystem{ ecs::GetSystem<CollisionSystem>() })
			{
				collisionSystem->GetQuadTree().ExecuteFuncPerNode([](const Vector2& center, float halfLength) -> void {
					halfLength *= 2.0f;
					util::DrawBoundingBox(center, { halfLength, halfLength }, { 1.0f, 1.0f, 0.5f }, 0.0f, 0.5f);
				});

				util::DrawBoundingBox(
					collisionSystem->GetQuadTree().GetCenter(),
					Vector2{ ST<GameSettings>::Get()->m_physicsSimulationSize, ST<GameSettings>::Get()->m_physicsSimulationSize },
					{ 0.5f, 1.0f, 1.0f }, 0.0f, 0.5f
				);
			}
		}
#endif

		return false;
	}

#pragma endregion // Collision

#pragma region Raycast

	RaycastResult::RaycastResult()
		: hitComp{}
		, distance{ std::numeric_limits<float>::max() }
	{
	}

	void MultiRaycastResult::SortHitsByDistance()
	{
		std::sort(hits.begin(), hits.end(), [](const RaycastResult& a, const RaycastResult& b) -> bool {
			return a.distance < b.distance;
		});
	}

	bool Raycast(const Vector2& origin, const Vector2& direction, RaycastResult* outRaycastResult)
	{
		// Raycast against all layers
		return Raycast(origin, direction, EntityLayersMask{}, outRaycastResult);
	}

	bool Raycast(const Vector2& origin, const Vector2& direction, const EntityLayersMask& mask, RaycastResult* outRaycastResult)
	{
		Ray ray{ .originPoint = origin, .direction = direction };

		bool collided{ false };
		for (auto iter{ ecs::GetCompsActiveBegin<ColliderComp>() }, end{ ecs::GetCompsEnd<ColliderComp>() }; iter != end; ++iter)
		{
			if (!iter->GetMask().TestMaskRaw(mask))
				continue;

			if (iter->CheckRaycast(ray, outRaycastResult))
				collided = true;
		}

		return collided;
	}

	bool MultiRaycast(const Vector2& origin, const Vector2& direction, float maxDistance, MultiRaycastResult* outRaycastResult)
	{
		// Raycast against all layers
		return MultiRaycast(origin, direction, maxDistance, EntityLayersMask{}, outRaycastResult);
	}

	bool MultiRaycast(const Vector2& origin, const Vector2& direction, float maxDistance, const EntityLayersMask& mask, MultiRaycastResult* outRaycastResult)
	{
		Ray ray{ .originPoint = origin, .direction = direction };
		RaycastResult raycastResult{};

		for (auto iter{ ecs::GetCompsActiveBegin<ColliderComp>() }, end{ ecs::GetCompsEnd<ColliderComp>() }; iter != end; ++iter)
		{
			if (!iter->GetMask().TestMaskRaw(mask))
				continue;

			if (iter->CheckRaycast(ray, &raycastResult))
			{
				if (raycastResult.distance <= maxDistance)
					outRaycastResult->hits.push_back(raycastResult);
				// This is the only variable required to be reset.
				raycastResult.distance = std::numeric_limits<float>::max();
			}
		}

		return !outRaycastResult->hits.empty();
	}

#pragma endregion // Raycast

}
