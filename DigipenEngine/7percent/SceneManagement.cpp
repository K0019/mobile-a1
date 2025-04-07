/******************************************************************************/
/*!
\file   SceneManagement.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing a set of classes that implement a scene
  management system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "SceneManagement.h"
#include "Editor.h"
#include "History.h"
#include "GameSettings.h"

#pragma region Helper

const std::string defaultSceneName{ "DefaultScene" };

/*****************************************************************//*!
\class SceneHelper
\brief
	Helper functions for scene function implementations.
*//******************************************************************/
class SceneHelper
{
public:
	SceneHelper() = delete;

	/*****************************************************************//*!
	\brief
		Gets the filepath to the scenes folder.
	\return
		The filepath to the scenes folder.
	*//******************************************************************/
	static const std::string& GetScenesFolder()
	{
		return ST<Filepaths>::Get()->scenesSave;
	}

	/*****************************************************************//*!
	\brief
		Gets the filepath to a scene in the scenes folder.
	\return
		The filepath to the scene.
	*//******************************************************************/
	static const std::string GetPathToSceneInSceneFolder(const std::string& sceneName)
	{
		return GetScenesFolder() + '/' + sceneName + ".scene";
	}

	/*****************************************************************//*!
	\brief
		Gets the filepath to the json file storing the state of previously
		open scenes.
	\return
		The filepath to the json file.
	*//******************************************************************/
	static const std::string GetOpenScenesJsonPath()
	{
		return GetScenesFolder() + "/openScenes.json";
	}

	/*****************************************************************//*!
	\brief
		Checks whether the scenes folder exists.
	\return
		True if the scenes folder exists. False otherwise.
	*//******************************************************************/
	static bool IsScenesFolderExist()
	{
		return std::filesystem::exists(GetScenesFolder());
	}

	/*****************************************************************//*!
	\brief
		Attempts to creates the scenes folder.
	\return
		True if the scenes folder was created. False otherwise.
	*//******************************************************************/
	static bool CreateScenesFolder()
	{
		return std::filesystem::create_directory(GetScenesFolder());
	}

	/*****************************************************************//*!
	\brief
		Creates the scenes folder if it exists.
	\return
		True if the scenes folder now exists. False otherwise.
	*//******************************************************************/
	static bool EnsureScenesFolderExists()
	{
		if (!IsScenesFolderExist() && !CreateScenesFolder())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to create scenes save directory!";
			return false;
		}
		return true;
	}
};

#pragma endregion // Helper

#pragma region SceneIndexComponent

SceneIndexComponent::SceneIndexComponent(int index)
	: sceneIndex{ index }
{
}

SceneIndexComponent::SceneIndexComponent(const SceneIndexComponent& other)
	: sceneIndex{ other.sceneIndex }
{
}

SceneIndexComponent::SceneIndexComponent(SceneIndexComponent&& other) noexcept
	: sceneIndex{ other.sceneIndex }
{
	other.sceneIndex = -1;
}

SceneIndexComponent::~SceneIndexComponent()
{
	if (sceneIndex >= 0 && ST<SceneManager>::IsInitialized())
		if (Scene* scene{ ST<SceneManager>::Get()->GetSceneAtIndex(sceneIndex) })
			scene->RemoveEntity(ecs::GetEntity(this));
}

int SceneIndexComponent::GetSceneIndex() const
{
	return sceneIndex;
}

void SceneIndexComponent::SetSceneIndex(int newSceneIndex)
{
	sceneIndex = newSceneIndex;
}

#pragma endregion // SceneIndexComponent

#pragma region Scene

Scene::Scene(const std::string& name, const std::string& filepath, int index)
	: name{ name }
	, filepath{ filepath }
	, index{ index }
{
}

Scene::~Scene()
{
	// Because deleting entities causes handle invalidation, we clear history whenever a scene is unloaded
	ST<History>::Get()->Clear();

	for (ecs::EntityHandle entity : entities)
		ecs::DeleteEntity(entity);
}

int Scene::GetIndex() const
{
	return index;
}

const std::string& Scene::GetName() const
{
	return name;
}

