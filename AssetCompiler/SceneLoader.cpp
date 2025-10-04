#include "SceneLoader.h"
#include "MeshLoader.h"
#include "MaterialLoader.h"

#include <assimp/material.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <numeric>
#include <chrono>

namespace compiler
{

    void extractTexturePath(const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath)
    {
        aiString path;
        if (aiMat->GetTexture(type, 0, &path) == AI_SUCCESS)
        {
            std::filesystem::path texturePath(path.C_Str());
            if (texturePath.is_relative())
            {
                outSlot.texturePaths[key] = modelBasePath / texturePath;
            }
            else
            {
                outSlot.texturePaths[key] = texturePath;
            }
        }
    }

    std::vector<const aiMesh*> SceneLoader::collectMeshPointers(const aiScene* scene, const MeshOptions& options)
    {
        if (!scene || !scene->mMeshes || scene->mNumMeshes == 0) return {};

        std::vector<const aiMesh*> meshPtrs;
        meshPtrs.reserve(scene->mNumMeshes);

        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            if (scene->mMeshes[i] && scene->mMeshes[i]->mNumVertices > 0)
            {
                meshPtrs.push_back(scene->mMeshes[i]);
            }
        }
        return meshPtrs;
    }

    void SceneLoader::extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const MeshOptions& options)
    {
        vertices.resize(aiMesh->mNumVertices);
        for (uint32_t i = 0; i < aiMesh->mNumVertices; ++i)
        {
            Vertex& vertex = vertices[i];

            // Position (always present)
            vertex.position = vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);

            // Normal
            if (aiMesh->mNormals)
            {
                vertex.normal = vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
            }
            else
            {
                vertex.normal = vec3(0.0f, 1.0f, 0.0f);
            }

            if (aiMesh->mTextureCoords[0])
            {
                const float u = aiMesh->mTextureCoords[0][i].x;
                const float v = aiMesh->mTextureCoords[0][i].y;
                vertex.setUV(u, options.flipUVs ? (1.0f - v) : v);
            }
            else
            {
                vertex.setUV(0.0f, 0.0f);
            }

            // Initialize tangent
            vertex.tangent = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    ProcessedMesh SceneLoader::extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const MeshOptions& options)
    {
        ProcessedMesh mesh;

        // 1. Extract name and material index
        mesh.name = aiMesh->mName.length > 0 ? aiMesh->mName.C_Str() : ("Mesh_" + std::to_string(meshIndex));
        mesh.materialIndex = aiMesh->mMaterialIndex;

        if (!aiMesh->mVertices || aiMesh->mNumVertices == 0)
        {
            return mesh; // Return empty mesh
        }

        extractVertices(aiMesh, mesh.vertices, options);

        mesh.indices.reserve(aiMesh->mNumFaces * 3);
        for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
        {
            const aiFace& face = aiMesh->mFaces[i];
            if (face.mNumIndices == 3)
            {
                mesh.indices.push_back(face.mIndices[0]);
                mesh.indices.push_back(face.mIndices[1]);
                mesh.indices.push_back(face.mIndices[2]);
            }
        }
        return mesh;
    }



    SceneLoadData SceneLoader::loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions)
    {
        SceneLoadData returnData;

        try
        {
            // Import scene file from disk using Assimp.
            auto assimpScenePtr = importScene(path);
            if (!assimpScenePtr)
            {
                returnData.errors.push_back("Failed to import scene file with Assimp: " + path.string());
                return returnData;
            }

            const aiScene* scene = assimpScenePtr.get();
            const auto baseDir = path.parent_path();

            // We incrementally build up the scene object
            // Cotnains: nodes, meshes, materials
            Scene loadedScene;
            loadedScene.name = path.filename().string();

            // Extract scene hierarchy (nodes).
            if (scene->mRootNode)
            {
                extractNodesForCompiler(scene->mRootNode, -1, scene, loadedScene.nodes);
            }

            // Process materials
            auto materialPtrs = collectMaterialPointers(scene);
            loadedScene.materials.reserve(materialPtrs.size());
            for (uint32_t i = 0; i < materialPtrs.size(); ++i)
            {
                // passing the base directory to correctly resolve relative texture paths
                loadedScene.materials.push_back(
                    extractMaterialSlot(materialPtrs[i], i, baseDir)
                );
            }

            // Process meshes
            auto meshPtrs = collectMeshPointers(scene, meshOptions);
            loadedScene.meshes.reserve(meshPtrs.size());
            for (uint32_t i = 0; i < meshPtrs.size(); ++i)
            {
                loadedScene.meshes.push_back(extractMesh(meshPtrs[i], i, meshOptions));
            }

            returnData.scene = std::move(loadedScene);
        }
        catch (const std::exception& e)
        {
            returnData.errors.push_back("An exception occurred during scene loading: " + std::string(e.what()));
        }

        return returnData;
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
            //LOG_ERROR("Scene file not found: {}", path.string());
            return { nullptr, [](const aiScene*) {} };
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
            //LOG_ERROR("Assimp import failed: {}", error.empty() ? "Unknown error" : error);
            return { nullptr, [](const aiScene*) {} };
        }

        // Return a unique_ptr that uses a no-op deleter, as Assimp's importer owns the memory.
        return { scene,
            [](const aiScene*)
            {
                /* Importer handles cleanup */
                } };
    }




    ProcessedMaterialSlot SceneLoader::extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath)
    {
        ProcessedMaterialSlot slot;
        slot.originalIndex = materialIndex;

        aiString name;
        if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && strlen(name.C_Str()) > 0)
        {
            slot.name = name.C_Str();
        }
        else
        {
            slot.name = "Material_" + std::to_string(materialIndex);
        }

        extractTexturePath(aiMat, aiTextureType_DIFFUSE, "albedo", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_NORMALS, "normal", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_METALNESS, "metallic", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_DIFFUSE_ROUGHNESS, "roughness", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_AMBIENT_OCCLUSION, "ao", slot, modelBasePath);

        // Extract Parameters
        // TODO the others as well....
        aiColor4D color;
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            slot.vectorParams["baseColorFactor"] = { color.r, color.g, color.b, color.a };
        }

        float floatValue;
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, floatValue) == AI_SUCCESS)
        {
            slot.scalarParams["roughnessFactor"] = floatValue;
        }
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, floatValue) == AI_SUCCESS)
        {
            slot.scalarParams["metallicFactor"] = floatValue;
        }

        return slot;
    }





    void SceneLoader::extractNodesForCompiler(
        const aiNode* node,
        const int32_t parentIdx,
        const aiScene* scene,
        std::vector<SceneNode>& outNodes) const
    {
        if (!node)
        {
            return;
        }

        SceneNode primaryNode;
        primaryNode.parentIndex = parentIdx;

        // Convert Assimp matrix to GLM matrix
        const aiMatrix4x4& aiMat = node->mTransformation;

        primaryNode.localTransform = mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
        primaryNode.name = node->mName.length > 0 ? node->mName.C_Str() : "";

        int32_t currentNodeIndex = -1;

        // 2. Handle the meshes for this node.
        if (node->mNumMeshes > 0)
        {
            // The first mesh is assigned to our primary node.
            primaryNode.meshIndex = node->mMeshes[0];
            outNodes.push_back(primaryNode);
            currentNodeIndex = outNodes.size() - 1;

            // If there are more meshes, create sibling nodes for them.
            for (uint32_t i = 1; i < node->mNumMeshes; ++i)
            {
                SceneNode siblingNode = primaryNode; // Copy parent, transform, and name
                siblingNode.meshIndex = node->mMeshes[i];
                outNodes.push_back(siblingNode);
            }
        }
        else
        {
            // If there are no meshes, we still add the node to preserve the hierarchy.
            // Its meshIndex will remain -1.
            outNodes.push_back(primaryNode);
            currentNodeIndex = outNodes.size() - 1;
        }

        // Recursively process child nodes
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            extractNodesForCompiler(node->mChildren[i], currentNodeIndex, scene, outNodes);
        }
    }


}

