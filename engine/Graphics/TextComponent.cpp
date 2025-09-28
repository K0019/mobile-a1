/******************************************************************************/
/*!
\file   TextComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of text component class

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TextComponent.h"
#include "GUICollection.h"

#include "ResourceManager.h"

TextComponent::TextComponent()
    : TextComponent{ "Arial", "Default Text" }
{
}

TextComponent::TextComponent(const std::string& fontName, Vec4 color)
    : fontNameHash{ util::GenHash(fontName) }
    , textString("")
    , color{ color }
{
}
TextComponent::TextComponent(const std::string& fontName, const std::string& text, Vec4 color)
    : fontNameHash{ util::GenHash(fontName) }
    , textString{ text }
    , color{ color }
{
}
TextComponent::TextComponent(size_t fontNameHash, Vec4 color)
    : fontNameHash{ fontNameHash }
    , textString("")
    , color{ color }
{
}
TextComponent::TextComponent(size_t fontNameHash, const std::string& text, Vec4 color)
    : fontNameHash{ fontNameHash }
    , textString{ text }
    , color{ color }
{
}
size_t TextComponent::GetFontHash() const
{
    return this->fontNameHash;
}

const std::string& TextComponent::GetText() const
{
    return this->textString;
}

void TextComponent::SetText(const std::string& text)
{
    this->textString = text;
    //dirtyTransform = true;
}
const Vec4& TextComponent::GetColor() const
{
    return this->color;
}

void TextComponent::SetColor(const Vec4& newColor)
{
    this->color = newColor;
}

Transform TextComponent::GetWorldTextTransform() const
{
    return worldTransform;
}

Vec2 TextComponent::GetTextStart() const {
    return textStart;
}

TextComponent::TextAlignment TextComponent::GetAlignment() const { return static_cast<TextAlignment>(alignment); }

void TextComponent::SetAlignment(TextAlignment newAlignment) { 
    alignment = static_cast<int>(newAlignment);
    CalculateWorldTransform();
}

bool TextComponent::isUI() const
{
    return UI;
}

void TextComponent::CalculateWorldTransform()
{
    CONSOLE_LOG_UNIMPLEMENTED() << "Text calculate world transform";

        //const auto& atlas = ResourceManagerOld::GetFont(GetFontHash());
        //const auto& transform = ecs::GetEntityTransform(this);
        //uint32_t previousChar = 0;
        //Vec2 position{transform.GetWorldPosition()};
        //Vec2 scale{transform.GetWorldScale()};
        //float minDescent = atlas.descender * scale.y;

        //// Calculate total width of the text including offsets
        //float totalWidth = 0.0f;
        //float maxAscent = 0.0f;
        //float maxDescent = 0.0f;

        //// First pass: calculate total dimensions
        //for(const char& c : textString) {
        //    uint32_t currentChar = static_cast<uint32_t>(c);
        //    if(currentChar < FontAtlas::FIRST_CHAR || currentChar > FontAtlas::LAST_CHAR) continue;

        //    size_t glyphIndex = currentChar - FontAtlas::FIRST_CHAR;
        //    const Glyph& glyph = atlas.glyphs[glyphIndex];

        //    if(previousChar != 0) {
        //        float kerning = atlas.getKerning(previousChar, currentChar);
        //        totalWidth += kerning * scale.x;
        //    }

        //    totalWidth += (glyph.advance * scale.x);
        //    maxAscent = std::max(maxAscent, (glyph.planeBounds[1].y) * scale.y);
        //    if(c != ' ') {
        //        maxDescent = std::max(maxDescent, std::max(-glyph.planeBounds[0].y * scale.y, minDescent));
        //    }

        //    previousChar = currentChar;
        //}
        //maxDescent = std::max(maxDescent, minDescent);
        //float totalHeight = maxAscent + maxDescent;

        //// Calculate text starting position based on alignment
        //textStart = position;
        //switch(GetAlignment()) {
        //    case TextAlignment::Left:
        //        // For left alignment, start at the entity's center
        //        break;
        //    case TextAlignment::Center:
        //        // For center alignment, move left by half the width
        //        textStart.x -= totalWidth / 2.0f;
        //        break;
        //    case TextAlignment::Right:
        //        // For right alignment, move left by full width
        //        textStart.x -= totalWidth;
        //        break;
        //}
        //textStart.y -= totalHeight / 2.0f; // Center vertically

        //Vec2 boundingBoxPos = position;
        //switch(GetAlignment()) {
        //    case TextAlignment::Left:
        //        // Move bounding box right by half width to center it over the text
        //        boundingBoxPos.x += totalWidth / 2.0f;
        //        break;
        //    case TextAlignment::Center:
        //        // Center alignment - bounding box stays at entity position
        //        break;
        //    case TextAlignment::Right:
        //        // Move bounding box left by half width to center it over the text
        //        boundingBoxPos.x -= totalWidth / 2.0f;
        //        break;
        //}
        //CONSOLE_LOG_UNIMPLEMENTED() << "Text world transform";
        ///*worldTransform.SetWorldPosition(boundingBoxPos);
        //worldTransform.SetWorldScale({totalWidth, totalHeight});
        //worldTransform.SetZPos(transform.GetZPos());*/
}
void TextComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    ImGui::Text("Font");
    ImGui::SameLine();
    auto& currentFontName = ResourceManagerOld::GetResourceName(fontNameHash);
    if(ImGui::BeginCombo("Font", currentFontName.c_str()))
    {
        //const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();
        /*for(const auto& fontName : fontAtlases | std::views::keys)
        {
            bool isSelected = (fontName == currentFontName);

            if(ImGui::Selectable(fontName.c_str(), isSelected))
            {
                fontNameHash = util::GenHash(fontName);
                CalculateWorldTransform(); // Recalculate after font change
            }
            // Set initial focus when opening the combo
            if(isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }*/
        ImGui::EndCombo();
    }
    const char* alignmentItems[] = { "Left", "Center", "Right" };
    int currentAlignment = static_cast<int>(GetAlignment());
    if(ImGui::Combo("Alignment", &currentAlignment, alignmentItems, IM_ARRAYSIZE(alignmentItems))) {
        SetAlignment(static_cast<TextAlignment>(currentAlignment));
    }
    auto color = GetColor();
    if(ImGui::ColorEdit4("Color", &color.x))
    {
        SetColor(color);
    }
    ImGui::Checkbox("UI Element", &UI);
    const auto& text = GetText();
    char buffer[256]; // Adjust size accordingly if text can be longer
    strncpy_s(buffer, text.c_str(), sizeof(buffer));
    if(ImGui::InputText("Text", buffer, sizeof(buffer)))
    {
        SetText(buffer); // Update the component's text after editing
        CalculateWorldTransform();
    }
