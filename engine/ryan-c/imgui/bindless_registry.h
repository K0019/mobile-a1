#pragma once

#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>
#include <atomic>
#include <shared_mutex>
#include <graphics/i_graph_observer.h>

namespace editor
{
  class TransientRegistry : public internal::ITransientResourceObserver
  {
    public:
      TransientRegistry() = default;

      ~TransientRegistry() override = default;

      TransientRegistry(const TransientRegistry&) = delete;

      TransientRegistry& operator=(const TransientRegistry&) = delete;

      std::optional<uint32_t> QueryBindlessID(std::string_view logicalName) const;

      bool IsReady() const;

      std::vector<std::string> GetAvailableResources() const;

      size_t GetResourceCount() const;

      void OnTransientResourcesUpdated(const std::unordered_map<std::string_view, uint32_t>& bindlessMap) override;

    private:
      mutable std::shared_mutex m_mutex;
      std::unordered_map<std::string, uint32_t> m_resolvedTransients;
      std::atomic<bool> m_hasData{false};
      std::atomic<uint64_t> m_updateCounter{0};
  };
} // namespace editor
