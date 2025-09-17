#pragma once
#include <vector>
#include <cassert>
#include "asset_handle.h"
#include "asset_types.h"

template <typename Asset>
class AssetPool
{
  static_assert(IsAsset<Asset>, "HotColdPool<T>: T must be a valid asset type.");

  using Traits = AssetTraits<Asset>;
  using HotData = typename Traits::HotData;
  using ColdData = typename Traits::ColdData;

  static constexpr uint32_t LIST_END = UINT32_MAX;

  struct PoolEntry
  {
    uint32_t generation = 1;
    uint32_t nextFreeIndex = LIST_END;
  };

  std::vector<PoolEntry> m_metadata;
  std::vector<HotData> m_hotData;
  std::vector<ColdData> m_coldData;

  uint32_t m_freeListHead = LIST_END;
  uint32_t m_activeCount = 0;

  void grow()
  {
    const size_t oldSize = m_metadata.size();
    const size_t newSize = std::max(oldSize + oldSize / 2, oldSize + 16);

    m_metadata.resize(newSize);
    m_hotData.resize(newSize);
    m_coldData.resize(newSize);

    for (size_t i = oldSize; i < newSize - 1; ++i) { m_metadata[i].nextFreeIndex = static_cast<uint32_t>(i) + 1; }
    m_metadata[newSize - 1].nextFreeIndex = LIST_END;
    m_freeListHead = static_cast<uint32_t>(oldSize);
  }

  public:
    AssetPool() = default;

    ~AssetPool() = default;

    AssetPool(const AssetPool&) = delete;

    AssetPool& operator=(const AssetPool&) = delete;

    AssetPool(AssetPool&&) noexcept = default;

    AssetPool& operator=(AssetPool&&) noexcept = default;

    Handle<Asset> create(const HotData& hot, const ColdData& cold)
    {
      if (m_freeListHead == LIST_END) { grow(); }
      const uint32_t index = m_freeListHead;
      m_freeListHead = m_metadata[index].nextFreeIndex;
      m_hotData[index] = hot;
      m_coldData[index] = cold;
      m_activeCount++;
      return Handle<Asset>(index + 1, m_metadata[index].generation);
    }

    Handle<Asset> create()
    {
      if (m_freeListHead == LIST_END) { grow(); }

      const uint32_t index = m_freeListHead;
      m_freeListHead = m_metadata[index].nextFreeIndex;
      m_hotData[index] = {};
      m_coldData[index] = {};

      m_activeCount++;
      return Handle<Asset>(index + 1, m_metadata[index].generation);
    }

    uint64_t handleToOpaque(Handle<Asset> handle) const { return handle.getOpaqueValue(); }

    Handle<Asset> opaqueToHandle(uint64_t opaqueValue) const
    {
      const uint32_t id = static_cast<uint32_t>(opaqueValue & 0xFFFFFFFF);
      const uint32_t generation = static_cast<uint32_t>(opaqueValue >> 32);

      if (id == 0)
        return {};

      const uint32_t index = id - 1;
      if (index >= m_metadata.size() || m_metadata[index].generation != generation)
      {
        return {}; // Invalid or stale handle
      }

      return Handle<Asset>(id, generation);
    }

    void destroy(Handle<Asset> handle)
    {
      if (!isValid(handle)) { return; }

      const uint32_t index = handle.id - 1;
      if (++m_metadata[index].generation == 0) { m_metadata[index].generation = 1; }

      m_metadata[index].nextFreeIndex = m_freeListHead;
      m_freeListHead = index;
      m_activeCount--;
    }

    void clear()
    {
      m_metadata.clear();
      m_hotData.clear();
      m_coldData.clear();
      m_freeListHead = LIST_END;
      m_activeCount = 0;
    }

    [[nodiscard]] bool isValid(Handle<Asset> handle) const
    {
      if (!handle.isValid())
        return false;
      const uint32_t index = handle.id - 1;
      return (index < m_metadata.size()) && (m_metadata[index].generation == handle.generation);
    }

    HotData* getHotData(Handle<Asset> handle)
    {
      if (!isValid(handle))
        return nullptr;
      return &m_hotData[handle.id - 1];
    }

    const HotData* getHotData(Handle<Asset> handle) const
    {
      if (!isValid(handle))
        return nullptr;
      return &m_hotData[handle.id - 1];
    }

    ColdData* getColdData(Handle<Asset> handle)
    {
      if (!isValid(handle))
        return nullptr;
      return &m_coldData[handle.id - 1];
    }

    const ColdData* getColdData(Handle<Asset> handle) const
    {
      if (!isValid(handle))
        return nullptr;
      return &m_coldData[handle.id - 1];
    }

    [[nodiscard]] uint32_t size() const { return m_activeCount; }

    [[nodiscard]] size_t capacity() const { return m_metadata.size(); }
};
