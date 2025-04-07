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
#include "RenderComponent.h"

#include "AnimatorComponent.h"
#include "ResourceManager.h"

RenderComponent::RenderComponent() : RenderComponent(0) {
}

RenderComponent::RenderComponent(size_t spriteID, bool flippedX, bool flippedY) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    spriteID(spriteID),
    flippedX(flippedX),
    flippedY(flippedY),
    m_materialInstance("default") {
}

RenderComponent::RenderComponent(const std::string& spriteName, bool flippedX, bool flippedY) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    spriteID(ResourceManager::GetSpriteID(spriteName)),
    flippedX(flippedX),
    flippedY(flippedY),
    m_materialInstance("default") {
}

size_t RenderComponent::GetSpriteID() const {
    return spriteID;
}

bool RenderComponent::GetFlippedX() const {
    return flippedX;
}

bool RenderComponent::GetFlippedY() const {
    return flippedY;
}

void RenderComponent::SetSpriteID(size_t new_spriteID) {
    spriteID = new_spriteID;
}

void RenderComponent::SetFlippedX(bool new_flippedX) {
    flippedX = new_flippedX;
}

void RenderComponent::SetFlippedY(bool new_flippedY) {
    flippedY = new_flippedY;
}

Vector4 RenderComponent::GetColor() const {
    return ST<MaterialSystem>::Get()->getEffectiveParameters(m_materialInstance).baseColor;
}

void RenderComponent::SetColor(Vector4 new_color) {
    // Store the current flags state before making any changes
    uint32_t currentFlags = ST<MaterialSystem>::Get()->getEffectiveFlags(m_materialInstance);
    
    // Update color parameters as before
    if (!m_materialInstance.hasParameterOverrides()) {
        MaterialParameters params = ST<MaterialSystem>::Get()->getEffectiveParameters(m_materialInstance);
        params.baseColor = new_color;
        m_materialInstance.setParameterOverrides(params);
    } else {
        MaterialParameters params = m_materialInstance.getOverrideParameters();
        params.baseColor = new_color;
        m_materialInstance.setParameterOverrides(params);
    }
    // Restore the flags after changing parameters
    // Check if flag was set in original effective flags
    if (currentFlags & MaterialFlags::ReceivesLight) {
        m_materialInstance.setOverrideFlag(MaterialFlags::ReceivesLight, true);
    }
    
    // Do the same for other important flags
    if (currentFlags & MaterialFlags::OccludesLight) {
        m_materialInstance.setOverrideFlag(MaterialFlags::OccludesLight, true);
    }
    
    if (currentFlags & MaterialFlags::SolidColor) {
        m_materialInstance.setOverrideFlag(MaterialFlags::SolidColor, true);
    }
    
    if (currentFlags & MaterialFlags::Repeating) {
        m_materialInstance.setOverrideFlag(MaterialFlags::Repeating, true);
    }
}

const MaterialInstance& RenderComponent::GetMaterialInstance() const {
    return m_materialInstance;
}

MaterialInstance& RenderComponent::GetMaterialInstance() {
    return m_materialInstance;
}

void RenderComponent::SetMaterial(const std::string& name)
{
    // Check if the material exists in the material system
    if (ST<MaterialSystem>::Get()->materialExists(name)) {
        // Create a new material instance with the specified base material
        // without preserving any existing parameter overrides
        m_materialInstance = MaterialInstance(name);
    }
    else {
        // Fallback to default material if the specified one doesn't exist
        m_materialInstance = MaterialInstance("default");
        
        // Log a warning about the missing material
        CONSOLE_LOG(LEVEL_WARNING) << "Material '" << name << "' not found, using default material.";
    }
}

std::string RenderComponent::GetMaterialName() const {
    return m_materialInstance.getBaseMaterialName();
}

