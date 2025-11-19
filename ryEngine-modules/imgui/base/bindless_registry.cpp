#include "bindless_registry.h"
#include <algorithm>
#include <iterator>
#include <logging/log.h>

namespace editor
{
  std::optional<uint32_t> TransientRegistry::QueryBindlessID(std::string_view logicalName) const
  {
    std::shared_lock lock(m_mutex);
    if (auto it = m_resolvedTransients.find(std::string(logicalName)); it != m_resolvedTransients.end())
    {
      return it->second;
    }
    return std::nullopt;
  }

  bool TransientRegistry::IsReady() const
  {
    return m_hasData.load(std::memory_order_acquire);
  }

  std::vector<std::string> TransientRegistry::GetAvailableResources() const
  {
    const std::shared_lock lock(m_mutex);
    std::vector<std::string> resources;
    resources.reserve(m_resolvedTransients.size());
    std::transform(m_resolvedTransients.begin(), m_resolvedTransients.end(), std::back_inserter(resources),
                   [](const auto& pair) { return pair.first; });
    std::sort(resources.begin(), resources.end());
    return resources;
  }

  size_t TransientRegistry::GetResourceCount() const
  {
    std::shared_lock lock(m_mutex);
    return m_resolvedTransients.size();
  }

  void TransientRegistry::OnTransientResourcesUpdated(const std::unordered_map<std::string_view, uint32_t>& bindlessMap)
  {
    {
      std::unique_lock lock(m_mutex);
      m_resolvedTransients.clear();
      m_resolvedTransients.reserve(bindlessMap.size());
      for (const auto& [name, bindlessId] : bindlessMap)
      {
        m_resolvedTransients.emplace(std::string(name), bindlessId);
      }
    }
    m_hasData.store(!bindlessMap.empty(), std::memory_order_release);
    m_updateCounter.fetch_add(1, std::memory_order_relaxed);
#ifdef DEBUG
    if constexpr (true)
    {
      LOG_DEBUG("TransientTextureRegistry updated with {} resources", bindlessMap.size());
      for (const auto& [name, id] : bindlessMap)
      {
        LOG_DEBUG("  {} -> bindless ID {}", name, id);
      }
    }
#endif
  }
} // namespace editor
