#pragma once
#include "TrailComponent.h"

class Renderer;

class TrailRendererSystem :  public ecs::System<TrailRendererSystem, TrailRendererComponent>
{
    public:
    explicit TrailRendererSystem();
private:
    void UpdateTrailComp(TrailRendererComponent& trailComp);
};

class TrailRendererDrawingSystem : public ecs::System<TrailRendererDrawingSystem, TrailRendererComponent>
{
    public:
    explicit TrailRendererDrawingSystem();
    private:
    Renderer* renderer;
    void DrawTrailComp(TrailRendererComponent& trailComp);
};