const std::string& Scene::GetFilepath() const
{
	return filepath;
}

void Scene::SetName(const std::string& newName)
{
	name = newName;

	// We'll need to update the filepath. Erase everything past the last '/', then reappend the new scene name
	filepath.erase(filepath.rfind('/') + 1);
	filepath.append(name);
	filepath.append(".scene");
}

void Scene::AddEntity(ecs::EntityHandle entity)
{
	entity->GetComp<SceneIndexComponent>()->SetSceneIndex(index);
	entities.insert(entity);
}

void Scene::RemoveEntity(ecs::EntityHandle entity)
{
	entities.erase(entity);
}

Scene::SceneEntityIterator Scene::GetEntitiesBegin()
{
	return SceneEntityIterator{ entities.begin(), entities.end() };
}

Scene::SceneEntityIterator Scene::GetEntitiesEnd()
{
	return SceneEntityIterator{ entities.end(), entities.end() };
}

bool Scene::IsEmpty() const
{
	return entities.empty();
}

void Scene::SaveToFile()
{
	// TODO: Copy swap files
	
	// Create file
	Serializer serializer{ ST<Filepaths>::Get()->scenesSave + "/" + filepath };
	if (!serializer.IsOpen())
		return;

	// Serialize each entity
	int id{};
	for (auto iter{ entities.begin() }, end{ entities.end() }; iter != end; ++iter, ++id)
		serializer.Serialize(*iter);
}

void Scene::LoadFromFile()
{
	// Open file
	Deserializer deserializer{ filepath };
	if (!deserializer.IsValid())
		return;

	// Delete all entities under this scene
	if (!entities.empty())
	{
		for (ecs::EntityHandle entity : entities)
			ecs::DeleteEntity(entity);
		ecs::FlushChanges();
	}

	// Load the entities from file.
	while (deserializer.HasEntity())
	{
		// Create entity and load serialized components
		ecs::EntityHandle entity{ ecs::CreateEntity() };
		deserializer.Deserialize(entity);

		// Register entity to this scene
		ST<SceneManager>::Get()->SetEntitySceneIndex_NoUnparent(entity, index);
	}
}

#pragma endregion // Scene

#pragma region ScenePool

ScenePool::ScenePool(bool autoLoadDefaultScene)
	: activeScene{ nullptr }
	, autoLoadDefaultScene{ autoLoadDefaultScene }
{
	// We always need a default scene for new entities to register themselves to.
	// openScenes.json may not have the default scene within. If so, we don't want to load the default scene from file.
	// Therefore we just create an empty default scene here.
	CreateEmptyScene(defaultSceneName);
}

Scene* ScenePool::GetActiveScene()
{
	return activeScene;
}

const Scene* ScenePool::GetActiveScene() const
{
	return activeScene;
}

void ScenePool::SetActiveScene(int index)
{
	Scene* toScene{ GetSceneAtIndex(index) };
	if (!toScene)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Scene index " << index << " does not exist! Unable to set active scene.";
		return;
	}
	activeScene = toScene;
}

Scene* ScenePool::GetSceneAtIndex(int index)
{
	auto sceneIter{ loadedScenes.find(index) };
	return (sceneIter != loadedScenes.end() ? &sceneIter->second : nullptr);
}

const Scene* ScenePool::GetSceneAtIndex(int index) const
{
	auto sceneIter{ loadedScenes.find(index) };
	return (sceneIter != loadedScenes.end() ? &sceneIter->second : nullptr);
}

Scene* ScenePool::GetSceneWithName(const std::string& name)
{
	auto sceneIndexIter{ sceneNameToIndex.find(name) };
	if (sceneIndexIter == sceneNameToIndex.end())
		return nullptr;
	return &loadedScenes.at(sceneIndexIter->second);
}

const Scene* ScenePool::GetSceneWithName(const std::string& name) const
{
	auto sceneIndexIter{ sceneNameToIndex.find(name) };
	if (sceneIndexIter == sceneNameToIndex.end())
		return nullptr;
	return &loadedScenes.at(sceneIndexIter->second);
}

SceneIterator ScenePool::GetScenesBegin()
{
	return SceneIterator{ loadedScenes.begin() };
}

