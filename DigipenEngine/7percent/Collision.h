/******************************************************************************/
/*!
\file   Collision.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for the Collision system, QuadTree, collision component,
  and various collider definitions such as AABB, that implement a structure that can detect
  and separate collisions.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "EntityLayers.h"

namespace Physics {

	/* Forward Declares */
	class ColliderBase;
	class Point;
	struct CollisionData;
	class ColliderComp;
	class CollisionSystem;
	enum class COLLISION_COMP_FLAG;
	enum class RAYCAST_OUTCOME;
	struct Ray;
	struct RaycastResult;

	using CollisionCompFlags = MaskTemplate<COLLISION_COMP_FLAG>;

	/* Physics forward declares */
	class PhysicsComp;
	class PhysicsSystem;

#pragma region Quadtree

	/*****************************************************************//*!
	\class BoxWrapper
	\brief
		Represents a collider as a AABB for purposes of Quadtree.
	*//******************************************************************/
	class BoxWrapper
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param center
			The center of the collider.
		\param halfLengths
			The half lengths of the AABB that encapsulates the collider.
		\param comp
			The collider component.
		*//******************************************************************/
		BoxWrapper(const Vector2& center, const Vector2& halfLengths, ecs::CompHandle<ColliderComp> comp);

		/*****************************************************************//*!
		\brief
			Gets the center of the BoxWrapper.
		\return
			The center of the BoxWrapper.
		*//******************************************************************/
		const Vector2& GetCenter() const;

		/*****************************************************************//*!
		\brief
			Gets the half lengths of the BoxWrapper.
		\return
			The half lengths of the BoxWrapper.
		*//******************************************************************/
		const Vector2& GetHalfLengths() const;

		/*****************************************************************//*!
		\brief
			Gets the collider component associated with this BoxWrapper.
		\return
			The collider component associated with this BoxWrapper.
		*//******************************************************************/
		ecs::CompHandle<ColliderComp> GetCompHandle() const;

	private:
		//! The center of this BoxWrapper.
		Vector2 center;
		//! The half lengths of this BoxWrapper.
		Vector2 halfLengths;
		//! The collider component associated with this BoxWrapper.
		ecs::CompHandle<ColliderComp> compHandle;

	};

	/*****************************************************************//*!
	\class Node
	\brief
		A node within a quadtree representing some square area.
	*//******************************************************************/
	class Node
	{
	public:
		/*****************************************************************//*!
		\brief
			Default constructor.
		*//******************************************************************/
		Node();

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~Node();

		/*****************************************************************//*!
		\brief
			Setup this node with the provided parameters. Does not redistribute existing colliders.
		\param newRemainingSplits
			The available depth remaining.
		\param newCenter
			The center of this node.
		\param newHalfLength
			The half lengths of this node.
		\param newIsRoot
			Whether this node is the root node.
		*//******************************************************************/
		void Setup(int newRemainingSplits, const Vector2& newCenter, float newHalfLength, bool newIsRoot = false);

		/*****************************************************************//*!
		\brief
			Attempts to insert a BoxWrapper into this node or its children.
		\param box
			The BoxWrapper to be inserted.
		\return
			True if the BoxWrapper was inserted. False otherwise.
		*//******************************************************************/
		bool Insert(BoxWrapper& box);

		/*****************************************************************//*!
		\brief
			Prunes children if all children of this node have no colliders.
		\return
			True if this children is not a parent of any colliders and thus is safe to prune. False otherwise.
		*//******************************************************************/
		bool Prune();

		/*****************************************************************//*!
		\brief
			Clears all colliders from this node, and recursively clears all colliders from children.
		*//******************************************************************/
		void Clear();

		/*****************************************************************//*!
		\brief
			Calls a function within collision system for every potential collision within this node.
		\param collisionSystem
			The collision system.
		*//******************************************************************/
		void CheckCollisionPairs(CollisionSystem& collisionSystem) const;

		/*****************************************************************//*!
		\brief
			Executes a function for each node given its position and scale.
		\param func
			The function to be executed for each node.
		*//******************************************************************/
		void ExecuteFuncPerNode(void (*func)(const Vector2& nodeCenter, float nodeHalfLength)) const;

		/*****************************************************************//*!
		\brief
			Gets the center position of this node.
		\return
			The center position of this node.
		*//******************************************************************/
		Vector2 GetCenter() const;

	private:
		/*****************************************************************//*!
		\brief
			Sets up the parameters of children based on the parameters of this node.
		*//******************************************************************/
		void SetupChildren();

		/*****************************************************************//*!
		\brief
			Checks if this node can fully contain a BoxWrapper.
		\param box
			The BoxWrapper to be checked.
		\return
			True if this node fully contains the BoxWrapper. False otherwise.
		*//******************************************************************/
		bool FullyContains(const BoxWrapper& box) const;

		/*****************************************************************//*!
		\brief
			Checks if this node is overlapping a BoxWrapper.
		\param box
			The BoxWrapper to be checked.
		\return
			True if this node is overlapping the BoxWrapper. False otherwise.
		*//******************************************************************/
		bool IsOverlapping(const BoxWrapper& box) const;

		/*****************************************************************//*!
		\brief
			Attempts to insert a BoxWrapper into one of this node's children.
		\param box
			The BoxWrapper to be inserted.
		\return
			True if the BoxWrapper was inserted into one of this node's children. False otherwise.
		*//******************************************************************/
		bool TryInsertToChildren(BoxWrapper& box);

		/*****************************************************************//*!
		\brief
			Calls a function within the collision system for every potential collision
			between a collider and all colliders within this node
		\param box
			The BoxWrapper to be checked.
		\param collisionSystem
			The collision system.
		\return
			True if the BoxWrapper was inserted into one of this node's children. False otherwise.
		*//******************************************************************/
		void CheckCollisionPairs(const BoxWrapper& box, CollisionSystem& collisionSystem) const;

		//! The number of available splits remaining at this node depth.
		int remainingSplits;
		//! Whether this node is the root node
		bool isRoot;

		//! Position of the center of this node.
		Vector2 center;
		//! The half length of one side of this node.
		float halfLength;

		// Child nodes of this node. This is an array of 4 Nodes.
		Node* children;
		// Colliders contained within this node
		std::vector<BoxWrapper> colliders;

	};

	/*****************************************************************//*!
	\class QuadTree
	\brief
		A quadtree using fixed size square nodes.
	*//******************************************************************/
	class QuadTree
	{
	public:
		/*****************************************************************//*!
		\brief
			Sets the parameters of this quadtree. Does not redistribute colliders that are already inserted.
		\param center
			The center of this quadtree.
		\param length
			The length of one side of this quadtree.
		*//******************************************************************/
		void Set(const Vector2& center, float length);

		/*****************************************************************//*!
		\brief
			Inserts a collider into the quadtree
		\param comp
			The collider component to insert into this quadtree.
		\param length
			The length of one side of this quadtree.
		*//******************************************************************/
		void Insert(ecs::CompHandle<ColliderComp> comp);

		/*****************************************************************//*!
		\brief
			Prunes nodes with no colliders.
		*//******************************************************************/
		void Prune();

		/*****************************************************************//*!
		\brief
			Empties this quadtree.
		*//******************************************************************/
		void Clear();

		/*****************************************************************//*!
		\brief
			Calls a function within collision system for every potential collision
			within this quadtree
		\param collisionSystem
			The collision system.
		*//******************************************************************/
		void CheckCollisionPairs(CollisionSystem& collisionSystem) const;

		/*****************************************************************//*!
		\brief
			Checks if an object is contained within a node with the specified parameters.
		\param nodeCenter
			The center of the node.
		\param nodeHalfLength
			The half length of the node.
		\param objCenter
			The object's center position.
		\param objHalfLength
			The object's half lengths.
		\return
			True if the object is contained within the node. False otherwise.
		*//******************************************************************/
		static bool CheckIsContained(const Vector2& nodeCenter, float nodeHalfLength, const Vector2& objCenter, const Vector2& objHalfLength);

		/*****************************************************************//*!
		\brief
			Checks if an object is overlapping a node with the specified parameters.
		\param nodeCenter
			The center of the node.
		\param nodeHalfLength
			The half length of the node.
		\param objCenter
			The object's center position.
		\param objHalfLength
			The object's half lengths.
		\return
			True if the object is overlapping the node. False otherwise.
		*//******************************************************************/
		static bool CheckIsOverlapping(const Vector2& nodeCenter, float nodeHalfLength, const Vector2& objCenter, const Vector2& objHalfLength);

		/*****************************************************************//*!
		\brief
			Executes a function for each node given its position and scale.
		\param func
			The function to be executed for each node.
		*//******************************************************************/
		void ExecuteFuncPerNode(void (*func)(const Vector2& nodeCenter, float nodeHalfLength)) const;

		/*****************************************************************//*!
		\brief
			Gets the center position of the quadtree.
		\return
			The center of the quadtree.
		*//******************************************************************/
		Vector2 GetCenter() const;

		//! The maximum number of colliders that can be placed in the same node.
		static constexpr int MAX_COLLIDERS{ 8 };

	private:
		//! The maximum depth of nodes. Depth of 1 means the root node never splits.
		static constexpr int MAX_DEPTH{ 8 };

		//! Root node.
		Node root;

	};

