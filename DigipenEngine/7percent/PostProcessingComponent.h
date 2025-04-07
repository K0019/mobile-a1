#pragma once

class PostProcessingComponent : public IRegisteredComponent<PostProcessingComponent>,public ecs::IComponentCallbacks
#ifdef IMGUI_ENABLED
    , IEditorComponent<PostProcessingComponent>
#endif
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
    Vector4 vignetteColor;
    float vignetteRadius;
    float vignetteSmoothness;

    void OnDetached() override;
    private:
    // Editor integration
#ifdef IMGUI_ENABLED
    static void EditorDraw(PostProcessingComponent& comp);
#endif
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
