/******************************************************************************/
/*!
\file   SceneManagement.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a set of classes that implement a scene
  management system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

// Forward declarations
struct SceneIterator;

/*****************************************************************//*!
\class SceneIndexComponent
\brief
	A component to track which scene an entity is under.
*//******************************************************************/
class SceneIndexComponent : private IHiddenComponent<SceneIndexComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param index
		The index of the scene that this entity is under.
	*//******************************************************************/
	SceneIndexComponent(int index);
	
	/*****************************************************************//*!
	\brief
		Copy constructor.
		We need to remove entity reference if the entity dies and we get removed from ecs,
		but ecs can potentially move us to a new location and destroy the old component data at any point in time.
		So we need to implement this move/destructor combo to identify and avoid cases like that
	\param other
		The scene index component to copy.
	*//******************************************************************/
	SceneIndexComponent(const SceneIndexComponent& other);

	/*****************************************************************//*!
	\brief
		Move constructor.
		We need to remove entity reference if the entity dies and we get removed from ecs,
		but ecs can potentially move us to a new location and destroy the old component data at any point in time.
		So we need to implement this move/destructor combo to identify and avoid cases like that
	\param other
		The scene index component to copy.
	*//******************************************************************/
	SceneIndexComponent(SceneIndexComponent&& other) noexcept;

	/*****************************************************************//*!
	\brief
		Destructor.
		Informs the scene manager to remove this entity from its scene.
	*//******************************************************************/
	~SceneIndexComponent();

	/*****************************************************************//*!
	\brief
		Gets the index of the scene that this entity is under.
	\return
		The index of the scene that this entity is under.
	*//******************************************************************/
	int GetSceneIndex() const;

	/*****************************************************************//*!
	\brief
		Sets the index of the scene that this entity is under.
		This should only be used by the scene system. DO NOT CALL THIS MANUALLY!
		Use SceneManager::SetEntitySceneIndex() instead.
	\param newSceneID
		The index of the scene that this entity is under.
	*//******************************************************************/
	void SetSceneIndex(int newSceneID);

private:
	//! The index of the scene that this entity is under.
	int sceneIndex;

};

/*****************************************************************//*!
\class Scene
\brief
	Represents a scene object. Tracks entities that are under it.
*//******************************************************************/
class Scene
{
public:
	using EntitiesContainerType = std::unordered_set<ecs::EntityHandle>;
	// Iterator type to iterate scene entities (piggy-backing on ECS implementation so iteration skips deleted entities)
	using SceneEntityIterator = ecs::internal::ECSPool::Entity_IteratorBlueprint<ecs::EntityHandle, EntitiesContainerType::iterator>;

	/*****************************************************************//*!
	\brief
		Constructor.
	\param name
		The name of this scene.
	\param filepath
		The filepath to store this scene at.
	\param index
		The index of this scene.
	*//******************************************************************/
	Scene(const std::string& name, const std::string& filepath, int index);

	/*****************************************************************//*!
	\brief
		Destructor. Deletes all entities under it.
	*//******************************************************************/
	~Scene();

	/*****************************************************************//*!
	\brief
		Gets the index of this scene.
	\return
		The index of this scene.
	*//******************************************************************/
	int GetIndex() const;

	/*****************************************************************//*!
	\brief
		Gets the name of this scene.
	\return
		The name of this scene.
	*//******************************************************************/
	const std::string& GetName() const;

	/*****************************************************************//*!
	\brief
		Gets the filepath of this scene.
	\return
		The filepath of this scene.
	*//******************************************************************/
	const std::string& GetFilepath() const;

	/*****************************************************************//*!
	\brief
		Sets the name, and filepath according to this new name, of this scene.
	\param newName
		The new name of this scene.
	*//******************************************************************/
	void SetName(const std::string& newName);

