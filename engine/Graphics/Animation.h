/******************************************************************************/
/*!
\file   Animation.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration for the Animation class. It manages animation frames and binding textures. 

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/**
 * @struct FrameData
 * @brief Holds data for each frame in the animation.
 */
struct FrameData {
    size_t spriteID; ///< ID of the sprite in the sprite sheet.
    float duration;   ///< Time each frame is displayed.
};


/**
 * @class Animation
 * @brief Manages animation frames and binding textures.
 */
class Animation : public ISerializeable
{
public:
    unsigned int Width; ///< Width of the loaded image in pixels.
    unsigned int Height; ///< Height of the loaded image in pixels.
    size_t totalFrames; ///< Total number of frames in the sprite sheet.
    std::vector<FrameData> frames; ///< Data for each frame.

    /**
     * @brief Constructs an Animation object.
     * @param frames The total number of frames in the animation.
     */
    explicit Animation(size_t frames);

    Animation();

public:
    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;

private:
    property_vtable()
};
property_begin(Animation)
{
}
property_vend_h(Animation)
