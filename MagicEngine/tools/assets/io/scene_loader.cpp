#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "scene_loader.h"
#include "assets/io/material_loader.h"
#include "assets/io/mesh_loader.h"
#include "assets/io/texture_loader.h"
#include "resource/morph_set.h"
#include "resource/resource_manager.h"
#include "resource/skeleton.h"
#include "logging/log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <chrono>
#include <limits>
#include <numeric>
#include <unordered_map>
#include "math/utils_math.h"

namespace
{
  using namespace Resource;

  inline vec3 toVec3(const aiVector3D& v)
  {
    return vec3(v.x, v.y, v.z);
  }

  inline glm::quat toQuat(const aiQuaternion& q)
  {
    return glm::quat(q.w, q.x, q.y, q.z);
  }

  inline float ticksToSeconds(double time, double ticksPerSecond)
  {
    if (ticksPerSecond <= 0.0) return static_cast<float>(time);
    return static_cast<float>(time / ticksPerSecond);
  }

  ProcessedAnimationChannel extractNodeChannel(const aiNodeAnim* channel, double ticksPerSecond)
  {
    ProcessedAnimationChannel result;
    result.nodeName = channel->mNodeName.C_Str();
    result.translationKeys.reserve(channel->mNumPositionKeys);
    for (uint32_t i = 0; i < channel->mNumPositionKeys; ++i)
    {
      const auto& key = channel->mPositionKeys[i];
      result.translationKeys.push_back({ticksToSeconds(key.mTime, ticksPerSecond), toVec3(key.mValue)});
    }
    result.rotationKeys.reserve(channel->mNumRotationKeys);
    for (uint32_t i = 0; i < channel->mNumRotationKeys; ++i)
    {
      const auto& key = channel->mRotationKeys[i];
      result.rotationKeys.push_back({ticksToSeconds(key.mTime, ticksPerSecond), toQuat(key.mValue)});
    }
    result.scaleKeys.reserve(channel->mNumScalingKeys);
    for (uint32_t i = 0; i < channel->mNumScalingKeys; ++i)
    {
      const auto& key = channel->mScalingKeys[i];
      result.scaleKeys.push_back({ticksToSeconds(key.mTime, ticksPerSecond), toVec3(key.mValue)});
    }
    return result;
  }

  ProcessedMorphChannel extractMorphChannel(const aiMeshMorphAnim* channel, double ticksPerSecond)
  {
    ProcessedMorphChannel result;
    result.meshName = channel->mName.C_Str();
    result.keys.reserve(channel->mNumKeys);
    for (uint32_t i = 0; i < channel->mNumKeys; ++i)
    {
      const aiMeshMorphKey& key = channel->mKeys[i];
      ProcessedMorphKey morphKey;
      morphKey.time = ticksToSeconds(key.mTime, ticksPerSecond);
      morphKey.targetIndices.assign(key.mValues, key.mValues + key.mNumValuesAndWeights);
      morphKey.weights.reserve(key.mNumValuesAndWeights);
      for (uint32_t w = 0; w < key.mNumValuesAndWeights; ++w)
      {
        morphKey.weights.push_back(static_cast<float>(key.mWeights[w]));
      }
      result.keys.push_back(std::move(morphKey));
    }
    return result;
  }

  ProcessedAnimationClip extractAnimationClip(const aiAnimation* animation)
  {
    ProcessedAnimationClip clip;
    clip.name = animation->mName.length > 0 ? animation->mName.C_Str() : "Animation";
    clip.ticksPerSecond = animation->mTicksPerSecond > 0.0 ? static_cast<float>(animation->mTicksPerSecond) : 1.0f;
    clip.duration = animation->mDuration > 0.0 ? static_cast<float>(animation->mDuration / clip.ticksPerSecond) : 0.0f;
    clip.skeletalChannels.reserve(animation->mNumChannels);
    for (uint32_t i = 0; i < animation->mNumChannels; ++i)
    {
      clip.skeletalChannels.push_back(extractNodeChannel(animation->mChannels[i], clip.ticksPerSecond));
    }
    clip.morphChannels.reserve(animation->mNumMorphMeshChannels);
    for (uint32_t i = 0; i < animation->mNumMorphMeshChannels; ++i)
    {
      clip.morphChannels.push_back(extractMorphChannel(animation->mMorphMeshChannels[i], clip.ticksPerSecond));
    }
    return clip;
  }
} // namespace
namespace Resource
{
  SceneLoader::SceneLoader(ResourceManager& assetSystem) : m_assetSystem(assetSystem)
  {
  }

