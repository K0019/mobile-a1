#include "TrailComponent.h"
#include "GUICollection.h"

TrailRendererComponent::TrailRendererComponent()
    : m_lastPosition()
    , m_points{}
    , m_pointCount(0)
    , m_headIndex(0)
{
}

TrailRendererComponent::TrailRendererComponent(
    float minVertexDistance,
    float lifetime,
    float startWidth,
    float endWidth,
    const Vec4& startColor,
    const Vec4& endColor
)
    : minVertexDistance(minVertexDistance)
    , lifetime(lifetime)
    , startWidth(startWidth)
    , endWidth(endWidth)
    , startColor(startColor)
    , endColor(endColor)
    , m_lastPosition()
    , m_points{}
    , m_pointCount(0)
    , m_headIndex(0)
{
}

void TrailRendererComponent::ClearTrail() {
    m_pointCount = 0;
    m_headIndex = 0;
}

bool TrailRendererComponent::IsEmitting() const {
    return emitting;
}

void TrailRendererComponent::SetEmitting(bool value) {
    emitting = value;
}

bool TrailRendererComponent::IsEnabled() const {
    return enabled;
}

void TrailRendererComponent::SetEnabled(bool value) {
    enabled = value;
}

void TrailRendererComponent::SetStartColor(const Vec4& color) {
    startColor = color;
}

void TrailRendererComponent::SetEndColor(const Vec4& color) {
    endColor = color;
}

void TrailRendererComponent::SetStartWidth(float width) {
    startWidth = width;
}

void TrailRendererComponent::SetEndWidth(float width) {
    endWidth = width;
}

const TrailRendererComponent::TrailPoint& TrailRendererComponent::GetPoint(int index) const {
    int actualIndex = (m_headIndex + index) % MAX_TRAIL_POINTS;
    return m_points[actualIndex];
}

void TrailRendererComponent::AddPoint(const Vec3& position) {
    // Check if we've reached the configured max points
    int effectiveMaxPoints = glm::min(maxPoints, (int)MAX_TRAIL_POINTS);

    // Calculate insert index
    int insertIndex;
    if(m_pointCount < effectiveMaxPoints) {
        insertIndex = (m_headIndex + m_pointCount) % MAX_TRAIL_POINTS;
        m_pointCount++;
    }
    else {
        // Buffer is full, overwrite oldest point and advance head
        m_headIndex = (m_headIndex + 1) % MAX_TRAIL_POINTS;
        insertIndex = (m_headIndex + effectiveMaxPoints - 1) % MAX_TRAIL_POINTS;
    }

    // Add the new point
    m_points[insertIndex].position = position;
    m_points[insertIndex].age = 0.0f;

    // Update last position
    m_lastPosition = position;
}

void TrailRendererComponent::RemoveExpiredPoints() {
    while(m_pointCount > 0) {
        int headIndex = m_headIndex % MAX_TRAIL_POINTS;
        if(m_points[headIndex].age > lifetime) {
            // Remove this point by advancing the head
            m_headIndex = (m_headIndex + 1) % MAX_TRAIL_POINTS;
            m_pointCount--;
        }
        else {
            // If the head point isn't expired, none of the newer ones will be
            break;
        }
    }
}

void TrailRendererComponent::UpdateAges(float dt) {
    for(int i = 0; i < m_pointCount; i++) {
        int index = (m_headIndex + i) % MAX_TRAIL_POINTS;
        m_points[index].age += dt;
    }
}

float TrailRendererComponent::CalculateWidth(float agePercent) const {
    return endWidth + agePercent * (startWidth - endWidth);
}


Vec4 TrailRendererComponent::CalculateColor(float agePercent) const {
    return Vec4(
        endColor.x + agePercent * (startColor.x - endColor.x),
        endColor.y + agePercent * (startColor.y - endColor.y),
        endColor.z + agePercent * (startColor.z - endColor.z),
        endColor.w + agePercent * (startColor.w - endColor.w)
    );
}

