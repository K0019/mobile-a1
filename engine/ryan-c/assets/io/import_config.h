#pragma once
#include <cstdint>

namespace AssetLoading
{
  struct LoadingConfig
  {
    // Performance settings
    float maxFrameTimeMs = 2.0f;
    bool enableParallelLoading = true;
    size_t jobSystemThreads = 0; // 0 = auto-detect

    // Mesh settings
    bool optimizeMeshes = true;
    bool autoDetectTangentNeed = true;
    bool generateTangents = true;
    bool flipUVs = true;
    uint32_t maxVerticesPerMesh = 1'000'000;

    // Texture settings
    bool compressTextures = true;
    uint32_t maxTextureResolution = 4096;
    bool generateMipmaps = true;

    // Material settings
    bool deduplicateMaterials = false;
    float deduplicationTolerance = 0.001f;

    // Scene settings
    bool extractLights = true;
    bool extractCameras = true;
    bool calculateBounds = true;

    // Memory limits
    uint32_t maxMeshes = 1000;
    uint32_t maxMaterials = 500;
    uint32_t maxTextures = 500;

    // Factory methods for common configs
    static LoadingConfig createFast()
    {
      LoadingConfig config;
      config.optimizeMeshes = false;
      config.compressTextures = false;
      config.deduplicateMaterials = false;
      config.extractLights = false;
      config.extractCameras = false;
      config.maxFrameTimeMs = 5.0f;
      return config;
    }

    static LoadingConfig createBalanced()
    {
      return LoadingConfig{}; // Use defaults
    }

    static LoadingConfig createQuality()
    {
      LoadingConfig config;
      config.generateTangents = true;
      config.maxFrameTimeMs = 1.0f;
      config.maxTextureResolution = 8192;
      return config;
    }
  };
} // namespace AssetLoading
