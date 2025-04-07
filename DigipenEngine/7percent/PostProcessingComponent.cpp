#include "PostProcessingComponent.h"
#include "Engine.h"

PostProcessingComponent::PostProcessingComponent(bool bloomEnabled):
#ifdef IMGUI_ENABLED
      REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
     bloomEnabled(bloomEnabled)
{

}
void PostProcessingComponent::OnDetached()
{
    ST<Engine>::Get()->_vulkan->_renderer->ResetPostProcessing();
}
#ifdef IMGUI_ENABLED
void PostProcessingComponent::EditorDraw(PostProcessingComponent& comp)
{
    
    // Bloom enabled toggle
    bool bloomEnabled = comp.bloomEnabled;
    if (ImGui::Checkbox("Bloom Enabled", &bloomEnabled)) {
        comp.bloomEnabled = bloomEnabled;
    }
    
    // Only show bloom settings if enabled
    if (comp.bloomEnabled) {
        // Bloom intensity slider
        ImGui::DragFloat("Bloom Intensity", &comp.bloomIntensity, 0.01f, 0.0f, 2.0f, "%.2f");
        
        // Bloom threshold slider
        ImGui::DragFloat("Bloom Threshold", &comp.bloomThreshold, 0.01f, 0.0f, 1.0f, "%.2f");
        
        // Blur size slider
        ImGui::DragFloat("Blur Size", &comp.blurSize, 0.1f, 0.0f, 20.0f, "%.2f");
    }
    
    ImGui::Separator();
    
    // Vignette Section
    bool vignetteEnabled = comp.vignetteEnabled;
    if (ImGui::Checkbox("Vignette Enabled", &vignetteEnabled)) {
        comp.vignetteEnabled = vignetteEnabled;
    }
    
    if (comp.vignetteEnabled) {
        // Vignette color picker
        float color[3] = { comp.vignetteColor.x, comp.vignetteColor.y, comp.vignetteColor.z };
        if (ImGui::ColorEdit3("Vignette Color", color)) {
            comp.vignetteColor = glm::vec4(color[0], color[1], color[2], 1.0f);
        }
        
        // Vignette intensity slider
        float vignetteIntensity = comp.vignetteIntensity;
        if (ImGui::SliderFloat("Vignette Intensity", &vignetteIntensity, 0.1f, 5.0f, "%.2f")) {
            comp.vignetteIntensity = vignetteIntensity;
        }
        
        // Vignette radius slider
        float vignetteRadius = comp.vignetteRadius;
        if (ImGui::SliderFloat("Vignette Radius", &vignetteRadius, 0.0f, 1.0f, "%.2f")) {
            comp.vignetteRadius = vignetteRadius;
        }
        
        // Vignette smoothness slider
        float vignetteSmoothness = comp.vignetteSmoothness;
        if (ImGui::SliderFloat("Edge Smoothness", &vignetteSmoothness, 0.0f, 1.0f, "%.2f")) {
            comp.vignetteSmoothness = vignetteSmoothness;
        }
    }
}
#endif

PostProcessingSystem::PostProcessingSystem() : System_Internal(&PostProcessingSystem::UpdatePostProcessingComp)
{
    renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}

void PostProcessingSystem::UpdatePostProcessingComp(PostProcessingComponent& ppComp)
{
    renderer->updatePostProcessing(ppComp);
}
