#pragma once
#include <cstdint>
#include <functional>
template <typename Tag>
class ResourcePool;

template <typename Tag>
class Handle
{
public:
  using TagType = Tag;

  Handle() = default;

  [[nodiscard]] bool isValid() const { return id != 0; }

  [[nodiscard]] uint32_t getId() const { return id; }

  [[nodiscard]] uint32_t getGeneration() const { return generation; }

  bool operator==(const Handle& other) const = default;

  static Handle fromRaw(uint32_t id, uint32_t generation) { return Handle(id, generation); }

  [[nodiscard]] uint64_t getOpaqueValue() const { return (uint64_t)generation << 32 | id; }

  static Handle fromOpaqueValue(uint64_t v) { return Handle(static_cast<uint32_t>(v), static_cast<uint32_t>(v >> 32)); }

private:
  friend class ResourcePool<Tag>;
  friend struct std::hash<Handle>;

  Handle(uint32_t id, uint32_t generation) : id(id), generation(generation)
  {
  }

  uint32_t id = 0;
  uint32_t generation = 1;
};

template <typename Tag>
struct std::hash<Handle<Tag>>
{
  size_t operator()(const Handle<Tag>& handle) const noexcept { return std::hash<uint64_t>{}(handle.getOpaqueValue()); }
};
