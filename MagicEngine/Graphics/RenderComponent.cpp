/******************************************************************************/
/*!
\file   RenderComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of RenderComponent.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VFS/VFS.h"
#include "Graphics/RenderComponent.h"
#include "Assets/AssetManager.h"
#include "Assets/Types/AssetTypes.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"
#include <ImGui/ImguiHeader.h>

const ResourceMesh* RenderComponent::GetMesh() const
{
    return meshHandle.GetResource();
}

const std::vector<AssetHandle<ResourceMaterial>>& RenderComponent::GetMaterialsList() const
{
    return materials;
}

std::vector<AssetHandle<ResourceMaterial>>& RenderComponent::GetMaterialsList() 
{
    return materials;
}

void RenderComponent::EditorDraw()
{
    // ===== MESH SELECTION =====
    const std::string* meshText{ ST<AssetManager>::Get()->Editor_GetName(meshHandle.GetHash()) };

    gui::TextUnformatted("Mesh");
    gui::SameLine();
    gui::TextBoxReadOnly("##mesh", meshText ? meshText->c_str() : "");
    gui::PayloadTarget<size_t>("MESH_HASH", [this](size_t hash) -> void {
        meshHandle = hash;
        auto newMesh{ meshHandle.GetResource() };
        if (!newMesh) return;
        size_t meshCount = newMesh->handles.size();
        materials.resize(meshCount);
        for (size_t i = 0; i < meshCount; i++)
        {
            materials[i] = newMesh->defaultMaterialHashes[i];
        }
    });

    // ===== SHADOW SETTINGS =====
    const char* shadowOptions[] = { "Use Material", "Off", "On" };
    int shadowIndex = castShadowOverride + 1;
    if (gui::Combo("Cast Shadow", shadowOptions, 3, &shadowIndex))
    {
        castShadowOverride = shadowIndex - 1;
    }

    auto mesh{ meshHandle.GetResource() };
    if (!mesh) return;

    size_t submeshCount = mesh->handles.size();
    auto& assetSystem = ST<GraphicsMain>::Get()->GetAssetSystem();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // ===== MESH INFO SECTION =====
    if (ImGui::CollapsingHeader(ICON_FA_CIRCLE_INFO " Mesh Info", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Calculate totals
        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
        bool hasSkeleton = false;
        bool hasMorphs = false;
        uint32_t boneCount = 0;
        uint32_t morphTargetCount = 0;
        const std::vector<std::string>* boneNames = nullptr;
        const std::vector<MeshMorphTargetInfo>* morphTargets = nullptr;

        for (size_t i = 0; i < submeshCount; i++)
        {
            if (i < mesh->handles.size())
            {
                const auto* gpuData = assetSystem.getMesh(mesh->handles[i]);
                const auto* metadata = assetSystem.getMeshMetadata(mesh->handles[i]);

                if (gpuData)
                {
                    totalVertices += gpuData->vertexCount;
                    totalIndices += gpuData->indexCount;

                    if (gpuData->animation.jointCount > 0)
                    {
                        hasSkeleton = true;
                        if (gpuData->animation.jointCount > boneCount)
                        {
                            boneCount = gpuData->animation.jointCount;
                            if (metadata && !metadata->jointNames.empty())
                                boneNames = &metadata->jointNames;
                        }
                    }
                    if (gpuData->animation.morphTargetCount > 0)
                    {
                        hasMorphs = true;
                        if (gpuData->animation.morphTargetCount > morphTargetCount)
                        {
                            morphTargetCount = gpuData->animation.morphTargetCount;
                            if (metadata && !metadata->morphTargets.empty())
                                morphTargets = &metadata->morphTargets;
                        }
                    }
                }
            }
        }

        // Compact stats
        ImGui::Text("%zu meshes, %u verts, %u tris", submeshCount, totalVertices, totalIndices / 3);
        if (hasSkeleton || hasMorphs)
        {
            if (hasSkeleton)
                ImGui::Text(ICON_FA_BONE " %u bones", boneCount);
            if (hasMorphs)
                ImGui::Text(ICON_FA_MASKS_THEATER " %u morphs", morphTargetCount);
        }

        // Bones list (collapsed by default)
        if (hasSkeleton && boneNames && !boneNames->empty())
        {
            if (ImGui::TreeNode(ICON_FA_LIST " Bone Names"))
            {
                for (size_t i = 0; i < boneNames->size(); i++)
                    ImGui::Text("%3zu  %s", i, (*boneNames)[i].c_str());
                ImGui::TreePop();
            }
        }

        // Morphs list (collapsed by default)
        if (hasMorphs && morphTargets && !morphTargets->empty())
        {
            if (ImGui::TreeNode(ICON_FA_LIST " Morph Names"))
            {
                for (size_t i = 0; i < morphTargets->size(); i++)
                    ImGui::Text("%3zu  %s", i, (*morphTargets)[i].name.c_str());
                ImGui::TreePop();
            }
        }
    }

    ImGui::Spacing();

    // ===== MATERIALS SECTION =====
    std::string materialsHeader = ICON_FA_PALETTE " Materials (" + std::to_string(submeshCount) + ")";
    if (ImGui::CollapsingHeader(materialsHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Reset all button
        if (ImGui::SmallButton(ICON_FA_ROTATE " Reset All"))
        {
            materials.resize(submeshCount);
            for (size_t i = 0; i < submeshCount; i++)
                materials[i] = mesh->defaultMaterialHashes[i];
        }
        ImGui::SameLine();
        ImGui::TextDisabled("Drag materials to assign");

        ImGui::Spacing();

        for (size_t i = 0; i < submeshCount; i++)
        {
            if (i >= materials.size()) break;

            ImGui::PushID(static_cast<int>(i));

            // Get names
            const auto* metadata = (i < mesh->handles.size()) ? assetSystem.getMeshMetadata(mesh->handles[i]) : nullptr;
            std::string submeshName = (metadata && !metadata->meshName.empty())
                ? metadata->meshName : "Submesh " + std::to_string(i);

            const std::string* materialText{ ST<AssetManager>::Get()->Editor_GetName(materials[i].GetHash()) };
            std::string materialName = materialText ? *materialText : "(none)";

            // Row 1: [Edit] [Reset] [idx]
            if (ImGui::SmallButton(ICON_FA_PEN))
            {
                size_t matHash = materials[i].GetHash();
                if (matHash != 0 && matHash != 1)
                    ST<EventsQueue>::Get()->AddEventForNextFrame(Events::OpenMaterialEditor{ matHash });
            }
            ImGui::SameLine();

            if (ImGui::SmallButton(ICON_FA_ROTATE))
                materials[i] = mesh->defaultMaterialHashes[i];
            ImGui::SameLine();

            ImGui::TextDisabled("[%zu]", i);

            // Row 2: Material name (wraps if too long)
            ImGui::Indent(10.0f);
            ImGui::TextWrapped("%s", materialName.c_str());
            gui::PayloadTarget<size_t>("MATERIAL_HASH", [this, i](size_t hash) -> void {
                materials[i] = hash;
            });
            ImGui::Unindent(10.0f);

            // Divider between entries
            if (i < submeshCount - 1)
                ImGui::Separator();

            ImGui::PopID();
        }
    }
}

// RenderSystem is no longer needed - GraphicsMain reads directly from ECS components
