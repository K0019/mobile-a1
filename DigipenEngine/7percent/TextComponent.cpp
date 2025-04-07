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

#include "ResourceManager.h"

TextComponent::TextComponent()
    : TextComponent{ "Arial", "Default Text" }
{
}

TextComponent::TextComponent(const std::string& fontName, Vector4 color) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    fontNameHash{ util::GenHash(fontName) },
    textString(""),
    color{ color }
{
}
TextComponent::TextComponent(const std::string& fontName, const std::string& text, Vector4 color) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw), 
#endif
    fontNameHash{ util::GenHash(fontName) }, textString{ text }, color{ color }
{
}
TextComponent::TextComponent(size_t fontNameHash, Vector4 color) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    fontNameHash{ fontNameHash }, textString(""), color{ color }
{
}
TextComponent::TextComponent(size_t fontNameHash, const std::string& text, Vector4 color) :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    fontNameHash{ fontNameHash }, textString{ text }, color{ color }
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
const Vector4& TextComponent::GetColor() const
{
    return this->color;
}

void TextComponent::SetColor(const Vector4& newColor)
{
    this->color = newColor;
}

Transform TextComponent::GetWorldTextTransform() const
{
    return worldTransform;
}

Vector2 TextComponent::GetTextStart() const {
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
        const auto& atlas = ResourceManager::GetFont(GetFontHash());
        const auto& transform = ecs::GetEntityTransform(this);
        uint32_t previousChar = 0;
        Vector2 position{transform.GetWorldPosition()};
        Vector2 scale{transform.GetWorldScale()};
        float minDescent = atlas.descender * scale.y;

        // Calculate total width of the text including offsets
        float totalWidth = 0.0f;
        float maxAscent = 0.0f;
        float maxDescent = 0.0f;

        // First pass: calculate total dimensions
        for(const char& c : textString) {
            uint32_t currentChar = static_cast<uint32_t>(c);
            if(currentChar < FontAtlas::FIRST_CHAR || currentChar > FontAtlas::LAST_CHAR) continue;

            size_t glyphIndex = currentChar - FontAtlas::FIRST_CHAR;
            const Glyph& glyph = atlas.glyphs[glyphIndex];

            if(previousChar != 0) {
                float kerning = atlas.getKerning(previousChar, currentChar);
                totalWidth += kerning * scale.x;
            }

            totalWidth += (glyph.advance * scale.x);
            maxAscent = std::max(maxAscent, (glyph.planeBounds[1].y) * scale.y);
            if(c != ' ') {
                maxDescent = std::max(maxDescent, std::max(-glyph.planeBounds[0].y * scale.y, minDescent));
            }

            previousChar = currentChar;
        }
        maxDescent = std::max(maxDescent, minDescent);
        float totalHeight = maxAscent + maxDescent;

        // Calculate text starting position based on alignment
        textStart = position;
        switch(GetAlignment()) {
            case TextAlignment::Left:
                // For left alignment, start at the entity's center
                break;
            case TextAlignment::Center:
                // For center alignment, move left by half the width
                textStart.x -= totalWidth / 2.0f;
                break;
            case TextAlignment::Right:
                // For right alignment, move left by full width
                textStart.x -= totalWidth;
                break;
        }
        textStart.y -= totalHeight / 2.0f; // Center vertically

        Vector2 boundingBoxPos = position;
        switch(GetAlignment()) {
            case TextAlignment::Left:
                // Move bounding box right by half width to center it over the text
                boundingBoxPos.x += totalWidth / 2.0f;
                break;
            case TextAlignment::Center:
                // Center alignment - bounding box stays at entity position
                break;
            case TextAlignment::Right:
                // Move bounding box left by half width to center it over the text
                boundingBoxPos.x -= totalWidth / 2.0f;
                break;
        }
        worldTransform.SetWorldPosition(boundingBoxPos);
        worldTransform.SetWorldScale({totalWidth, totalHeight});
        worldTransform.SetZPos(transform.GetZPos());
    }
#ifdef IMGUI_ENABLED
void TextComponent::EditorDraw(TextComponent& comp)
{
    ImGui::Text("Font");
    ImGui::SameLine();
    auto& currentFontName = ResourceManager::GetResourceName(comp.fontNameHash);
    if(ImGui::BeginCombo("Font", currentFontName.c_str()))
    {
        const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();
        for(const auto& fontName : fontAtlases | std::views::keys)
        {
            bool isSelected = (fontName == currentFontName);

            if(ImGui::Selectable(fontName.c_str(), isSelected))
            {
                comp.fontNameHash = util::GenHash(fontName);
                comp.CalculateWorldTransform(); // Recalculate after font change
            }
            // Set initial focus when opening the combo
            if(isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    const char* alignmentItems[] = { "Left", "Center", "Right" };
    int currentAlignment = static_cast<int>(comp.GetAlignment());
    if(ImGui::Combo("Alignment", &currentAlignment, alignmentItems, IM_ARRAYSIZE(alignmentItems))) {
        comp.SetAlignment(static_cast<TextAlignment>(currentAlignment));
    }
    auto color = comp.GetColor();
    if(ImGui::ColorEdit4("Color", &color.x))
    {
        comp.SetColor(color);
    }
    auto UI = comp.UI;
    if(ImGui::Checkbox("UI Element", &UI))
    {
        comp.UI = UI;
    }
    const auto& text = comp.GetText();
    char buffer[256]; // Adjust size accordingly if text can be longer
    strncpy_s(buffer, text.c_str(), sizeof(buffer));
    if(ImGui::InputText("Text", buffer, sizeof(buffer)))
    {
        comp.SetText(buffer); // Update the component's text after editing
        comp.CalculateWorldTransform();
    }
}
#endif

FPSTextComponent::FPSTextComponent() :
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
    doDisplay{ true }
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

#ifdef IMGUI_ENABLED
void FPSTextComponent::EditorDraw(FPSTextComponent& comp)
{
    ImGui::Checkbox("Display", &comp.doDisplay);
}
#endif