#ifdef IMGUI_ENABLED
void RenderComponent::EditorDraw(RenderComponent& comp)
{
    if(!ResourceManager::SpriteExists(comp.spriteID))
    {
        ImGui::Text("No Sprite Assigned, drag a sprite here to assign it");
        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_ID"))
            {
                comp.spriteID = *static_cast<size_t*>(payload->Data);
            }
            ImGui::EndDragDropTarget();
        }
        return;
    }

    auto& sprite = ResourceManager::GetSprite(comp.spriteID);
    ImGui::Text("Sprite Name: %s", sprite.name.c_str());
    if(ecs::GetEntity(&comp)->GetComp<AnimatorComponent>())
    {
        ImGui::Text("Modify Animator Component to change sprites");
    }
    else
    {
        ImGui::Text("Drag an Sprite from the browser to assign it");
    }
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::ImageButton("Preview", VulkanManager::Get().VkTextureManager().getTexture(sprite.textureID).ImGui_handle, ImVec2(100, 100),
                       ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                       ImVec2(sprite.texCoords.z, sprite.texCoords.w));
    ImGui::PopItemFlag();
    if(ImGui::BeginDragDropTarget())
    {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_ID"))
            comp.spriteID = *static_cast<size_t*>(payload->Data);
        ImGui::EndDragDropTarget();
    }
    ImGui::Text("Width: %d px, Height: %d px", sprite.width, sprite.height);

   ImGui::Separator();
    ImGui::Text("Material Properties");

    // Material Selection Combo Box
    {
        const char* current_material = comp.GetMaterialInstance().getBaseMaterialName().c_str();
        if(ImGui::BeginCombo("Base Material", current_material)) {
            for(const auto& name : ST<MaterialSystem>::Get()->getMaterials() | std::views::keys) {
                bool is_selected = (comp.GetMaterialInstance().getBaseMaterialName() == name);
                if(ImGui::Selectable(name.c_str(), is_selected)) {
                    MaterialInstance newInstance(name);
                    if(comp.GetMaterialInstance().hasParameterOverrides()) {
                        newInstance.setParameterOverrides(
                            comp.GetMaterialInstance().getOverrideParameters()
                        );
                    }
                    comp.GetMaterialInstance() = std::move(newInstance);
                }
                if(is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    }

    // Material Override Controls
    bool hasOverrides = comp.GetMaterialInstance().hasParameterOverrides();
    if(hasOverrides) {
        if(ImGui::Button("Edit Material Properties")) {
            ImGui::OpenPopup("MaterialEditorModal");
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
        if(ImGui::Button("Reset to Base Material")) {
            comp.GetMaterialInstance().clearParameterOverrides();
        }
        ImGui::PopStyleColor();
    }
    else {
        if(ImGui::Button("Override Material Properties")) {
            MaterialParameters params = ST<MaterialSystem>::Get()->getEffectiveParameters(
                comp.GetMaterialInstance()
            );
            comp.GetMaterialInstance().setParameterOverrides(params);
        }
    }

    // Flip controls
    if(ImGui::Checkbox("Flip X", &comp.flippedX)) {
    }
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle to flip the sprite horizontally");

    ImGui::SameLine();

    if(ImGui::Checkbox("Flip Y", &comp.flippedY)) {
    }
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle to flip the sprite vertically");

    // Material Editor Modal
    static char materialNameBuffer[256] = "";
    static bool showSaveModal = false;

    if (ImGui::BeginPopupModal("MaterialEditorModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    if(hasOverrides) {
        MaterialParameters params = comp.GetMaterialInstance().getOverrideParameters();
        bool paramsModified = false;

        // Base Properties
        if(ImGui::CollapsingHeader("Base Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            paramsModified |= ImGui::ColorEdit4("Base Color", &params.baseColor.x);
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("RGB: Color, A: Opacity (Premultiplied)");
            }
        }

        // Light Properties
        if(ImGui::CollapsingHeader("Light Interaction", ImGuiTreeNodeFlags_DefaultOpen)) {
            uint32_t flags = comp.GetMaterialInstance().getOverrideFlags();

            bool receivesLight = (flags & MaterialFlags::ReceivesLight) != 0;
            if(ImGui::Checkbox("Receives Light", &receivesLight)) {
                comp.GetMaterialInstance().setOverrideFlag(MaterialFlags::ReceivesLight, receivesLight);
            }
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable for objects affected by scene lighting");
            }

            bool occludesLight = (flags & MaterialFlags::OccludesLight) != 0;
            if(ImGui::Checkbox("Occludes Light", &occludesLight)) {
                comp.GetMaterialInstance().setOverrideFlag(MaterialFlags::OccludesLight, occludesLight);
            }
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable for objects that cast shadows");
            }
        }

        // Render Properties
        if(ImGui::CollapsingHeader("Render Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
            uint32_t flags = comp.GetMaterialInstance().getOverrideFlags();

            bool solidColor = (flags & MaterialFlags::SolidColor) != 0;
            if(ImGui::Checkbox("Solid Color", &solidColor)) {
                comp.GetMaterialInstance().setOverrideFlag(MaterialFlags::SolidColor, solidColor);
            }
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Uses base color instead of texture");
            }

			bool repeating = (flags & MaterialFlags::Repeating) != 0;
            if (ImGui::Checkbox("Repeating", &repeating)) {
                comp.GetMaterialInstance().setOverrideFlag(MaterialFlags::Repeating, repeating);
            }
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Texture repeats instead of being to scale");
			}
			if (repeating) {
				paramsModified |= ImGui::DragFloat2("Tile Scale", &params.tiling.x, 0.1f, 0.0f, 10.0f);
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("The scale of the repeating texture");
				}
			}
        }

        if(paramsModified) {
            comp.GetMaterialInstance().setParameterOverrides(params);
        }

        ImGui::Separator();

        if(ImGui::Button("Save as New Material...")) {
            showSaveModal = true;
            ImGui::OpenPopup("SaveMaterialModal");
            materialNameBuffer[0] = '\0';
        }

        ImGui::SameLine();
        if(ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndPopup();
    }

    // Save Material Modal
    if (showSaveModal)
    {
        ImGui::OpenPopup("SaveMaterialModal");
        showSaveModal = false;
    }

    if (ImGui::BeginPopupModal("SaveMaterialModal", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter new material name:");
        ImGui::InputText("##materialname", materialNameBuffer, sizeof(materialNameBuffer));

        bool nameExists = ST<MaterialSystem>::Get()->materialExists(materialNameBuffer);
        if(nameExists) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                             "Warning: Material with this name already exists!");
        }

        bool canSave = strlen(materialNameBuffer) > 0 && !nameExists;
        
        if(ImGui::Button("Save", ImVec2(120, 0)) && canSave) {
            ST<MaterialSystem>::Get()->createMaterial(
                materialNameBuffer,
                comp.GetMaterialInstance().getOverrideFlags()
            );

            ST<MaterialSystem>::Get()->updateMaterialParameters(
                materialNameBuffer,
                comp.GetMaterialInstance().getOverrideParameters()
            );

            comp.GetMaterialInstance() = MaterialInstance(materialNameBuffer);
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        
        if(ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
#endif