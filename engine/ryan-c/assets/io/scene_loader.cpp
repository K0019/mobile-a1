#include "scene_loader.h"
#include "assets/core/asset_system.h"
#include "assets/io/material_loader.h"
#include "assets/io/mesh_loader.h"
#include "assets/io/texture_loader.h"
#include "logging/log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <numeric>
#include <chrono>

namespace AssetLoading
{
  SceneLoader::SceneLoader(AssetSystem& assetSystem) : m_assetSystem(assetSystem)
  {}

  SceneLoadResult SceneLoader::loadScene(const std::filesystem::path& path, const LoadingConfig& config, ProgressCallback onProgress)
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
      for (const auto& source : textureSources)
      {
        processedTextures.push_back(TextureLoading::extractTexture(source, config));
      }

      // Step 5: Process meshes from Assimp data
      reportProgress(onProgress, 0.60f, "Processing meshes...");
      auto meshPtrs = MeshLoading::collectMeshPointers(scene, config);
      std::vector<ProcessedMesh> processedMeshes;
      processedMeshes.reserve(meshPtrs.size());
      for (uint32_t i = 0; i < meshPtrs.size(); ++i)
      {
        processedMeshes.push_back(MeshLoading::extractMesh(meshPtrs[i], i, config));
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

  std::unique_ptr<const aiScene, void(*)(const aiScene*)> SceneLoader::importScene(const std::filesystem::path& path) const
  {
    if (!exists(path))
    {
      LOG_ERROR("Scene file not found: {}", path.string());
      return {nullptr, [](const aiScene*) {}};
    }

    // Use a thread_local importer to ensure thread safety with Assimp
    thread_local Assimp::Importer importer;

    // Configure importer properties
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 1'000'000);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

    const uint32_t flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_ImproveCacheLocality | aiProcess_ValidateDataStructure;

    const aiScene* scene = importer.ReadFile(path.string(), flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
      std::string error = importer.GetErrorString();
      LOG_ERROR("Assimp import failed: {}", error.empty() ? "Unknown error" : error);
      return {nullptr, [](const aiScene*) {}};
    }

    // Return a unique_ptr that uses a no-op deleter, as Assimp's importer owns the memory.
    return {scene,
      [](const aiScene*)
      {
        /* Importer handles cleanup */
      }};
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
    light.attenuation = vec3(aiLight->mAttenuationConstant, aiLight->mAttenuationLinear, aiLight->mAttenuationQuadratic);

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

  void SceneLoader::extractObjectsFromNode(const aiNode* node, const mat4& parentTransform, const aiScene* scene, std::vector<SceneObject>& objects) const
  {
    if (!node)
      return;

    // Convert Assimp matrix to GLM matrix
    const aiMatrix4x4& aiMat = node->mTransformation;
    const mat4 nodeTransform(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
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

  void SceneLoader::resolveSceneObjectHandles(std::vector<SceneObject>& objects, const std::vector<MeshHandle>& meshHandles, const std::vector<MaterialHandle>& materialHandles) const
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
void SceneLoader::calculateBounds(Scene& scene, AssetSystem& assetSystem) const
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
        if (obj.type != SceneObjectType::Mesh || !obj.mesh.isValid()) 
            continue;

        // Get actual mesh bounds from asset system
        const auto* meshGPUData = assetSystem.getMesh(obj.mesh);
        if (!meshGPUData) 
        {
            LOG_WARNING("Could not get mesh data for object '{}'", obj.name);
            continue;
        }
        
        LOG_INFO("Processing object '{}' at position ({:.3f}, {:.3f}, {:.3f})", 
                 obj.name, obj.transform[3][0], obj.transform[3][1], obj.transform[3][2]);
        
        // Extract mesh bounds from GPU data (bounds = vec4(center.x, center.y, center.z, radius))
        const vec3 meshCenter = vec3(meshGPUData->bounds);
        const float meshRadius = meshGPUData->bounds.w;
        
        // Convert sphere bounds to box bounds for more accurate calculation
        vec3 meshMin = meshCenter - vec3(meshRadius);
        vec3 meshMax = meshCenter + vec3(meshRadius);
        
        LOG_INFO("  Mesh bounds: center=({:.3f}, {:.3f}, {:.3f}), radius={:.3f}",
                 meshCenter.x, meshCenter.y, meshCenter.z, meshRadius);
        LOG_INFO("  Mesh local bounds: min=({:.3f}, {:.3f}, {:.3f}), max=({:.3f}, {:.3f}, {:.3f})",
                 meshMin.x, meshMin.y, meshMin.z, meshMax.x, meshMax.y, meshMax.z);
        
        // Transform the 8 corners of the mesh bounding box
        vec3 corners[8] = {
            vec3(meshMin.x, meshMin.y, meshMin.z),
            vec3(meshMax.x, meshMin.y, meshMin.z),
            vec3(meshMin.x, meshMax.y, meshMin.z),
            vec3(meshMax.x, meshMax.y, meshMin.z),
            vec3(meshMin.x, meshMin.y, meshMax.z),
            vec3(meshMax.x, meshMin.y, meshMax.z),
            vec3(meshMin.x, meshMax.y, meshMax.z),
            vec3(meshMax.x, meshMax.y, meshMax.z)
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
                LOG_INFO("    Corner {}: local=({:.3f}, {:.3f}, {:.3f}) -> world=({:.3f}, {:.3f}, {:.3f})",
                         i, corners[i].x, corners[i].y, corners[i].z, worldPos.x, worldPos.y, worldPos.z);
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
             globalMinBounds.x, globalMinBounds.y, globalMinBounds.z, 
             globalMaxBounds.x, globalMaxBounds.y, globalMaxBounds.z);

    // Calculate original scene properties
    const vec3 originalCenter = (globalMinBounds + globalMaxBounds) * 0.5f;
    const vec3 originalSize = globalMaxBounds - globalMinBounds;
    const float originalRadius = glm::length(originalSize) * 0.5f;
    
    LOG_INFO("Original center: ({:.3f}, {:.3f}, {:.3f})", originalCenter.x, originalCenter.y, originalCenter.z);
    LOG_INFO("Original size: ({:.3f}, {:.3f}, {:.3f})", originalSize.x, originalSize.y, originalSize.z);
    LOG_INFO("Original radius: {:.3f}", originalRadius);
    
    // Calculate scaling to fit within camera view range
    // Camera is at z = -1.5f looking towards origin, with near=0.1f, far=100.0f
    const float maxAllowedRadius = 5.0f; // Keep scene at reasonable size for initial viewing
    const float scaleFactor = originalRadius > 0.0f ? (maxAllowedRadius / originalRadius) : 1.0f;
    
    LOG_INFO("Max allowed radius: {:.3f}", maxAllowedRadius);
    LOG_INFO("Scale factor: {:.6f}", scaleFactor);
    
    // Calculate transforms: first center, then scale
    const vec3 centeringOffset = -originalCenter;
    const mat4 centeringTransform = glm::translate(mat4(1.0f), centeringOffset);
    const mat4 scalingTransform = glm::scale(mat4(1.0f), vec3(scaleFactor));
    const mat4 combinedTransform = scalingTransform * centeringTransform;

    LOG_INFO("Centering offset: ({:.3f}, {:.3f}, {:.3f})", centeringOffset.x, centeringOffset.y, centeringOffset.z);

    // Apply transformation to all mesh objects
    for (auto& obj : scene.objects)
    {
        if (obj.type == SceneObjectType::Mesh)
        {
            const vec3 oldPos = vec3(obj.transform[3]);
            obj.transform = combinedTransform * obj.transform;
            const vec3 newPos = vec3(obj.transform[3]);
            
            LOG_INFO("  Object '{}': ({:.3f}, {:.3f}, {:.3f}) -> ({:.3f}, {:.3f}, {:.3f})", 
                     obj.name, oldPos.x, oldPos.y, oldPos.z, newPos.x, newPos.y, newPos.z);
        }
    }

    // Update scene bounds after transformation
    const vec3 finalSize = originalSize * scaleFactor;
    scene.boundingMin = -finalSize * 0.5f;
    scene.boundingMax = finalSize * 0.5f;
    scene.center = vec3(0.0f); // Centered at origin
    scene.radius = originalRadius * scaleFactor;
    
    LOG_INFO("Final bounds: min=({:.3f}, {:.3f}, {:.3f}), max=({:.3f}, {:.3f}, {:.3f})", 
             scene.boundingMin.x, scene.boundingMin.y, scene.boundingMin.z, 
             scene.boundingMax.x, scene.boundingMax.y, scene.boundingMax.z);
    LOG_INFO("Final center: ({:.3f}, {:.3f}, {:.3f})", scene.center.x, scene.center.y, scene.center.z);
    LOG_INFO("Final radius: {:.3f}", scene.radius);
    
    if (scaleFactor != 1.0f)
    {
        LOG_INFO("Scene '{}' scaled by factor {:.3f} to fit view range (original radius: {:.2f}, new radius: {:.2f})", 
                 scene.name, scaleFactor, originalRadius, scene.radius);
    }
    
    LOG_INFO("=== End Scene Bounds Debug ===");
}
  bool SceneLoader::isValidScene(const aiScene* scene) const
  {
    return scene && scene->mRootNode && !(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE);
  }
} // namespace AssetLoading
