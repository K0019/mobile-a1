#include "grid_feature.h"
#include "renderer/hina_context.h"
#include "renderer/linear_color.h"
#include <hina_vk.h>
#include <cstring>
#include <iostream>

// HSL shader for infinite grid - inspired by https://bgolus.medium.com/the-best-darn-grid-shader-yet-727f9278b9d8
static const char* kGridShaderHSL = R"(
#hina
// No bind groups needed - everything is in push constants
push_constant PushConstants {
    mat4 mvp;
    vec4 cameraPos;
    vec4 origin;
    vec4 gridParams; // .x = majorGridDiv, .y = planeAxis (0=X, 1=Y, 2=Z)
} pc;

struct Varyings {
    vec3 worldPos;
    vec2 uv_grid;
    vec2 uv_world;
};

struct FragOut {
    vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
const vec3 pos[4] = vec3[4](
    vec3(-1.0, -1.0, 0.0),
    vec3( 1.0, -1.0, 0.0),
    vec3( 1.0,  1.0, 0.0),
    vec3(-1.0,  1.0, 0.0)
);
const int indices[6] = int[6](0, 1, 2, 2, 3, 0);
const float gridSize = 5000.0;

vec2 getAxisComponents(vec3 worldPos, float planeAxisEnum) {
    if (planeAxisEnum == 0.0) { return worldPos.yz; }
    else if (planeAxisEnum == 2.0) { return worldPos.xy; }
    else { return worldPos.xz; }
}

Varyings VSMain() {
    Varyings out;
    float majorDiv = max(2.0, round(pc.gridParams.x));
    float planeAxis = pc.gridParams.y;

    int idx = indices[gl_VertexIndex];
    vec3 basePos;
    if (planeAxis == 0.0) {
        basePos = vec3(0.0, pos[idx].x, pos[idx].y) * gridSize;
    } else if (planeAxis == 2.0) {
        basePos = vec3(pos[idx].x, pos[idx].y, 0.0) * gridSize;
    } else {
        basePos = vec3(pos[idx].x, 0.0, pos[idx].y) * gridSize;
    }

    vec3 camPosPlane = pc.cameraPos.xyz;
    if (planeAxis == 0.0) {
        basePos.y += camPosPlane.y; basePos.z += camPosPlane.z;
    } else if (planeAxis == 2.0) {
        basePos.x += camPosPlane.x; basePos.y += camPosPlane.y;
    } else {
        basePos.x += camPosPlane.x; basePos.z += camPosPlane.z;
    }

    vec3 worldPosition = basePos + pc.origin.xyz;
    out.worldPos = worldPosition;
    vec2 worldComponents = getAxisComponents(worldPosition, planeAxis);
    vec2 camComponents = getAxisComponents(pc.cameraPos.xyz, planeAxis);
    vec2 cameraCenteringOffset = floor(camComponents / majorDiv) * majorDiv;

    out.uv_grid = worldComponents - cameraCenteringOffset;
    out.uv_world = worldComponents;

    gl_Position = pc.mvp * vec4(worldPosition, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
const float _AxisLineWidth  = 0.07;
const float _MajorLineWidth = 0.05;
const float _MinorLineWidth = 0.03;
const float _AxisDashScale  = 0.67;

const vec4 baseColor      = vec4(1.0, 1.0, 1.0, 0.0);
const vec4 minorLineColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 majorLineColor = vec4(1.0, 1.0, 1.0, 1.0);
const vec4 axisColorX     = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 axisColorY     = vec4(0.0, 1.0, 0.0, 1.0);
const vec4 axisColorZ     = vec4(0.0, 0.0, 1.0, 1.0);
const vec4 axisDashColorX = vec4(0.5, 0.0, 0.0, 1.0);
const vec4 axisDashColorY = vec4(0.0, 0.5, 0.0, 1.0);
const vec4 axisDashColorZ = vec4(0.0, 0.0, 0.5, 1.0);

float satf(float x) { return clamp(x, 0.0, 1.0); }
vec2 satv(vec2 x) { return clamp(x, 0.0, 1.0); }

FragOut FSMain(Varyings in) {
    FragOut out;
    float majorGridDiv = max(2.0, round(pc.gridParams.x));
    float planeAxis = pc.gridParams.y;

    vec2 uv_grid_ddx = dFdx(in.uv_grid);
    vec2 uv_grid_ddy = dFdy(in.uv_grid);
    vec2 uvDeriv = vec2(length(vec2(uv_grid_ddx.x, uv_grid_ddy.x)),
                        length(vec2(uv_grid_ddx.y, uv_grid_ddy.y)));

    // Axis Lines
    float axisLineWidth = max(_MajorLineWidth, _AxisLineWidth);
    vec2 axisDrawWidth = max(vec2(axisLineWidth), uvDeriv);
    vec2 axisLineAA = uvDeriv * 1.5;
    vec2 axisLines2 = smoothstep(axisDrawWidth + axisLineAA, axisDrawWidth - axisLineAA, abs(in.uv_world * 2.0));
    axisLines2 *= satv(axisLineWidth / axisDrawWidth);

    // Major Lines
    float div = majorGridDiv;
    vec2 majorUVDeriv = uvDeriv / div;
    float majorTargetWidth = _MajorLineWidth / div;
    vec2 majorDrawWidth = clamp(vec2(majorTargetWidth), majorUVDeriv, vec2(0.5));
    vec2 majorLineAA = majorUVDeriv * 1.5;
    vec2 majorGridUV = 1.0 - abs(fract(in.uv_grid / div) * 2.0 - 1.0);
    vec2 majorAxisOffset = (1.0 - satv(abs(in.uv_world / div * 2.0))) * 2.0;
    majorGridUV += majorAxisOffset;
    vec2 majorGrid2 = smoothstep(majorDrawWidth + majorLineAA, majorDrawWidth - majorLineAA, majorGridUV);
    majorGrid2 *= satv(majorTargetWidth / majorDrawWidth);
    majorGrid2 = satv(majorGrid2 - axisLines2);
    majorGrid2 = mix(majorGrid2, vec2(majorTargetWidth), satv(majorUVDeriv * 2.0 - 1.0));

    // Minor Lines
    float minorLineWidth = min(_MinorLineWidth, _MajorLineWidth);
    float minorTargetWidth = minorLineWidth;
    vec2 minorDrawWidth = clamp(vec2(minorTargetWidth), uvDeriv, vec2(0.5));
    vec2 minorLineAA = uvDeriv * 1.5;
    vec2 minorGridUV = 1.0 - abs(fract(in.uv_grid) * 2.0 - 1.0);
    vec2 minorMajorOffset = (1.0 - satv((1.0 - abs(fract(in.uv_world / div) * 2.0 - 1.0)) * div)) * 2.0;
    minorGridUV += minorMajorOffset;
    vec2 minorGrid2 = smoothstep(minorDrawWidth + minorLineAA, minorDrawWidth - minorLineAA, minorGridUV);
    minorGrid2 *= satv(minorTargetWidth / minorDrawWidth);
    minorGrid2 = satv(minorGrid2 - axisLines2);
    minorGrid2 = mix(minorGrid2, vec2(minorTargetWidth), satv(uvDeriv * 2.0 - 1.0));

    // Combine Lines
    float minorGrid = mix(minorGrid2.x, 1.0, minorGrid2.y);
    float majorGrid = mix(majorGrid2.x, 1.0, majorGrid2.y);

    // Axis dashing
    vec2 axisDashUV = abs(fract((in.uv_world + axisLineWidth * 0.5) * _AxisDashScale) * 2.0 - 1.0) - 0.5;
    vec2 axisDashDeriv = uvDeriv * _AxisDashScale * 1.5;
    vec2 axisDash = smoothstep(-axisDashDeriv, axisDashDeriv, axisDashUV);
    axisDash.x = (in.uv_world.x < 0.0) ? axisDash.x : 1.0;
    axisDash.y = (in.uv_world.y < 0.0) ? axisDash.y : 1.0;

    vec4 aAxisColor, bAxisColor, aAxisDashColor, bAxisDashColor;
    if (planeAxis == 0.0) {
        aAxisColor = axisColorY; bAxisColor = axisColorZ;
        aAxisDashColor = axisDashColorY; bAxisDashColor = axisDashColorZ;
    } else if (planeAxis == 2.0) {
        aAxisColor = axisColorX; bAxisColor = axisColorY;
        aAxisDashColor = axisDashColorX; bAxisDashColor = axisDashColorY;
    } else {
        aAxisColor = axisColorX; bAxisColor = axisColorZ;
        aAxisDashColor = axisDashColorX; bAxisDashColor = axisDashColorZ;
    }

    aAxisColor = mix(aAxisDashColor, aAxisColor, axisDash.y);
    bAxisColor = mix(bAxisDashColor, bAxisColor, axisDash.x);
    vec4 axisPartB = bAxisColor * axisLines2.y;
    vec4 axisLines = mix(axisPartB, aAxisColor, axisLines2.x);

    vec4 finalColor = mix(baseColor, minorLineColor, minorGrid * minorLineColor.a);
    finalColor = mix(finalColor, majorLineColor, majorGrid * majorLineColor.a);
    finalColor = mix(finalColor, axisLines, axisLines.a);
    out.color = finalColor;
    return out;
}
#hina_end
)";

GridFeature::~GridFeature() = default;

const char* GridFeature::GetName() const
{
  return "GridFeature";
}

void GridFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  PassDeclarationInfo gPassInfo;
  gPassInfo.framebufferDebugName = "GridDraw";
  gPassInfo.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR, .loadOp = gfx::LoadOp::Load, .storeOp = gfx::StoreOp::Store
  };
  gPassInfo.depthAttachment = {
    .textureName = RenderResources::SCENE_DEPTH, .loadOp = gfx::LoadOp::Load, .storeOp = gfx::StoreOp::Store
  };
  passBuilder.CreatePass().UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite)
             .UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite)
             .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(550))
             .ExecuteAfter("SceneComposite")
             .AddGraphicsPass("GridDraw", gPassInfo, [this](const internal::ExecutionContext& context)
             {
               const Parameters& params_ = *static_cast<const Parameters*>(GetParameterBlock_RT());
               if (!params_.enabled) return;
               auto& cmd = context.GetCommandBuffer();
               EnsurePipelineCreated(context);
               if (!gfx::isValid(pipeline_.get())) return;
               struct PushConstant
               {
                 mat4 mvp;
                 vec4 cameraPos;
                 vec4 origin;
                 vec4 gridParams;
               } pushConstants_;
               auto& framedata = context.GetFrameData();
               // projMatrix already has Vulkan Y-flip applied at source (UploadToPipeline)
               pushConstants_.mvp = framedata.projMatrix * framedata.viewMatrix;
               pushConstants_.cameraPos = vec4(framedata.cameraPos, 1.0f);
               pushConstants_.origin = vec4(params_.originOffset, 1.0f);
               pushConstants_.gridParams = vec4(static_cast<float>(params_.majorGridDivisions),
                                                static_cast<float>(params_.axis), 0.0f, 0.0f);
               cmd.bindPipeline(pipeline_.get());
               cmd.pushConstants(pushConstants_);
               cmd.draw(6);
             });
}