	/*****************************************************************//*!
	\brief
		Adds an entity to this scene.
		DO NOT CALL THIS MANUALLY! Use SceneManager::SetEntitySceneIndex() instead.
	\param entity
		The entity to add to this scene.
	*//******************************************************************/
	void AddEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Removes an entity from this scene.
		DO NOT CALL THIS MANUALLY! Use SceneManager::SetEntitySceneIndex() instead.
	\param entity
		The entity to remove from this scene.
	*//******************************************************************/
	void RemoveEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Gets an iterator to the first entity in this scene.
	\return
		An iterator to the first entity in this scene.
	*//******************************************************************/
	SceneEntityIterator GetEntitiesBegin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to one past the last entity in this scene.
	\return
		An iterator to one past the last entity in this scene.
	*//******************************************************************/
	SceneEntityIterator GetEntitiesEnd();

	/*****************************************************************//*!
	\brief
		Gets whether this scene has no entities.
	\return
		True if this scene has no entities. False otherwise.
	*//******************************************************************/
	bool IsEmpty() const;

	/*****************************************************************//*!
	\brief
		Saves all entities under this scene to filepath.
	*//******************************************************************/
	void SaveToFile();

	/*****************************************************************//*!
	\brief
		Loads all entities from filepath and initializes them in the
		ECS system.
	*//******************************************************************/
	void LoadFromFile();

private:
	//! The entities under this scene.
	EntitiesContainerType entities;
	//! The name of this scene.
	std::string name;
	//! The filepath of this scene.
	std::string filepath;
	//! The index of this scene.
	int index;
};

