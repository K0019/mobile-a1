#include "grid_feature.h"
// Inspired by https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8#56b0
static const char* codeVS = R"(
#version 460 core

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 cameraPos; // .w = 1.0f
    vec4 origin;    // .w = 1.0f
    vec4 gridParams;// .x = majorGridDiv, .y = planeAxis (0=X, 1=Y, 2=Z)
};
layout (location=0) out vec3 out_worldPos;
layout (location=1) out vec2 out_uv_grid;
layout (location=2) out vec2 out_uv_world;
const vec3 pos[4] = vec3[4](
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0),
    vec3(-1.0,  1.0, 0.0)
);
const int indices[6] = int[6](0, 1, 2, 2, 3, 0);
const float gridSize = 5000.0;
vec2 getAxisComponents(vec3 worldPos, float planeAxisEnum) {
    if (planeAxisEnum == 0.0) { // X Axis Plane (YZ)
        return worldPos.yz;
    } else if (planeAxisEnum == 2.0) { // Z Axis Plane (XY)
        return worldPos.xy;
    } else { // Y Axis Plane (XZ) - Default (1.0)
        return worldPos.xz;
    }
}

void main() {
    float majorDiv = max(2.0, round(gridParams.x));
    float planeAxis = gridParams.y; // 0=X, 1=Y(default), 2=Z

    // --- Calculate world position ---
    int idx = indices[gl_VertexIndex];
    vec3 basePos;
    if (planeAxis == 0.0) { // YZ Plane (X is fixed)
        basePos = vec3(0.0, pos[idx].x, pos[idx].y) * gridSize;
    } else if (planeAxis == 2.0) { // XY Plane (Z is fixed)
        basePos = vec3(pos[idx].x, pos[idx].y, 0.0) * gridSize;
    } else { // XZ Plane (Y is fixed) - Default
        basePos = vec3(pos[idx].x, 0.0, pos[idx].y) * gridSize; // Map quad Y to world Z
    }
    vec3 camPosPlane = cameraPos.xyz;
    if (planeAxis == 0.0) { // YZ
        basePos.y += camPosPlane.y; basePos.z += camPosPlane.z;
    } else if (planeAxis == 2.0) { // XY
        basePos.x += camPosPlane.x; basePos.y += camPosPlane.y;
    } else { // XZ
        basePos.x += camPosPlane.x; basePos.z += camPosPlane.z;
    }
    vec3 worldPosition = basePos + origin.xyz;
    out_worldPos = worldPosition;
    vec2 worldComponents = getAxisComponents(worldPosition, planeAxis);
    vec2 camComponents = getAxisComponents(cameraPos.xyz, planeAxis);
    vec2 cameraCenteringOffset = floor(camComponents / majorDiv) * majorDiv;

    out_uv_grid = worldComponents - cameraCenteringOffset; 
    out_uv_world = worldComponents;           

    gl_Position = mvp * vec4(worldPosition, 1.0);
}
)";

static const char* codeFS = R"(
#version 460 core

layout (location=0) in vec3 in_worldPos;
layout (location=1) in vec2 in_uv_grid;
layout (location=2) in vec2 in_uv_world;

layout (location=0) out vec4 out_FragColor;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 cameraPos;
    vec4 origin;
    vec4 gridParams;// .x = majorGridDiv, .y = planeAxis (0=X, 1=Y, 2=Z)
};

// --- Hardcoded Parameters ---
const float _AxisLineWidth  = 0.07;
const float _MajorLineWidth = 0.05;
const float _MinorLineWidth = 0.03;
const float _AxisDashScale  = 0.67;

const vec4 baseColor      = vec4(1.0, 1.0, 1.0, 0.0);
const vec4 minorLineColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 majorLineColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 centerColor    = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 axisColorX     = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 axisColorY     = vec4(0.0, 1.0, 0.0, 1.0);
const vec4 axisColorZ     = vec4(0.0, 0.0, 1.0, 1.0);
const vec4 axisDashColorX = vec4(0.5, 0.0, 0.0, 1.0);
const vec4 axisDashColorY = vec4(0.0, 0.5, 0.0, 1.0);
const vec4 axisDashColorZ = vec4(0.0, 0.0, 0.5, 1.0);

// Helper: saturate
float satf(float x) { return clamp(x, 0.0, 1.0); }
vec2 satv(vec2 x) { return clamp(x, 0.0, 1.0); }

