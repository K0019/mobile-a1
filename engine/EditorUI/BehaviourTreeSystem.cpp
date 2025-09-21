#include "BehaviourTreeSystem.h"
#include <filesystem>
namespace fs = std::filesystem;

bool BehaviorTreeSystem:: LoadAll(const std::string& dir)
{

	size_t loadedCount = 0;

    //only use filesystem here so i can loop through a directory
    //serialising still done through the default serialiser.h
    for (auto& entry : fs::directory_iterator(dir))
    {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto& path = entry.path();
        const auto ext = path.extension().string();

        //checking if file extention is legit
        if (ext != ".bht" && ext != ".json") {
            continue;
        }

        Deserializer reader(path.string().c_str());
        if (!reader.IsValid())
        {
            CONSOLE_LOG(LEVEL_ERROR) << "[BTSystem] Failed to open BT file: " << path << "\n";
            continue;
        }

        BehaviorTreeAsset asset;
        if (!reader.Deserialize(&asset))
        {
            CONSOLE_LOG(LEVEL_ERROR) << "[BTSystem] Failed to deserialize asset: " << path << "\n";
            continue;
        }

        if (asset.name.empty())
        {
            // Use filename as fallback
            asset.name = path.stem().string();
        }

        //call create functions from behaviourTree.h
        if (!CreateFromAsset(asset))
        {
            CONSOLE_LOG(LEVEL_ERROR) << "[BTSystem] CreateFromAsset failed for: " << asset.name << "\n";
            continue;
        }

        ++loadedCount;
    }

    CONSOLE_LOG(LEVEL_INFO) << "[BTSystem] Loaded " << loadedCount << " behavior tree(s) from " << dir << "\n";
    return loadedCount > 0;

}

void BehaviorTreeSystem::Initialize()
{
    if (auto* fp = ST<Filepaths>::Get())
    {
        defaultDir = fp->behaviourTreeSave ;
    }
    else
    {
        defaultDir = "Assets/BehaviorTrees";// default path but shouldnt be in used
    }

    if (!LoadAll(defaultDir)) {
        CONSOLE_LOG(LEVEL_ERROR) << "[BTSystem] No trees loaded from '" << defaultDir
            << "'. (This can be fine for first run.)\n";
    }
    
}

void BehaviorTreeSystem::UpdateAll(float dt)
{
    for (auto& [name, tree] : trees)
    {
        if (tree) {
            tree->Update(dt);
        }
    }
}

BehaviorTree* BehaviorTreeSystem::Get(const std::string& name) const
{
    auto it = trees.find(name);
    return it != trees.end() ? it->second.get() : nullptr;
}

bool BehaviorTreeSystem::Exists(const std::string& name) const
{
    return trees.find(name) != trees.end();
}

void BehaviorTreeSystem::Remove(const std::string& name)
{
    trees.erase(name);
}

void BehaviorTreeSystem::Shutdown()
{
    // SaveAll(defaultDir);
    trees.clear(); // unique_ptr cleanup
}

bool BehaviorTreeSystem::CreateFromAsset(const BehaviorTreeAsset& asset)
{
    // Build a new runtime tree
    auto tree = std::make_unique<BehaviorTree>();
    tree->InitFromAsset(asset);

    //if (tree->rootNode == nullptr)  // if you expose root for validation, or add a Validate() method
    //{
    //    std::cerr << "[BTSystem] Tree has no root after InitFromAsset: " << asset.name << "\n";
    //    return false;
    //}

    trees[asset.name] = std::move(tree);
    return true;
}