/*****************************************************************//*!
\class ScenePool
\brief
	Represents a set of scenes. To be used in conjunction with ECS pools.
*//******************************************************************/
class ScenePool
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	\param autoLoadDefaultScene
		Whether to load the default scene from file if this pool becomes empty of loaded scenes.
		If false, this pool simply creates an empty default scene.
	*//******************************************************************/
	ScenePool(bool autoLoadDefaultScene);

	/*****************************************************************//*!
	\brief
		Gets the active scene.
	\return
		A pointer to the active scene.
	*//******************************************************************/
	Scene* GetActiveScene();

	/*****************************************************************//*!
	\brief
		Gets the active scene.
	\return
		A pointer to the active scene.
	*//******************************************************************/
	const Scene* GetActiveScene() const;

	/*****************************************************************//*!
	\brief
		Sets the active scene.
	\param index
		The index of the scene to be set active.
	*//******************************************************************/
	void SetActiveScene(int index);

	/*****************************************************************//*!
	\brief
		Gets a scene at the given index.
	\param index
		The index of the scene to retrieve.
	\return
		The scene at the given index. If none exists, returns nullptr.
	*//******************************************************************/
	Scene* GetSceneAtIndex(int index);

	/*****************************************************************//*!
	\brief
		Gets a scene at the given index.
	\param index
		The index of the scene to retrieve.
	\return
		The scene at the given index. If none exists, returns nullptr.
	*//******************************************************************/
	const Scene* GetSceneAtIndex(int index) const;

	/*****************************************************************//*!
	\brief
		Gets a scene with the given name.
	\param name
		The name of the scene to retrieve.
	\return
		The scene with the given name. If none exists, returns nullptr.
	*//******************************************************************/
	Scene* GetSceneWithName(const std::string& name);

	/*****************************************************************//*!
	\brief
		Gets a scene with the given name.
	\param name
		The name of the scene to retrieve.
	\return
		The scene with the given name. If none exists, returns nullptr.
	*//******************************************************************/
	const Scene* GetSceneWithName(const std::string& name) const;

	/*****************************************************************//*!
	\brief
		Gets an iterator to the first scene.
	\return
		An iterator to the first scene.
	*//******************************************************************/
	SceneIterator GetScenesBegin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to one past the last scene.
	\return
		An iterator to one past the last scene.
	*//******************************************************************/
	SceneIterator GetScenesEnd();

	/*****************************************************************//*!
	\brief
		Creates an empty scene.
	\param name
		The name of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int CreateEmptyScene(const std::string& name, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Creates an empty scene.
	\param name
		The name of the scene.
	\param filepath
		The filepath of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int CreateEmptyScene(const std::string& name, const std::string& filepath, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Creates a scene from the specified filepath.
	\param filepath
		The filepath of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int LoadScene(const std::filesystem::path& path, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Checks whether a scene is currently loaded.
	\param name
		The name of the scene to query.
	\return
		True if the scene is currently loaded. False otherwise.
	*//******************************************************************/
	bool CheckIsSceneLoaded(const std::string& name) const;

	/*****************************************************************//*!
	\brief
		Checks if a scene can be unloaded.
	\param index
		The index of the scene.
	\return
		Whether the scene can be unloaded.
	*//******************************************************************/
	bool CheckCanUnloadScene(int index) const;

	/*****************************************************************//*!
	\brief
		Unloads a scene. Creates an empty default scene afterwards if there
		are no loaded scenes left.
	\param index
		The index of the scene to be unloaded.
	\return
		True if the scene was unloaded. False otherwise.
	*//******************************************************************/
	bool UnloadScene(int index);

	/*****************************************************************//*!
	\brief
		Unloads all scenes, then loads the specified scene. If unspecified,
		creates an empty default scene afterwards.
	\param replacingScenePath
		The scene that will be loaded after all scenes are unloaded. If empty,
		loads the default scene.
	*//******************************************************************/
	void UnloadAllScenes(const std::filesystem::path& replacingScenePath);

	/*****************************************************************//*!
	\brief
		Saves all loaded scenes to file.
	*//******************************************************************/
	void SaveAllScenes();

	/*****************************************************************//*!
	\brief
		Save the state of which scenes are currently loaded.
	*//******************************************************************/
	void SaveWhichScenesOpened();

	/*****************************************************************//*!
	\brief
		Unloads all scenes, then loads previously opened scenes.
	*//******************************************************************/
	void ResetAndLoadPrevOpenScenes();

	/*****************************************************************//*!
	\brief
		Reloads the specified scene. Note that this may change the index
		of the reloaded scene.
	\param index
		The index of the scene to reload.
	\return
		The reloaded scene's index. -1 if the scene failed to be reloaded.
	*//******************************************************************/
	int ReloadScene(int index);

	/*****************************************************************//*!
	\brief
		Reloads all open scenes, then loads them back from file.
	*//******************************************************************/
	void ReloadAllScenes();

private:
	/*****************************************************************//*!
	\brief
		Unloads a scene. If doCheckOrCreateDefault is true, only unloads if
		the scene to unload is not the default scene or there is 2 or more
		scenes open, and creates an empty default scene afterwards if there
		are no loaded scenes left.
	\param index
		The index of the scene to be unloaded.
	\param doCheckOrCreateDefault
		Whether to check for default scene and to create the default scene.
	\return
		True if the scene was unloaded. False otherwise.
	*//******************************************************************/
	bool UnloadScene(int index, bool doCheckOrCreateDefault);

	/*****************************************************************//*!
	\brief
		If the active scene does not exist, sets the active scene to a
		loaded scene if there is one.
	*//******************************************************************/
	void EnsureValidActiveScene();

	/*****************************************************************//*!
	\brief
		Loads/Creates a default scene if it doesn't exist.
	*//******************************************************************/
	void EnsureDefaultSceneExists();

	/*****************************************************************//*!
	\brief
		Unloads all scenes without creating an empty default scene afterwards.
	*//******************************************************************/
	void UnloadAllScenes_NoDefaultScene();

