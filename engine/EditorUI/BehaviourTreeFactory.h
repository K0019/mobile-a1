#pragma once
#include <string>
#include <vector>
#include <unordered_map>
class BehaviorNode;

class BTFactory {
public:
    using Maker = BehaviorNode * (*)();

    static BTFactory& Instance();

    bool Register(const std::string& type, Maker fn);   // for both C++ + C#
    BehaviorNode* Create(const std::string& type) const;
    std::vector<std::string> RegisteredTypes() const;

private:
    BTFactory();
    struct Impl;
    Impl* impl;
};

// Registration macro (put this in each node’s .cpp)
#define BT_REGISTER_NODE(NodeType, TypeNameString) \
    namespace { \
        static BehaviorNode* __bt_make_##NodeType() { return new NodeType(); } \
        struct __bt_reg_##NodeType { \
            __bt_reg_##NodeType() { \
                BTFactory::Instance().Register(TypeNameString, &__bt_make_##NodeType); \
            } \
        } __bt_reg_instance_##NodeType; \
    }