  SceneLoadResult SceneLoader::loadScene(const std::filesystem::path& path, const LoadingConfig& config,
                                         ProgressCallback onProgress)
  {
    auto startTime = std::chrono::steady_clock::now();
    SceneLoadResult result;
    result.scene.name = path.filename().string();
    try
    {
      // Step 1: Import scene file from disk
      reportProgress(onProgress, 0.05f, "Importing scene file...");
      auto assimpScenePtr = importScene(path);
      if (!assimpScenePtr)
      {
        result.success = false;
        result.warnings.push_back("Failed to import scene file: " + path.string());
        return result;
      }
      const aiScene* scene = assimpScenePtr.get();
      const std::string basePath = path.string();
      const std::string baseDir = path.parent_path().string();
      // Step 2: Extract scene hierarchy (lights, cameras, objects)
      reportProgress(onProgress, 0.15f, "Extracting scene hierarchy...");
      for (uint32_t i = 0; i < scene->mNumLights; ++i)
      {
        result.scene.lights.push_back(extractSingleLight(scene->mLights[i], i));
      }
      for (uint32_t i = 0; i < scene->mNumCameras; ++i)
      {
        result.scene.cameras.push_back(extractSingleCamera(scene->mCameras[i], i));
      }
      if (scene->mRootNode)
      {
        extractObjectsFromNode(scene->mRootNode, mat4(1.0f), scene, result.scene.objects);
      }
      // Step 3: Process materials from Assimp data
      reportProgress(onProgress, 0.30f, "Processing materials...");
      auto materialPtrs = MaterialLoading::collectMaterialPointers(scene, config);
      std::vector<ProcessedMaterial> processedMaterials;
      processedMaterials.reserve(materialPtrs.size());
      for (uint32_t i = 0; i < materialPtrs.size(); ++i)
      {
        processedMaterials.push_back(MaterialLoading::extractMaterial(materialPtrs[i], i, basePath, baseDir, scene));
      }
      // Step 4: Process textures (dependent on materials)
      reportProgress(onProgress, 0.45f, "Processing textures...");
      auto textureSources = TextureLoading::collectUniqueTextures(processedMaterials);
      std::vector<ProcessedTexture> processedTextures;
      processedTextures.reserve(textureSources.size());
      for (const auto& [source, isSRGB] : textureSources)
      {
        processedTextures.push_back(TextureLoading::extractTexture(source, config, isSRGB));
      }
      // Step 5: Process meshes from Assimp data
      reportProgress(onProgress, 0.60f, "Processing meshes...");
      auto meshPtrs = MeshLoading::collectMeshPointers(scene, config);
      std::vector<ProcessedMesh> processedMeshes;
      processedMeshes.reserve(meshPtrs.size());
      for (uint32_t i = 0; i < meshPtrs.size(); ++i)
      {
        processedMeshes.push_back(MeshLoading::extractMesh(scene, meshPtrs[i], i, config));
      }
      std::unordered_map<std::string, uint32_t> meshNameToIndex;
      for (uint32_t i = 0; i < processedMeshes.size(); ++i)
      {
        meshNameToIndex[processedMeshes[i].name] = i;
      }

      result.scene.animationClips.reserve(scene->mNumAnimations);
      for(uint32_t animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex)
      {
        ProcessedAnimationClip clip = extractAnimationClip(scene->mAnimations[animIndex]);
        for (auto& morphChannel : clip.morphChannels)
        {
          if (auto it = meshNameToIndex.find(morphChannel.meshName); it != meshNameToIndex.end())
          {
            morphChannel.meshIndex = it->second;
          }
        }
        ClipId clipId = m_assetSystem.createClip(clip);
        result.scene.animationClips.push_back(clipId);
      }
      // Step 6: Upload assets to the GPU
      reportProgress(onProgress, 0.75f, "Uploading assets to GPU...");
      std::vector<TextureHandle> textureHandles;
      textureHandles.reserve(processedTextures.size());
      for (const auto& texture : processedTextures)
      {
        textureHandles.push_back(m_assetSystem.createTexture(texture));
      }
      std::vector<MaterialHandle> materialHandles;
      materialHandles.reserve(processedMaterials.size());
      for (const auto& material : processedMaterials)
      {
        materialHandles.push_back(m_assetSystem.createMaterial(material));
      }
      std::vector<MeshHandle> meshHandles;
      meshHandles.reserve(processedMeshes.size());
      for (const auto& mesh : processedMeshes)
      {
        meshHandles.push_back(m_assetSystem.createMesh(mesh));
      }
      // Step 7: Finalize scene
      reportProgress(onProgress, 0.90f, "Finalizing scene...");
      resolveSceneObjectHandles(result.scene.objects, meshHandles, materialHandles);
      for (auto& object : result.scene.objects)
      {
        if (object.type != SceneObjectType::Mesh || !object.mesh.isValid()) continue;
        const auto* meshMetadata = m_assetSystem.getMeshMetadata(object.mesh);
        if (!meshMetadata) continue;
        const bool hasSkeleton = meshMetadata->skeletonId != INVALID_SKELETON_ID;
        const bool hasMorphs = meshMetadata->morphSetId != INVALID_MORPH_SET_ID;
        if (!hasSkeleton && !hasMorphs) continue;
        SceneObject::AnimBinding binding;
        binding.skeleton = meshMetadata->skeletonId;
        binding.morphSet = meshMetadata->morphSetId;
        binding.clipA = INVALID_CLIP_ID;
        binding.clipB = INVALID_CLIP_ID;
        binding.speed = 1.0f;
        binding.blend = 0.0f;
        binding.timeA = 0.0f;
        binding.timeB = 0.0f;
        binding.flags = SceneObject::AnimBinding::Playing | SceneObject::AnimBinding::Loop;
        if (hasSkeleton)
        {
          const Skeleton& skeleton = m_assetSystem.Skeleton(meshMetadata->skeletonId);
          const uint32_t jointCount = skeleton.jointCount();
          binding.jointCount = static_cast<uint16_t>(std::min<uint32_t>(
            jointCount, std::numeric_limits<uint16_t>::max()));
          binding.skinMatrices.assign(binding.jointCount, glm::mat4(1.0f));
        }
        if (hasMorphs)
        {
          const MorphSet& morphSet = m_assetSystem.Morph(meshMetadata->morphSetId);
          const uint32_t morphCount = morphSet.count();
          binding.morphCount = static_cast<uint16_t>(std::min<uint32_t>(
            morphCount, std::numeric_limits<uint16_t>::max()));
          binding.morphWeights.assign(binding.morphCount, 0.0f);
        }
        object.anim = std::move(binding);
      }
      calculateBounds(result.scene, m_assetSystem);
      // Final stats
      auto endTime = std::chrono::steady_clock::now();
      result.stats.totalTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
      result.stats.meshesLoaded = meshHandles.size();
      result.stats.materialsLoaded = materialHandles.size();
      result.stats.texturesLoaded = textureHandles.size();
      result.success = true;
      reportProgress(onProgress, 1.0f, "Load complete");
      m_assetSystem.FlushUploads();
    }
    catch (const std::exception& e)
    {
      result.success = false;
      result.warnings.push_back("An exception occurred: " + std::string(e.what()));
      LOG_ERROR("Scene loading exception: {}", e.what());
    }
    return result;
  }