#pragma endregion // Quadtree

#pragma region Collision

#define D_COLLIDER_TYPE \
X(TYPE_AABB, "AABB") \
X(TYPE_POINT, "Point")

	/*****************************************************************//*!
	\brief
		Enums that identify the type of collider a ColliderBase is.
	*//******************************************************************/
#define X(name, str) name,
	enum class COLLIDER_TYPE : char
	{
		D_COLLIDER_TYPE
	};
#undef X

	/*****************************************************************//*!
	\class ColliderBase
	\brief
		The base class that all colliders inherit from. Simply stores the collider type and an interface to test collisions against another collider.
	*//******************************************************************/
	class ColliderBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Checks if a collision has occured between this collider and another collider.
		\param thisPos
			The world position of this collider.
		\param other
			The other collider's ColliderBase
		\param otherPos
			The world position of the other collider.
		\param outCollisionData
			If a collision has occured, the characteristics of the collision will be written here.
		\return
			True if a collision occured. False otherwise.
		*//******************************************************************/
		bool TestCollision(const Vector2& thisPos, ColliderBase& other, const Vector2& otherPos, CollisionData* outCollisionData);

		/*****************************************************************//*!
		\brief
			Checks if a ray intersects this collider.
		\param thisPos
			The world position of this collider.
		\param ray
			The ray.
		\param outResult
			If an intersection has occured, the characteristics of the raycast collision will be written here.
		\return
			The result of the raycast.
		*//******************************************************************/
		RAYCAST_OUTCOME TestRaycast(const Vector2& thisPos, const Ray& ray, RaycastResult* outResult);

		/*****************************************************************//*!
		\brief
			Gets half lengths of an AABB that can fully contain this collider.
		\return
			Half lengths of an AABB that can fully contain this collider.
		*//******************************************************************/
		Vector2 GetAABBHalfLengths() const;

		/*****************************************************************//*!
		\brief
			Gets the type of collider.
		\return
			The type of the collider.
		*//******************************************************************/
		COLLIDER_TYPE GetType() const;

	protected:
		static bool CalculateRay_CollisionOnAxis(const Vector2& axis, const Vector2& posOffset, float halfLength, const Vector2& rayDir, float* outTMin, float* outTMax);

	protected:
		/*****************************************************************//*!
		\brief
			Constructor that only child classes can call. Sets the type of the collider.
		*//******************************************************************/
		ColliderBase(COLLIDER_TYPE type);

		//! The type of this collider.
		COLLIDER_TYPE type;
	};

	/*****************************************************************//*!
	\class AABB
	\brief
		The collider class that implements the AABB collider.
	*//******************************************************************/
	class AABB : public ColliderBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructs an AABB.
		\param halfLengths
			The half lengths of an AABB rectangle.
		*//******************************************************************/
		AABB(const Vector2& halfLengths);

		/*****************************************************************//*!
		\brief
			Tests AABB vs AABB collisions.
		\param thisPos
			The world position of this collider.
		\param other
			The other AABB collider.
		\param otherPos
			The world position of the other collider.
		\param outCollisionData
			If a collision has occured, the characteristics of the collision will be written here.
		\return
			True if a collision occured. False otherwise.
		*//******************************************************************/
		bool TestCollision_AABB(const Vector2& thisPos, AABB& other, const Vector2& otherPos, CollisionData* outCollisionData);

		/*****************************************************************//*!
		\brief
			Tests AABB vs Point collisions.
		\param thisPos
			The world position of this collider.
		\param other
			The other Point collider.
		\param otherPos
			The world position of the other collider.
		\param outCollisionData
			If a collision has occured, the characteristics of the collision will be written here.
		\return
			True if a collision occured. False otherwise.
		*//******************************************************************/
		bool TestCollision_Point(const Vector2& thisPos, Point& other, const Vector2& otherPos, CollisionData* outCollisionData);

		/*****************************************************************//*!
		\brief
			Checks if a ray intersects this collider.
		\param thisPos
			The world position of this collider.
		\param ray
			The ray.
		\param outResult
			If an intersection has occured, the characteristics of the raycast collision will be written here.
		\return
			The result of the raycast.
		*//******************************************************************/
		RAYCAST_OUTCOME TestRaycast_AABB(const Vector2& thisPos, const Ray& ray, RaycastResult* outResult);

		/*****************************************************************//*!
		\brief
			Gets the moment of inertia of this collider, based on the size of the AABB.
		\return
			The moment of inertia.
		*//******************************************************************/
		float GetMomentOfInertia() const;

		/*****************************************************************//*!
		\brief
			Gets the half lengths of this collider.
		\return
			The half lengths of this collider.
		*//******************************************************************/
		const Vector2& GetHalfLengths() const;

		/*****************************************************************//*!
		\brief
			Sets the half lengths of this collider.
		\param newHalfLengths
			The new half lengths of this collider.
		*//******************************************************************/
		void SetHalfLengths(const Vector2& newHalfLengths);

	private:
		//! The half lengths of the AABB.
		Vector2 halfLengths;
	};

	/*****************************************************************//*!
	\class Point
	\brief
		The collider class that implements the Point collider.
	*//******************************************************************/
	class Point : public ColliderBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		Point();
	};

	/*****************************************************************//*!
	\struct CollisionData
	\brief
		Collision characteristics are contained within this struct.
	*//******************************************************************/
	struct CollisionData
	{
	public:
		//! The reference collider from which all collision characteristics are relative to.
		ColliderBase* referenceCollider;
		//! The other collider that the reference collider is colliding with.
		ColliderBase* otherCollider;
		//! The point of collision.
		Vector2 collisionPoint;
		//! The normal of the collision.
		Vector2 collisionNormal;
		//! The penetration depth of the collision.
		float penetrationDepth;

		/*****************************************************************//*!
		\brief
			Default constructs a CollisionData struct.
		*//******************************************************************/
		CollisionData();

		/*****************************************************************//*!
		\brief
			Constructs a CollisionData struct with the provided collision characteristics.
		\param referenceCollider
			The reference collider from which all the collision characteristics are relative to.
		\param otherCollider
			The other collider that the reference collider is colliding with.
		\param penetrationDepth
			The penetration depth of the collision.
		\param collisionNormal
			The normal of the collision.
		*//******************************************************************/
		CollisionData(ColliderBase* referenceCollider, ColliderBase* otherCollider, float penetrationDepth, const Vector2& collisionNormal);

		/*****************************************************************//*!
		\brief
			Compares different possible collision axes between another CollisionData to find the axis with the smallest penetration.
		\param other
			The CollisionData to compare against.
		\return
			True if this CollisionData has a smaller penetration. False if the other CollisionData has a smaller penetration.
		*//******************************************************************/
		bool operator<(const CollisionData& other) const;
	};

	/*****************************************************************//*!
	\struct CollisionEventData
	\brief
		A struct to pack all data of a collision, to be sent as a payload in the messaging system.
	*//******************************************************************/
	struct CollisionEventData
	{
		//! Whether a trigger collider is involved and hence no velocity or position changes should happen due to physics.
		bool isCollisionWithTrigger;

		//! The ColliderComp from which collision parameters are relative to.
		ecs::CompHandle<ColliderComp> refComp;
		//! The PhysicsComp of the reference entity.
		ecs::CompHandle<PhysicsComp> refPhysComp;
		//! The ColliderComp which the reference entity is colliding with.
		ecs::CompHandle<ColliderComp> otherComp;
		//! The PhysicsComp of the entity which the reference entity is colliding with.
		ecs::CompHandle<PhysicsComp> otherPhysComp;
		//! The collision parameters.
		CollisionData* collisionData;
	};

	/*****************************************************************//*!
	\enum COLLISION_COMP_FLAG
	\brief
		Components flags.
	*//******************************************************************/
