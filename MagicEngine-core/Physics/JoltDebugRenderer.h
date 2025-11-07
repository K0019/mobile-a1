#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <im3d.h>
#include <iostream>

class JoltDebugRenderer : public JPH::DebugRendererSimple {
public:
    JoltDebugRenderer() { JPH::DebugRenderer::Initialize(); }
    
    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        Im3d::DrawLine(
            Im3d::Vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()),
            Im3d::Vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()),
            10,
            Im3d::Color(inColor.r, inColor.g, inColor.b, inColor.a)
        );
    }
    
    void DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3,
                     JPH::ColorArg color, ECastShadow) override {
        DrawLine(v1, v2, color);
        DrawLine(v2, v3, color);
        DrawLine(v3, v1, color);
    }
    
    void DrawText3D(JPH::RVec3Arg pos, const std::string_view& str,
                   JPH::ColorArg color, float height) override 
    {
        Im3d::Text(
            Im3d::Vec3(static_cast<float>(pos.GetX()), 
                      static_cast<float>(pos.GetY()), 
                      static_cast<float>(pos.GetZ())),
            height,
            Im3d::Color(color.r, color.g, color.b, color.a),
            Im3d::TextFlags_Default, 
            str.data()
        );
    }
};