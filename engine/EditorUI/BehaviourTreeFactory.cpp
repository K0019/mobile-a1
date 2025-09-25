#include "BehaviourTreeFactory.h"

BTFactory::BTFactory()
    : nodeTypes{}
{
    SetAllFilePath();
}

BTFactory::~BTFactory()
{
    nodeTypes.clear();
}

bool BTFactory::Register(const std::string& type, Maker fn) 
{
    return nodeTypes.emplace(type, fn).second;
}

BehaviorNode* BTFactory::Create(const std::string& type) const 
{
    auto it = nodeTypes.find(type);
    return it == nodeTypes.end() ? nullptr : it->second();
}

std::vector<std::string> BTFactory::RegisteredTypes() const 
{
    std::vector<std::string> out;
    for (auto& kv : nodeTypes) 
        out.push_back(kv.first);
    return out;
}

void BTFactory::SetAllFilePath()
{
    for (const auto& entry : std::filesystem::directory_iterator(ST<Filepaths>::Get()->behaviourTreeSave))
        if (std::filesystem::is_regular_file(entry.status()))
            filePaths[entry.path().stem().string()] = entry.path().filename().string();
    
}

std::string const& BTFactory::GetFilePath(const std::string& btName) const
{
    return filePaths.contains(btName) ? filePaths.at(btName) : "";
}

void BTFactory::GetAllBTNames(std::vector<std::string>& out) const
{
    for (auto const& pair : filePaths)
        out.push_back(pair.first);
    std::sort(out.begin(), out.end());
}

void BTFactory::Clear() 
{
    nodeTypes.clear();
}