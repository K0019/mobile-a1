/******************************************************************************/
/*!
\file   ECS.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for the Entity Component System (ECS) that is the
  backbone of the 7percent engine. Includes functionality to attach and detach
  components of any type to entities, and storing of systems that iterate over
  components, as well as additional useful functions such as iterating over all
  entities or components in an adhoc manner, cloning entities and their components,
  and more.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECSInternal.h"

// Forward declare of systems layers enum (defined in ECSSysLayers.h)
enum class ECS_LAYER;

namespace ecs {

	// Current ecs features implemented:
	//   - Entities (create empty, clone, delete)
	//   - Attaching and detaching components
	//   - Getting/Iterating components in entities
	//   - Buffering adding/removing of components (so adding/removing while iterating is possible)
	//   - Const semantics
	//   - Transforms in entities
	//   - Parenting of transforms
	//   - Systems (required components, layering)
	// Planned ecs features (in order of priority I think)
	//   - Prefabs

	/* REGARDING THE FEATURE OF MULTIPLE INSTANCES OF ECS WHERE YOU CAN SEPARATE ENTITIES, COMPONENTS AND SYSTEMS INTO DIFFERENT POOLS/INSTANCES,
	   IT IS AN ERROR TO PASS ANY ENTITIES/COMPONENTS/SYSTEMS THAT ARE NOT IN THE CURRENTLY ACTIVE ECS POOL/INSTANCE INTO ANY ECS FUNCTION! 
	   THIS ERROR IS SILENT AND MAY CAUSE SEVERE MEMORY ERRORS! PLEASE TAKE EXTRA CARE WHEN WORKING WITH MULTIPLE ECS POOLS/INSTANCES! */

	/* Forward Declares */

	// The type that represents an entity in ECS.
	class Entity;

	/* Type Declares */

	// The user-facing handle to an entity
	using EntityHandle = Entity*;
	// The user-facing const handle to an entity
	using ConstEntityHandle = const Entity*;
	// The user-facing type of an entity hash
	using EntityHash = internal::EntityHash;

	// The user-facing iterator to iterate over entities
	using EntityIterator = internal::ECSPool::Entity_IteratorBlueprint<EntityHandle, internal::EntMapType::iterator>;


	// The user-facing handle to a component
	template <typename CompType>
	using CompHandle = internal::InternalCompHandle<CompType>;
	// The user-facing const handle to a component
	template <typename CompType>
	using ConstCompHandle = internal::InternalCompHandle<const CompType>;

	// The user-facing type of a component hash
	using CompHash = internal::CompHash;

	// The user-facing iterator to iterate over a component array
	template <typename CompType>
	using CompIterator = internal::CompArr::iterator_blueprint<CompType, EntityHandle>;
	// The user-facing const iterator to iterate over a component array
	template <typename CompType>
	using ConstCompIterator = internal::CompArr::iterator_blueprint<const CompType, ConstEntityHandle>;

	// The user-facing iterator to iterate over an entity's components
	using EntityCompsIterator = internal::Entity_Internal::EntityComps_IteratorBlueprint<CompHandle, internal::EntCompMapType::iterator>;
	// The user-facing const iterator to iterate over an entity's components
	using ConstEntityCompsIterator = internal::Entity_Internal::EntityComps_IteratorBlueprint<ConstCompHandle, internal::EntCompMapType::const_iterator>;

	// This can be inherited from to listen for component events:
	//   OnAttached() - Called once the component is fully attached to an entity.
	//   OnDetached() - Called right before the component is detached from an entity.
	using IComponentCallbacks = internal::IComponentCallbacks;


	// The user-facing system type to inherit from
	//   Additional functions that may be overriden:
	//		OnAdded()   - called once when the system has been fully added into ecs.
	//		OnRemoved() - called once when the system is about to be removed from ecs.
	//		PreRun()    - called once before all entities have been processed.
	//		PostRun()   - called once all entities have been processed.
	template <typename SysType, typename ...Args>
	using System = internal::System_Internal<SysType, Args...>;
	// The user-facing handle to a system
	template <typename SysType>
	using SysHandle = internal::InternalGenericSysHandle<SysType>;
	// The user-facing type of a system hash
	using SysHash = internal::SysHash;


	// The user-facing type that holds meta information about component types
	using CompTypeMeta = internal::CompTypeMeta;
	// The user-facing type that holds meta information about system types
	using SysTypeMeta = internal::SysTypeMeta;

	// The type of the iterator of system type meta
	using SysTypeMetaIterator = std::unordered_map<SysHash, SysTypeMeta>::const_iterator;


	/* PUBLIC CLASSES */
	
	/*****************************************************************//*!
	\class Entity
	\brief
		The core of ECS, this entity class is simply like an empty gameobject that components can attach to.
		For standardization, use ecs::EntityHandle to keep track of entities.
	*//******************************************************************/
	class Entity : private internal::Entity_Internal
	{
	public:
		/*****************************************************************//*!
		\brief COPYING/MOVING IS DISALLOWED - Use EntityHandle to reference entities.
		*//******************************************************************/
		Entity(const Entity&) = delete;

		/*****************************************************************//*!
		\brief
			Add and attach a component. This change is buffered until ecs::FlushChanges() is called.
			Until then this component will not be accessible through GetComp().
			ENTITY COMPONENT ITERATORS WILL BE INVALIDATED!
		\tparam T
			The type of component.
		\param comp
			The component whose data will be moved and attached to this entity.
			AFTER THIS CALL IT IS NOT SAFE TO MODIFY THE COMPONENT OBJECT THAT WAS PASSED IN!
		\return
			True if the component was successfully registered to this entity.
			False if another component of this type is already attached to this entity, or ecs::DeleteEntity() was already called on this entity, in which case no changes are made.
		\code{.cpp}
			// Example of adding a physics component to an entity.
			entity->AddComp(PhysicsComponent{});
			// ... At a later point in time ...
			ecs::FlushChanges();
		\endcode
		*//******************************************************************/
		template <typename T>
		bool AddComp(T&& comp);

		/*****************************************************************//*!
		\brief
			Add and attach a component immediately to this entity. This provides better performance than AddComp().
			ENTITY COMPONENT ITERATORS AND COMPONENT ARRAY ITERATORS ITERATING THE ADDED COMPONENT TYPE WILL BE INVALIDATED! Use AddComp() instead if you are iterating the same component array type.
		\tparam T
			The type of component.
		\param comp
			The component whose data will be moved and attached to the entity.
			AFTER THIS CALL IT IS NOT SAFE TO MODIFY THE COMPONENT OBJECT THAT WAS PASSED IN!
		\param isActive
			Whether this component is active or inactive.
		\return
			ecs::CompHandle<T> to the added component. This can be used to modify a component's data.
			nullptr if another component of this type is already attached to this entity, or ecs::DeleteEntity() was already called on this entity, in which case no changes are made.
		\code{.cpp}
			// Example of adding a physics component to an entity.
			entity->AddCompNow(PhysicsComponent{});
		\endcode
		*//******************************************************************/
		template <typename T>
		CompHandle<T> AddCompNow(T&& comp, bool isActive = true);

		/*****************************************************************//*!
		\brief
			Detach and remove a component from this entity. This change is buffered until ecs::FlushChanges() is called.
			This component will immediately be inaccessible through GetComp().
			Entity component iterators will be safely valid even after this call.
		\tparam T
			The type of component.
		\return
			True if the component was successfully removed from this entity.
			False if the component doesn't exist on this entity, the component is already pending removal, or ecs::DeleteEntity() was already called on this entity.
		\code{.cpp}
			// Example of removing a physics component to an entity.
			entity->RemoveComp<PhysicsComponent>();
			// ... At a later point in time ...
			ecs::FlushChanges();
		\endcode
		*//******************************************************************/
		template <typename T>
		bool RemoveComp();

		/*****************************************************************//*!
		\brief
			Detach and remove a component from this entity. This change is buffered until ecs::FlushChanges() is called.
			This component will immediately be inaccessible through GetComp().
			Entity component iterators will be safely valid even after this call.
		\param compHash
			The hash of the component type.
		\return
			True if the component was successfully removed from this entity.
			False if the component doesn't exist on this entity, the component is already pending removal, or ecs::DeleteEntity() was already called on this entity.
		*//******************************************************************/
		bool RemoveComp(ecs::CompHash compHash);

		/*****************************************************************//*!
		\brief
			Detach and remove a component immediately from this entity. This provides better performance than RemoveComp().
			ENTITY COMPONENT ITERATORS AND COMPONENT ARRAY ITERATORS ITERATING THE ADDED COMPONENT TYPE WILL BE INVALIDATED! Use RemoveComp() instead if you are iterating the same component array type.
		\tparam T
			The type of component.
		\return
			True if the component was successfully removed from this entity.
			False if the component doesn't exist on this entity, the component is already pending removal, or ecs::DeleteEntity() was already called on this entity.
		\code{.cpp}
			// Example of removing a physics component to an entity.
			entity->RemoveCompNow<PhysicsComponent>();
		\endcode
		*//******************************************************************/
		template <typename T>
		bool RemoveCompNow();

		/*****************************************************************//*!
		\brief
			Detach and remove a component immediately from this entity. This provides better performance than RemoveComp().
			ENTITY COMPONENT ITERATORS AND COMPONENT ARRAY ITERATORS ITERATING THE ADDED COMPONENT TYPE WILL BE INVALIDATED! Use RemoveComp() instead if you are iterating the same component array type.
		\param compHash
			The hash of the component type.
		\return
			True if the component was successfully removed from this entity.
			False if the component doesn't exist on this entity, the component is already pending removal, or ecs::DeleteEntity() was already called on this entity.
		*//******************************************************************/
		bool RemoveCompNow(ecs::CompHash compHash);

		/*****************************************************************//*!
		\brief
			Checks whether an entity has a component attached.
		\tparam T
			The type of component.
		\return
			True if the entity has the component attached. False otherwise.
		*//******************************************************************/
		template <typename T>
		bool HasComp();

		/*****************************************************************//*!
		\brief
			Checks whether an entity has a component attached.
		\param compHash
			The component hash.
		\return
			True if the entity has the component attached. False otherwise.
		*//******************************************************************/
		bool HasComp(CompHash compHash);

		/*****************************************************************//*!
		\brief
			Gets a ecs::CompHandle<T> to the requested component that is attached to this entity.
		\tparam T
			The type of component.
		\return
			ecs::CompHandle<T> to the component if it is attached to this entity.
			nullptr if the component doesn't exist on this entity.
		\code{.cpp}
			// Example of checking if a physics component exists on an entity and modifying it if it does.
			if (ecs::CompHandle<PhysicsComponent> physComp{ entity->GetComp<PhysicsComponent>() })
				physComp->SetIsDynamic(true);
		\endcode
		*//******************************************************************/
		template <typename T>
		CompHandle<T> GetComp();

		/*****************************************************************//*!
		\brief
			Gets a ecs::ConstCompHandle<T> to the requested component that is attached to this entity.
		\tparam T
			The type of component.
		\return
			ecs::ConstCompHandle<T> to the component if it is attached to this entity.
			nullptr if the component doesn't exist on this entity.
		\code{.cpp}
			// Example of checking if a physics component exists on an entity and returning something if it does.
			if (ecs::ConstCompHandle<PhysicsComponent> physComp{ constEntity->GetComp<PhysicsComponent>() })
				return physComp->GetIsDynamic();
			else
				return false;
		\endcode
		*//******************************************************************/
		template <typename T>
		ConstCompHandle<T> GetComp() const;

		/*****************************************************************//*!
		\brief
			Gets a ecs::CompHandle<void*> to the requested component that is attached to this entity via component type hash.
			THIS IS VERY DANGEROUS. IF YOU DON'T CAST THE HANDLE TO THE CORRECT TYPE, TONS OF MEMORY CORRUPTION CAN OCCUR. ONLY USE IF YOU KNOW WHAT YOU'RE DOING.
		\return
			ecs::CompHandle<void*> to the component if it is attached to this entity.
			nullptr if the component doesn't exist on this entity.
		*//******************************************************************/
		CompHandle<void> GetComp(CompHash compHash);

		/*****************************************************************//*!
		\brief
			Gets a ecs::CompHandle<void*> to the requested component that is attached to this entity via component type hash.
			THIS IS VERY DANGEROUS. IF YOU DON'T CAST THE HANDLE TO THE CORRECT TYPE, TONS OF MEMORY CORRUPTION CAN OCCUR. ONLY USE IF YOU KNOW WHAT YOU'RE DOING.
		\return
			ecs::CompHandle<void*> to the component if it is attached to this entity.
			nullptr if the component doesn't exist on this entity.
		*//******************************************************************/
		ConstCompHandle<void> GetComp(CompHash compHash) const;

		/*****************************************************************//*!
		\brief
			Gets a ecs::CompHandle<T> to the requested component that is attached to any parent (and parents of those parents) of this entity.
			Could be expensive as you'd expect.
		\tparam T
			The type of component.
		\param maxIterations
			The maximum number of parents allowed to be searched.
		\return
			ecs::CompHandle<T> to the component if it is attached to a parent.
			nullptr if the component doesn't exist on any parent.
		*//******************************************************************/
		template <typename T>
		CompHandle<T> GetCompInParents(int maxIterations = 999);

		/*****************************************************************//*!
		\brief
			Gets a ecs::CompHandle<T> to the requested component that is attached to any children (and children of those children) of this entity.
			Could be very expnensive as you'd expect.
		\tparam T
			The type of component.
		\return
			ecs::CompHandle<T> to the component if it is attached to a child.
			nullptr if the component doesn't exist on any parent.
		*//******************************************************************/
		template <typename T>
		CompHandle<T> GetCompInChildren();

		/*****************************************************************//*!
		\brief
			Similar to GetCompInChildren(), but returns a vector of all comps found.
		\tparam T
			The type of component.
		\return
			Vector of component handles.
			Empty vector if the component does not exist on any children.
		*//******************************************************************/
		template <typename T>
		std::vector<CompHandle<T>> GetCompInChildrenVec();

		/*****************************************************************//*!
		\brief
			Gets an iterator to the beginning of all components attached to this entity.
		\return
			EntityCompsIterator to the start of all components attached to this entity. The order of components is unspecified.
		*//******************************************************************/
		EntityCompsIterator Comps_Begin();

		/*****************************************************************//*!
		\brief
			Gets a const iterator to the beginning of all components attached to this entity.
		\return
			ConstEntityCompsIterator to the start of all components attached to this entity. The order of components is unspecified.
		*//******************************************************************/
		ConstEntityCompsIterator Comps_Begin() const;

		/*****************************************************************//*!
		\brief
			Gets an iterator to 1 past the end of all components attached to this entity.
		\return
			EntityCompsIterator to 1 past the end of all components attached to this entity. The order of components is unspecified.
		*//******************************************************************/
		EntityCompsIterator Comps_End();

		/*****************************************************************//*!
		\brief
			Gets a const iterator to 1 past the end of all components attached to this entity.
		\return
			ConstEntityCompsIterator to 1 past the end of all components attached to this entity. The order of components is unspecified.
		*//******************************************************************/
		ConstEntityCompsIterator Comps_End() const;

		/*****************************************************************//*!
		\brief
			Sets the activeness of all components attached to this entity.
		\param active
			Whether the components are active or inactive.
		*//******************************************************************/
		void SetActive(bool active);

		/*****************************************************************//*!
		\brief
			Gets the hash of this entity.
		\return
			The hash of this entity.
		*//******************************************************************/
		EntityHash GetHash() const;

		/*****************************************************************//*!
		\brief
			Gets the transform of this entity as a reference.
		\return
			Transform& to the transform of this entity.
		*//******************************************************************/
		Transform& GetTransform();

		/*****************************************************************//*!
		\brief
			Gets the transform of this entity as a const reference.
		\return
			const Transform& to the transform of this entity.
		*//******************************************************************/
		const Transform& GetTransform() const;

	public:
		/*****************************************************************//*!
		\brief
			This is a constructor for internal use. DO NOT CALL THIS AS A USER OF THIS HEADER!
		*//******************************************************************/
		Entity(internal::EntityHash mapKey);

	private:
		/*****************************************************************//*!
		\brief
			Gets a handle to this entity.
		\return
			ecs::EntityHandle to this entity.
		*//******************************************************************/
		// Get the handle to this entity.
		EntityHandle GetHandle();

		/*****************************************************************//*!
		\brief
			Gets a const handle to this entity.
		\return
			ecs::ConstEntityHandle to this entity.
		*//******************************************************************/
		ConstEntityHandle GetHandle() const;

	};


	/* PUBLIC FUNCTIONS */

	/*****************************************************************//*!
	\brief
		Initializes ecs. Call this at program start before any other ecs function.
	*//******************************************************************/
	void Initialize();

	/*****************************************************************//*!
	\brief
		Shuts down ecs gracefully, calling destructors in a safe order and
		freeing all data.
	*//******************************************************************/
	void Shutdown();

	/*****************************************************************//*!
	\brief
		Creates an empty entity. If a parent is specified, the new entity is parented to it.
	\param parent
		Optional parameter specifying the entity that the new empty entity will be parented to.
		If nullptr, the new entity is unparented.
	\return
		EntityHandle to the newly created entity.
	*//******************************************************************/
	EntityHandle CreateEntity(EntityHandle parent = nullptr);

	/*****************************************************************//*!
	\brief
		Creates an empty entity, copying the values of the provided Transform into its own Transform.
		If the provided Transform is parented to another entity, the new entity will also be parented to that entity.
		Child entities attached to the provided transform are ignored.
	\param transformCopy
		The Transform whose values will be copied into the new empty entity.
	\return
		EntityHandle to the newly created entity.
	*//******************************************************************/
	EntityHandle CreateEntity(Transform& transformCopy);

	/*****************************************************************//*!
	\brief
		Clones an entity, creating a duplicate entity that contains the transform, all components and all data
		of the original entity exactly how it is.
		If the provided entity is parented to another entity, the new entity will also be parented to that entity.
		ALL COMPONENTS IN THE NEW ENTITY WILL BE BUFFERED! ecs::Entity::GetComp() on the new entity will only work once ecs::FlushChanges() is called.
	\param entity
		The entity to be cloned.
	\param recursive
		Whether to clone child entities as well.
	\return
		EntityHandle to the newly created cloned entity.
	*//******************************************************************/
	EntityHandle CloneEntity(EntityHandle entity, bool recursive = false);

	/*****************************************************************//*!
	\brief
		Clones an entity, creating a duplicate entity that contains the transform, all components and all data
		of the original entity exactly how it is immediately. This provides better performance than ecs::CloneEntity().
		If the provided entity is parented to another entity, the new entity will also be parented to that entity.
		COMPONENT ITERATORS TO COMPONENT ARRAYS OF ANY OF THE CLONED ENTITY'S COMPONENTS WILL BE INVALIDATED! Use ecs::CloneEntity() if this is an issue.
	\param entity
		The entity to be cloned.
	\param recursive
		Whether to clone child entities as well.
	\return
		EntityHandle to the newly created cloned entity.
	*//******************************************************************/
	EntityHandle CloneEntityNow(EntityHandle entity, bool recursive = false);

	/*****************************************************************//*!
	\brief
		Deletes an entity, detaching and removing all attached components along the way. This change is buffered until ecs::FlushChanges() is called.
	\param entity
		The entity to be deleted.
	\param recursive
		Whether to delete child entities as well.
	*//******************************************************************/
	void DeleteEntity(EntityHandle entity, bool recursive = true);

	/*****************************************************************//*!
	\brief
		Deletes an entity immediately, detaching and removing all attached components along the way.
	\param entity
		The entity to be deleted.
	\param recursive
		Whether to delete child entities as well.
	*//******************************************************************/
	void DeleteEntityNow(EntityHandle entity, bool recursive = true);

	/*****************************************************************//*!
	\brief
		Whenever components are added/removed to entities, or entities are cloned/deleted, they are not executed immediately
		and are instead first buffered in a separate container, to allow adding/removing components while iterating
		over components. Whenever you have reached a safe point where you need changes to be reflected in the entities,
		call this function before calling GetComp() on the entities.
	*//******************************************************************/
	void FlushChanges();

	/*****************************************************************//*!
	\brief
		Checks if an entity handle is to a valid entity in the currently loaded pool.
	\param handle
		The entity handle.
	\return
		True if the entity handle is valid. False otherwise.
	*//******************************************************************/
	bool IsEntityHandleValid(EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Gets an iterator to the beginning of the entities list.
	\return
		EntityIterator to the first stored entity. The order of entities is unspecified.
	*//******************************************************************/
	EntityIterator GetEntitiesBegin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to 1 past the end of the entities list.
	\return
		EntityIterator to 1 past the end of all stored entities. The order of entities is unspecified.
	*//******************************************************************/
	EntityIterator GetEntitiesEnd();


	/*****************************************************************//*!
	\brief
		Gets an iterator to the beginning of the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		CompIterator<CompType> to the beginning of the component array.
	*//******************************************************************/
	template <typename CompType>
	CompIterator<CompType> GetCompsBegin();

	/*****************************************************************//*!
	\brief
		Gets a const iterator to the beginning of the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		ConstCompIterator<CompType> to the beginning of the component array.
	*//******************************************************************/
	template <typename CompType>
	ConstCompIterator<CompType> GetCompsBeginConst();

	/*****************************************************************//*!
	\brief
		Gets an iterator to the first active component in the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		CompIterator<CompType> to the beginning of the component array.
	*//******************************************************************/
	template <typename CompType>
	CompIterator<CompType> GetCompsActiveBegin();

	/*****************************************************************//*!
	\brief
		Gets a const iterator to the first active component in the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		ConstCompIterator<CompType> to the beginning of the component array.
	*//******************************************************************/
	template <typename CompType>
	ConstCompIterator<CompType> GetCompsActiveBeginConst();

	/*****************************************************************//*!
	\brief
		Gets an iterator to 1 past the end of the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		CompIterator<CompType> to 1 past the end of the component array.
	*//******************************************************************/
	template <typename CompType>
	CompIterator<CompType> GetCompsEnd();

	/*****************************************************************//*!
	\brief
		Gets a const iterator to 1 past the end of the component array storing components of the specified component type.
	\tparam CompType
		The component type.
	\return
		ConstCompIterator<CompType> to 1 past the end of the component array.
	*//******************************************************************/
	template <typename CompType>
	ConstCompIterator<CompType> GetCompsEndConst();

	/*****************************************************************//*!
	\brief
		If you have a handle to a component, this function allows you to obtain an iterator to that component within the component array storing it.
	\param comp
		A CompHandle<CompType> to the component.
	\tparam CompType
		The component type.
	\return
		CompIterator<CompType> to the specified component within the component array.
	*//******************************************************************/
	template <typename CompType>
	CompIterator<CompType> GetCompsIter(CompHandle<CompType> comp);

	/*****************************************************************//*!
	\brief
		Gets whether a component is active or not.
	\param component
		The address to the component object. All ConstCompHandle<T> can be passed as an argument.
	\return
		True if the component is active. False otherwise.
	*//******************************************************************/
	bool GetCompActive(const void* component);

	/*****************************************************************//*!
	\brief
		Sets whether a component is active or not. This will not take effect
		until the next changes flush.
	\param component
		The address to the component object. All CompHandle<T> can be passed as an argument.
	\param isActive
		Whether the component should be set active or inactive.
	*//******************************************************************/
	void SetCompActive(void* component, bool isActive);

	/*****************************************************************//*!
	\brief
		Gets an EntityHandle to the entity that the specified component is attached to.
	\param component
		The address to the component object. All CompHandle<T> can be passed as an argument.
	\return
		EntityHandle to the entity that the specified component is attached to.
	*//******************************************************************/
	EntityHandle GetEntity(void* component);

	/*****************************************************************//*!
	\brief
		Gets a ConstEntityHandle to the entity that the specified component is attached to.
	\param component
		The address to the component object. All ConstCompHandle<T> can be passed as an argument.
	\return
		ConstEntityHandle to the entity that the specified component is attached to.
	*//******************************************************************/
	ConstEntityHandle GetEntity(const void* component);

	/*****************************************************************//*!
	\brief
		Gets an EntityHandle to the entity of the provided hash.
	\param hash
		The hash of the component.
	\return
		EntityHandle to the entity of the specified hash.
	*//******************************************************************/
	EntityHandle GetEntity(ecs::EntityHash hash);

	/*****************************************************************//*!
	\brief
		Gets the transform of the entity that the specified component is attached to as a reference.
	\param component
		The address to the component object. All CompHandle<T> can be passed as an argument.
	\return
		EntityHandle to the entity that the specified component is attached to.
	*//******************************************************************/
	Transform& GetEntityTransform(void* component);

	/*****************************************************************//*!
	\brief
		Gets the transform of the entity that the specified component is attached to as a const reference.
	\param component
		The address to the component object. All ConstCompHandle<T> can be passed as an argument.
	\return
		ConstEntityHandle to the entity that the specified component is attached to.
	*//******************************************************************/
	const Transform& GetEntityTransform(const void* component);

	/*****************************************************************//*!
	\brief
		Gets the hash of a component type. This hash is unique for each component type, even across program instances.
	\tparam CompType
		The component type.
	\return
		CompHash of the component type.
	*//******************************************************************/
	template <typename CompType>
	constexpr CompHash GetCompHash();

	/*****************************************************************//*!
	\brief
		Gets the metadata of a component type that ecs registers for each component type added to the ecs system.
		nullptr is returned if the requested component type has never been added to ecs.
	\tparam CompType
		The component type.
	\return
		const CompTypeMeta* pointing to the component type's metadata struct.
		nullptr if the component type has never been added to ecs.
	*//******************************************************************/
	// Get meta information about a component type
	template <typename CompType>
	const CompTypeMeta* GetCompMeta();

	/*****************************************************************//*!
	\brief
		Gets the metadata of a component type that ecs registers for each component type added to the ecs system.
		nullptr is returned if the requested component type has never been added to ecs.
	\param compHash
		The component type's CompHash.
	\return
		const CompTypeMeta* pointing to the component type's metadata struct.
		nullptr if the component type has never been added to ecs.
	*//******************************************************************/
	const CompTypeMeta* GetCompMeta(CompHash compHash);


	/*****************************************************************//*!
	\brief
		Adds a system object into the ecs system, which will be stored and managed by ecs.
	\tparam SysType
		The type of the system object. This type must be a child class of ecs::System.
	\param layer
		The layer to categorize the system object into.
	\param system
		The system object which will be moved and stored within ecs.
		AFTER THIS CALL IT IS NOT SAFE TO MODIFY THE SYSTEM OBJECT THAT WAS PASSED IN!
	\return
		SysHandle<SysType> to the added system. This can be used to modify the system's data.
	*//******************************************************************/
	template <typename SysType>
	SysHandle<SysType> AddSystem(ECS_LAYER layer, SysType&& system);

	/*****************************************************************//*!
	\brief
		Gets a system that is managed by ecs. This is slightly expensive and so should not be called often.
	\tparam SysType
		The type of the system object.
	\return
		SysHandle<SysType> to the system within ecs. This can be used to modify the system's data.
		nullptr if the system does not exist within ecs.
	*//******************************************************************/
	template <typename SysType>
	SysHandle<SysType> GetSystem();

	/*****************************************************************//*!
	\brief
		Gets all systems stored within the current ECS pool.
		This operation is quite expensive, so try not to call this unless required.
	\return
		A container of handles to all systems.
	*//******************************************************************/
	std::vector<SysHandle<internal::System_Internal_Base>> GetAllSystems();
	
	/*****************************************************************//*!
	\brief
		Executes all systems registered to the specified layer.
		The order of execution between systems is unspecified.
	\param layer
		The layer that the systems are categorized to.
	*//******************************************************************/
	void RunSystems(ECS_LAYER layer);

	/*****************************************************************//*!
	\brief
		Removes a system of the provided type from ecs management.
	\tparam SysType
		The type of the system.
	*//******************************************************************/
	template <typename SysType>
	bool RemoveSystem();

	/*****************************************************************//*!
	\brief
		Removes all systems in the specified layer category.
	\param layer
		The layer that the systems are categorized to.
	*//******************************************************************/
	void RemoveSystemsInLayer(ECS_LAYER layer);

	/*****************************************************************//*!
	\brief
		Removes all systems that are managed by ecs.
	*//******************************************************************/
	void RemoveAllSystems();

	/*****************************************************************//*!
	\brief
		Gets the hash of a system type. This hash is unique for each system type, even across program instances.
	\tparam SysType
		The system type.
	\return
		SysHash of the system type.
	*//******************************************************************/
	template <typename SysType>
	constexpr SysHash GetSysHash();

	/*****************************************************************//*!
	\brief
		Gets the metadata of a system type that ecs registers for each system type in this project.
		nullptr is returned if the requested system type is not a system.
	\return
		const SysTypeMeta* pointing to the system type's metadata struct.
		nullptr if the system type is not a system.
	*//******************************************************************/
	template <typename SysType>
	const SysTypeMeta* GetSysMeta();

	/*****************************************************************//*!
	\brief
		Gets the metadata of a system type that ecs registers for each system type in this project.
		nullptr is returned if the requested system type is not a system.
	\param sysHash
		The system type's SysHash.
	\return
		const SysTypeMeta* pointing to the system type's metadata struct.
		nullptr if the system type is not a system.
	*//******************************************************************/
	const SysTypeMeta* GetSysMeta(SysHash sysHash);

	/*****************************************************************//*!
	\brief
		Gets an iterator to the first registered system type meta.
	\return
		An iterator to the first registered system type meta.
	*//******************************************************************/
	SysTypeMetaIterator GetSysMetaBegin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to 1 past the last registered system type meta.
	\return
		An iterator to 1 past the last registered system type meta.
	*//******************************************************************/
	SysTypeMetaIterator GetSysMetaEnd();

	/*****************************************************************//*!
	\enum POOL
		Identifies ECS pools. Format: (name, whether to call component callbacks)
	*//******************************************************************/
#define M_ECS_POOL \
X(DEFAULT, true)		/* Default pool is ALWAYS 0 */ \
X(PREFAB, false)		/* For prefab editor */ \
X(PREFAB_CACHE, false)	/* For prefab caching */ \
X(UNDO, false)			/* For the undo feature - deleted entities are stored within this pool. */

#define X(name, callbacksEnabled) name,
	enum class POOL : int
	{
		M_ECS_POOL
	};
#undef X

	/*****************************************************************//*!
	\brief
		Switches to the specified pool of entities/components/systems.
		The default pool id at initialization is 0.
	\param id
		The id of the ECS pool.
	*//******************************************************************/
	void SwitchToPool(POOL id);

	/*****************************************************************//*!
	\brief
		Gets the id of the currently active pool.
	\return
		The id of the currently active ECS pool.
	*//******************************************************************/
	POOL GetCurrentPoolId();

	/*****************************************************************//*!
	\brief
		Deletes the specified pool of entities/components/systems, then
		returns to the default pool (0). The default pool cannot be deleted.
	\param id
		The id of the ECS pool to delete.
	*//******************************************************************/
	void DeletePool(POOL id);

	/*****************************************************************//*!
	\brief
		Clones an entity into the specified pool, creating a duplicate entity that contains the transform, all components and all data
		of the original entity exactly how it is immediately. This provides better performance than ecs::CloneEntityToPool().
		The parent entity attached to the provided entity (if it exists) are ignored.
		COMPONENT ITERATORS TO COMPONENT ARRAYS OF ANY OF THE CLONED ENTITY'S COMPONENTS WILL BE INVALIDATED! Use ecs::CloneEntityToPool() if this is an issue.
		THE ENTITY MUST BE UNDER THE CURRENTLY ACTIVE POOL.
	\param entity
		The entity to be cloned.
	\param id
		The id of the pool to clone the entity to.
	\param recursive
		Whether to clone child entities as well.
	\return
		EntityHandle to the newly created cloned entity.
	*//******************************************************************/
	EntityHandle CloneEntityToPoolNow(EntityHandle entity, POOL id, bool recursive = false);

	// TODO: Make MoveEntityToPool - required to ensure entity UID is consistent in cases where this is desired

}

#include "ECS.ipp"