/*
    void SceneLoader::extractObjectsFromNode(const aiNode* node, const mat4& parentTransform, const int32_t parentIndex, const aiScene* scene, std::vector<SceneObject>& objects) const
    {
        if (!node)
            return;

        // Convert Assimp matrix to GLM matrix
        const aiMatrix4x4& aiMat = node->mTransformation;
        const mat4 nodeTransform(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
        const mat4 globalTransform = parentTransform * nodeTransform;

        int currentNodeIndex = objects.size();
    
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
            extractObjectsFromNode(node->mChildren[i], globalTransform, currentNodeIndex, scene, objects);
        }
    }

void SceneLoader::calculateBounds(Scene& scene, std::vector<ProcessedMesh>& meshes)
{
    if (scene.objects.empty())
    {
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
        if (obj.type != SceneObjectType::Mesh)
            continue;

        const ProcessedMesh& mesh = meshes[obj.meshIndex];

        const vec3 meshCenter = vec3(mesh.bounds);
        const float meshRadius = mesh.bounds.w;

        // Convert sphere bounds to box bounds for more accurate calculation
        vec3 meshMin = meshCenter - vec3(meshRadius);
        vec3 meshMax = meshCenter + vec3(meshRadius);

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
        }

        foundAnyGeometry = true;
    }

    if (!foundAnyGeometry)
    {
        scene.center = vec3(0);
        scene.radius = 0;
        return;
    }

    // Calculate original scene properties
    const vec3 originalCenter = (globalMinBounds + globalMaxBounds) * 0.5f;
    const vec3 originalSize = globalMaxBounds - globalMinBounds;
    const float originalRadius = glm::length(originalSize) * 0.5f;

    // Calculate scaling to fit within camera view range
    // Camera is at z = -1.5f looking towards origin, with near=0.1f, far=100.0f
    const float maxAllowedRadius = 5.0f; // Keep scene at reasonable size for initial viewing
    const float scaleFactor = originalRadius > 0.0f ? (maxAllowedRadius / originalRadius) : 1.0f;

    // Calculate transforms: first center, then scale
    const vec3 centeringOffset = -originalCenter;
    const mat4 centeringTransform = glm::translate(mat4(1.0f), centeringOffset);
    const mat4 scalingTransform = glm::scale(mat4(1.0f), vec3(scaleFactor));
    const mat4 combinedTransform = scalingTransform * centeringTransform;

    // Apply transformation to all mesh objects
    for (auto& obj : scene.objects)
    {
        if (obj.type == SceneObjectType::Mesh)
        {
            const vec3 oldPos = vec3(obj.transform[3]);
            obj.transform = combinedTransform * obj.transform;
            const vec3 newPos = vec3(obj.transform[3]);

        }
    }

    // Update scene bounds after transformation
    const vec3 finalSize = originalSize * scaleFactor;
    scene.boundingMin = -finalSize * 0.5f;
    scene.boundingMax = finalSize * 0.5f;
    scene.center = vec3(0.0f); // Centered at origin
    scene.radius = originalRadius * scaleFactor;

}
*/



/*

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
                //LOG_WARNING("Invalid mesh index {} for object '{}'", obj.meshIndex, obj.name);
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
*/