SceneIterator ScenePool::GetScenesEnd()
{
	return SceneIterator{ loadedScenes.end() };
}

int ScenePool::CreateEmptyScene(const std::string& name, bool setActiveScene)
{
	return CreateEmptyScene(name, SceneHelper::GetPathToSceneInSceneFolder(name), setActiveScene);
}

int ScenePool::CreateEmptyScene(const std::string& name, const std::string& filepath, bool setActiveScene)
{
	// Ensure a scene with the requested name doesn't already exist.
	auto sceneIndexIter{ sceneNameToIndex.find(name) };
	if (sceneIndexIter != sceneNameToIndex.end())
	{
		CONSOLE_LOG(LEVEL_DEBUG) << "Did not create empty scene " << name << " as it already exists.";
		return -1;
	}

	CONSOLE_LOG(LEVEL_DEBUG) << "Creating empty scene " << name;

	// Pick an unused index
	int index{};
	if (name == defaultSceneName)
		index = 0;
	else if (!availableIndexes.empty())
		index = availableIndexes.extract(availableIndexes.begin()).value();
	else
		// Add an offset to index if the default scene is not loaded.
		index = static_cast<int>(sceneNameToIndex.size()) + (loadedScenes.find(0) == loadedScenes.end());

	// Create the scene
	sceneNameToIndex.emplace(name, index);
	loadedScenes.try_emplace(index, name, filepath, index);

	// Set as active if requested
	if (setActiveScene)
		SetActiveScene(index);

	return index;
}

int ScenePool::LoadScene(const std::filesystem::path& path, bool setActiveScene)
{
	CONSOLE_LOG(LEVEL_DEBUG) << "Loading scene " << path.string();

	// Ensure scene exists
	if (!std::filesystem::exists(path))
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Attempted to load scene from path " << path << " which does not exist!";
		return -1;
	}

	// This creates an empty scene if it doesn't exist. If it already does, this will get the existing scene.
	const std::string sceneName{ path.stem().string() };
	int index{ CreateEmptyScene(sceneName, path.string(), setActiveScene) };
	if (index < 0)
	{
		CONSOLE_LOG(LEVEL_DEBUG) << "Loading the default scene. The previous debug message can be ignored.";
		index = sceneNameToIndex.at(sceneName);
	}
	Scene* scene{ GetSceneAtIndex(index) };

	// Ensure the scene is empty before loading.
	if (!scene->IsEmpty())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Attempted to load a scene which is already loaded and has entities! Aborting scene load.";
		return index;
	}

	// Populate it with the entities loaded from the folder
	scene->LoadFromFile();

	return index;
}

bool ScenePool::CheckIsSceneLoaded(const std::string& name) const
{
	return sceneNameToIndex.contains(name);
}

bool ScenePool::CheckCanUnloadScene(int index) const
{
	// Firstly, there must be at least 2 scenes or we're trying to unload a non-default scene.
	// Next, the scene must exist.
	return (loadedScenes.size() >= 2 || index > 0) &&
		loadedScenes.find(index) != loadedScenes.end();
}

bool ScenePool::UnloadScene(int index)
{
	return UnloadScene(index, true);
}

bool ScenePool::UnloadScene(int index, bool doCheckOrCreateDefault)
{
	// Ensure the scene exists
	auto sceneIter{ loadedScenes.find(index) };
	if (sceneIter == loadedScenes.end())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Attempted to unload scene index " << index << " which does not exist!";
		return false;
	}

	// If we're unloading the default scene, only allow it when there's at least 1 other scene loaded
	bool isDefaultScene{ sceneIter->second.GetName() == defaultSceneName };
	if (doCheckOrCreateDefault && isDefaultScene && loadedScenes.size() < 2)
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Disallowing unload of default scene when no other scenes are loaded.";
		return false;
	}

	CONSOLE_LOG(LEVEL_DEBUG) << "Unloading scene " << sceneIter->second.GetName();

	// Need to unselect entity if it is under the scene that we're unloading
