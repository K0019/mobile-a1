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
    const std::string* meshText{ ST<AssetManager>::Get()->Editor_GetName(meshHandle.GetHash()) };

    gui::TextUnformatted("Mesh");
    gui::SameLine();
    gui::TextBoxReadOnly("##", meshText ? meshText->c_str() : "");
    gui::PayloadTarget<size_t>("MESH_HASH", [this](size_t hash) -> void {
        meshHandle = hash;

        //now based on this new mesh, update the default materials
        auto newMesh{ meshHandle.GetResource() };
        if (!newMesh) return;
        size_t meshCount = newMesh->handles.size();
        materials.resize(meshCount);
        for (int i = 0; i < meshCount; i++)
        {
            materials[i] = newMesh->defaultMaterialHashes[i];
        }
    });

    // Cast Shadow override
    const char* shadowOptions[] = { "Use Material", "Off", "On" };
    int shadowIndex = castShadowOverride + 1; // -1->0, 0->1, 1->2
    if (gui::Combo("CastShadow", shadowOptions, 3, &shadowIndex))
    {
        castShadowOverride = shadowIndex - 1;
    }

    auto mesh{ meshHandle.GetResource() };

    if (mesh)
    {
        size_t submeshCount = mesh->handles.size();
        auto& assetSystem = ST<GraphicsMain>::Get()->GetAssetSystem();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ===== MESH INFO SECTION =====
        if (ImGui::CollapsingHeader(ICON_FA_CIRCLE_INFO " Mesh Info", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10.0f);

            // Calculate totals across all submeshes and collect bone/morph info
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
                                {
                                    boneNames = &metadata->jointNames;
                                }
                            }
                        }
                        if (gpuData->animation.morphTargetCount > 0)
                        {
                            hasMorphs = true;
                            if (gpuData->animation.morphTargetCount > morphTargetCount)
                            {
                                morphTargetCount = gpuData->animation.morphTargetCount;
                                if (metadata && !metadata->morphTargets.empty())
                                {
                                    morphTargets = &metadata->morphTargets;
                                }
                            }
                        }
                    }
                }
            }

            // Display stats in a compact table format
            ImGui::Columns(2, "meshstats", false);
            ImGui::SetColumnWidth(0, 120);

            ImGui::TextDisabled("Submeshes:");
            ImGui::NextColumn();
            ImGui::Text("%zu", submeshCount);
            ImGui::NextColumn();

            ImGui::TextDisabled("Vertices:");
            ImGui::NextColumn();
            ImGui::Text("%s", totalVertices > 0 ? std::to_string(totalVertices).c_str() : "-");
            ImGui::NextColumn();

            ImGui::TextDisabled("Triangles:");
            ImGui::NextColumn();
            ImGui::Text("%s", totalIndices > 0 ? std::to_string(totalIndices / 3).c_str() : "-");
            ImGui::NextColumn();

            ImGui::TextDisabled("Skinned:");
            ImGui::NextColumn();
            if (hasSkeleton)
            {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), ICON_FA_CHECK " Yes (%u bones)", boneCount);
            }
            else
            {
                ImGui::TextDisabled(ICON_FA_XMARK " No");
            }
            ImGui::NextColumn();

            ImGui::TextDisabled("Morph Targets:");
            ImGui::NextColumn();
            if (hasMorphs)
            {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), ICON_FA_CHECK " Yes (%u)", morphTargetCount);
            }
            else
            {
                ImGui::TextDisabled(ICON_FA_XMARK " No");
            }
            ImGui::NextColumn();

            ImGui::Columns(1);

            // ===== SKELETON BONES LIST =====
            if (hasSkeleton && boneNames && !boneNames->empty())
            {
                ImGui::Spacing();
                std::string bonesHeader = ICON_FA_BONE " Bones (" + std::to_string(boneNames->size()) + ")";
                if (ImGui::TreeNode(bonesHeader.c_str()))
                {
                    ImGui::BeginChild("BonesList", ImVec2(0, std::min(120.0f, boneNames->size() * 18.0f + 8.0f)), true);
                    for (size_t i = 0; i < boneNames->size(); i++)
                    {
                        ImGui::TextDisabled("%3zu", i);
                        ImGui::SameLine();
                        ImGui::TextUnformatted((*boneNames)[i].c_str());
                    }
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            }

            // ===== MORPH TARGETS LIST =====
            if (hasMorphs && morphTargets && !morphTargets->empty())
            {
                ImGui::Spacing();
                std::string morphHeader = ICON_FA_MASKS_THEATER " Morph Targets (" + std::to_string(morphTargets->size()) + ")";
                if (ImGui::TreeNode(morphHeader.c_str()))
                {
                    ImGui::BeginChild("MorphList", ImVec2(0, std::min(100.0f, morphTargets->size() * 18.0f + 8.0f)), true);
                    for (size_t i = 0; i < morphTargets->size(); i++)
                    {
                        ImGui::TextDisabled("%3zu", i);
                        ImGui::SameLine();
                        ImGui::TextUnformatted((*morphTargets)[i].name.c_str());
                    }
                    ImGui::EndChild();
                    ImGui::TreePop();
                }
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::Spacing();

        // ===== SUBMESHES SECTION =====
        std::string submeshHeader = ICON_FA_CUBES " Submeshes (" + std::to_string(submeshCount) + ")";
        if (ImGui::CollapsingHeader(submeshHeader.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Indent(10.0f);

            // Reset all button
            if (ImGui::SmallButton(ICON_FA_ROTATE " Reset All Materials"))
            {
                materials.resize(submeshCount);
                for (size_t i = 0; i < submeshCount; i++)
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }

            ImGui::Spacing();

            for (size_t i = 0; i < submeshCount; i++)
            {
                if (i >= materials.size()) break;
                gui::SetID id(static_cast<int>(i));

                // Get submesh data
                const auto* gpuData = (i < mesh->handles.size()) ? assetSystem.getMesh(mesh->handles[i]) : nullptr;
                const auto* metadata = (i < mesh->handles.size()) ? assetSystem.getMeshMetadata(mesh->handles[i]) : nullptr;

                // Get submesh name
                std::string submeshName;
                if (metadata && !metadata->meshName.empty())
                {
                    submeshName = metadata->meshName;
                }
                else
                {
                    submeshName = "Submesh " + std::to_string(i);
                }

                // Submesh entry with tree node style
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.2f, 0.3f, 0.5f));
                bool nodeOpen = ImGui::TreeNodeEx(("##submesh" + std::to_string(i)).c_str(),
                    ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap,
                    ICON_FA_CUBE " %s", submeshName.c_str());

                // Show vertex/tri count on same line as header
                if (gpuData && gpuData->vertexCount > 0)
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%u verts, %u tris)", gpuData->vertexCount, gpuData->indexCount / 3);
                }

                ImGui::PopStyleColor();

                // Material drop target on the header itself
                gui::PayloadTarget<size_t>("MATERIAL_HASH", [&, idx = i](size_t hash) -> void {
                    materials[idx] = hash;
                });

                if (nodeOpen)
                {
                    ImGui::Indent(10.0f);

                    // Material assignment row
                    const std::string* materialText{ ST<AssetManager>::Get()->Editor_GetName(materials[i].GetHash()) };
                    std::string displayMaterial = materialText ? *materialText : "(none)";

                    // Truncate long material names
                    if (displayMaterial.length() > 40)
                    {
                        displayMaterial = displayMaterial.substr(0, 37) + "...";
                    }

                    ImGui::Text(ICON_FA_PALETTE " Material:");
                    ImGui::SameLine();

                    // Material text box with drop target
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30);
                    gui::TextBoxReadOnly("##mat", displayMaterial.c_str());
                    gui::PayloadTarget<size_t>("MATERIAL_HASH", [&, idx = i](size_t hash) -> void {
                        materials[idx] = hash;
                    });

                    // Show full name tooltip if truncated
                    if (materialText && materialText->length() > 40 && ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", materialText->c_str());
                    }

                    ImGui::SameLine();

                    // Reset to default button
                    if (ImGui::SmallButton(ICON_FA_ROTATE))
                    {
                        materials[i] = mesh->defaultMaterialHashes[i];
                    }
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("Reset to default material");
                    }

                    ImGui::Unindent(10.0f);
                    ImGui::TreePop();
                }
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Drag materials from Asset Browser onto submeshes");

            ImGui::Unindent(10.0f);
        }
    }
}

// RenderSystem is no longer needed - GraphicsMain reads directly from ECS components
