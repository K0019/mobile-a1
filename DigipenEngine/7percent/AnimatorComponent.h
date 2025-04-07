#pragma once
/******************************************************************************/
/*!
\file   AnimatorComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
The component class that handles animation playback. Will interface with the render component to update it depending on what frame the animation is on.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "RenderComponent.h"
#include "ResourceManager.h"

class AnimatorComponent : public IRegisteredComponent<AnimatorComponent>
#ifdef IMGUI_ENABLED
    , IEditorComponent<AnimatorComponent>
#endif
{
    friend class AnimatorSystem;
public:
    AnimatorComponent();
    AnimatorComponent(const std::string& animationName, bool autoPlay = true);
    AnimatorComponent(size_t animationHash, bool autoPlay = true);

    // Core animation controls
    void Play();
    void Pause();
    void Reset();

    // Animation state
    bool IsPlaying() const;
    bool IsLooping() const;
    float GetSpeed() const;
    size_t GetCurrentFrame() const;
    const std::string& GetCurrentAnimationName() const;
    float GetAnimationLength() const;

    // Animation properties
    void SetAnimation(const std::string& animationName);
    void SetAnimation(size_t animationHash);
    void SetLooping(bool loop);
    void SetSpeed(float speed);
    void SetFrame(size_t frameIndex);

    

private:
    // Editor integration
#ifdef IMGUI_ENABLED
    static void EditorDraw(AnimatorComponent& comp);
#endif

    size_t  currentAnimationHash{0};        // Hash of current animation name
    size_t  currentFrame{0};                // Current frame index
    float   currentFrameTime{0.0f};          // Time accumulated in current frame
    float   playbackSpeed{1.0f};             // Animation playback speed multiplier
    bool    isPlaying{false};                 // Is animation currently playing
    bool    isLooping{true};                  // Should animation loop

    property_vtable()
};
property_begin(AnimatorComponent)
{
    property_var(currentAnimationHash),
    property_var(currentFrame),
    property_var(currentFrameTime),
    property_var(playbackSpeed),
    property_var(isPlaying),
    property_var(isLooping)
}
property_vend_h(AnimatorComponent)

