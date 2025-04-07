#include "TrailSystem.h"

#include "Engine.h"

TrailRendererSystem::TrailRendererSystem() : System_Internal(&TrailRendererSystem::UpdateTrailComp) {
}

void TrailRendererSystem::UpdateTrailComp(TrailRendererComponent& trailComp)
{
    if(!trailComp.enabled)
    {
        return;
    }
    // Get delta time
    float dt = GameTime::FixedDt();
    // Get entity transform
    auto& transform = ecs::GetEntityTransform(&trailComp);
    // Update all existing points with new age
    trailComp.UpdateAges(dt);
    // Remove expired points
    trailComp.RemoveExpiredPoints();
    // Add new point if emitting
    if(trailComp.emitting) {
        // Get current position from transform
        Vector2 currentPosition = transform.GetWorldPosition();

        // Check if enough distance has been traveled
        if(trailComp.GetPointCount() == 0 ||
           (currentPosition - trailComp.m_lastPosition).LengthSquared() >=
           trailComp.minVertexDistance * trailComp.minVertexDistance) {
            trailComp.AddPoint(currentPosition);
        }
    }
}

TrailRendererDrawingSystem::TrailRendererDrawingSystem() : System_Internal(&TrailRendererDrawingSystem::DrawTrailComp) {
    renderer = ST<Engine>::Get()->_vulkan->_renderer.get();
}

void TrailRendererDrawingSystem::DrawTrailComp(TrailRendererComponent& trailComp)
{
    renderer->AddTrailInstance(trailComp);
}


