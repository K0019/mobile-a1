/******************************************************************************/
/*!
\file   TextComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Declaration of the TextComponent class, which stores text to be rendered on the screen.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

class TextComponent : public IRegisteredComponent<TextComponent>
#ifdef IMGUI_ENABLED
    , IEditorComponent<TextComponent>
#endif
{
  friend class TextSystem;
public:
    /**
     * \brief Default constructor for TextComponent.
     *        Used for compliance with IRegisteredComponent.
     */

 enum class TextAlignment {
    Left = 0,
    Center = 1,
    Right = 2
};

    explicit TextComponent();

    /**
     * \brief Constructor for TextComponent with font name and color.
     * \param fontName The name of the font.
     * \param color The color of the text.
     */
    TextComponent(const std::string& fontName, Vector4 color = glm::vec4{0.0f,0.0f,0.0f,1.0f});

    /**
     * \brief Constructor for TextComponent with font name, text, and color.
     * \param fontName The name of the font.
     * \param text The text to be rendered.
     * \param color The color of the text.
     */
    TextComponent(const std::string& fontName, const std::string& text, Vector4 color = glm::vec4{0.0f,0.0f,0.0f,1.0f});

    /**
     * \brief Constructor for TextComponent with font name hash and color.
     * \param fontNameHash The hash value of the font name.
     * \param color The color of the text.
     */
    TextComponent(size_t fontNameHash, Vector4 color = glm::vec4{0.0f,0.0f,0.0f,1.0f});

    /**
     * \brief Constructor for TextComponent with font name hash, text, and color.
     * \param fontNameHash The hash value of the font name.
     * \param text The text to be rendered.
     * \param color The color of the text.
     */
    TextComponent(size_t fontNameHash, const std::string& text, Vector4 color = glm::vec4{0.0f,0.0f,0.0f,1.0f});

    /**
     * \brief Get the hash value of the font name.
     * \return The hash value of the font name.
     */
    size_t GetFontHash() const;

    /**
     * \brief Get the text to be rendered.
     * \return The text to be rendered.
     */
    const std::string& GetText() const;

    /**
     * \brief Set the text to be rendered.
     * \param text The text to be rendered.
     */
    void SetText(const std::string& text);

    /**
     * \brief Get the color of the text.
     * \return The color of the text.
     */
    const Vector4& GetColor() const;

    /**
     * \brief Set the color of the text.
     * \param color The color of the text.
     */
    void SetColor(const Vector4& color);

    /**
     * \brief Get the world transform of the text.
     * \return The world transform of the text.
     */
    Transform GetWorldTextTransform() const;

    Vector2 GetTextStart() const;

    TextAlignment GetAlignment() const;

    void SetAlignment(TextAlignment newAlignment);

    bool isUI () const;

   private:
    size_t fontNameHash; ///< The hash value of the font name.
    std::string textString; ///< The text to be rendered.
    Vector4 color; ///< The color of the text.
    Transform worldTransform; ///< The world transform of the text.
    int alignment{1}; ///< The alignment of the text.
    Vector2 textStart; ///< The starting position of the text.
    bool UI = false; ///< Whether the text is UI text.

    void CalculateWorldTransform();

    /**
     * \brief Editor support function for drawing the TextComponent.
     * \param comp The TextComponent to be drawn.
     */
#ifdef IMGUI_ENABLED
    static void EditorDraw(TextComponent& comp);
#endif
    property_vtable()
};
property_begin(TextComponent)
{
    property_var(fontNameHash),
    property_var(textString),
    property_var(color),
    property_var(alignment),
    property_var(UI)
}
property_vend_h(TextComponent)

/*****************************************************************//*!
\class FPSTextComponent
\brief
    Identifies an entity as an entity displaying FPS with a text component.
*//******************************************************************/
class FPSTextComponent : public IRegisteredComponent<FPSTextComponent>
#ifdef IMGUI_ENABLED
    , IEditorComponent<FPSTextComponent>
#endif
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    FPSTextComponent();

    /*****************************************************************//*!
    \brief
        Gets whether this FPSTextComponent is set to do display.
    \return
        Whether this FPSTextComponent is set to do display.
    *//******************************************************************/
    bool GetDoDisplay() const;

    /*****************************************************************//*!
    \brief
        Sets whether this FPSTextComponent is set to do display.
    \param newDoDisplay
        Whether this FPSTextComponent is set to do display.
    *//******************************************************************/
    void SetDoDisplay(bool newDoDisplay);

private:
    //! Whether to display FPS text.
    bool doDisplay;

#ifdef IMGUI_ENABLED
    /*****************************************************************//*!
    \brief
        Draws this component's properties to editor.
    *//******************************************************************/
    static void EditorDraw(FPSTextComponent& comp);
#endif

    property_vtable()
};
property_begin(FPSTextComponent)
{
    property_var(doDisplay)
}
property_vend_h(FPSTextComponent)
