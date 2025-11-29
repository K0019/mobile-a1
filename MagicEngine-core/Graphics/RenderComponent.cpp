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
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Editor/Containers/GUICollection.h"
#include <ImGui/ImguiHeader.h>

const ResourceMesh* RenderComponent::GetMesh() const
{
    return meshHandle.GetResource();
}

const std::vector<UserResourceHandle<ResourceMaterial>>& RenderComponent::GetMaterialsList() const
{
    return materials;
}

std::vector<UserResourceHandle<ResourceMaterial>>& RenderComponent::GetMaterialsList() 
{
    return materials;
}

void RenderComponent::EditorDraw()
{
    const std::string* meshText{ ST<MagicResourceManager>::Get()->Editor_GetName(meshHandle.GetHash()) };

    gui::TextUnformatted("Mesh");
    gui::SameLine();
    gui::TextBoxReadOnly("##", meshText ? meshText->c_str() : "");
    gui::PayloadTarget<size_t>("MESH_HASH", [this](size_t hash) -> void {
        meshHandle = hash;

        //now based on this new mesh, update the default materials
        auto newMesh{ meshHandle.GetResource() };
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
    gui::TextUnformatted("Cast Shadow");
    gui::SameLine();
    if (ImGui::Combo("##CastShadow", &shadowIndex, shadowOptions, 3))
    {
        castShadowOverride = shadowIndex - 1;
    }

    auto mesh{ meshHandle.GetResource() };


    if (mesh)
    {
        size_t meshCount = mesh->handles.size();
        std::string materialHeaderLabel = "Materials (" + std::to_string(meshCount) + ")";
        if (gui::CollapsingHeader materialHeader{ materialHeaderLabel.c_str() })
        {
            for (int i = 0; i < meshCount; i++)
            {
                if (i >= materials.size()) break;    //go reassign the mesh
                gui::SetID id(i);

                const std::string* materialText{ ST<MagicResourceManager>::Get()->Editor_GetName(materials[i].GetHash()) };

                gui::TextUnformatted(std::string("Material") + std::to_string(i));
                gui::SameLine();
                gui::TextBoxReadOnly("##", materialText ? materialText->c_str() : "");
                gui::PayloadTarget<size_t>("MATERIAL_HASH", [&](size_t hash) -> void {
                    materials[i] = hash;
                });
                gui::SameLine();
            
                if (gui::Button repeatButton{ ICON_FA_REPEAT })
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }

            if (gui::Button resetButton{ "Reset Materials" })
            {
                materials.resize(meshCount);
                for (int i = 0; i < meshCount; i++)
                {
                    materials[i] = mesh->defaultMaterialHashes[i];
                }
            }
        }
    }
}

// RenderSystem is no longer needed - GraphicsMain reads directly from ECS components