  void SceneLoader::reportProgress(const ProgressCallback& callback, float progress, const std::string& status)
  {
    if (callback)
    {
      callback(progress, status);
    }
  }

  std::unique_ptr<const aiScene, void(*)(const aiScene*)> SceneLoader::importScene(
    const std::filesystem::path& path) const
  {
    if (!exists(path))
    {
      LOG_ERROR("Scene file not found: {}", path.string());
      return {
        nullptr, [](const aiScene*)
        {
        }
      };
    }
    // Use a thread_local importer to ensure thread safety with Assimp
    thread_local Assimp::Importer importer;
    // Configure importer properties
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 1'000'000);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

    // Base flags for all formats
    // CalcTangentSpace: compute tangents/bitangents from UVs (essential for normal mapping)
    uint32_t flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals |
      aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_ValidateDataStructure;

    // Check file extension for format-specific flags
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    bool isFbx = (ext == ".fbx");

    // SortByPType only for FBX (can cause issues with glTF)
    if (isFbx)
    {
      flags |= aiProcess_SortByPType;
    }

    // FixInfacingNormals: DISABLED by default - can cause inconsistent normals
    // Uncomment the next line to test WITH FixInfacingNormals:
    // flags |= aiProcess_FixInfacingNormals;

    LOG_INFO("[SceneLoader] Importing {} with flags: 0x{:X} (FBX={}, FixInfacingNormals=OFF)",
             path.filename().string(), flags, isFbx);

