#include "BehaviourTreeFactory.h"

BTFactory::BTFactory()
    : nodeTypes{}
{
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
    std::vector<std::string> out(nodeTypes.size());
    for (auto& kv : nodeTypes) 
        out.push_back(kv.first);
    return out;
}

void BTFactory::Clear() 
{
    nodeTypes.clear();
}