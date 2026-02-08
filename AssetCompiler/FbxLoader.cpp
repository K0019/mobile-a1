/******************************************************************************/
/*!
\file   FbxLoader.cpp
\brief  FBX scene loader using ufbx library. Replaces Assimp for FBX files.
*/
/******************************************************************************/

#include "FbxLoader.h"

#include <ufbx.h>

#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace compiler
{
    // ===== ufbx -> GLM conversion helpers =====

    inline vec3 UfbxToVec3(const ufbx_vec3& v)
    {
        return vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
    }

    inline vec4 UfbxToVec4(const ufbx_vec4& v)
    {
        return vec4(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z), static_cast<float>(v.w));
    }

    inline quat UfbxToQuat(const ufbx_quat& q)
    {
        // GLM quat constructor: (w, x, y, z)
        return quat(static_cast<float>(q.w), static_cast<float>(q.x), static_cast<float>(q.y), static_cast<float>(q.z));
    }

    // ufbx_matrix is column-major 4x3 (cols[0..2] = basis, cols[3] = translation)
    // GLM mat4 is column-major 4x4
    inline mat4 UfbxToMat4(const ufbx_matrix& m)
    {
        return mat4(
            static_cast<float>(m.m00), static_cast<float>(m.m10), static_cast<float>(m.m20), 0.0f,
            static_cast<float>(m.m01), static_cast<float>(m.m11), static_cast<float>(m.m21), 0.0f,
            static_cast<float>(m.m02), static_cast<float>(m.m12), static_cast<float>(m.m22), 0.0f,
            static_cast<float>(m.m03), static_cast<float>(m.m13), static_cast<float>(m.m23), 1.0f
        );
    }

    // ===== Skinning helpers =====

    static void accumulateWeightFbx(SkinningData& data, uint32_t jointIndex, float weight)
    {
        if (weight <= 0.0f) return;

        for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
        {
            if (data.weights[i] == 0.0f)
            {
                data.boneIndices[i] = jointIndex;
                data.weights[i] = weight;
                return;
            }
        }

        // Replace smallest weight if this is larger
        size_t smallest = 0;
        for (size_t i = 1; i < MAX_BONES_PER_VERTEX; ++i)
        {
            if (data.weights[i] < data.weights[smallest])
                smallest = i;
        }

        if (weight > data.weights[smallest])
        {
            data.boneIndices[smallest] = jointIndex;
            data.weights[smallest] = weight;
        }
    }

    static void normalizeWeightsFbx(SkinningData& data)
    {
        float total = 0.0f;
        for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
            total += data.weights[i];

        if (total > 0.0f)
        {
            float inv = 1.0f / total;
            for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
                data.weights[i] *= inv;
        }
        else
        {
            data = SkinningData{};
        }
    }

    // ===== Vertex hashing for deduplication =====

    struct VertexKey
    {
        vec3 position;
        vec3 normal;
        float uv_x, uv_y;

        bool operator==(const VertexKey& other) const
        {
            return position == other.position && normal == other.normal
                && uv_x == other.uv_x && uv_y == other.uv_y;
        }
    };

    struct VertexKeyHash
    {
        size_t operator()(const VertexKey& k) const
        {
            // Simple spatial hash
            auto h = [](float f) -> size_t {
                uint32_t bits;
                memcpy(&bits, &f, sizeof(bits));
                return std::hash<uint32_t>{}(bits);
            };
            size_t seed = h(k.position.x);
            seed ^= h(k.position.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.position.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.normal.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.normal.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.normal.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.uv_x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h(k.uv_y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    // ===== Scene Deleter =====

    void FbxLoader::UfbxSceneDeleter::operator()(ufbx_scene* s) const
    {
        if (s) ufbx_free_scene(s);
    }

    // ===== Scene Import =====

    FbxLoader::UfbxScenePtr FbxLoader::importScene(const std::filesystem::path& path) const
    {
        if (!exists(path))
        {
            return UfbxScenePtr(nullptr);
        }

        ufbx_load_opts opts = {};
        opts.target_axes = ufbx_axes_right_handed_y_up;
        opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
        opts.generate_missing_normals = true;

        ufbx_error error = {};
        ufbx_scene* scene = ufbx_load_file(path.string().c_str(), &opts, &error);

        if (!scene)
        {
            std::cerr << "[FbxLoader] Failed to load: " << path.string() << "\n";
            if (error.description.length > 0)
            {
                std::cerr << "[FbxLoader] Error: " << std::string(error.description.data, error.description.length) << "\n";
            }
            return UfbxScenePtr(nullptr);
        }

        return UfbxScenePtr(scene);
    }

    // ===== Skeleton Extraction =====

    void FbxLoader::extractSkeleton(const ufbx_scene* scene, Scene& outScene)
    {
        // Iterate all skin clusters across all skin deformers to find bones
        for (size_t i = 0; i < scene->skin_clusters.count; ++i)
        {
            const ufbx_skin_cluster* cluster = scene->skin_clusters.data[i];
            if (!cluster->bone_node) continue;

            std::string boneName(cluster->bone_node->name.data, cluster->bone_node->name.length);

            if (outScene.skeleton.boneNameToIndex.find(boneName) != outScene.skeleton.boneNameToIndex.end())
                continue;

            uint32_t index = static_cast<uint32_t>(outScene.skeleton.bones.size());
            outScene.skeleton.boneNameToIndex[boneName] = index;

            ProcessedBone bone;
            bone.name = boneName;
            // geometry_to_bone is the inverse bind pose matrix
            bone.inverseBindPose = UfbxToMat4(cluster->geometry_to_bone);
            // bind_to_world is the rest pose transform
            bone.bindPose = UfbxToMat4(cluster->bind_to_world);

            outScene.skeleton.bones.push_back(bone);
        }

        // Build parent hierarchy by walking the node tree
        for (auto& [boneName, boneIdx] : outScene.skeleton.boneNameToIndex)
        {
            // Find the ufbx_node for this bone
            for (size_t n = 0; n < scene->nodes.count; ++n)
            {
                const ufbx_node* node = scene->nodes.data[n];
                std::string nodeName(node->name.data, node->name.length);

                if (nodeName == boneName && node->parent)
                {
                    std::string parentName(node->parent->name.data, node->parent->name.length);
                    auto parentIt = outScene.skeleton.boneNameToIndex.find(parentName);
                    if (parentIt != outScene.skeleton.boneNameToIndex.end())
                    {
                        outScene.skeleton.bones[boneIdx].parentIndex = static_cast<int32_t>(parentIt->second);
                    }
                    break;
                }
            }
        }
    }

    // ===== Animation Extraction =====

    void FbxLoader::extractAnimations(const ufbx_scene* scene, Scene& outScene)
    {
        outScene.animations.reserve(scene->anim_stacks.count);

        for (size_t stackIdx = 0; stackIdx < scene->anim_stacks.count; ++stackIdx)
        {
            const ufbx_anim_stack* stack = scene->anim_stacks.data[stackIdx];
            ProcessedAnimation anim;
            anim.name = std::string(stack->name.data, stack->name.length);
            if (anim.name.empty())
                anim.name = "Anim_" + std::to_string(stackIdx);

            double duration = stack->time_end - stack->time_begin;
            anim.duration = static_cast<float>(duration);
            // ufbx baked animation is in seconds, set ticksPerSecond to 1.0
            anim.ticksPerSecond = 1.0f;

            // Bake the animation into linearly interpolable keyframes
            ufbx_bake_opts bakeOpts = {};
            ufbx_error error = {};
            ufbx_baked_anim* baked = ufbx_bake_anim(scene, stack->anim, &bakeOpts, &error);
            if (!baked)
            {
                std::cerr << "[FbxLoader] Failed to bake animation: " << anim.name << "\n";
                continue;
            }

            // Extract skeletal channels from baked nodes
            for (size_t nodeIdx = 0; nodeIdx < baked->nodes.count; ++nodeIdx)
            {
                const ufbx_baked_node& bakedNode = baked->nodes.data[nodeIdx];

                // Get the node name via typed_id
                if (bakedNode.typed_id >= scene->nodes.count) continue;
                const ufbx_node* node = scene->nodes.data[bakedNode.typed_id];
                std::string nodeName(node->name.data, node->name.length);

                // Only include bones that are in our skeleton
                auto boneIt = outScene.skeleton.boneNameToIndex.find(nodeName);
                if (boneIt == outScene.skeleton.boneNameToIndex.end())
                    continue;

                ProcessedBoneChannel channel;
                channel.nodeName = nodeName;

                // Translation keys
                channel.positionKeys.resize(bakedNode.translation_keys.count);
                for (size_t k = 0; k < bakedNode.translation_keys.count; ++k)
                {
                    const ufbx_baked_vec3& key = bakedNode.translation_keys.data[k];
                    channel.positionKeys[k].time = static_cast<float>(key.time);
                    channel.positionKeys[k].value = UfbxToVec3(key.value);
                }

                // Rotation keys
                channel.rotationKeys.resize(bakedNode.rotation_keys.count);
                for (size_t k = 0; k < bakedNode.rotation_keys.count; ++k)
                {
                    const ufbx_baked_quat& key = bakedNode.rotation_keys.data[k];
                    channel.rotationKeys[k].time = static_cast<float>(key.time);
                    channel.rotationKeys[k].value = UfbxToQuat(key.value);
                }

                // Scale keys
                channel.scaleKeys.resize(bakedNode.scale_keys.count);
                for (size_t k = 0; k < bakedNode.scale_keys.count; ++k)
                {
                    const ufbx_baked_vec3& key = bakedNode.scale_keys.data[k];
                    channel.scaleKeys[k].time = static_cast<float>(key.time);
                    channel.scaleKeys[k].value = UfbxToVec3(key.value);
                }

                anim.skeletalChannels.push_back(std::move(channel));
            }

            // Extract morph animations from baked elements
            // Blend channels animate a "DeformPercent" or "weight" property
            for (size_t elemIdx = 0; elemIdx < baked->elements.count; ++elemIdx)
            {
                const ufbx_baked_element& bakedElem = baked->elements.data[elemIdx];

                // Check if this element is a blend channel
                if (bakedElem.element_id >= scene->elements.count) continue;
                const ufbx_element* elem = scene->elements.data[bakedElem.element_id];
                if (elem->type != UFBX_ELEMENT_BLEND_CHANNEL) continue;

                const ufbx_blend_channel* blendChannel = (const ufbx_blend_channel*)elem;

                // Find which mesh this blend channel belongs to
                // Walk up: blend_channel -> blend_deformer -> mesh
                bool found = false;
                for (size_t meshIdx = 0; meshIdx < scene->meshes.count && !found; ++meshIdx)
                {
                    const ufbx_mesh* mesh = scene->meshes.data[meshIdx];
                    for (size_t bdIdx = 0; bdIdx < mesh->blend_deformers.count && !found; ++bdIdx)
                    {
                        const ufbx_blend_deformer* bd = mesh->blend_deformers.data[bdIdx];
                        for (size_t chIdx = 0; chIdx < bd->channels.count && !found; ++chIdx)
                        {
                            if (bd->channels.data[chIdx] != blendChannel) continue;
                            found = true;

                            // The target shape index corresponds to the channel's position
                            // among all blend channels for this mesh
                            uint32_t targetShapeIndex = 0;
                            for (size_t prevBd = 0; prevBd < bdIdx; ++prevBd)
                                targetShapeIndex += static_cast<uint32_t>(mesh->blend_deformers.data[prevBd]->channels.count);
                            targetShapeIndex += static_cast<uint32_t>(chIdx);

                            // Extract weight keys from the first baked property
                            if (bakedElem.props.count > 0)
                            {
                                const ufbx_baked_prop& prop = bakedElem.props.data[0];

                                ProcessedMorphChannel morphChannel;
                                morphChannel.meshIndex = static_cast<uint32_t>(meshIdx);
                                morphChannel.meshName = std::string(mesh->name.data, mesh->name.length);

                                morphChannel.keys.resize(prop.keys.count);
                                for (size_t kIdx = 0; kIdx < prop.keys.count; ++kIdx)
                                {
                                    ProcessedMorphKey morphKey;
                                    morphKey.time = static_cast<float>(prop.keys.data[kIdx].time);
                                    morphKey.targetIndices.push_back(targetShapeIndex);
                                    // ufbx blend weight is 0-100, convert to 0-1
                                    morphKey.weights.push_back(static_cast<float>(prop.keys.data[kIdx].value.x) / 100.0f);
                                    morphChannel.keys[kIdx] = std::move(morphKey);
                                }

                                anim.morphChannels.push_back(std::move(morphChannel));
                            }
                        }
                    }
                }
            }

            ufbx_free_baked_anim(baked);
            outScene.animations.push_back(std::move(anim));
        }
    }

    // ===== Mesh Extraction =====

    // Determine which UV set a material's textures reference on this mesh.
    // Returns the UV set index (into fbxMesh->uv_sets) that the material's textures use.
    // Defaults to 0 if no textures reference a named UV set.
    static uint32_t resolveUVSetIndex(const ufbx_mesh* fbxMesh, const ufbx_material* mat)
    {
        if (!mat || fbxMesh->uv_sets.count <= 1)
            return 0;

        auto getUVSetName = [](const ufbx_material_map& map) -> std::string {
            if (map.texture && map.texture->uv_set.length > 0)
                return std::string(map.texture->uv_set.data, map.texture->uv_set.length);
            return "";
        };

        // Prioritize baseColor/diffuse UV set
        std::string targetName = getUVSetName(mat->pbr.base_color);
        if (targetName.empty()) targetName = getUVSetName(mat->fbx.diffuse_color);

        // If baseColor doesn't specify, check other textures
        if (targetName.empty())
        {
            for (const auto& map : { mat->pbr.normal_map, mat->pbr.metalness,
                                      mat->pbr.emission_color, mat->pbr.ambient_occlusion,
                                      mat->fbx.normal_map, mat->fbx.bump,
                                      mat->fbx.emission_color })
            {
                targetName = getUVSetName(map);
                if (!targetName.empty()) break;
            }
        }

        if (targetName.empty())
            return 0;

        // Find the UV set index by name
        for (size_t i = 0; i < fbxMesh->uv_sets.count; ++i)
        {
            const auto& uvSet = fbxMesh->uv_sets.data[i];
            if (uvSet.name.length > 0 &&
                std::string(uvSet.name.data, uvSet.name.length) == targetName)
            {
                return static_cast<uint32_t>(i);
            }
        }

        return 0;
    }

    ProcessedMesh FbxLoader::extractMesh([[maybe_unused]] const ufbx_scene* scene, const ufbx_mesh* fbxMesh,
                                          const ufbx_node* meshNode,
                                          uint32_t meshIndex, const MeshOptions& options,
                                          const ProcessedSkeleton& skeleton,
                                          const ufbx_mesh_part* materialPart)
    {
        ProcessedMesh mesh;
        mesh.name = fbxMesh->name.length > 0
            ? std::string(fbxMesh->name.data, fbxMesh->name.length)
            : ("Mesh_" + std::to_string(meshIndex));

        // Append part index for split meshes
        if (materialPart && fbxMesh->material_parts.count > 1)
            mesh.name += "_part" + std::to_string(materialPart->index);

        // Material index: use the material_part's material when available
        const ufbx_material* meshMaterial = nullptr;
        if (materialPart && materialPart->index < fbxMesh->materials.count)
        {
            meshMaterial = fbxMesh->materials.data[materialPart->index];
            if (meshMaterial) mesh.materialIndex = meshMaterial->typed_id;
        }
        else if (fbxMesh->materials.count > 0 && fbxMesh->materials.data[0])
        {
            meshMaterial = fbxMesh->materials.data[0];
            mesh.materialIndex = meshMaterial->typed_id;
        }

        if (fbxMesh->num_vertices == 0) return mesh;

        // Resolve which UV set this mesh's material textures reference
        const uint32_t targetUVSetIndex = resolveUVSetIndex(fbxMesh, meshMaterial);

        const bool hasNormals = fbxMesh->vertex_normal.exists;
        const bool hasUVs = fbxMesh->vertex_uv.exists;

        // Get geometric transform matrix (transforms mesh geometry independently of node hierarchy)
        // This handles FBX GeometricTranslation, GeometricRotation, GeometricScaling
        ufbx_matrix geomToNode = meshNode ? meshNode->geometry_to_node : ufbx_identity_matrix;
        ufbx_matrix normalMatrix = ufbx_matrix_for_normals(&geomToNode);

        // Triangulate and build per-face-corner vertices, then deduplicate
        // Pre-allocate for triangulated faces
        std::vector<Vertex> rawVertices;
        std::vector<uint32_t> rawIndices;
        rawVertices.reserve(fbxMesh->num_triangles * 3);
        rawIndices.reserve(fbxMesh->num_triangles * 3);

        // Temporary buffer for triangulation (max face size)
        std::vector<uint32_t> triFaceIndices(fbxMesh->max_face_triangles * 3);

        // Map from ufbx vertex index -> raw vertex indices (for skinning/morphs later)
        // Each ufbx logical vertex may produce multiple output vertices due to split UVs/normals
        std::vector<std::vector<uint32_t>> vertexToOutputMap(fbxMesh->num_vertices);

        // Vertex deduplication map
        std::unordered_map<VertexKey, uint32_t, VertexKeyHash> vertexMap;
        vertexMap.reserve(fbxMesh->num_indices);

        const size_t numFaces = materialPart ? materialPart->face_indices.count : fbxMesh->num_faces;
        for (size_t fi = 0; fi < numFaces; ++fi)
        {
            size_t faceIdx = materialPart ? materialPart->face_indices.data[fi] : fi;
            ufbx_face face = fbxMesh->faces.data[faceIdx];
            uint32_t numTris = ufbx_triangulate_face(triFaceIndices.data(),
                triFaceIndices.size(), fbxMesh, face);

            for (uint32_t triIdx = 0; triIdx < numTris; ++triIdx)
            {
                for (uint32_t corner = 0; corner < 3; ++corner)
                {
                    uint32_t meshIndex_local = triFaceIndices[triIdx * 3 + corner];

                    Vertex vertex;

                    // Position - apply geometric transform
                    ufbx_vec3 pos = ufbx_get_vertex_vec3(&fbxMesh->vertex_position, meshIndex_local);
                    pos = ufbx_transform_position(&geomToNode, pos);
                    vertex.position = vec3(static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));

                    // Normal - apply geometric transform (using normal matrix for correct handling)
                    if (hasNormals)
                    {
                        ufbx_vec3 norm = ufbx_get_vertex_vec3(&fbxMesh->vertex_normal, meshIndex_local);
                        norm = ufbx_transform_direction(&normalMatrix, norm);
                        norm = ufbx_vec3_normalize(norm);
                        vertex.normal = vec3(static_cast<float>(norm.x), static_cast<float>(norm.y), static_cast<float>(norm.z));
                    }
                    else
                    {
                        vertex.normal = vec3(0.0f, 1.0f, 0.0f);
                    }

                    // UV — use resolved UV set instead of default vertex_uv (always UV set 0)
                    if (hasUVs)
                    {
                        const ufbx_vertex_vec2& uvSource = (targetUVSetIndex < fbxMesh->uv_sets.count)
                            ? fbxMesh->uv_sets.data[targetUVSetIndex].vertex_uv
                            : fbxMesh->vertex_uv;
                        ufbx_vec2 uv = ufbx_get_vertex_vec2(&uvSource, meshIndex_local);
                        float u = static_cast<float>(uv.x);
                        float v = static_cast<float>(uv.y);
                        vertex.setUV(u, options.flipUVs ? (1.0f - v) : v);
                    }
                    else
                    {
                        vertex.setUV(0.0f, 0.0f);
                    }

                    // Tangent (initialize, will be computed later by mikktspace if needed)
                    vertex.tangent = vec4(0.0f, 0.0f, 0.0f, 1.0f);

                    // Deduplicate
                    VertexKey key{ vertex.position, vertex.normal, vertex.uv_x, vertex.uv_y };
                    auto it = vertexMap.find(key);
                    uint32_t outputIndex;
                    if (it != vertexMap.end())
                    {
                        outputIndex = it->second;
                    }
                    else
                    {
                        outputIndex = static_cast<uint32_t>(rawVertices.size());
                        vertexMap[key] = outputIndex;
                        rawVertices.push_back(vertex);
                    }

                    rawIndices.push_back(outputIndex);

                    // Track mapping from ufbx logical vertex to output vertices
                    uint32_t logicalVertex = fbxMesh->vertex_indices.data[meshIndex_local];
                    if (logicalVertex < fbxMesh->num_vertices)
                    {
                        auto& outputs = vertexToOutputMap[logicalVertex];
                        if (std::find(outputs.begin(), outputs.end(), outputIndex) == outputs.end())
                            outputs.push_back(outputIndex);
                    }
                }
            }
        }

        mesh.vertices = std::move(rawVertices);
        mesh.indices = std::move(rawIndices);

        // ===== Skinning =====
        if (fbxMesh->skin_deformers.count > 0 && !skeleton.empty())
        {
            mesh.skinning.resize(mesh.vertices.size());

            for (size_t sdIdx = 0; sdIdx < fbxMesh->skin_deformers.count; ++sdIdx)
            {
                const ufbx_skin_deformer* skin = fbxMesh->skin_deformers.data[sdIdx];

                for (size_t clusterIdx = 0; clusterIdx < skin->clusters.count; ++clusterIdx)
                {
                    const ufbx_skin_cluster* cluster = skin->clusters.data[clusterIdx];
                    if (!cluster->bone_node) continue;

                    std::string boneName(cluster->bone_node->name.data, cluster->bone_node->name.length);
                    auto boneIt = skeleton.boneNameToIndex.find(boneName);
                    if (boneIt == skeleton.boneNameToIndex.end()) continue;

                    uint32_t globalBoneIndex = boneIt->second;

                    // Validate bone index is within skeleton bounds
                    if (globalBoneIndex >= skeleton.bones.size()) {
                        std::cerr << "[FbxLoader] Warning: Bone index " << globalBoneIndex
                                  << " exceeds skeleton size " << skeleton.bones.size()
                                  << " for bone '" << boneName << "'. Skipping weights.\n";
                        continue;
                    }

                    // cluster->vertices/weights are indexed by logical vertex
                    for (size_t wIdx = 0; wIdx < cluster->num_weights; ++wIdx)
                    {
                        uint32_t logicalVertex = cluster->vertices.data[wIdx];
                        float weight = static_cast<float>(cluster->weights.data[wIdx]);

                        if (logicalVertex >= vertexToOutputMap.size()) continue;

                        // Apply weight to all output vertices that came from this logical vertex
                        for (uint32_t outputIdx : vertexToOutputMap[logicalVertex])
                        {
                            accumulateWeightFbx(mesh.skinning[outputIdx], globalBoneIndex, weight);
                        }
                    }
                }
            }

            for (auto& skinData : mesh.skinning)
                normalizeWeightsFbx(skinData);
        }

        // ===== Morph Targets (Blend Shapes) =====
        for (size_t bdIdx = 0; bdIdx < fbxMesh->blend_deformers.count; ++bdIdx)
        {
            const ufbx_blend_deformer* bd = fbxMesh->blend_deformers.data[bdIdx];

            for (size_t chIdx = 0; chIdx < bd->channels.count; ++chIdx)
            {
                const ufbx_blend_channel* channel = bd->channels.data[chIdx];

                // Use the target shape (last keyframe shape) if available
                const ufbx_blend_shape* shape = channel->target_shape;
                if (!shape) continue;

                MorphTargetData target;
                target.name = shape->name.length > 0
                    ? std::string(shape->name.data, shape->name.length)
                    : ("Morph_" + std::to_string(mesh.morphTargets.size()));

                // shape->offset_vertices are logical vertex indices
                // shape->position_offsets are the deltas
                for (size_t offIdx = 0; offIdx < shape->num_offsets; ++offIdx)
                {
                    uint32_t logicalVertex = shape->offset_vertices.data[offIdx];
                    ufbx_vec3 posOff = shape->position_offsets.data[offIdx];

                    ufbx_vec3 normOff = {};
                    if (offIdx < shape->normal_offsets.count)
                        normOff = shape->normal_offsets.data[offIdx];

                    // Apply geometric transform to deltas (use direction transform since these are offsets)
                    posOff = ufbx_transform_direction(&geomToNode, posOff);
                    normOff = ufbx_transform_direction(&normalMatrix, normOff);

                    if (logicalVertex >= vertexToOutputMap.size()) continue;

                    // Create a delta for each output vertex from this logical vertex
                    for (uint32_t outputIdx : vertexToOutputMap[logicalVertex])
                    {
                        MorphTargetVertexDelta delta;
                        delta.vertexIndex = outputIdx;
                        delta.deltaPosition = vec3(
                            static_cast<float>(posOff.x),
                            static_cast<float>(posOff.y),
                            static_cast<float>(posOff.z));
                        delta.deltaNormal = vec3(
                            static_cast<float>(normOff.x),
                            static_cast<float>(normOff.y),
                            static_cast<float>(normOff.z));
                        delta.deltaTangent = vec3(0.0f); // FBX doesn't typically have tangent offsets
                        target.deltas.push_back(delta);
                    }
                }

                mesh.morphTargets.push_back(std::move(target));
            }
        }

        // NOTE: Unlike the Assimp path, we do NOT run FixWindingOrder/FixInconsistentNormals here.
        // ufbx handles coordinate system conversion (including handedness mirroring and winding
        // reversal) correctly via UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY. Running heuristic-based
        // winding detection on top of ufbx's correct output causes double-fixing artifacts.
        // Commercial engines (Unity, Unreal, Godot) also trust the loader's output directly
        // rather than running post-hoc winding heuristics.

        return mesh;
    }

    // ===== Material Extraction =====

    ProcessedMaterialSlot FbxLoader::extractMaterial([[maybe_unused]] const ufbx_scene* scene, const ufbx_material* mat,
                                                      uint32_t materialIndex,
                                                      const std::filesystem::path& modelBasePath)
    {
        ProcessedMaterialSlot slot;
        slot.originalIndex = materialIndex;
        slot.name = mat->name.length > 0
            ? std::string(mat->name.data, mat->name.length)
            : ("Material_" + std::to_string(materialIndex));

        // Helper to extract texture path from a material map
        auto extractTextureFromMap = [&](const ufbx_material_map& map, const std::string& key) {
            if (!map.texture) return;

            const ufbx_texture* tex = map.texture;

            // Check for embedded texture content
            if (tex->content.size > 0)
            {
                EmbeddedTextureSource embedded;
                embedded.name = tex->name.length > 0
                    ? std::string(tex->name.data, tex->name.length)
                    : key;
                // Copy the texture data to avoid dangling pointer when ufbx scene is freed
                const uint8_t* srcData = static_cast<const uint8_t*>(tex->content.data);
                embedded.compressedData.assign(srcData, srcData + tex->content.size);
                slot.texturePaths[key] = std::move(embedded);
                return;
            }

            // File-based texture
            if (tex->filename.length > 0)
            {
                FilePathSource fileSource;
                std::filesystem::path texturePath(std::string(tex->filename.data, tex->filename.length));
                if (texturePath.is_relative())
                {
                    fileSource.path = (modelBasePath / texturePath).string();
                }
                else
                {
                    fileSource.path = texturePath.string();
                }
                slot.texturePaths[key] = fileSource;
                return;
            }

            // Try relative filename
            if (tex->relative_filename.length > 0)
            {
                FilePathSource fileSource;
                std::filesystem::path texturePath(std::string(tex->relative_filename.data, tex->relative_filename.length));
                fileSource.path = (modelBasePath / texturePath).string();
                slot.texturePaths[key] = fileSource;
            }
        };

        // Extract PBR textures
        extractTextureFromMap(mat->pbr.base_color, texturekeys::BASE_COLOR);
        // Fallback: if no PBR base color texture, try FBX diffuse
        if (slot.texturePaths.find(texturekeys::BASE_COLOR) == slot.texturePaths.end())
            extractTextureFromMap(mat->fbx.diffuse_color, texturekeys::BASE_COLOR);

        extractTextureFromMap(mat->pbr.metalness, texturekeys::METALLIC_ROUGHNESS);
        // Fallback: if no metalness texture, try roughness texture
        if (slot.texturePaths.find(texturekeys::METALLIC_ROUGHNESS) == slot.texturePaths.end())
            extractTextureFromMap(mat->pbr.roughness, texturekeys::METALLIC_ROUGHNESS);

        extractTextureFromMap(mat->pbr.normal_map, texturekeys::NORMAL);
        // Fallback: if no PBR normal map, try FBX normal_map, then FBX bump
        if (slot.texturePaths.find(texturekeys::NORMAL) == slot.texturePaths.end())
            extractTextureFromMap(mat->fbx.normal_map, texturekeys::NORMAL);
        if (slot.texturePaths.find(texturekeys::NORMAL) == slot.texturePaths.end())
            extractTextureFromMap(mat->fbx.bump, texturekeys::NORMAL);

        // Normal map scale/strength
        if (mat->pbr.normal_map.has_value)
        {
            float scale = static_cast<float>(mat->pbr.normal_map.value_real);
            if (scale > 0.0f)
                slot.normalScale = scale;
        }

        extractTextureFromMap(mat->pbr.emission_color, texturekeys::EMISSIVE);
        // Fallback: if no PBR emissive texture, try FBX emission_color
        if (slot.texturePaths.find(texturekeys::EMISSIVE) == slot.texturePaths.end())
            extractTextureFromMap(mat->fbx.emission_color, texturekeys::EMISSIVE);
        extractTextureFromMap(mat->pbr.ambient_occlusion, texturekeys::OCCLUSION);

        // Occlusion strength
        if (mat->pbr.ambient_occlusion.has_value)
        {
            slot.occlusionStrength = std::clamp(
                static_cast<float>(mat->pbr.ambient_occlusion.value_real), 0.0f, 1.0f);
        }

        // ===== PBR factors =====

        // Base color
        if (mat->pbr.base_color.has_value)
        {
            const auto& c = mat->pbr.base_color.value_vec4;
            slot.baseColorFactor = vec4(
                static_cast<float>(c.x), static_cast<float>(c.y),
                static_cast<float>(c.z), static_cast<float>(c.w));
        }
        else if (mat->fbx.diffuse_color.has_value)
        {
            const auto& c = mat->fbx.diffuse_color.value_vec3;
            slot.baseColorFactor = vec4(
                static_cast<float>(c.x), static_cast<float>(c.y),
                static_cast<float>(c.z), 1.0f);
        }

        // Metallic
        if (mat->pbr.metalness.has_value)
        {
            slot.metallicFactor = std::clamp(static_cast<float>(mat->pbr.metalness.value_real), 0.0f, 1.0f);
        }
        else if (mat->fbx.reflection_factor.has_value)
        {
            slot.metallicFactor = std::clamp(static_cast<float>(mat->fbx.reflection_factor.value_real), 0.0f, 1.0f);
        }

        // Roughness
        if (mat->pbr.roughness.has_value)
        {
            slot.roughnessFactor = std::clamp(static_cast<float>(mat->pbr.roughness.value_real), 0.0f, 1.0f);
        }
        else if (mat->fbx.specular_exponent.has_value)
        {
            // Shininess to roughness conversion (same as Assimp path)
            float shininess = static_cast<float>(mat->fbx.specular_exponent.value_real);
            slot.roughnessFactor = std::clamp(1.0f - (shininess / 256.0f), 0.0f, 1.0f);
        }

        // Emissive
        if (mat->pbr.emission_color.has_value)
        {
            const auto& c = mat->pbr.emission_color.value_vec3;
            slot.emissiveFactor = vec3(
                static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));

            // Apply emission factor if present
            if (mat->pbr.emission_factor.has_value)
            {
                float factor = static_cast<float>(mat->pbr.emission_factor.value_real);
                slot.emissiveFactor *= factor;
            }
        }
        else if (mat->fbx.emission_color.has_value)
        {
            const auto& c = mat->fbx.emission_color.value_vec3;
            slot.emissiveFactor = vec3(
                static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z));
            if (mat->fbx.emission_factor.has_value)
            {
                float factor = static_cast<float>(mat->fbx.emission_factor.value_real);
                slot.emissiveFactor *= factor;
            }
        }

        // Opacity
        // Note: Many FBX exporters (especially Blender) set transparency_factor = 1.0 as a
        // default value even for opaque materials. This is backwards from what you'd expect
        // (1.0 should mean fully transparent), so we ignore transparency_factor = 1.0.
        if (mat->pbr.opacity.has_value)
        {
            slot.baseColorFactor.a = std::clamp(static_cast<float>(mat->pbr.opacity.value_real), 0.0f, 1.0f);
        }
        else if (mat->fbx.transparency_factor.has_value)
        {
            float transparency = static_cast<float>(mat->fbx.transparency_factor.value_real);
            // Ignore transparency_factor = 1.0 (common bogus default from Blender)
            // and values very close to 0 (essentially opaque)
            if (transparency > 0.01f && transparency < 0.99f)
            {
                slot.baseColorFactor.a = std::clamp(1.0f - transparency, 0.0f, 1.0f);
            }
            // else: keep alpha at 1.0 (opaque)
        }

        // Alpha mode - FBX doesn't have explicit alpha mode like glTF
        // Default to opaque, only use Blend if alpha is actually low
        slot.alphaMode = AlphaMode::Opaque;
        if (slot.baseColorFactor.a < 0.99f)
        {
            slot.alphaMode = AlphaMode::Blend;
        }

        // Two-sided detection (check FBX property)
        // ufbx doesn't expose this directly, but we can check props
        ufbx_prop* doubleSidedProp = ufbx_find_prop(&mat->props, "DoubleSided");
        if (doubleSidedProp && doubleSidedProp->value_int != 0)
        {
            slot.flags |= MaterialFlags::DOUBLE_SIDED;
        }

        // Unlit detection via FBX shading model
        if (mat->shading_model_name.length > 0)
        {
            std::string shadingModel(mat->shading_model_name.data, mat->shading_model_name.length);
            // "Constant" = no lighting response in FBX (like Maya surface shader)
            if (shadingModel == "Constant" || shadingModel == "constant")
            {
                slot.flags |= MaterialFlags::UNLIT;
            }
        }

        return slot;
    }

    // ===== Node Extraction =====

    void FbxLoader::extractNodes(const ufbx_node* node, int32_t parentIdx,
                                  std::vector<SceneNode>& outNodes) const
    {
        if (!node) return;

        SceneNode primaryNode;
        primaryNode.parentIndex = parentIdx;
        primaryNode.name = node->name.length > 0 ? std::string(node->name.data, node->name.length) : "";

        // Convert local transform to mat4
        ufbx_matrix localMat = ufbx_transform_to_matrix(&node->local_transform);
        primaryNode.localTransform = UfbxToMat4(localMat);

        int32_t currentNodeIndex = -1;

        if (node->mesh)
        {
            primaryNode.meshIndex = static_cast<int32_t>(node->mesh->typed_id);
            outNodes.push_back(primaryNode);
            currentNodeIndex = static_cast<int32_t>(outNodes.size()) - 1;
        }
        else
        {
            outNodes.push_back(primaryNode);
            currentNodeIndex = static_cast<int32_t>(outNodes.size()) - 1;
        }

        for (size_t i = 0; i < node->children.count; ++i)
        {
            extractNodes(node->children.data[i], currentNodeIndex, outNodes);
        }
    }

    // ===== Main Load Entry Point =====

    SceneLoadData FbxLoader::loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions)
    {
        SceneLoadData returnData;

        try
        {
            auto scenePtr = importScene(path);
            if (!scenePtr)
            {
                returnData.errors.push_back("Failed to import FBX file with ufbx: " + path.string());
                return returnData;
            }

            const ufbx_scene* fbxScene = scenePtr.get();
            const auto baseDir = path.parent_path();

            Scene loadedScene;
            loadedScene.name = path.filename().string();

            // Extract scene hierarchy
            if (fbxScene->root_node)
            {
                extractNodes(fbxScene->root_node, -1, loadedScene.nodes);
            }

            // Extract materials
            loadedScene.materials.reserve(fbxScene->materials.count);
            for (size_t i = 0; i < fbxScene->materials.count; ++i)
            {
                loadedScene.materials.push_back(
                    extractMaterial(fbxScene, fbxScene->materials.data[i],
                                   static_cast<uint32_t>(i), baseDir));
            }

            // Extract skeleton and animations
            extractSkeleton(fbxScene, loadedScene);
            extractAnimations(fbxScene, loadedScene);

            // Extract meshes — split multi-material meshes into separate sub-meshes
            // Track how each ufbx mesh maps to output mesh indices
            // meshSplitMap[ufbx_typed_id] = {firstOutputIdx, count}
            std::vector<std::pair<uint32_t, uint32_t>> meshSplitMap(fbxScene->meshes.count, {0, 0});

            loadedScene.meshes.reserve(fbxScene->meshes.count);
            for (size_t i = 0; i < fbxScene->meshes.count; ++i)
            {
                const ufbx_mesh* mesh = fbxScene->meshes.data[i];

                // Collect all nodes referencing this mesh (Fix 7: instancing awareness)
                const ufbx_node* meshNode = nullptr;
                for (size_t n = 0; n < fbxScene->nodes.count; ++n)
                {
                    if (fbxScene->nodes.data[n]->mesh == mesh)
                    {
                        meshNode = fbxScene->nodes.data[n];
                        break;
                    }
                }

                uint32_t firstIdx = static_cast<uint32_t>(loadedScene.meshes.size());

                if (mesh->material_parts.count <= 1)
                {
                    // Single material — extract as before (pass nullptr for materialPart)
                    loadedScene.meshes.push_back(
                        extractMesh(fbxScene, mesh, meshNode, static_cast<uint32_t>(i),
                                    meshOptions, loadedScene.skeleton, nullptr));
                }
                else
                {
                    // Multi-material — one sub-mesh per material part
                    for (size_t p = 0; p < mesh->material_parts.count; ++p)
                    {
                        const ufbx_mesh_part& part = mesh->material_parts.data[p];
                        if (part.num_faces == 0) continue;  // Skip empty parts
                        loadedScene.meshes.push_back(
                            extractMesh(fbxScene, mesh, meshNode, static_cast<uint32_t>(i),
                                        meshOptions, loadedScene.skeleton, &part));
                    }
                }

                uint32_t count = static_cast<uint32_t>(loadedScene.meshes.size()) - firstIdx;
                meshSplitMap[i] = { firstIdx, count };
            }

            // Fix up node mesh indices and create sibling nodes for multi-material meshes
            std::vector<SceneNode> siblingNodes;
            for (auto& node : loadedScene.nodes)
            {
                if (node.meshIndex < 0) continue;
                uint32_t ufbxId = static_cast<uint32_t>(node.meshIndex);
                if (ufbxId >= meshSplitMap.size()) continue;

                auto [firstIdx, count] = meshSplitMap[ufbxId];
                if (count == 0) { node.meshIndex = -1; continue; }

                node.meshIndex = static_cast<int32_t>(firstIdx);
                for (uint32_t s = 1; s < count; ++s)
                {
                    SceneNode sibling = node;  // Same parent, same transform
                    sibling.meshIndex = static_cast<int32_t>(firstIdx + s);
                    siblingNodes.push_back(sibling);
                }
            }
            loadedScene.nodes.insert(loadedScene.nodes.end(),
                                     siblingNodes.begin(), siblingNodes.end());

            returnData.scene = std::move(loadedScene);
        }
        catch (const std::exception& e)
        {
            returnData.errors.push_back("An exception occurred during FBX loading: " + std::string(e.what()));
        }

        return returnData;
    }
}
