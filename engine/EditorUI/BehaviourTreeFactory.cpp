#include "BehaviourTreeFactory.h"

// Implementation struct hidden behind a pointer
struct BTFactory::Impl {
    // Maps type name strings (Eg. ComSelector) to Maker functions 
    std::unordered_map<std::string, Maker> makers;
};

BTFactory& BTFactory::Instance() {
    static BTFactory f; // This static instance exists only once for the whole program
    return f;
}

// allocate the Impl so that the header doesn't see the heavy data
BTFactory::BTFactory() : impl(new Impl) {}

//cleanup  Impl memory
BTFactory::~BTFactory() { delete impl; }

// Register a type to constructor mapping
// Returns true if successfully inserted, false if key already existed
bool BTFactory::Register(const std::string& type, Maker fn) {
    return impl->makers.emplace(type, fn).second;
}

// Create a node given its type name string
// If  type is registered, call the Maker function to allocate  new instance
BehaviorNode* BTFactory::Create(const std::string& type) const {
    auto it = impl->makers.find(type);
    return it == impl->makers.end() ? nullptr : it->second();
}

// Return a list of all registered type names (To be use for imgui prob)
std::vector<std::string> BTFactory::RegisteredTypes() const {
    std::vector<std::string> out;
    out.reserve(impl->makers.size());
    for (auto& kv : impl->makers) out.push_back(kv.first);
    return out;
}
// Clear all registered types (empties the registry)
// After this, no nodes can be created until they are registered again
void BTFactory::Clear() {
    impl->makers.clear();
}