private:
	//! A map of scene names to their indexes.
	std::unordered_map<std::string, int> sceneNameToIndex;
	//! A map of scene indexes to scene objects.
	std::unordered_map<int, Scene> loadedScenes;
	//! The active scene.
	Scene* activeScene;

	//! Available scene indexes that new scenes can use.
	std::set<int> availableIndexes;
	
	//! Whether to load the default scene from file when this pool becomes empty of loaded scenes.
	//! If false, the pool simply creates an empty default scene.
	bool autoLoadDefaultScene;

};

/*****************************************************************//*!
\class SceneManager
\brief
	Interface for interacting with scenes.
*//******************************************************************/
class SceneManager
{
public:
	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~SceneManager();

	/*****************************************************************//*!
	\brief
		Get the name of the default scene (main game scene)
	\return
		Name of the default scene.
	*//******************************************************************/
	std::string const& GetDefaultSceneName();

	/*****************************************************************//*!
	\brief
		Gets the active scene.
	\return
		A pointer to the active scene.
	*//******************************************************************/
	Scene* GetActiveScene();

	/*****************************************************************//*!
	\brief
		Gets the active scene.
	\return
		A pointer to the active scene.
	*//******************************************************************/
	const Scene* GetActiveScene() const;

	/*****************************************************************//*!
	\brief
		Sets the active scene.
	\param index
		The index of the scene to be set active.
	*//******************************************************************/
	void SetActiveScene(int index);

	/*****************************************************************//*!
	\brief
		Gets a scene at the given index.
	\param index
		The index of the scene to retrieve.
	\return
		The scene at the given index. If none exists, returns nullptr.
	*//******************************************************************/
	Scene* GetSceneAtIndex(int index);

	/*****************************************************************//*!
	\brief
		Gets a scene at the given index.
	\param index
		The index of the scene to retrieve.
	\return
		The scene at the given index. If none exists, returns nullptr.
	*//******************************************************************/
	const Scene* GetSceneAtIndex(int index) const;

	/*****************************************************************//*!
	\brief
		Gets a scene with the given name.
	\param name
		The name of the scene to retrieve.
	\return
		The scene with the given name. If none exists, returns nullptr.
	*//******************************************************************/
	Scene* GetSceneWithName(const std::string& name);

	/*****************************************************************//*!
	\brief
		Gets a scene with the given name.
	\param name
		The name of the scene to retrieve.
	\return
		The scene with the given name. If none exists, returns nullptr.
	*//******************************************************************/
	const Scene* GetSceneWithName(const std::string& name) const;

	/*****************************************************************//*!
	\brief
		Gets an iterator to the first scene.
	\return
		An iterator to the first scene.
	*//******************************************************************/
	SceneIterator GetScenesBegin();

	/*****************************************************************//*!
	\brief
		Gets an iterator to one past the last scene.
	\return
		An iterator to one past the last scene.
	*//******************************************************************/
	SceneIterator GetScenesEnd();

	/*****************************************************************//*!
	\brief
		Gets the scene index of an entity.
	\param entity
		The entity whose scene index is to be checked.
	\return
		The entity's scene index.
	*//******************************************************************/
	int GetEntitySceneIndex(ecs::EntityHandle entity) const;

	/*****************************************************************//*!
	\brief
		Sets the scene index of an entity.
	\param entity
		The entity whose scene index is to be set.
	\param index
		The entity's scene index.
	*//******************************************************************/
	void SetEntitySceneIndex(ecs::EntityHandle entity, int index);

	/*****************************************************************//*!
	\brief
		Sets the scene index of an entity without unparenting.
		FOR INTERNAL USE. THIS SHOULD NOT BE NEEDED ELSEWHERE.
		HIGH LIKELIHOOD OF ERRORS IF USED IMPROPERLY!
	\param entity
		The entity whose scene index is to be set.
	\param index
		The entity's scene index.
	\return
		True if the entity's scene index was successfully changed. False otherwise.
	*//******************************************************************/
	bool SetEntitySceneIndex_NoUnparent(ecs::EntityHandle entity, int index);

