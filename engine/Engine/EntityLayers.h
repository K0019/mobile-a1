/******************************************************************************/
/*!
\file   EntityLayers.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for entity layers.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

#pragma region Interface

/*****************************************************************//*!
\enum ENTITY_LAYER
\brief
	Layer mask for entity layers.
*//******************************************************************/
// We're gonna use X Macro to obtain these enums' names at runtime.
// https://stackoverflow.com/questions/11714325/how-to-get-enum-item-name-from-its-value
#define D_ENTITY_LAYER \
X(DEFAULT, "Default") \
X(ENVIRONMENT, "Environment") \
X(PLAYER, "Player") \
X(ENEMY, "Enemy") \
X(WEAPON, "Weapon") \
X(PLAYERBULLET, "Player Bullet") \
X(ENEMYBULLET, "Enemy Bullet") \
X(OBJECTIVE, "Objective") \
X(DROPPOD, "Drop Pod") \
X(MASKENTITY, "Mask Entity") \
X(PLAYERDETECTOR, "Player Detector") \
X(WEAPONDETECTOR, "Weapon Detector") 

#define X(name, str) name,
enum class ENTITY_LAYER : uint32_t
{
	D_ENTITY_LAYER

	TOTAL,
	ALL = TOTAL // For indicating to mask everything
};
#undef X
GENERATE_ENUM_CLASS_ITERATION_OPERATORS(ENTITY_LAYER)

//! The type to store the layers.
using EntityLayersMask = MaskTemplate<ENTITY_LAYER, true>;

/*****************************************************************//*!
\class EntityLayerComponent
\brief
	Contains the layer that the entity is under.
*//******************************************************************/
class EntityLayerComponent
	: public IRegisteredComponent<EntityLayerComponent>
	, public IHiddenComponent<EntityLayerComponent>
	, public ecs::IComponentCallbacks
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	EntityLayerComponent();

	/*****************************************************************//*!
	\brief
		Sets the layer that this entity is under.
	\param newLayer
		The new layer.
	*//******************************************************************/
	void SetLayer(ENTITY_LAYER newLayer);

	/*****************************************************************//*!
	\brief
		Gets the layer that this entity is under.
	\return
		The layer.
	*//******************************************************************/
	ENTITY_LAYER GetLayer() const;

	/*****************************************************************//*!
	\brief
		Converts a layer enum to a string.
	\param layer
		The layer enum.
	\return
		The name of the layer as a string.
	*//******************************************************************/
	static const char* GetLayerName(ENTITY_LAYER layer);

	/*****************************************************************//*!
	\brief
		Serializes the entity layers matrix (that determines whether the layers "collide").
	\param writer
		The serializer object.
	\param key
		The key of the json object.
	*//******************************************************************/
	static void SerializeLayersMatrix(Serializer& writer, const std::string& key);

	/*****************************************************************//*!
	\brief
		Deserializes the entity layers matrix (that determines whether the layers "collide").
	\param reader
		The deserializer object.
	\param key
		The key of the json object.
	*//******************************************************************/
	static void DeserializeLayersMatrix(Deserializer& reader, const std::string& key);

public:
	/*****************************************************************//*!
	\brief
		Registers the attached entity to the lookup table in EntitiesByLayer.
	*//******************************************************************/
	void OnAttached() override;

	/*****************************************************************//*!
	\brief
		Removes the attached entity from the lookup table in EntitiesByLayer.
	*//******************************************************************/
	void OnDetached() override;

public:
	/*****************************************************************//*!
	\brief
		Serializes this component.
	\param writer
		The serializer.
	*//******************************************************************/
	void Serialize(Serializer& writer) const override;

	/*****************************************************************//*!
	\brief
		Deserializes this component from file.
	\param reader
		The deserializer.
	*//******************************************************************/
	void Deserialize(Deserializer& reader) override;

private:
	//! The layer that the entity is under. Mask to utilize its collision matrix.
	EntityLayersMask mask;

	property_vtable()

};
property_begin(EntityLayerComponent)
{
}
property_vend_h(EntityLayerComponent)