#ifdef IMGUI_ENABLED

	if (ecs::EntityHandle selectedEntity{ ST<Editor>::Get()->GetSelectedEntity() })
		if (selectedEntity->GetComp<SceneIndexComponent>()->GetSceneIndex() == index)
			ST<Editor>::Get()->ForceUnselectEntity();
#endif
	bool isActiveSceneBeingUnloaded{ activeScene->GetIndex() == index };

	if (!isDefaultScene)
		availableIndexes.insert(index);
	sceneNameToIndex.erase(sceneIter->second.GetName());
	loadedScenes.erase(sceneIter);
	ecs::FlushChanges();

	// If the active scene was unloaded, set it to the first loaded scene in the list
	// If there are no scenes loaded, load the default scene.
	// If loading default is disabled, set active scene to null.
	if (isActiveSceneBeingUnloaded)
		if (!loadedScenes.empty())
			activeScene = &loadedScenes.begin()->second;
		else if (doCheckOrCreateDefault)
			EnsureDefaultSceneExists();
		else
			activeScene = nullptr;

	return true;
}

void ScenePool::UnloadAllScenes(const std::filesystem::path& replacingScenePath)
{
	UnloadAllScenes_NoDefaultScene();

	if (!replacingScenePath.empty())
	{
		int newSceneIndex{ LoadScene(replacingScenePath) };
		activeScene = GetSceneAtIndex(newSceneIndex);
	}
	else
		EnsureDefaultSceneExists();
}

void ScenePool::SaveAllScenes()
{
	if (!SceneHelper::EnsureScenesFolderExists())
		return;

	// Save each scene
	for (auto& scenePair : loadedScenes)
		scenePair.second.SaveToFile();
}

void ScenePool::SaveWhichScenesOpened()
{
	if (!SceneHelper::EnsureScenesFolderExists())
		return;

	Serializer serializer{ SceneHelper::GetOpenScenesJsonPath() };
	serializer.StartArray("scenes");

	for (const auto& scenePair : loadedScenes)
	{
		// Sanitize the filepath, removing everything up to /Assets/
		std::string filepath{ scenePair.second.GetFilepath() };
		size_t rootPos{ filepath.find("/Assets/") };
		if (rootPos == std::string::npos)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Scene " << scenePair.second.GetName() << " has a filepath which does not include \"Assets\"! All scenes must reside in the Assets folder!";
			CONSOLE_LOG(LEVEL_ERROR) << "Skipping saving the above scene as having been opened at shutdown.";
			continue;
		}
		filepath.erase(0, rootPos);

		serializer.Serialize("", filepath);
	}

	serializer.EndArray();
}

void ScenePool::ResetAndLoadPrevOpenScenes()
{
	CONSOLE_LOG(LEVEL_DEBUG) << "Loading state of previously loaded scenes...";

	// Ensure saves directory exists
	if (!SceneHelper::IsScenesFolderExist())
	{
		CONSOLE_LOG(LEVEL_WARNING) << "No scene directory exists.";
		return;
	}
	// Ensure openScenes.json exists
	Deserializer deserializer{ SceneHelper::GetOpenScenesJsonPath() };
	if (!deserializer.IsValid())
	{
		CONSOLE_LOG(LEVEL_WARNING) << SceneHelper::GetOpenScenesJsonPath() << " does not exist, aborting loading scenes.";
		return;
	}

	// Save current active scene name so we can restore it later
	std::string prevActiveSceneName{ activeScene->GetName() };
	// Delete all scenes and entities
	UnloadAllScenes_NoDefaultScene();
	// Ensure the default scene at least exists so entities loaded can register to default scene first when created.
	// Note: We're not using normal UnloadAllScenes() because we don't want the auto loading of the default scene from file.
	CreateEmptyScene(defaultSceneName);

	// Load each scene from the json
	std::vector<std::string> scenePaths{};
	if (!deserializer.DeserializeVar("scenes", &scenePaths))
	{
		CONSOLE_LOG(LEVEL_WARNING) << "Read error while reading " << SceneHelper::GetOpenScenesJsonPath() << ". Try deleting this file and restarting.";
		return;
	}
	for (const std::string& path : scenePaths)
		LoadScene(ST<Filepaths>::Get()->workingDir + path, false);

	// If there is another scene loaded and the default scene has nothing (didn't load), unload the default scene
	if (loadedScenes.size() > 1 && activeScene->IsEmpty())
		UnloadScene(0);

	// Reset active scene
	auto activeSceneIter{ sceneNameToIndex.find(prevActiveSceneName) };
	if (activeSceneIter != sceneNameToIndex.end())
		activeScene = GetSceneAtIndex(activeSceneIter->second);
	else
		// Active scene is not loaded. Default to the first loaded scene in the list
		activeScene = &loadedScenes.begin()->second;

	CONSOLE_LOG(LEVEL_DEBUG) << "Scenes loading finished.";
}