	/*****************************************************************//*!
	\brief
		Creates an empty scene.
	\param name
		The name of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int CreateEmptyScene(const std::string& name, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Creates an empty scene.
	\param name
		The name of the scene.
	\param filepath
		The filepath of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int CreateEmptyScene(const std::string& name, const std::string& filepath, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Creates a scene from the specified filepath.
	\param filepath
		The filepath of the scene.
	\param setActiveScene
		Whether to set this new scene as active.
	\return
		The scene's index. -1 if the scene failed to be created.
	*//******************************************************************/
	int LoadScene(const std::filesystem::path& path, bool setActiveScene = true);

	/*****************************************************************//*!
	\brief
		Unloads a scene. Creates an empty default scene afterwards if there
		are no loaded scenes left.
	\param index
		The index of the scene to be unloaded.
	\return
		True if the scene was unloaded. False otherwise.
	*//******************************************************************/
	bool UnloadScene(int index);

	/*****************************************************************//*!
	\brief
		Unloads all scenes, then loads the specified scene. If unspecified,
		creates an empty default scene afterwards.
	\param replacingScenePath
		The scene that will be loaded after all scenes are unloaded. If empty,
		loads the default scene.
	*//******************************************************************/
	void UnloadAllScenes(const std::filesystem::path& replacingScenePath = "");

	/*****************************************************************//*!
	\brief
		Checks whether a scene is currently loaded.
	\param name
		The name of the scene to query.
	\return
		True if the scene is currently loaded. False otherwise.
	*//******************************************************************/
	bool CheckIsSceneLoaded(const std::string& name) const;

	/*****************************************************************//*!
	\brief
		Checks if a scene can be unloaded.
	\param index
		The index of the scene.
	\return
		Whether the scene can be unloaded.
	*//******************************************************************/
	bool CheckCanUnloadScene(int index) const;

	/*****************************************************************//*!
	\brief
		Saves all loaded scenes to file.
	*//******************************************************************/
	void SaveAllScenes();

	/*****************************************************************//*!
	\brief
		Save the state of which scenes are currently loaded.
	*//******************************************************************/
	void SaveWhichScenesOpened();

	/*****************************************************************//*!
	\brief
		Unloads all scenes, then loads previously opened scenes.
	*//******************************************************************/
	void ResetAndLoadPrevOpenScenes();

	/*****************************************************************//*!
	\brief
		Reloads the specified scene. Note that this may change the index
		of the reloaded scene.
	\param index
		The index of the scene to reload.
	\return
		The reloaded scene's index. -1 if the scene failed to be reloaded.
	*//******************************************************************/
	int ReloadScene(int index);

	/*****************************************************************//*!
	\brief
		Reloads all open scenes, then loads them back from file.
	*//******************************************************************/
	void ReloadAllScenes();

private:
	/*****************************************************************//*!
	\brief
		Attaches a scene index component to the specified entity.
	\param entity
		The entity to attach a scene index component to.
	*//******************************************************************/
	static void RegisterEntity(ecs::EntityHandle entity);

	/*****************************************************************//*!
	\brief
		Sets the scene index of an entity and all of its children.
	\param entity
		The entity whose scene index and its children's scene indexes are to be set.
	\param comp
		The entity's scene index component.
	\param index
		The entity's scene index.
	*//******************************************************************/
	void SetEntitySceneIndex_Recursive(ecs::EntityHandle entity, ecs::CompHandle<SceneIndexComponent> comp, Scene* scene);

	/*****************************************************************//*!
	\brief
		Ensures a scene pool exists for the newly active ECS pool.
	\param id
		The ECS pool's index.
	*//******************************************************************/
	static void OnECSPoolSwitched(ecs::POOL id);

	/*****************************************************************//*!
	\brief
		Frees the scene pool associated with the deleted ECS pool.
	\param id
		The ECS pool's index.
	*//******************************************************************/
	static void OnECSPoolDeleted(ecs::POOL id);