/*****************************************************************//*!
\class EntitiesByLayer
\brief
	Tracks entities in the DEFAULT ecs pool separated by layer.
*//******************************************************************/
class EntitiesByLayer
{
private:
	//! The entities sorted by layers.
	std::array<std::unordered_set<ecs::EntityHandle>, +ENTITY_LAYER::TOTAL> layers;

public:
	/*****************************************************************//*!
	\brief
		Registers an entity to the specified layer.
	\param layer
		The layer.
	\param entity
		The entity.
	*//******************************************************************/
	void AddEntity(ENTITY_LAYER layer, ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Moves entity from a layer to another layer.
	\param fromLayer
		The layer that the entity is currently in.
	\param toLayer
		The layer to move the entity to.
	\param entity
		The entity.
	*//******************************************************************/
	void MoveEntity(ENTITY_LAYER fromLayer, ENTITY_LAYER toLayer, ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Removes an entity from the specified layer.
	\param layer
		The layer.
	\param entity
		The entity.
	*//******************************************************************/
	void RemoveEntity(ENTITY_LAYER layer, ecs::EntityHandle entity);

	template <typename Callable>
		requires std::regular_invocable<Callable, ecs::EntityHandle>
	void ExecutePerEntityInLayer(ENTITY_LAYER layer, Callable func);

	/*****************************************************************//*!
	\brief
		Gets the total number of entities in the specified layers.
	\param layersMask
		The layers to check.
	\return
		The total number of entities in the specified layers.
	*//******************************************************************/
	size_t GetNumEntitiesInLayers(EntityLayersMask layersMask);

};

// Inject a custom base system type that obtains entities based on the layer system instead.
namespace ecs {

	/*****************************************************************//*!
	\class SystemOperatingByLayer
	\brief
		A base ECS system implementation that executes only on entities in certain layers.
	\tparam SysType
		The system type.
	\tparam Args
		The components that the system requires before it processes the entity's components.
		If 2 or more components are specified, entities must have all components before being processed by this system.
		In the case of 0 components, the base ecs::System implementation should be used instead.
	*//******************************************************************/
	template <typename SysType, typename ...Args>
	class SystemOperatingByLayer : public System<SysType, Args...>
	{
	private:
		/*****************************************************************//*!
		\brief
			Registers the system type into RegisteredSystemsOperatingByLayer.
		\return
			Dummy bool.
		*//******************************************************************/
		static bool RegisterSystem();

		//! Calls RegisterSystem() once when this static is initialized at startup
		inline static bool isRegistered{ RegisterSystem() };

	public:
		/*****************************************************************//*!
		\brief
			Constructor. User-defined systems requiring 1 or more components will need to pass a member function that takes those components as references.
			e.g. System<PhysicsComp, CollisionComp> // a system requiring PhysicsComp and CollisionComp
				 will require a function defined as void FuncName(PhysicsComp&, CollisionComp&)
				 and pass it as &ClassName::FuncName to this constructor.
			If a user-defined system requires no components, no explicit constructor call is required.
		\tparam ReturnType
			The return type of the value returned by the user-defined member function.
		\tparam ClassType
			The class type of the user-defined member function.
		\param memberFunc
			The user-defined member function that will be called per entity to be processed.
		*//******************************************************************/
		template <typename ReturnType = void, typename ClassType = internal::System_Internal_Base>
		SystemOperatingByLayer(ReturnType(ClassType::*memberFunc)(Args&...) = &internal::System_Internal_Base::DummyFunc);

		/*****************************************************************//*!
		\brief
			Called by ECS to executes the system on each entity whose components meet the system's requirements.
			This implementation gathers required components from each entity that meets this system's layer requirements and calls the user function to process those components.
			If user implementations need to override this, it's likely that the original ecs::System class is a better fit.
		*//******************************************************************/
		void Run() final;

	protected:
		//! The layers that this system operates on.
		static EntityLayersMask layers;

	};

	/*****************************************************************//*!
	\class RegisteredSystemsOperatingByLayer
	\brief
		Stores information regarding systems in this project that operate by layer.
	*//******************************************************************/
	class RegisteredSystemsOperatingByLayer
	{
		using GetEntityLayersMaskSig = EntityLayersMask&(*)();
	public:
		/*****************************************************************//*!
		\brief
			Registers a system as a system operating by layer.
		\param sysHash
			The hash of the system.
		\param func
			The function that returns the system's entity layers mask.
		*//******************************************************************/
		void RegisterSystem(SysHash sysHash, GetEntityLayersMaskSig func);

		/*****************************************************************//*!
		\brief
			Gets a function that gets a system's entity layers mask.
		\param sysHash
			The hash of a system.
		\return
			The function that gets the system's entity layers mask.
			If there is no system with the provided hash compatible with entity layers mask, returns null.
		*//******************************************************************/
		GetEntityLayersMaskSig GetLayerMaskFunc(SysHash sysHash) const;

	public:
		/*****************************************************************//*!
		\brief
			Serializes this system's layer settings to file.
		\param writer
			The serializer.
		\param key
			The key to associate this system's layer settings with.
		*//******************************************************************/
		void SerializeLayerSettings(Serializer& writer, const std::string& key);

		/*****************************************************************//*!
		\brief
			Deserializes this system's layer settings from file.
		\param reader
			The dserializer.
		\param key
			The key that this system's layer settings is associated with.
		*//******************************************************************/
		void DeserializeLayerSettings(Deserializer& reader, const std::string& key);

	private:
		//! The functions that get each compatible system's entity layer mask.
		std::unordered_map<SysHash, GetEntityLayersMaskSig> systemsOperatingByLayer;

	};

}

#pragma endregion // Interface

#pragma region Definition

template <typename SysType, typename ...Args>
EntityLayersMask ecs::SystemOperatingByLayer<SysType, Args...>::layers{ ENTITY_LAYER::ALL };

template<typename Callable>
	requires std::regular_invocable<Callable, ecs::EntityHandle>
void EntitiesByLayer::ExecutePerEntityInLayer(ENTITY_LAYER layer, Callable func)
{
	std::for_each(layers[+layer].begin(), layers[+layer].end(), func);
}

namespace ecs {

	template<typename SysType, typename ...Args>
	template<typename ReturnType, typename ClassType>
	SystemOperatingByLayer<SysType, Args...>::SystemOperatingByLayer(ReturnType(ClassType::*memberFunc)(Args&...))
		: System<SysType, Args...>{ memberFunc }
	{
	}

	template<typename SysType, typename ...Args>
	bool SystemOperatingByLayer<SysType, Args...>::RegisterSystem()
	{
		ST<RegisteredSystemsOperatingByLayer>::Get()->RegisterSystem(GetSysHash<SysType>(), []() -> EntityLayersMask& {
			return layers;
		});
		return true;
	}

	template<typename SysType, typename ...Args>
	void SystemOperatingByLayer<SysType, Args...>::Run()
	{
		// If this system runs on no components, skip processing.
		if constexpr (sizeof...(Args) == 0)
			return;
		else
		{
			// If all layers are active, behavior is just like default.
			if (layers.TestMaskAll())
			{
				System<SysType, Args...>::Run();
				return;
			}

			// There are 2 ways to obtain the entities to execute on: check all entities that have the components we need, or all entities on the layers that we're operating on.
			// Let's execute the algorithm that requires checking the fewest entities.
			internal::CompArr* compArrWithFewestComps{ internal::GetCompArrWithLeastComps<Args...>() };
			size_t numEntitiesInLayers{ ST<EntitiesByLayer>::Get()->GetNumEntitiesInLayers(layers) };

			if (compArrWithFewestComps->GetNumComps() <= numEntitiesInLayers)
			{
				// Execute based on components
				System<SysType, Args...>::RunOnCompArr(*compArrWithFewestComps, [layers = layers](internal::InternalEntityHandle entity) -> bool {
					return layers.TestMask(reinterpret_cast<EntityHandle>(entity)->GetComp<EntityLayerComponent>()->GetLayer());
				});
				return;
			}

			// Execute based on layers
			layers.ExecutePerActiveBit([system = this, &callProcessEntity = System<SysType, Args...>::callProcessEntity](ENTITY_LAYER layer) -> void {
				ST<EntitiesByLayer>::Get()->ExecutePerEntityInLayer(layer, [system = system, &callProcessEntity = callProcessEntity](EntityHandle entity) -> void {
					// Need to check whether this entity has the components that we need.
					auto compPtrs{ std::make_tuple(entity->GetComp<Args>()...) };
					if (!std::apply([](auto&&... compPtrs) -> bool { return (compPtrs, ...); }, compPtrs))
						// This entity doesn't have all the components we need
						return;

					// Execute on this entity.
					std::apply([system = system, &callProcessEntity = callProcessEntity](auto&&... compPtrs) -> void {
						callProcessEntity(system, *compPtrs...);
					}, compPtrs);
				});
			});
		}
	}

}

#pragma endregion // Definition
