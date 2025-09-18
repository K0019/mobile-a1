/******************************************************************************/
/*!
\file   TextSystem.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Declaration of the TextSystem class, which renders text using a font loaded with FreeType.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ResourceManager.h"
#include "TextComponent.h"

class Renderer;
// A renderer class for rendering text displayed by a font loaded using the 
// FreeType library. A single font is loaded, processed into a list of Character
// items for later rendering.
class TextSystem : public ecs::System<TextSystem, TextComponent>
{
public:
    /**
     * \brief Constructor for the TextSystem class.
     */
    TextSystem();

    Renderer* renderer;
    //void UpdateWindowSize(unsigned int width, unsigned int height);
    /**
     * \brief Renders a string of text using the precompiled list of characters.
     * \param textComp The TextComponent containing the text to be rendered.
     */
    void DrawTextComp(TextComponent& textComp);
    // render state
   /*void RenderText(Font& font, const std::string& text, const Transform& transform, const glm::vec3& color = glm::vec3(1.0f));

    void RenderText(Font& font, const std::string& text, glm::vec2 position, const glm::vec2& scale, const glm::vec3& color =
                      glm::vec3(1.0f)) const;

    void InitRenderData();*/
    // render state
    /*size_t ShaderHash;
    unsigned int VAO, VBO;*/
};

class FPSTextSystem : public ecs::System<FPSTextSystem, FPSTextComponent>
{
public:
    FPSTextSystem();

    bool PreRun() final;

private:
    void UpdateFPSText(FPSTextComponent& comp);

    std::ostringstream ss;
    //! Whether to toggle the display of FPS text.
    bool doToggle;

};