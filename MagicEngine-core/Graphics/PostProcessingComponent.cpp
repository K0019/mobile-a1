#include "Graphics/PostProcessingComponent.h"
#include "Engine/Engine.h"

PostProcessingComponent::PostProcessingComponent(bool bloomEnabled)
    : bloomEnabled{ bloomEnabled }
{

}
void PostProcessingComponent::OnDetached()
{
   // ST<Engine>::Get()->_vulkan->_renderer->ResetPostProcessing();
}
void PostProcessingComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    
    // Bloom enabled toggle
    ImGui::Checkbox("Bloom Enabled", &bloomEnabled);
    
    // Only show bloom settings if enabled
    if (bloomEnabled) {
        // Bloom intensity slider
        ImGui::DragFloat("Bloom Intensity", &bloomIntensity, 0.01f, 0.0f, 2.0f, "%.2f");
        
        // Bloom threshold slider
        ImGui::DragFloat("Bloom Threshold", &bloomThreshold, 0.01f, 0.0f, 1.0f, "%.2f");
        
        // Blur size slider
        ImGui::DragFloat("Blur Size", &blurSize, 0.1f, 0.0f, 20.0f, "%.2f");
    }
    
    ImGui::Separator();
    
    // Vignette Section
    ImGui::Checkbox("Vignette Enabled", &vignetteEnabled);
    
    if (vignetteEnabled) {
        // Vignette color picker
        float color[3] = { vignetteColor.x, vignetteColor.y, vignetteColor.z };
        if (ImGui::ColorEdit3("Vignette Color", color)) {
            vignetteColor = Vec4(color[0], color[1], color[2], 1.0f);
        }
        
        // Vignette intensity slider
        ImGui::SliderFloat("Vignette Intensity", &vignetteIntensity, 0.1f, 5.0f, "%.2f");
        
        // Vignette radius slider
        ImGui::SliderFloat("Vignette Radius", &vignetteRadius, 0.0f, 1.0f, "%.2f");
        
        // Vignette smoothness slider
        ImGui::SliderFloat("Edge Smoothness", &vignetteSmoothness, 0.0f, 1.0f, "%.2f");
    }
#endif
}

PostProcessingSystem::PostProcessingSystem() : System_Internal(&PostProcessingSystem::UpdatePostProcessingComp)
{
    //renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}

void PostProcessingSystem::UpdatePostProcessingComp(PostProcessingComponent&)
{
    //renderer->updatePostProcessing(ppComp);
}