#endif
}

FPSTextComponent::FPSTextComponent()
    : doDisplay{ true }
{
}

bool FPSTextComponent::GetDoDisplay() const
{
    return doDisplay;
}

void FPSTextComponent::SetDoDisplay(bool newDoDisplay)
{
    doDisplay = newDoDisplay;
}

void FPSTextComponent::EditorDraw()
{
    gui::Checkbox("Display", &doDisplay);
}


#pragma region Scripting

/*****************************************************************//*!
\brief
    Copy of the equivalent Text structure in the C# project.
*//******************************************************************/
struct CS_Text
{
    ecs::EntityHandle entity;
    Vec4 color;
    char text[256];
};

/*****************************************************************//*!
\brief
    Returns the TextComponent of an entity to C# side.
\param entity
    The entity.
\return
    The text component of the entity. If it doesn't exist, returns
    a Text structure with no entity registered.
*//******************************************************************/
SCRIPT_CALLABLE CS_Text CS_GetComp_Text(ecs::EntityHandle entity)
{
    util::AssertEntityHandleValid(entity);

    if (auto textComp{ entity->GetComp<TextComponent>() })
    {
        CS_Text ret{ entity, textComp->GetColor() };
        textComp->GetText().copy(ret.text, 256);
        return ret;
    }

    return CS_Text{ nullptr };
}

//! Setters for setting variables within TextComponent.
SCRIPT_CALLABLE_COMP_SETTER(TextComponent, CS_Text_SetText, const char*, SetText)
SCRIPT_CALLABLE_COMP_SETTER(TextComponent, CS_Text_SetColor, Vec4, SetColor)

#pragma endregion // Scripting