void main() {
    float majorGridDiv = max(2.0, round(gridParams.x));
    float planeAxis = gridParams.y; // 0=X, 1=Y(default), 2=Z
    vec2 uv_grid_ddx = dFdx(in_uv_grid);
    vec2 uv_grid_ddy = dFdy(in_uv_grid);
    vec2 uvDeriv = vec2(length(vec2(uv_grid_ddx.x, uv_grid_ddy.x)),
                        length(vec2(uv_grid_ddx.y, uv_grid_ddy.y)));

    // --- Axis Lines ---
    float axisLineWidth = max(_MajorLineWidth, _AxisLineWidth);
    vec2 axisDrawWidth = max(vec2(axisLineWidth), uvDeriv);
    vec2 axisLineAA = uvDeriv * 1.5;
    vec2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(in_uv_world * 2.0));
    axisLines2 *= satv(axisLineWidth / axisDrawWidth);

    // --- Major Lines ---
    float div = majorGridDiv;
    vec2 majorUVDeriv = uvDeriv / div;
    float majorTargetWidth = _MajorLineWidth / div;
    vec2 majorDrawWidth = clamp(vec2(majorTargetWidth), majorUVDeriv, vec2(0.5));
    vec2 majorLineAA = majorUVDeriv * 1.5;
    vec2 majorGridUV = 1.0 - abs(fract(in_uv_grid / div) * 2.0 - 1.0);
    vec2 majorAxisOffset = (1.0 - satv(abs(in_uv_world / div * 2.0))) * 2.0;
    majorGridUV += majorAxisOffset;
    vec2 majorGrid2 = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
    majorGrid2 *= satv(majorTargetWidth / majorDrawWidth);
    majorGrid2 = satv(majorGrid2 - axisLines2);
    majorGrid2 = mix(majorGrid2, vec2(majorTargetWidth), satv(majorUVDeriv * 2.0 - 1.0));

    // --- Minor Lines ---
    float minorLineWidth = min(_MinorLineWidth, _MajorLineWidth);
    float minorTargetWidth = minorLineWidth;
    vec2 minorDrawWidth = clamp(vec2(minorTargetWidth), uvDeriv, vec2(0.5));
    vec2 minorLineAA = uvDeriv * 1.5;
    vec2 minorGridUV = 1.0 - abs(fract(in_uv_grid) * 2.0 - 1.0);
    vec2 minorMajorOffset = (1.0 - satv((1.0 - abs(fract(in_uv_world / div) * 2.0 - 1.0)) * div)) * 2.0;
    minorGridUV += minorMajorOffset;
    vec2 minorGrid2 = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
    minorGrid2 *= satv(minorTargetWidth / minorDrawWidth);
    minorGrid2 = satv(minorGrid2 - axisLines2);
    minorGrid2 = mix(minorGrid2, vec2(minorTargetWidth), satv(uvDeriv * 2.0 - 1.0));

    // --- Combine Lines ---
    float minorGrid = mix(minorGrid2.x, 1.0, minorGrid2.y);
    float majorGrid = mix(majorGrid2.x, 1.0, majorGrid2.y);

    vec2 axisDashUV = abs(fract((in_uv_world + axisLineWidth * 0.5) * _AxisDashScale) * 2.0 - 1.0) - 0.5;
    vec2 axisDashDeriv = uvDeriv * _AxisDashScale * 1.5;
    vec2 axisDash = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
    axisDash.x = (in_uv_world.x < 0.0) ? axisDash.x : 1.0;
    axisDash.y = (in_uv_world.y < 0.0) ? axisDash.y : 1.0;
    vec4 aAxisColor, bAxisColor, aAxisDashColor, bAxisDashColor;
    if (planeAxis == 0.0) { aAxisColor=axisColorY; bAxisColor=axisColorZ; aAxisDashColor=axisDashColorY; bAxisDashColor=axisDashColorZ; }
    else if (planeAxis == 2.0) { aAxisColor=axisColorX; bAxisColor=axisColorY; aAxisDashColor=axisDashColorX; bAxisDashColor=axisDashColorY; }
    else { aAxisColor=axisColorX; bAxisColor=axisColorZ; aAxisDashColor=axisDashColorX; bAxisDashColor=axisDashColorZ; }
    aAxisColor = mix(aAxisDashColor, aAxisColor, axisDash.y);
    bAxisColor = mix(bAxisDashColor, bAxisColor, axisDash.x);
    vec4 aAxisColorFinal = aAxisColor;
    vec4 bAxisColorFinal = bAxisColor;
    vec4 axisPartB = bAxisColorFinal * axisLines2.y;
    vec4 axisLines = mix(axisPartB, aAxisColorFinal, axisLines2.x);
    vec4 finalColor = mix(baseColor, minorLineColor, minorGrid * minorLineColor.a);
    finalColor = mix(finalColor, majorLineColor, majorGrid * majorLineColor.a);
    finalColor = mix(finalColor, axisLines, axisLines.a);
    out_FragColor = finalColor;
})";

