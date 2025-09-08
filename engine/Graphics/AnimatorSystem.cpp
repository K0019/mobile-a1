/******************************************************************************/
/*!
\file   AnimatorSystem.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of AnimationSystem.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AnimatorSystem.h"

AnimatorSystem::AnimatorSystem() : System_Internal(&AnimatorSystem::UpdateAnimatorComp) {}

void AnimatorSystem::UpdateAnimatorComp(AnimatorComponent& animatorComp)
{
    if(animatorComp.GetCurrentAnimationName().empty() || !ResourceManager::AnimationExists(animatorComp.GetCurrentAnimationName()) || !animatorComp.IsPlaying()) return;
    auto entity = ecs::GetEntity(&animatorComp);
    if(!entity->GetComp<RenderComponent>())
    {
        return;
    }
    float dt{ GameTime::FixedDt() };
    const Animation& anim = ResourceManager::GetAnimation(animatorComp.GetCurrentAnimationName());
    const FrameData& frameData = anim.frames[animatorComp.GetCurrentFrame()];

    animatorComp.currentFrameTime += dt * animatorComp.playbackSpeed;

    if(animatorComp.currentFrameTime >= frameData.duration) {
        animatorComp.currentFrameTime -= frameData.duration;
        animatorComp.currentFrame++;

        // Handle animation completion
        if(animatorComp.currentFrame >= anim.totalFrames) {
            if(animatorComp.IsLooping()) {
                animatorComp.currentFrame = 0;
            }
            else {
                animatorComp.currentFrame = anim.totalFrames - 1;
                animatorComp.isPlaying = false;
            }
        }

    }
    const FrameData& new_frameData = anim.frames[animatorComp.currentFrame];
    entity->GetComp<RenderComponent>()->SetSpriteID(new_frameData.spriteID);
}