	/*****************************************************************//*!
	\brief
		Helper function to check if the current ECS pool is the default pool,
		and prints a message if it isn't.
	\param message
		Prints message + " when current ECS pool is not default." if the current pool is not default.
	\return
		True if the current ECS pool is the default pool. False otherwise.
	*//******************************************************************/
	static bool CheckIsDefaultECSPool(const char* message);

private:
	//! ECS pool index to Scene pools
	std::unordered_map<ecs::POOL, ScenePool> scenePoolsMap;

	//! The current pool that contains entities within the current ECS pool.
	ScenePool* currentScenePool;

	friend ST<SceneManager>;

	/*****************************************************************//*!
	\brief
		Constructor. Initializes an empty default scene.
	*//******************************************************************/
	SceneManager();
};

/*****************************************************************//*!
\struct SceneIterator
\brief
	This is the implementation for an iterator that iterates all loaded scenes.
*//******************************************************************/
struct SceneIterator
{
	// note: these types may not be correct... further research needed if we want to support <algorithm> library
	using iterator_category = std::bidirectional_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using value_type = Scene;
	using pointer = value_type*;
	using reference = value_type&;

	/*****************************************************************//*!
	\brief
		Constructs an iterator to the scene map with the provided iterator to the map.
	\param sceneMapIter
		An iterator to the scene.
	*//******************************************************************/
	SceneIterator(std::unordered_map<int, Scene>::iterator&& sceneMapIter);

	/*****************************************************************//*!
	\brief
		Dereferences the iterator, accessing the scene pointed to by this iterator.
	\return
		A pointer to the scene.
	*//******************************************************************/
	pointer operator*() const;

	/*****************************************************************//*!
	\brief
		Dereferences the iterator, accessing the scene pointed to by this iterator.
	\return
		A pointer to the scene.
	*//******************************************************************/
	pointer operator->() const;

	/*****************************************************************//*!
	\brief
		Prefix increments this iterator to the next scene within the map.
	\return
		A reference to this iterator.
	*//******************************************************************/
	SceneIterator& operator++();

	/*****************************************************************//*!
	\brief
		Postfix increments this iterator to the next scene within the map.
	\return
		A temporary of the original state of this iterator.
	*//******************************************************************/
	SceneIterator operator++(int);

	/*****************************************************************//*!
	\brief
		Prefix decrements this iterator to the previous scene within the map.
	\return
		A reference to this iterator.
	*//******************************************************************/
	SceneIterator& operator--();

	/*****************************************************************//*!
	\brief
		Postfix decrements this iterator to the previous scene within the map.
	\return
		A temporary of the original state of this iterator.
	*//******************************************************************/
	SceneIterator operator--(int);

	/*****************************************************************//*!
	\brief
		Increments or decrements an iterator by the specified amount.
	\param iter
		The iterator to increment/decrement.
	\param offset
		The number of times to increment/decrement the iterator by.
	\return
		A temporary of this iterator incremented/decremented by the specified number of times.
	*//******************************************************************/
	friend SceneIterator operator+(const SceneIterator& iter, int offset);

	/*****************************************************************//*!
	\brief
		Tests if 2 iterators point to the same scene
	\param a
		Left-hand side iterator.
	\param b
		Right-hand side iterator.
	\return
		True if both iterators point to the same scene. False otherwise.
	*//******************************************************************/
	friend bool operator==(const SceneIterator& a, const SceneIterator& b);

	/*****************************************************************//*!
	\brief
		Tests if 2 iterators do not point to the same scene
	\param a
		Left-hand side iterator.
	\param b
		Right-hand side iterator.
	\return
		True if both iterators do not point to the same scene. False otherwise.
	*//******************************************************************/
	friend bool operator!=(const SceneIterator& a, const SceneIterator& b);

private:
	//! The scene map iterator pointing to the current scene.
	std::unordered_map<int, Scene>::iterator iter;
};