void GridFeature::EnsurePipelineCreated(const internal::ExecutionContext& context)
{
  if (resourcesCreated_) return;

  const Parameters& params_ = *(const Parameters*)GetParameterBlock_RT();

  // Compile HSL shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kGridShaderHSL, "grid_feature_shader", &error);
  if (!module) {
    if (error) hslc_free_log(error);
    return;
  }

  // Use HDR scene format since SCENE_COLOR uses linear workflow
  hina_format colorFormat = LinearColor::HDR_SCENE_FORMAT;
  hina_format depthFormat = HINA_FORMAT_D32_SFLOAT;

  // Pipeline descriptor for HSL module
  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = false;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS_OR_EQUAL;
  pip_desc.depth_format = depthFormat;
  pip_desc.depth_bias.constant = -1.0f;  // Push grid behind coplanar geometry
  pip_desc.depth_bias.slope = -1.0f;
  pip_desc.samples = static_cast<hina_sample_count>(params_.pipelineSamples);

  // Color attachment with alpha blending
  pip_desc.blend[0].enable = true;
  pip_desc.blend[0].src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
  pip_desc.blend[0].dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pip_desc.blend[0].src_alpha = HINA_BLEND_FACTOR_ZERO;
  pip_desc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].color_op = HINA_BLEND_OP_ADD;
  pip_desc.blend[0].alpha_op = HINA_BLEND_OP_ADD;

  // Color format must match render target (SCENE_COLOR)
  pip_desc.color_formats[0] = colorFormat;

  pipeline_.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(pipeline_.get())) {
    return;
  }

  resourcesCreated_ = true;
}