int ScenePool::ReloadScene(int index)
{
	Scene* sceneToUnload{ GetSceneAtIndex(index) };
	if (!sceneToUnload)
		return -1;

	std::string filepath{ sceneToUnload->GetFilepath() };
	
	// Unload the scene while ignoring default scene checks.
	if (!UnloadScene(index, false))
		return -1;

	// Load scene, and set it as active if there's no active scene.
	return LoadScene(filepath, !activeScene);
}

void ScenePool::ReloadAllScenes()
{
	CONSOLE_LOG(LEVEL_DEBUG) << "Reloading all scenes...";

	// We can't reload scenes while iterating the scenes list, so store the scene's indexes in a separate container that is safe to iterate.
	std::vector<int> scenesToReload{};
	for (const auto& [index, _] : loadedScenes)
		scenesToReload.push_back(index);

	// Reload each loaded scene
	for (int index : scenesToReload)
		ReloadScene(index);
}

void ScenePool::EnsureValidActiveScene()
{
	if (activeScene)
		return;

	if (!loadedScenes.empty())
	{
		activeScene = &loadedScenes.begin()->second;
		return;
	}

	// Absolutely no scenes loaded. Create a default scene and set that as active.
	CreateEmptyScene(defaultSceneName);
}

void ScenePool::EnsureDefaultSceneExists()
{
	if (GetSceneAtIndex(0))
		return;

	if (!autoLoadDefaultScene)
	{
		CreateEmptyScene(defaultSceneName);
		return;
	}

	if (LoadScene(SceneHelper::GetPathToSceneInSceneFolder(defaultSceneName)) < 0)
	{
		CONSOLE_LOG(LEVEL_INFO) << "Default scene does not exist on filesystem. Creating an empty default scene.";
		CreateEmptyScene(defaultSceneName);
	}
}

void ScenePool::UnloadAllScenes_NoDefaultScene()
{
	CONSOLE_LOG(LEVEL_DEBUG) << "Unloading all scenes...";

	// Need to unselect entity if there is one selected since it will be deleted.
#ifdef IMGUI_ENABLED
	ST<Editor>::Get()->ForceUnselectEntity();
#endif
	activeScene = nullptr;
	sceneNameToIndex.clear();
	loadedScenes.clear();
	availableIndexes.clear();
	ecs::FlushChanges();
}

#pragma endregion // ScenePool

#pragma region SceneManager

SceneManager::SceneManager()
	: currentScenePool{ nullptr }
{
	// Create a scene pool for the default ECS pool
	currentScenePool = &scenePoolsMap.try_emplace(ecs::POOL::DEFAULT, true).first->second;

	// Register to ECS pool events
	Messaging::Subscribe("OnECSPoolSwitched", SceneManager::OnECSPoolSwitched);
	Messaging::Subscribe("OnECSPoolDeleted", SceneManager::OnECSPoolDeleted);

	// Register to OnEntityCreated event so we can attach a scene index component to all entities
	Messaging::Subscribe("OnEntityCreated", SceneManager::RegisterEntity);
}

SceneManager::~SceneManager()
{
	Messaging::Unsubscribe("OnEntityCreated", SceneManager::RegisterEntity);

	Messaging::Unsubscribe("OnECSPoolDeleted", SceneManager::OnECSPoolDeleted);
	Messaging::Unsubscribe("OnECSPoolSwitched", SceneManager::OnECSPoolSwitched);
}

std::string const& SceneManager::GetDefaultSceneName()
{
	return defaultSceneName;
}

