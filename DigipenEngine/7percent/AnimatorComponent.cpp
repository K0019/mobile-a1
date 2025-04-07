/******************************************************************************/
/*!
\file   AnimatorComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of AnimationComponent.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AnimatorComponent.h"
#include "RenderComponent.h"
AnimatorComponent::AnimatorComponent()
    : AnimatorComponent(0) {
}

AnimatorComponent::AnimatorComponent(const std::string& animationName, bool autoPlay) : 
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw), 
#endif
    isPlaying(autoPlay)
{
    SetAnimation(animationName);
}

AnimatorComponent::AnimatorComponent(size_t animationHash, bool autoPlay) : 
#ifdef IMGUI_ENABLED
    REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw), 
#endif
    isPlaying(autoPlay) {
    SetAnimation(animationHash);
}

void AnimatorComponent::Play() {
    isPlaying = true;
}

void AnimatorComponent::Pause() {
    isPlaying = false;
}

void AnimatorComponent::Reset() {
    currentFrame = 0;
    currentFrameTime = 0.0f;

    // Fix: Force reset of the current frame
    if (ecs::CompHandle<RenderComponent> renderComp{ ecs::GetEntity(this)->GetComp<RenderComponent>() })
    {
        const Animation& anim = ResourceManager::GetAnimation(GetCurrentAnimationName());
        const FrameData& new_frameData = anim.frames[currentFrame];
        renderComp->SetSpriteID(new_frameData.spriteID);
    }
}

bool AnimatorComponent::IsPlaying() const {
    return isPlaying;
}

bool AnimatorComponent::IsLooping() const {
    return isLooping;
}

float AnimatorComponent::GetSpeed() const {
    return playbackSpeed;
}

size_t AnimatorComponent::GetCurrentFrame() const {
    return currentFrame;
}

const std::string& AnimatorComponent::GetCurrentAnimationName() const {
    return ResourceManager::GetResourceName(currentAnimationHash);
}

float AnimatorComponent::GetAnimationLength() const
{
    if (!ResourceManager::AnimationExists(ResourceManager::GetResourceName(currentAnimationHash)))
        return 0.0f;

    float totalTime{};
    for (const FrameData& frame : ResourceManager::GetAnimation(currentAnimationHash).frames)
        totalTime += frame.duration;
    return totalTime;
}

void AnimatorComponent::SetAnimation(const std::string& animationName) {
    SetAnimation(util::GenHash(animationName));
}

void AnimatorComponent::SetAnimation(size_t animationHash) {
    if(!ResourceManager::AnimationExists(ResourceManager::GetResourceName(animationHash))) {
        return;
    }

    if(currentAnimationHash != animationHash) {
        currentAnimationHash = animationHash;

        //TODO RIP OFF THE BANDAID
        //Reset();
        currentFrame = 0;
        currentFrameTime = 0.0f;

    }
}

void AnimatorComponent::SetLooping(bool loop) {
    isLooping = loop;
}

void AnimatorComponent::SetSpeed(float speed) {
    playbackSpeed = speed;
}

void AnimatorComponent::SetFrame(size_t frameIndex) {
    if(!currentAnimationHash) {
        return;
    }

    const Animation& animation = ResourceManager::GetAnimation(currentAnimationHash);
    if(frameIndex < animation.totalFrames) {
        currentFrame = frameIndex;
        currentFrameTime = 0.0f;
    }
}
#ifdef IMGUI_ENABLED
void AnimatorComponent::EditorDraw(AnimatorComponent& comp) {
    ecs::EntityHandle this_entity = ecs::GetEntity(&comp);
    if(!this_entity->GetComp<RenderComponent>())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.0f, 1.0f), "No Render Component!");
        return;
    }


    if(!comp.currentAnimationHash) {
        ImGui::Text("No animation set");
        ImGui::Text("Drag an Animation from the browser to assign it");
        if(ImGui::BeginDragDropTarget())
        {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIM_HASH"))
            {
                comp.SetAnimation(*static_cast<size_t*>(payload->Data));
            }
            ImGui::EndDragDropTarget();
        }
        return;
    }

    const std::string& animName = ResourceManager::GetResourceName(comp.currentAnimationHash);
    const Animation& animation = ResourceManager::GetAnimation(comp.currentAnimationHash);
    ImGui::Text("Drag an Animation from the browser to assign it");
    if(ImGui::BeginDragDropTarget())
    {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANIM_HASH"))
        {
            comp.SetAnimation(*static_cast<size_t*>(payload->Data));
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::Text("Animation: %s", animName.c_str());

    ImGui::Text("Frame: %zu / %zu", comp.currentFrame + 1, animation.totalFrames);
    ImGui::Text("Frame Time: %.2f", comp.currentFrameTime);

    if(ImGui::Button(comp.isPlaying ? "Pause" : "Play")) {
        comp.isPlaying ? comp.Pause() : comp.Play();
    }
    ImGui::SameLine();
    if(ImGui::Button("Reset")) {
        comp.Reset();
    }

    ImGui::Checkbox("Looping", &comp.isLooping);
    ImGui::DragFloat("Speed", &comp.playbackSpeed, 0.1f, 0.1f, 10.0f);

    // Frame scrubber
    int frame = static_cast<int>(comp.currentFrame);
    if(ImGui::SliderInt("Frame", &frame, 0, static_cast<int>(animation.totalFrames) - 1)) {
        comp.SetFrame(static_cast<size_t>(frame));
    }
}
#endif