#define D_COLLISION_COMP_FLAG \
X(IS_TRIGGER, "Is Trigger")

#define X(name, str) name,
	enum class COLLISION_COMP_FLAG : int
	{
		D_COLLISION_COMP_FLAG

		TOTAL,
		ALL = TOTAL
	};
#undef X

	/*****************************************************************//*!
	\class ColliderComp
	\brief
		The collision component that contains the collider structure and interfaces for checking collisions with other collision components.
	*//******************************************************************/
	class ColliderComp : public IRegisteredComponent<ColliderComp>
#ifdef IMGUI_ENABLED
		, IEditorComponent<ColliderComp>
#endif
	{
	public:
		union ColliderData {
			AABB aabb;
			Point point;
		};

	public:
		/*****************************************************************//*!
		\brief
			Default constructs a ColliderComp.
		*//******************************************************************/
		explicit ColliderComp(); // For compliance with IRegisteredComponent

		/*****************************************************************//*!
		\brief
			Constructs a ColliderComp with the requested collider type and scale of the collider.
		\param type
			The collider type used.
		\param scale
			The scale of the collider.
		*//******************************************************************/
		ColliderComp(COLLIDER_TYPE type, const Vector2& scale = { 1.0f, 1.0f });

		/*****************************************************************//*!
		\brief
			Updates the dimensions, etc of the collider stored within this component
			based on the entity's transform.
		*//******************************************************************/
		void UpdateColliderDimensions();

		/*****************************************************************//*!
		\brief
			Creates a box wrapper containing this collider.
		\return
			A BoxWrapper that contains this collider.
		*//******************************************************************/
		BoxWrapper CreateBoxWrapper();

		/*****************************************************************//*!
		\brief
			Checks if a collision is occuring with another ColliderComp.
		\param otherCollider
			The other ColliderComp to test collision with.
		\param outCollisionData
			If a collision has occured, the characteristics of the collision will be written here.
		\return
			True if a collision occured. False otherwise.
		*//******************************************************************/
		bool CheckCollision(ColliderComp& otherCollider, CollisionData* outCollisionData);

		/*****************************************************************//*!
		\brief
			Checks if a ray intersects this collider.
		\param ray
			The ray.
		\param outResult
			If an intersection has occured, the characteristics of the raycast collision will be written here.
		\return
			True if the ray intersects this collider. False otherwise.
		*//******************************************************************/
		bool CheckRaycast(const Ray& ray, RaycastResult* outResult);

		/*****************************************************************//*!
		\brief
			Gets if this component contains the reference collider for a collision or not.
		\param collisionData
			The collision data to check the reference collider of.
		\return
			True if this component contains the reference collider within the specified CollisionData. False otherwise.
		*//******************************************************************/
		bool IsReferenceCollider(const CollisionData& collisionData) const;

		/*****************************************************************//*!
		\brief
			Gets the moment of inertia of the collider.
		\return
			The moment of inertia of the collider.
		*//******************************************************************/
		float GetMomentOfInertia() const;

		/*****************************************************************//*!
		\brief
			Gets the reciprocal of the moment of inertia of the collider.
		\return
			The reciprocal of the moment of inertia of the collider.
		*//******************************************************************/
		float GetMomentOfInertiaReciprocal() const;

		/*****************************************************************//*!
		\brief
			Gets the scale of this collider.
		\return
			The scale of this collider.
		*//******************************************************************/
		const Vector2& GetScale() const;

		/*****************************************************************//*!
		\brief
			Sets the scale of this collider.
		*//******************************************************************/
		void SetScale(const Vector2& newScale);

		/*****************************************************************//*!
		\brief
			Whether this collider is a trigger.
		\return
			True if this collider is a trigger. False otherwise.
		*//******************************************************************/
		bool IsTrigger() const;

		/*****************************************************************//*!
		\brief
			Whether this collider supports rotation.
		\return
			True if this collider has a type that supports rotation. False otherwise.
		*//******************************************************************/
		bool SupportsRotation() const;

		/*****************************************************************//*!
		\brief
			Queries the EntityLayerComponent to get its layer mask.
			IDEALLY THIS SHOULD NOT BE CALLED OFTEN SINCE THIS REQUIRES MULTIPLE STEPS TO QUERY.
		\return
			The entity's layer as a mask.
		*//******************************************************************/
		EntityLayersMask GetMask() const;

		/*****************************************************************//*!
		\brief
			Gets the type of collider.
		\return
			The collider type.
		*//******************************************************************/
		COLLIDER_TYPE GetColliderType() const;

		/*****************************************************************//*!
		\brief
			Sets the type of collider.
		\param newType
			The collider type.
		*//******************************************************************/
		void SetColliderType(COLLIDER_TYPE newType);

	private:
		/*****************************************************************//*!
		\brief
			Gets the ColliderBase stored within this ColliderComp.
		\return
			The ColliderBase stored within this ColliderComp.
		*//******************************************************************/
		ColliderBase& GetActiveCollider();

		/*****************************************************************//*!
		\brief
			Gets the ColliderBase stored within this ColliderComp.
		\return
			The ColliderBase stored within this ColliderComp.
		*//******************************************************************/
		const ColliderBase& GetActiveCollider() const;

		//! Flags of this component
		CollisionCompFlags flags;

		//! The type of the collider stored.
		COLLIDER_TYPE type;
		//! The collider stored.
		ColliderData collider;
		//! The scale of the collider. This is multiplied by the entity's scale to obtain the collider's actual dimensions.
		Vector2 scale;

	private:
#ifdef IMGUI_ENABLED
		static void EditorDraw(ColliderComp& comp);
#endif

	public:
		// Custom serialization (to support the different possible colliders)
		void Serialize(Serializer& writer) const override;
		void Deserialize(Deserializer& reader) override;

	private:
		property_vtable()
	};

	/*****************************************************************//*!
	\class CollisionSystem
	\brief
		The system that checks for collisions between ColliderComps.
	*//******************************************************************/
	class CollisionSystem : public ecs::SystemOperatingByLayer<CollisionSystem, ColliderComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructs a CollisionSystem.
		*//******************************************************************/
		CollisionSystem();

		/*****************************************************************//*!
		\brief
			Updates the dimensions of all colliders and populates the QuadTree.
		\return
			True. The system will process components every time.
		*//******************************************************************/
		bool PreRun() override;

		/*****************************************************************//*!
		\brief
			Checks for and resolves collisions within the QuadTree, then clears
			the QuadTree for the next update.
		*//******************************************************************/
		void PostRun() override;

		/*****************************************************************//*!
		\brief
			Checks for collision between 2 ColliderComps.
		\param comp1
			A ColliderComp.
		\param comp2
			Another ColliderComp.
		*//******************************************************************/
		void CheckCollision(ColliderComp& comp1, ColliderComp& comp2);

		/*****************************************************************//*!
		\brief
			Gets the quadtree.
		\return
			The quadtree.
		*//******************************************************************/
		const QuadTree& GetQuadTree() const;

	private:
		/*****************************************************************//*!
		\brief
			Adds a component into the QuadTree.
		\param comp
			The component to be added into the QuadTree.
		*//******************************************************************/
		void AddCompToQuadTree(ColliderComp& comp);

		/*****************************************************************//*!
		\brief
			Resolves a detected collision by separating colliders and informing PhysicsSystem of the collision.
		\param refComp
			The reference collision component that all values stored in CollisionData are relative to.
		\param refPhysComp
			The physics component that the reference entity has.
		\param otherComp
			The other collision component that the reference collision component is colliding with.
		\param otherPhysComp
			The physics component that the other entity has.
		\param inCollisionData
			The characteristics of the collision.
		*//******************************************************************/
		void ResolveCollision(ColliderComp& refComp, ecs::CompHandle<PhysicsComp> refPhysComp,
			ColliderComp& otherComp, ecs::CompHandle<PhysicsComp> otherPhysComp, CollisionData* inCollisionData);

		/*****************************************************************//*!
		\brief
			Seperates colliders.
		\param refComp
			The reference collision component that all values stored in CollisionData are relative to.
		\param isRefPhysCompDynamic
			Whether the physics component that the reference entity has is dynamic.
		\param otherComp
			The other collision component that the reference collision component is colliding with.
		\param isOtherPhysCompDynamic
			Whether the physics component that the other entity has is dynamic.
		\param collisionData
			The characteristics of the collision.
		*//******************************************************************/
		void ResolveIntersection(ColliderComp& refComp, bool isRefPhysCompDynamic, ColliderComp& otherComp, bool isOtherPhysCompDynamic, const CollisionData* inCollisionData);

		/*****************************************************************//*!
		\brief
			Gets if a given physics component is dynamic.
		\param physComp
			The physics component to check for if it is dynamic or not.
		\return
			True if the physics component is dynamic. False otherwise or if physComp is nullptr.
		*//******************************************************************/
		static bool IsPhysCompDynamic(ecs::CompHandle<const PhysicsComp> physComp);

	private:
		//! Colliders will be populated into this QuadTree for broad phase collision checking.
		QuadTree quadTree;

		//! A buffer for collision data to avoid reallocating stack memory for CollisionData per collision component.
		CollisionData collisionData;
		//! Temporary: A handle to PhysicsSystem for informing it of collisions.
		ecs::SysHandle<Physics::PhysicsSystem> physSystemHandle;
	};

	/*****************************************************************//*!
	\class ColliderBorderSystem
	\brief
		A system that renders borders around colliders if the editor setting
		to do so is active.
	*//******************************************************************/
	class ColliderBorderSystem : public ecs::System<ColliderBorderSystem, ColliderComp>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		*//******************************************************************/
		ColliderBorderSystem();

		/*****************************************************************//*!
		\brief
			Determines whether to run this system depending on whether the setting
			to draw physics boxes is on.
		\return
			True if the editor has draw physics boxes turned on. False otherwise.
		*//******************************************************************/
		bool PreRun() override;

	private:
		/*****************************************************************//*!
		\brief
			Render a border around a ColliderComp.
		\param comp
			The ColliderComp to be rendered.
		*//******************************************************************/
		void RenderComp(ColliderComp& comp);
	};

	/*****************************************************************//*!
	\class QuadtreeRenderSystem
	\brief
		A system that renders the collision quadtree to screen for debug
		purposes.
	*//******************************************************************/
	class QuadtreeRenderSystem : public ecs::System<QuadtreeRenderSystem>
	{
	public:
		/*****************************************************************//*!
		\brief
			Renders the quadtree.
		\return
			False. The system will never process components.
		*//******************************************************************/
		bool PreRun() override;
	};

