#include "BehaviourTreeFactory.h"

struct BTFactory::Impl {
    std::unordered_map<std::string, Maker> makers;
};

BTFactory& BTFactory::Instance() {
    static BTFactory f;
    return f;
}

BTFactory::BTFactory() : impl(new Impl) {}
BTFactory::~BTFactory() { delete impl; }

bool BTFactory::Register(const std::string& type, Maker fn) {
    return impl->makers.emplace(type, fn).second;
}

BehaviorNode* BTFactory::Create(const std::string& type) const {
    auto it = impl->makers.find(type);
    return it == impl->makers.end() ? nullptr : it->second();
}

std::vector<std::string> BTFactory::RegisteredTypes() const {
    std::vector<std::string> out;
    out.reserve(impl->makers.size());
    for (auto& kv : impl->makers) out.push_back(kv.first);
    return out;
}

void BTFactory::Clear() {
    impl->makers.clear();
}