GridFeature::~GridFeature() = default;

const char* GridFeature::GetName() const
{
  return "GridFeature";
}

void GridFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  PassDeclarationInfo gPassInfo;
  gPassInfo.framebufferDebugName = "GridDraw";
  gPassInfo.colorAttachments[0] = {.textureName = RenderResources::SCENE_COLOR,
    .loadOp = vk::LoadOp::Load,
    .storeOp = vk::StoreOp::Store,};

  gPassInfo.depthAttachment = {.textureName = RenderResources::SCENE_DEPTH,
    .loadOp = vk::LoadOp::Load,
    .storeOp = vk::StoreOp::Store,
    .clearDepth = 1.0f,};

  passBuilder.CreatePass().UseResource(RenderResources::SCENE_COLOR,
               AccessType::ReadWrite)
             // Declare as write since it's a render target
             .UseResource(RenderResources::SCENE_DEPTH,AccessType::ReadWrite)
             // Declare as write since it's a depth target
             .SetPriority(
               internal::RenderPassBuilder::PassPriority::Transparent)
             // Optional: set priority
             .AddGraphicsPass("GridDraw",
               gPassInfo,
               [this](const internal::ExecutionContext& context)
               {
                 // Get target properties for pipeline creation
                 auto& cmd = context.GetvkCommandBuffer();

                 EnsurePipelineCreated(context);

                 const Parameters& params_ = *static_cast<const Parameters*>(
                   GetParameterBlock_RT());
                 if (!pipeline_.valid())
                 {
                   return;
                 }
                 struct PushConstant
                 {
                   mat4 mvp;
                   vec4 cameraPos;
                   vec4 origin;
                   vec4 gridParams;
                   // .x = majorGridDiv, .y = planeAxis (0=X, 1=Y, 2=Z)
                 } pushConstants_;

                 auto& framedata = context.GetFrameData();

                 pushConstants_.mvp =
                   framedata.projMatrix * framedata.viewMatrix;
                 pushConstants_.cameraPos = vec4(framedata.cameraPos,1.0f);
                 pushConstants_.origin = vec4(params_.originOffset,1.0f);
                 pushConstants_.gridParams = vec4(
                   static_cast<float>(params_.majorGridDivisions),
                   static_cast<float>(params_.axis),
                   0.0f,
                   0.0f);

                 cmd.cmdBindRenderPipeline(pipeline_);
                 cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Less,
                   .isDepthWriteEnabled = false});
                 cmd.cmdPushConstants(pushConstants_);
                 cmd.cmdDraw(6);
               });
}

void GridFeature::EnsurePipelineCreated(
  const internal::ExecutionContext& context)
{
  const Parameters& params_ = *(const Parameters*)GetParameterBlock_RT();
  auto& ctx = context.GetvkContext();
  auto handle = context.GetTexture(RenderResources::SCENE_COLOR);
  auto depth = context.GetTexture(RenderResources::SCENE_DEPTH);
  if (pipeline_.empty() || cachedSamples_ != params_.pipelineSamples)
  {
    pipeline_.reset();
    vert_ = ctx.createShaderModule({codeVS,
      vk::ShaderStage::Vert,
      "Shader Module: Grid (vert)"});
    frag_ = ctx.createShaderModule({codeFS,
      vk::ShaderStage::Frag,
      "Shader Module: Grid (frag)"});
    pipeline_ = ctx.createRenderPipeline({.smVert = vert_,
        .smFrag = frag_,
        .color = {{.format = ctx.getFormat(handle),
          .blendEnabled = true,
          .alphaBlendOp = vk::BlendOp::Add,
          .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha,
          .srcAlphaBlendFactor = vk::BlendFactor::Zero,
          .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha,
          .dstAlphaBlendFactor = vk::BlendFactor::One}},
        .depthFormat = depth.valid() ? ctx.getFormat(depth) :
                         vk::Format::Invalid,
        .samplesCount = params_.pipelineSamples,
        .debugName = "Pipeline: Draw Grid",},
      nullptr);
    cachedSamples_ = params_.pipelineSamples;
  }
}
