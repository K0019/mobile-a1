#pragma once

class ResourceNames
{
public:
    const std::string* GetName(size_t hash) const;
    void SetName(size_t hash, const std::string& name);
    void RemoveName(size_t hash);

private:
    std::unordered_map<size_t, std::string> names;

};