int TrailRendererComponent::GetPointCount() const {
    return m_pointCount;
}
void TrailRendererComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    // Core trail state
    ImGui::Checkbox("Enabled", &enabled);
    if (!enabled)
        return;
    
    // Emitter control
    ImGui::SameLine();
    ImGui::Checkbox("Emitting", &emitting);
    
    ImGui::SeparatorText("Trail Properties");
    
    // Basic properties with wider ranges and more precision
    ImGui::DragFloat("Lifetime", &lifetime, 0.1f, 0.01f, 30.0f, "%.2f sec");
    
    // Max points with input field for precise control
    ImGui::SliderInt("Max Points", &maxPoints, 2, MAX_TRAIL_POINTS);
    
    // Min vertex distance with more precision
    float minDist = minVertexDistance;
    if (ImGui::DragFloat("Min Vertex Distance", &minDist, 0.005f, 0.001f, 5.0f, "%.3f units")) {
        minVertexDistance = minDist;
    }
    
    // Width controls with much broader range
    ImGui::SeparatorText("Width Properties");
    ImGui::DragFloat("Start Width", &startWidth, 0.1f, 0.0f, 50.0f, "%.2f units");
    
    ImGui::DragFloat("End Width", &endWidth, 0.1f, 0.0f, 50.0f, "%.2f units");
    
    // Color controls with alpha
    ImGui::SeparatorText("Color Properties");
    ImGui::ColorEdit4("Start Color", &startColor.x, ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("End Color", &endColor.x, ImGuiColorEditFlags_AlphaBar);
    
    // Visual quality with more precision
    ImGui::SeparatorText("Visual Quality");
    ImGui::DragFloat("Edge Smoothing", &smoothing, 0.001f, 0.0f, 1.0f, "%.4f");
    
    // Glow settings in a collapsible section
    if (ImGui::CollapsingHeader("Glow Effect", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Enable/disable glow
        ImGui::Checkbox("Enable Glow", &glow.enabled);
        
        // Only show glow settings if enabled
        if (glow.enabled) {
            // Glow color with alpha
            ImGui::ColorEdit4("Glow Color", &glow.color.x, 
                            ImGuiColorEditFlags_AlphaBar);
            
            // Glow radius multiplier
            float glowRadius = glow.radius;
            if (ImGui::DragFloat("Glow Radius", &glowRadius, 0.1f, 1.0f, 10.0f, "%.2fx")) {
                glow.radius = glowRadius;
            }
            ImGui::SameLine();
            
            // Glow intensity
            float glowIntensity = glow.intensity;
            if (ImGui::DragFloat("Glow Intensity", &glowIntensity, 0.05f, 0.0f, 5.0f, "%.2f")) {
                glow.intensity = glowIntensity;
            }
            
            // Glow decay
            float glowDecay = glow.decay;
            if (ImGui::SliderFloat("Glow Decay", &glowDecay, 0.0f, 1.0f, "%.2f")) {
                glow.decay = glowDecay;
            }
            ImGui::SameLine();
            
            // Preview of combined trail and glow effect
            ImGui::Separator();
            ImGui::Text("Effect Preview:");
            
            // Draw a simple preview (could be enhanced with actual rendering)
            const float previewWidth = ImGui::GetContentRegionAvail().x;
            const float previewHeight = 40.0f;
            ImVec2 startPos = ImGui::GetCursorScreenPos();
            
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            
            // Draw glow background
            ImColor glowStartColor(
                glow.color.x, 
                glow.color.y, 
                glow.color.z, 
                glow.color.w * glow.intensity);
            
            ImColor glowEndColor(
                glow.color.x, 
                glow.color.y, 
                glow.color.z, 
                0.0f);
            
            float glowWidth = startWidth * glow.radius * 2.0f;
            
            drawList->AddRectFilledMultiColor(
                ImVec2(startPos.x, startPos.y + previewHeight/2 - glowWidth/2),
                ImVec2(startPos.x + previewWidth, startPos.y + previewHeight/2 + glowWidth/2),
                glowStartColor, glowEndColor, glowEndColor, glowStartColor);
            
            // Draw trail on top
            ImColor trailStartColor(
                startColor.x, 
                startColor.y, 
                startColor.z, 
                startColor.w);
            
            ImColor trailEndColor(
                endColor.x, 
                endColor.y, 
                endColor.z, 
                endColor.w);
            
            drawList->AddRectFilledMultiColor(
                ImVec2(startPos.x, startPos.y + previewHeight/2 - startWidth/2),
                ImVec2(startPos.x + previewWidth, startPos.y + previewHeight/2 + startWidth/2),
                trailStartColor, trailEndColor, trailEndColor, trailStartColor);
            
            ImGui::Dummy(ImVec2(previewWidth, previewHeight));
        }
    }
    
    // Actions
    ImGui::SeparatorText("Actions");
    if (ImGui::Button("Clear Trail", ImVec2(120, 0))) {
        ClearTrail();
    }
    
    // Debug information
    if (ImGui::CollapsingHeader("Debug Info")) {
        ImGui::BeginDisabled();
        ImGui::LabelText("Active Points", "%d / %d", 
                        GetPointCount(), maxPoints);
        ImGui::LabelText("Points Capacity", "%d", MAX_TRAIL_POINTS);
        
        // Calculate total trail length
        float totalLength = 0.0f;
        if (GetPointCount() > 1) {
            for (int i = 0; i < GetPointCount() - 1; i++) {
                const auto& p0 = GetPoint(i);
                const auto& p1 = GetPoint(i + 1);
                totalLength += (p0.position - p1.position).Length();
            }
        }
        ImGui::LabelText("Total Length", "%.3f units", totalLength);
        
        // Show oldest point age
        if (GetPointCount() > 0) {
            ImGui::LabelText("Oldest Point Age", "%.3f sec", GetPoint(0).age);
        }
        
        // Show effective buffer usage
        float bufferUsage = static_cast<float>(GetPointCount()) / 
                          static_cast<float>(MAX_TRAIL_POINTS);
        ImGui::ProgressBar(bufferUsage, ImVec2(-1, 0), 
                         "Buffer Usage");
        ImGui::EndDisabled();
    }
#endif
}