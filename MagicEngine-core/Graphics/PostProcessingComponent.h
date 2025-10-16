#pragma once
#include "IRegisteredComponent.h"
#include "IEditorComponent.h"

class PostProcessingComponent
    : public IRegisteredComponent<PostProcessingComponent>
    , public IEditorComponent<PostProcessingComponent>
    , public ecs::IComponentCallbacks
{
    friend class PostProcessingSystem;
public:
    PostProcessingComponent(bool bloomEnabled = true);
    bool bloomEnabled{ true };        // Whether bloom effect is enabled
    float bloomIntensity{ 0.5f };     // Intensity of bloom effect
    float bloomThreshold{ 0.8f };     // Brightness threshold for bloom
    float blurSize{ 2.0f };           // Size of blur for bloom effect

    bool vignetteEnabled;
    float vignetteIntensity;
    Vec4 vignetteColor;
    float vignetteRadius;
    float vignetteSmoothness;

    void OnDetached() override;
    private:
    // Editor integration
    virtual void EditorDraw() override;

    property_vtable()
};

property_begin(PostProcessingComponent)
{
    property_var(bloomEnabled),
    property_var(bloomIntensity),
    property_var(bloomThreshold),
    property_var(blurSize),
    property_var(vignetteEnabled),
    property_var(vignetteIntensity),
    property_var(vignetteColor),
    property_var(vignetteRadius),
    property_var(vignetteSmoothness)
}
property_vend_h(PostProcessingComponent)

class Renderer;

class PostProcessingSystem : public ecs::System<PostProcessingSystem, PostProcessingComponent>
{
public:
    PostProcessingSystem();
private:
    Renderer* renderer;
    void UpdatePostProcessingComp(PostProcessingComponent& ppComp);
};
