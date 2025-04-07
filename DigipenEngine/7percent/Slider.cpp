#include "Slider.h"

/******************************************************************************/
/*!
\file   Slider.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   13/03/2025

\author Chua Wen Shing Bryan (100%)
\par    email: c.wenshingbryan\@digipen.edu
\par    DigiPen login: c.wenshingbryan

\brief
    Cpp for slider component to adjust volume

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

SliderComponent::SliderComponent() :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    isPressed(false), spriteID_Pressed(0), spriteID_Unpressed(0), sliderValue(0.5f), setInitial(false), sound(static_cast<int>(VOLUME::SFX))
{

}


bool SliderComponent::GetIsPressed() { return isPressed; }
void SliderComponent::SetIsPressed(bool val) { isPressed = val; }
size_t	SliderComponent::GetSpriteIDPressed() { return spriteID_Pressed; }
size_t  SliderComponent::GetSpriteIDUnPressed() { return spriteID_Unpressed; }
void SliderComponent::SetSpriteIDPressed(size_t val) { spriteID_Pressed = val; }
void SliderComponent::SetSpriteIDUnPressed(size_t val) { spriteID_Unpressed = val; }
float SliderComponent::GetSliderValue() { return sliderValue; }
void SliderComponent::SetSliderValue(float val) { sliderValue = val;}
void SliderComponent::SetInitialSpriteSet(bool val) { setInitial = val; }
bool SliderComponent::GetInitialSpriteSet() { return setInitial; }


void SliderComponent::OnAttached()
{
   
}

void SliderComponent::OnDetached()
{
   
}


#ifdef IMGUI_ENABLED
void SliderComponent::EditorDraw(SliderComponent& comp)
{
    auto& spritePressed = ResourceManager::GetSprite(comp.GetSpriteIDPressed());
    auto& spriteUnPressed = ResourceManager::GetSprite(comp.GetSpriteIDUnPressed());


  
    { // Sound Type
#define X(enumName, strName) strName,
        static const char* soundTypes[] = { M_SOUND_TYPE };
#undef X
        gui::Combo roleCombo{ "Sound Type", soundTypes, std::size(soundTypes), &comp.sound };
    }

    

    ImGui::Text("Drag an Sprite from the browser to assign Unpressed");

    // Existing sprite handling code...
    ImGui::ImageButton("Preview_UnPressed", VulkanManager::Get().VkTextureManager().getTexture(spriteUnPressed.textureID).ImGui_handle, ImVec2(100, 100),
        ImVec2(spriteUnPressed.texCoords.x, spriteUnPressed.texCoords.y),
        ImVec2(spriteUnPressed.texCoords.z, spriteUnPressed.texCoords.w));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_ID"))
        {
            comp.SetSpriteIDUnPressed(*static_cast<size_t*>(payload->Data));
            if (ecs::CompHandle<RenderComponent> renderComp{ ecs::GetEntity(&comp)->GetComp<RenderComponent>() })
                renderComp->SetSpriteID(comp.GetSpriteIDUnPressed());
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::ImageButton("Preview_Pressed", VulkanManager::Get().VkTextureManager().getTexture(spritePressed.textureID).ImGui_handle, ImVec2(100, 100),
        ImVec2(spritePressed.texCoords.x, spritePressed.texCoords.y),
        ImVec2(spritePressed.texCoords.z, spritePressed.texCoords.w));
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPRITE_ID"))
        {
            comp.SetSpriteIDPressed(*static_cast<size_t*>(payload->Data));
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::Text("Drag an Sprite from the browser to assign Pressed");
}
#endif

SliderSystem::SliderSystem()
    : System_Internal(&SliderSystem::UpdateSliderComp)
{
}


void SliderSystem::UpdateSliderComp(SliderComponent& comp)
{
    bool onClicked{ Input::GetKeyPressed(KEY::M_LEFT) };
    bool isReleased{ Input::GetKeyReleased(KEY::M_LEFT) };
    ecs::EntityHandle sliderButtonEntity{ ecs::GetEntity(&comp) };
    ecs::EntityHandle barEntity{ ecs::GetEntity(sliderButtonEntity->GetCompInParents<SliderBar>()) };
    float barEntityMin = barEntity->GetTransform().GetWorldPosition().x - (barEntity->GetTransform().GetWorldScale().x / 2);
    float barEntityMax = barEntity->GetTransform().GetWorldPosition().x + (barEntity->GetTransform().GetWorldScale().x / 2);
    float sliderButtonY = sliderButtonEntity->GetTransform().GetWorldPosition().y;
    float percentage = 0;

    if (!comp.GetInitialSpriteSet())
    {
        if (ecs::CompHandle<RenderComponent> renderComp{ ecs::GetEntity(&comp)->GetComp<RenderComponent>() })
        {

            switch (static_cast<VOLUME>(comp.sound))
            {
            case VOLUME::SFX:
                percentage = ST<GameSettings>::Get()->m_volumeSFX;
                break;
            case VOLUME::BGM:
                percentage = ST<GameSettings>::Get()->m_volumeBGM;
                break;

            default:
                break;
            }

            float sliderButtonX = (percentage * (barEntityMax - barEntityMin)) + barEntityMin;
            sliderButtonEntity->GetTransform().SetWorldPosition({ sliderButtonX, sliderButtonY });
        }
        comp.SetInitialSpriteSet(true);
    }

    if (isReleased)
    {
        comp.SetIsPressed(false);
        if (ecs::CompHandle<RenderComponent> renderComp{ ecs::GetEntity(&comp)->GetComp<RenderComponent>() })
            renderComp->SetSpriteID(comp.GetSpriteIDUnPressed());
    }
        

    if (!onClicked && !comp.GetIsPressed())
        return;

    // Get whether the cursor is on the button. We'll need this later.
    bool isCursorOnButton{ util::IsPointInside(Input::GetMousePosWorld(), ecs::GetEntityTransform(&comp)) };

    if (isCursorOnButton)
    {
        comp.SetIsPressed(true);
    }

    if (comp.GetIsPressed())
    {
        onSliderHold(comp);
    }

}

void SliderSystem::onSliderHold(SliderComponent& comp)
{
    if (ecs::CompHandle<RenderComponent> renderComp{ ecs::GetEntity(&comp)->GetComp<RenderComponent>() })
        renderComp->SetSpriteID(comp.GetSpriteIDPressed());

    ecs::EntityHandle sliderButtonEntity{ ecs::GetEntity(&comp) };
    ecs::EntityHandle barEntity{ ecs::GetEntity(sliderButtonEntity->GetCompInParents<SliderBar>()) };
    float barEntityMin = barEntity->GetTransform().GetWorldPosition().x - (barEntity->GetTransform().GetWorldScale().x / 2);
    float barEntityMax = barEntity->GetTransform().GetWorldPosition().x + (barEntity->GetTransform().GetWorldScale().x / 2);
    float sliderButtonY = sliderButtonEntity->GetTransform().GetWorldPosition().y;
    float sliderButtonX = 0;
    Vector2 mou = Input::GetMousePosWorld();
  

    //If mous pos is more than the end of the bar or less than start of the bar
    if (mou.x > barEntityMax || mou.x < barEntityMin)
    {
        if (mou.x > barEntityMax) 
            sliderButtonEntity->GetTransform().SetWorldPosition({ barEntityMax, sliderButtonY });
        else 
            sliderButtonEntity->GetTransform().SetWorldPosition({ barEntityMin, sliderButtonY });

        sliderButtonX = (mou.x > barEntityMax) ? barEntityMax : barEntityMin;
    }
    else
    {
        sliderButtonEntity->GetTransform().SetWorldPosition({ mou.x, sliderButtonY });
        sliderButtonX = mou.x;
    }

    float sliderPercentage = (std::fabs(sliderButtonX - barEntityMin) / std::fabs(barEntityMax - barEntityMin));

    switch (static_cast<VOLUME>(comp.sound))
    {
        case VOLUME::SFX :
            ST<GameSettings>::Get()->m_volumeSFX = sliderPercentage;
            break;
        case VOLUME::BGM:
            ST<GameSettings>::Get()->m_volumeBGM = sliderPercentage;
            break;

        default:
            break;
    }
    
    ST<GameSettings>::Get()->ApplyVolumes();
    ST<GameSettings>::Get()->Save();

    
}