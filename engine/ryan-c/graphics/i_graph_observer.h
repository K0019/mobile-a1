// graphics/interface/ITransientResourceObserver.h
#pragma once

#include <unordered_map>
#include <string_view>
#include <cstdint>

namespace Render::internal
{
  /**
   * Pure observer interface for transient resource updates.
   * The engine notifies observers when transient resources are resolved to bindless indices.
  */
  class ITransientResourceObserver
  {
    public:
      virtual ~ITransientResourceObserver() = default;

      /**
       * Called after render graph compilation when transient resources are resolved.
       * @param bindlessMap - Map of logical resource names to their bindless indices
       */
      virtual void OnTransientResourcesUpdated(const std::unordered_map<std::string_view, uint32_t>& bindlessMap) = 0;
  };
} // namespace internal