Scene* SceneManager::GetActiveScene()
{
	return currentScenePool->GetActiveScene();
}
const Scene* SceneManager::GetActiveScene() const
{
	return currentScenePool->GetActiveScene();
}

void SceneManager::SetActiveScene(int index)
{
	currentScenePool->SetActiveScene(index);
}

Scene* SceneManager::GetSceneAtIndex(int index)
{
	return currentScenePool->GetSceneAtIndex(index);
}
const Scene* SceneManager::GetSceneAtIndex(int index) const
{
	return currentScenePool->GetSceneAtIndex(index);
}

Scene* SceneManager::GetSceneWithName(const std::string& name)
{
	return currentScenePool->GetSceneWithName(name);
}
const Scene* SceneManager::GetSceneWithName(const std::string& name) const
{
	return currentScenePool->GetSceneWithName(name);
}

SceneIterator SceneManager::GetScenesBegin()
{
	return currentScenePool->GetScenesBegin();
}

SceneIterator SceneManager::GetScenesEnd()
{
	return currentScenePool->GetScenesEnd();
}

int SceneManager::GetEntitySceneIndex(ecs::EntityHandle entity) const
{
	if (ecs::CompHandle<SceneIndexComponent> comp{ entity->GetComp<SceneIndexComponent>() })
		return comp->GetSceneIndex();
	else
		return 0;
}

void SceneManager::SetEntitySceneIndex(ecs::EntityHandle entity, int index)
{
	// Move the entity to the target scene
	if (!SetEntitySceneIndex_NoUnparent(entity, index))
		// The entity was not moved due to some reason. Don't need to unparent the entity.
		return;

	// Unparent the entity if it is parented, as the parent is most definitely in a different scene.
	entity->GetTransform().SetParent(nullptr);
}

bool SceneManager::SetEntitySceneIndex_NoUnparent(ecs::EntityHandle entity, int index)
{
	Scene* toScene{ GetSceneAtIndex(index) };
	if (!toScene)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Scene index " << index << " does not exist! Unable to set entity index.";
		return false;
	}

	// If this entity is already in the destination scene, do nothing.
	ecs::CompHandle<SceneIndexComponent> comp{ entity->GetComp<SceneIndexComponent>() }; // Assume exists because of RegisterEntity()
	if (comp->GetSceneIndex() == index)
		return false;

	// Move the entity and its children to the destination scene.
	SetEntitySceneIndex_Recursive(entity, comp, toScene);
	return true;
}

void SceneManager::SetEntitySceneIndex_Recursive(ecs::EntityHandle entity, ecs::CompHandle<SceneIndexComponent> comp, Scene* scene)
{
	GetSceneAtIndex(comp->GetSceneIndex())->RemoveEntity(entity);
	scene->AddEntity(entity);

	// Need to switch the scene index of all children as well
	const std::set<Transform*>& children{ entity->GetTransform().GetChildren() };
	for (Transform* childTransform : children)
		SetEntitySceneIndex_Recursive(childTransform->GetEntity(), childTransform->GetEntity()->GetComp<SceneIndexComponent>(), scene);
}

void SceneManager::OnECSPoolSwitched(ecs::POOL id)
{
	auto& sceneManager{ *ST<SceneManager>::Get() };

	auto scenePoolIter{ sceneManager.scenePoolsMap.find(id) };
	if (scenePoolIter == sceneManager.scenePoolsMap.end())
		// Emplace an ecs pool that doesn't auto load default scene from file
		sceneManager.currentScenePool = &sceneManager.scenePoolsMap.try_emplace(id, false).first->second;
	else
		sceneManager.currentScenePool = &scenePoolIter->second;
}

void SceneManager::OnECSPoolDeleted(ecs::POOL id)
{
	auto& sceneManager{ *ST<SceneManager>::Get() };

	sceneManager.scenePoolsMap.erase(id);
}

bool SceneManager::CheckIsDefaultECSPool(const char* message)
{
	if (ecs::GetCurrentPoolId() != ecs::POOL::DEFAULT)
	{
		CONSOLE_LOG(LEVEL_DEBUG) << message << " when current ECS pool is not default.";
		return false;
	}
	return true;
}