#pragma endregion // Collision

#pragma region Raycast

	/*****************************************************************//*!
	\struct Ray
	\brief
		Represents a ray.
	*//******************************************************************/
	struct Ray
	{
		//! The origin of the ray.
		Vector2 originPoint;
		//! The direction of the ray.
		Vector2 direction;
	};

	/*****************************************************************//*!
	\struct RayResult
	\brief
		Stores the result of a raycast calculation.
	*//******************************************************************/
	struct RaycastResult
	{
		//! The collision component that was hit by the raycast.
		ecs::CompHandle<ColliderComp> hitComp;
		//! The distance from the origin point of the ray to the hit collision component.
		float distance;
		//! The collision point of the raycast.
		Vector2 collisionPoint;
		//! The collision normal of the raycast.
		Vector2 collisionNormal;

		/*****************************************************************//*!
		\brief
			Constructor
		*//******************************************************************/
		RaycastResult();
	};

	/*****************************************************************//*!
	\struct MultiRaycastResult
	\brief
		Stores the result of a raycast calculation.
	*//******************************************************************/
	struct MultiRaycastResult
	{
		//! The hits obtained by the raycast.
		std::vector<RaycastResult> hits;

		/*****************************************************************//*!
		\brief
			Sorts the hits vector based on distance.
		*//******************************************************************/
		void SortHitsByDistance();
	};

	/*****************************************************************//*!
	\enum RAYCAST_OUTCOME
	\brief
		The outcome of the raycast calculation. For use internally.
	*//******************************************************************/
	enum class RAYCAST_OUTCOME
	{
		MISS,
		NON_CLOSEST_HIT,
		CLOSEST_HIT
	};

	/*****************************************************************//*!
	\brief
		Calculates a raycast through the world.
	\param origin
		The origin of the ray.
	\param direction
		The direction of the ray.
	\param outRaycastResult
		The result of the raycast will be written here.
	\return
		True if the raycast hit something. False otherwise.
	*//******************************************************************/
	bool Raycast(const Vector2& origin, const Vector2& direction, RaycastResult* outRaycastResult);

	/*****************************************************************//*!
	\brief
		Calculates a raycast that only collides on certain layers through the world.
	\param origin
		The origin of the ray.
	\param direction
		The direction of the ray.
	\param mask
		The collision layers that the ray collides against.
	\param outRaycastResult
		The result of the raycast will be written here.
	\return
		True if the raycast hit something. False otherwise.
	*//******************************************************************/
	bool Raycast(const Vector2& origin, const Vector2& direction, const EntityLayersMask& mask, RaycastResult* outRaycastResult);

	/*****************************************************************//*!
	\brief
		Calculates a raycast through the world. Obtains all collisions
		along a raycast.
	\param origin
		The origin of the ray.
	\param direction
		The direction of the ray.
	\param maxDistance
		The maximum distance of the ray.
	\param outRaycastResult
		The result of the raycast will be written here.
	\return
		True if the raycast hit something. False otherwise.
	*//******************************************************************/
	bool MultiRaycast(const Vector2& origin, const Vector2& direction, float maxDistance, MultiRaycastResult* outRaycastResult);

	/*****************************************************************//*!
	\brief
		Calculates a raycast that only collides on certain layers through the world.
	\param origin
		The origin of the ray.
	\param direction
		The direction of the ray.
	\param maxDistance
		The maximum distance of the ray.
	\param mask
		The collision layers that the ray collides against.
	\param outRaycastResult
		The result of the raycast will be written here.
	\return
		True if the raycast hit something. False otherwise.
	*//******************************************************************/
	bool MultiRaycast(const Vector2& origin, const Vector2& direction, float maxDistance, const EntityLayersMask& mask, MultiRaycastResult* outRaycastResult);

#pragma region // Raycast

}

property_begin(Physics::ColliderComp)
{
}
property_vend_h(Physics::ColliderComp)
