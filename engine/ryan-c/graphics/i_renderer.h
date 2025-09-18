#pragma once
#include <cstdint>
#include <functional>
#include <memory>

#include "render_feature.h"

class IRenderer
{
  public:
    virtual ~IRenderer() = default;

    template <typename TFeature>
    uint64_t CreateFeature() { return DoCreateFeature([] { return std::make_unique<TFeature>(); }); }

    virtual void DestroyFeature(uint64_t feature_handle) = 0;

    virtual void* GetFeatureParameterBlockPtr(uint64_t feature_handle) = 0;

  private:
    virtual uint64_t DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory) = 0;
};