int SceneManager::CreateEmptyScene(const std::string& name, bool setActiveScene)
{
	return currentScenePool->CreateEmptyScene(name, setActiveScene);
}

int SceneManager::CreateEmptyScene(const std::string& name, const std::string& filepath, bool setActiveScene)
{
	return currentScenePool->CreateEmptyScene(name, filepath, setActiveScene);
}

int SceneManager::LoadScene(const std::filesystem::path& path, bool setActiveScene)
{
	CheckIsDefaultECSPool("Loading a new scene");

	return currentScenePool->LoadScene(path, setActiveScene);
}

bool SceneManager::UnloadScene(int index)
{
	return currentScenePool->UnloadScene(index);
}

void SceneManager::UnloadAllScenes(const std::filesystem::path& replacingScenePath)
{
	currentScenePool->UnloadAllScenes(replacingScenePath);
}

bool SceneManager::CheckIsSceneLoaded(const std::string& name) const
{
	return currentScenePool->CheckIsSceneLoaded(name);
}

bool SceneManager::CheckCanUnloadScene(int index) const
{
	return currentScenePool->CheckCanUnloadScene(index);
}

void SceneManager::SaveAllScenes()
{
	CheckIsDefaultECSPool("Saving all scenes");

	currentScenePool->SaveAllScenes();
}

void SceneManager::SaveWhichScenesOpened()
{
	CheckIsDefaultECSPool("Saving which scenes are open");

	currentScenePool->SaveWhichScenesOpened();
}

void SceneManager::ResetAndLoadPrevOpenScenes()
{
	CheckIsDefaultECSPool("Resetting and loading previous scenes");

	currentScenePool->ResetAndLoadPrevOpenScenes();
}

int SceneManager::ReloadScene(int index)
{
	CheckIsDefaultECSPool("Reloading scene");

	return currentScenePool->ReloadScene(index);
}

void SceneManager::ReloadAllScenes()
{
	CheckIsDefaultECSPool("Reload all scenes");

	currentScenePool->ReloadAllScenes();
}

void SceneManager::RegisterEntity(ecs::EntityHandle entity)
{
	// If the entity was cloned, a scene index component would already be initialized.
	// Otherwise, initially register all entities to DefaultScene. SetEntitySceneIndex() will later set the entity to the appropriate scene.
	ecs::CompHandle<SceneIndexComponent> sceneIndexComp{ entity->GetComp<SceneIndexComponent>() };
	if (!sceneIndexComp)
		sceneIndexComp = entity->AddCompNow(SceneIndexComponent{ 0 });

	ST<SceneManager>::Get()->GetActiveScene()->AddEntity(entity);
}

#pragma endregion // SceneManager

#pragma region SceneIterator

SceneIterator::SceneIterator(std::unordered_map<int, Scene>::iterator&& sceneMapIter)
	: iter{ std::forward<std::remove_reference_t<decltype(sceneMapIter)>>(sceneMapIter) }
{
}

SceneIterator::pointer SceneIterator::operator*() const
{
	return &iter->second;
}

SceneIterator::pointer SceneIterator::operator->() const
{
	return &iter->second;
}

SceneIterator& SceneIterator::operator++()
{
	++iter;
	return *this;
}

SceneIterator SceneIterator::operator++(int)
{
	SceneIterator copy{ *this };
	++iter;
	return copy;
}

SceneIterator& SceneIterator::operator--()
{
	--iter;
	return *this;
}

SceneIterator SceneIterator::operator--(int)
{
	SceneIterator copy{ *this };
	--iter;
	return copy;
}

SceneIterator operator+(const SceneIterator& iter, int offset)
{
	auto iterCopy{ iter.iter };
	for (; offset; --offset)
		++iterCopy;
	return SceneIterator{ std::move(iterCopy) };
}

bool operator==(const SceneIterator& a, const SceneIterator& b)
{
	return a.iter == b.iter;
}

bool operator!=(const SceneIterator& a, const SceneIterator& b)
{
	return a.iter != b.iter;
}

#pragma endregion SceneIterator