    const aiScene* scene = importer.ReadFile(path.string(), flags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      std::string error = importer.GetErrorString();
      LOG_ERROR("Assimp import failed: {}", error.empty() ? "Unknown error" : error);
      return {
        nullptr, [](const aiScene*)
        {
        }
      };
    }
    // Return a unique_ptr that uses a no-op deleter, as Assimp's importer owns the memory.
    return {
      scene, [](const aiScene*)
      {
        /* Importer handles cleanup */
      }
    };
  }

  SceneLight SceneLoader::extractSingleLight(const aiLight* aiLight, uint32_t index) const
  {
    SceneLight light;
    light.name = aiLight->mName.length > 0 ? aiLight->mName.C_Str() : ("Light_" + std::to_string(index));
    light.position = vec3(aiLight->mPosition.x, aiLight->mPosition.y, aiLight->mPosition.z);
    light.direction = vec3(aiLight->mDirection.x, aiLight->mDirection.y, aiLight->mDirection.z);
    light.color = vec3(aiLight->mColorDiffuse.r, aiLight->mColorDiffuse.g, aiLight->mColorDiffuse.b);
    switch (aiLight->mType)
    {
    case aiLightSource_DIRECTIONAL:
      light.type = LightType::Directional;
      break;
    case aiLightSource_POINT:
      light.type = LightType::Point;
      break;
    case aiLightSource_SPOT:
      light.type = LightType::Spot;
      break;
    case aiLightSource_AREA:
      light.type = LightType::Area;
      break;
    default:
      light.type = LightType::Point;
      break;
    }
    light.intensity = 1.0f; // Default intensity
    light.attenuation = vec3(aiLight->mAttenuationConstant, aiLight->mAttenuationLinear,
                             aiLight->mAttenuationQuadratic);
    if (light.type == LightType::Spot)
    {
      light.innerConeAngle = aiLight->mAngleInnerCone;
      light.outerConeAngle = aiLight->mAngleOuterCone;
    }
    return light;
  }

  SceneCamera SceneLoader::extractSingleCamera(const aiCamera* aiCamera, uint32_t index) const
  {
    SceneCamera camera;
    camera.name = aiCamera->mName.length > 0 ? aiCamera->mName.C_Str() : ("Camera_" + std::to_string(index));
    camera.position = vec3(aiCamera->mPosition.x, aiCamera->mPosition.y, aiCamera->mPosition.z);
    camera.up = vec3(aiCamera->mUp.x, aiCamera->mUp.y, aiCamera->mUp.z);
    const vec3 lookDirection = vec3(aiCamera->mLookAt.x, aiCamera->mLookAt.y, aiCamera->mLookAt.z);
    camera.target = camera.position + lookDirection;
    camera.fov = glm::degrees(aiCamera->mHorizontalFOV);
    camera.nearPlane = aiCamera->mClipPlaneNear;
    camera.farPlane = aiCamera->mClipPlaneFar;
    camera.aspectRatio = aiCamera->mAspect > 0 ? aiCamera->mAspect : (16.0f / 9.0f);
    // Clamp values to a reasonable range
    camera.fov = glm::clamp(camera.fov, 1.0f, 179.0f);
    camera.nearPlane = std::max(0.001f, camera.nearPlane);
    camera.farPlane = std::max(camera.nearPlane + 0.1f, camera.farPlane);
    return camera;
  }

  void SceneLoader::extractObjectsFromNode(const aiNode* node, const mat4& parentTransform, const aiScene* scene,
                                           std::vector<SceneObject>& objects) const
  {
    if (!node) return;
    // Convert Assimp matrix to GLM matrix
    const aiMatrix4x4& aiMat = node->mTransformation;
    const mat4 nodeTransform(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3,
                             aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
    const mat4 globalTransform = parentTransform * nodeTransform;
    // Create scene objects for each mesh in this node
    for (uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
      const uint32_t meshIndex = node->mMeshes[i];
      if (meshIndex < scene->mNumMeshes)
      {
        SceneObject object;
        object.type = SceneObjectType::Mesh;
        object.transform = globalTransform;
        object.name = node->mName.length > 0 ? node->mName.C_Str() : ("Object_" + std::to_string(objects.size()));
        // Store original indices for later resolution
        object.meshIndex = meshIndex;
        const aiMesh* aiMesh = scene->mMeshes[meshIndex];
        object.materialIndex = aiMesh->mMaterialIndex;
        objects.push_back(object);
      }
    }
    // Recursively process child nodes
    for (uint32_t i = 0; i < node->mNumChildren; ++i)
    {
      extractObjectsFromNode(node->mChildren[i], globalTransform, scene, objects);
    }
  }

  void SceneLoader::resolveSceneObjectHandles(std::vector<SceneObject>& objects,
                                              const std::vector<MeshHandle>& meshHandles,
                                              const std::vector<MaterialHandle>& materialHandles) const
  {
    for (auto& obj : objects)
    {
      if (obj.type == SceneObjectType::Mesh)
      {
        // Resolve mesh handle from original index
        if (obj.meshIndex < meshHandles.size())
        {
          obj.mesh = meshHandles[obj.meshIndex];
        }
        else
        {
          LOG_WARNING("Invalid mesh index {} for object '{}'", obj.meshIndex, obj.name);
        }
        // Resolve material handle from original index
        if (obj.materialIndex < materialHandles.size())
        {
          obj.material = materialHandles[obj.materialIndex];
        }
        else if (!materialHandles.empty())
        {
          obj.material = materialHandles[0]; // Fallback to a default material
        }
        // Clear temporary indices
        obj.meshIndex = UINT32_MAX;
        obj.materialIndex = UINT32_MAX;
      }
    }
  }

  // Modified calculateBounds method with extensive debugging
  void SceneLoader::calculateBounds(Scene& scene, ResourceManager& assetSystem) const
  {
    LOG_INFO("=== Scene Bounds Calculation Debug ===");
    LOG_INFO("Scene name: '{}'", scene.name);
    LOG_INFO("Number of objects: {}", scene.objects.size());
    if (scene.objects.empty())
    {
      LOG_WARNING("Scene is empty, setting default bounds");
      scene.center = vec3(0);
      scene.radius = 0;
      return;
    }
    vec3 globalMinBounds(std::numeric_limits<float>::max());
    vec3 globalMaxBounds(std::numeric_limits<float>::lowest());
    bool foundAnyGeometry = false;
    // Calculate bounds by transforming actual mesh vertices
    for (const auto& obj : scene.objects)
    {
      if (obj.type != SceneObjectType::Mesh || !obj.mesh.isValid()) continue;
      // Get actual mesh bounds from asset system
      const auto* meshGPUData = assetSystem.getMesh(obj.mesh);
      if (!meshGPUData)
      {
        LOG_WARNING("Could not get mesh data for object '{}'", obj.name);
        continue;
      }
      LOG_INFO("Processing object '{}' at position ({:.3f}, {:.3f}, {:.3f})", obj.name, obj.transform[3][0],
               obj.transform[3][1], obj.transform[3][2]);
      // Extract mesh bounds from GPU data (bounds = vec4(center.x, center.y, center.z, radius))
      const vec3 meshCenter = vec3(meshGPUData->bounds);
      const float meshRadius = meshGPUData->bounds.w;
      // Convert sphere bounds to box bounds for more accurate calculation
      vec3 meshMin = meshCenter - vec3(meshRadius);
      vec3 meshMax = meshCenter + vec3(meshRadius);
      LOG_INFO("  Mesh bounds: center=({:.3f}, {:.3f}, {:.3f}), radius={:.3f}", meshCenter.x, meshCenter.y,
               meshCenter.z, meshRadius);
      LOG_INFO("  Mesh local bounds: min=({:.3f}, {:.3f}, {:.3f}), max=({:.3f}, {:.3f}, {:.3f})", meshMin.x, meshMin.y,
               meshMin.z, meshMax.x, meshMax.y, meshMax.z);
      // Transform the 8 corners of the mesh bounding box
      vec3 corners[8] = {
        vec3(meshMin.x, meshMin.y, meshMin.z), vec3(meshMax.x, meshMin.y, meshMin.z),
        vec3(meshMin.x, meshMax.y, meshMin.z), vec3(meshMax.x, meshMax.y, meshMin.z),
        vec3(meshMin.x, meshMin.y, meshMax.z), vec3(meshMax.x, meshMin.y, meshMax.z),
        vec3(meshMin.x, meshMax.y, meshMax.z), vec3(meshMax.x, meshMax.y, meshMax.z)
      };
      // Transform each corner and expand global bounds
      for (int i = 0; i < 8; ++i)
      {
        vec4 worldCorner = obj.transform * vec4(corners[i], 1.0f);
        vec3 worldPos = vec3(worldCorner);
        globalMinBounds = glm::min(globalMinBounds, worldPos);
        globalMaxBounds = glm::max(globalMaxBounds, worldPos);
        if (i < 2) // Log first couple corners for debugging
        {
          LOG_INFO("    Corner {}: local=({:.3f}, {:.3f}, {:.3f}) -> world=({:.3f}, {:.3f}, {:.3f})", i, corners[i].x,
                   corners[i].y, corners[i].z, worldPos.x, worldPos.y, worldPos.z);
        }
      }
      foundAnyGeometry = true;
    }
    if (!foundAnyGeometry)
    {
      LOG_WARNING("No valid mesh geometry found");
      scene.center = vec3(0);
      scene.radius = 0;
      return;
    }
    // Debug original bounds
    LOG_INFO("Global bounds before transform: min=({:.3f}, {:.3f}, {:.3f}), max=({:.3f}, {:.3f}, {:.3f})",
             globalMinBounds.x, globalMinBounds.y, globalMinBounds.z, globalMaxBounds.x, globalMaxBounds.y,
             globalMaxBounds.z);
    LOG_INFO("=== End Scene Bounds Debug ===");
  }

  bool SceneLoader::isValidScene(const aiScene* scene) const
  {
    return scene && scene->mRootNode && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE);
  }
} // namespace AssetLoading
