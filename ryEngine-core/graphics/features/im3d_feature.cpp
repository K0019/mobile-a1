#include "im3d_feature.h"
#include <cstring>   // memcpy
#include <glm/glm.hpp>

// ============================================================================
// Shaders (separate VS/FS per primitive; unified push constants)
// ============================================================================

// Common push constants layout for all three shaders
// - Keep identical across VS/FS and across pipelines
// - Unused fields are harmless in specific shaders
// layout(push_constant) uniform Push { ... } PC;

static const char* kPointVS = R"(
#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

struct PointGPU { vec3 p; float sizePx; uint abgr; uint pad0; uint pad1; uint pad2; };
layout(buffer_reference, std430) readonly buffer PBuf { PointGPU pts[]; };

layout(push_constant) uniform Push {
  mat4  viewProj;
  vec2  viewportSize;  // unused
  uvec2 baseAddr;
  uint  offPoints;
  uint  offLines;
  uint  offTris;
  uint  mode;
} PC;

layout(location=0) out vec4 vColor;

vec4 unpackRGBA(uint c){  // <-- FIXED
  return vec4(
    float((c>>24)&0xFFu)/255.0,
    float((c>>16)&0xFFu)/255.0,
    float((c>> 8)&0xFFu)/255.0,
    float((c>> 0)&0xFFu)/255.0 );
}

uvec2 addBytes(uvec2 a, uint off){ uint lo=a.x+off; return uvec2(lo, a.y + uint(lo<a.x)); }

void main() {
  PBuf B = PBuf(addBytes(PC.baseAddr, PC.offPoints));
  PointGPU p = B.pts[gl_VertexIndex];

  gl_Position = PC.viewProj * vec4(p.p, 1.0);
  gl_PointSize = p.sizePx;
  vColor = unpackRGBA(p.abgr);
}
)";

static const char* kPointFS = R"(
#version 460
layout(location=0) in  vec4 vColor;
layout(location=0) out vec4 outColor;

void main() {
  vec2 uv = gl_PointCoord * 2.0 - 1.0;
  float d = length(uv);
  if (d > 1.0) discard;
  float alpha = 1.0 - smoothstep(0.9, 1.0, d);
  outColor = vec4(vColor.rgb, vColor.a * alpha);
}
)";

static const char* kLineVS = R"(
#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

struct LineGPU { vec3 p0; float sizePx; vec3 p1; uint abgr; };
layout(buffer_reference, std430) readonly buffer LBuf { LineGPU lns[]; };

layout(push_constant) uniform Push {
  mat4  viewProj;
  vec2  viewportSize;  // needed for pixel->NDC scale
  uvec2 baseAddr;
  uint  offPoints;
  uint  offLines;      // byte offset to LineGPU[]
  uint  offTris;
  uint  mode;
} PC;

layout(location=0) out vec4 vColor;

vec4 unpackRGBA(uint c){
  return vec4(
    float((c>>24)&0xFFu)/255.0,
    float((c>>16)&0xFFu)/255.0,
    float((c>> 8)&0xFFu)/255.0,
    float((c>> 0)&0xFFu)/255.0 );
}

uvec2 addBytes(uvec2 a, uint off){ uint lo=a.x+off; return uvec2(lo, a.y + uint(lo<a.x)); }

void main() {
  // Instance = one segment, 6 vertices: (p0a,p0b,p1b) (p0a,p1b,p1a)
  LBuf B = LBuf(addBytes(PC.baseAddr, PC.offLines));
  LineGPU s = B.lns[gl_InstanceIndex];

  // Clip-space endpoints
  vec4 p0c = PC.viewProj * vec4(s.p0, 1.0);
  vec4 p1c = PC.viewProj * vec4(s.p1, 1.0);

  // NDC endpoints
  vec2 p0n = p0c.xy / p0c.w;
  vec2 p1n = p1c.xy / p1c.w;

  // Perp direction in NDC and half width in NDC
  vec2 dir  = normalize(p1n - p0n);
  vec2 perp = vec2(-dir.y, dir.x);
  float halfW = (s.sizePx * 0.5) * (2.0 / PC.viewportSize.y);

  // Four NDC corners
  vec2 p0a_n = p0n - perp * halfW;
  vec2 p0b_n = p0n + perp * halfW;
  vec2 p1a_n = p1n - perp * halfW;
  vec2 p1b_n = p1n + perp * halfW;

  // Convert each corner back to CLIP using its endpoint w,z (no mixing!)
  vec4 p0a_c = vec4(p0a_n * p0c.w, p0c.z, p0c.w);
  vec4 p0b_c = vec4(p0b_n * p0c.w, p0c.z, p0c.w);
  vec4 p1a_c = vec4(p1a_n * p1c.w, p1c.z, p1c.w);
  vec4 p1b_c = vec4(p1b_n * p1c.w, p1c.z, p1c.w);

  // Triangle list index within the instance: 0..5
  uint vid = uint(gl_VertexIndex) % 6u;

  // Two triangles: [p0a,p0b,p1b] and [p0a,p1b,p1a]
  vec4 outPos =
      (vid==0u) ? p0a_c :
      (vid==1u) ? p0b_c :
      (vid==2u) ? p1b_c :
      (vid==3u) ? p0a_c :
      (vid==4u) ? p1b_c :
                  p1a_c;

  gl_Position = outPos;
  vColor = unpackRGBA(s.abgr);
}

)";

static const char* kLineFS = R"(
#version 460
layout(location=0) in  vec4 vColor;
layout(location=0) out vec4 outColor;
void main(){ outColor = vColor; }
)";

static const char* kTriVS = R"(

#version 460
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require

struct TriGPU { vec3 p0; uint c0; vec3 p1; uint c1; vec3 p2; uint c2; };
layout(buffer_reference, std430) readonly buffer TBuf { TriGPU trs[]; };

layout(push_constant) uniform Push {
  mat4  viewProj;
  vec2  viewportSize;  // unused
  uvec2 baseAddr;
  uint  offPoints;
  uint  offLines;
  uint  offTris;       // byte offset to TriGPU[]
  uint  mode;
} PC;

layout(location=0) out vec4 vColor;

vec4 unpackRGBA(uint c){  // <-- FIXED
  return vec4(
    float((c>>24)&0xFFu)/255.0,
    float((c>>16)&0xFFu)/255.0,
    float((c>> 8)&0xFFu)/255.0,
    float((c>> 0)&0xFFu)/255.0 );
}

uvec2 addBytes(uvec2 a, uint off){ uint lo=a.x+off; return uvec2(lo, a.y + uint(lo<a.x)); }

void main() {
  TBuf B = TBuf(addBytes(PC.baseAddr, PC.offTris));
  uint tri = uint(gl_VertexIndex) / 3u;
  uint vid = uint(gl_VertexIndex) % 3u;

  TriGPU t = B.trs[tri];
  vec3 P = (vid==0u)? t.p0 : (vid==1u)? t.p1 : t.p2;
  uint C = (vid==0u)? t.c0 : (vid==1u)? t.c1 : t.c2;

  gl_Position = PC.viewProj * vec4(P, 1.0);
  vColor = unpackRGBA(C);
}
)";

static const char* kTriFS = R"(
#version 460
layout(location=0) in  vec4 vColor;
layout(location=0) out vec4 outColor;
void main(){ outColor = vColor; }
)";

// ============================================================================
// Implementation
// ============================================================================

Im3dRenderFeature::~Im3dRenderFeature() = default;
const char* Im3dRenderFeature::GetName() const { return "Im3dRenderFeature"; }

void Im3dRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
    PassDeclarationInfo passInfo;
    passInfo.framebufferDebugName = "Im3dDraw";
    passInfo.colorAttachments[0] = {
      .textureName = RenderResources::SCENE_COLOR,
      .loadOp = vk::LoadOp::Load,
      .storeOp = vk::StoreOp::Store
    };
    passInfo.depthAttachment = {
      .textureName = RenderResources::SCENE_DEPTH,
      .loadOp = vk::LoadOp::Load,
      .storeOp = vk::StoreOp::Store
    };

    // Single SSBO for all primitives
    vk::BufferDesc ssboDesc{
      .usage = vk::BufferUsageBits::BufferUsageBits_Storage,
      .storage = vk::StorageType::HostVisible,
      .size = 256 * 1024,
      .debugName = "Im3dSSBO"
    };

    passBuilder.CreatePass()
        .DeclareTransientResource("Im3dSSBO", ssboDesc)
        .UseResource("Im3dSSBO", AccessType::Read)
        .UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite)
        .UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite)
        .SetPriority(internal::RenderPassBuilder::PassPriority::Transparent)
        .AddGraphicsPass("Im3dDraw", passInfo,
            [this](const internal::ExecutionContext& ctx) { ExecuteDrawPass(ctx); });
}

void Im3dRenderFeature::EnsurePipelinesCreated(const internal::ExecutionContext& context)
{
    const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
    auto& ctx = context.GetvkContext();

    auto colorHandle = context.GetTexture(RenderResources::SCENE_COLOR);
    auto depthHandle = context.GetTexture(RenderResources::SCENE_DEPTH);

    if (pointPipeline_.valid() && linePipeline_.valid() && trianglePipeline_.valid()
        && cachedSamples_ == params.pipelineSamples)
        return;

    // Compile shaders
    pointVS_ = ctx.createShaderModule({ kPointVS, vk::ShaderStage::Vert, "Im3d Point VS" });
    pointFS_ = ctx.createShaderModule({ kPointFS, vk::ShaderStage::Frag, "Im3d Point FS" });
    lineVS_ = ctx.createShaderModule({ kLineVS,  vk::ShaderStage::Vert, "Im3d Line VS" });
    lineFS_ = ctx.createShaderModule({ kLineFS,  vk::ShaderStage::Frag, "Im3d Line FS" });
    triVS_ = ctx.createShaderModule({ kTriVS,   vk::ShaderStage::Vert, "Im3d Tri VS" });
    triFS_ = ctx.createShaderModule({ kTriFS,   vk::ShaderStage::Frag, "Im3d Tri FS" });

    auto makeDesc = [&](vk::Topology topo, const char* name, vk::ShaderModuleHandle vs, vk::ShaderModuleHandle fs) {
        vk::RenderPipelineDesc d{};
        d.smVert = vs;
        d.smFrag = fs;
        if (colorHandle.valid()) {
            d.color[0].format = ctx.getFormat(colorHandle);
            d.color[0].blendEnabled = true;
            d.color[0].rgbBlendOp = vk::BlendOp::Add;
            d.color[0].alphaBlendOp = vk::BlendOp::Add;
            d.color[0].srcRGBBlendFactor = vk::BlendFactor::SrcAlpha;
            d.color[0].dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
            d.color[0].srcAlphaBlendFactor = vk::BlendFactor::One;
            d.color[0].dstAlphaBlendFactor = vk::BlendFactor::OneMinusSrcAlpha;
        }
        d.depthFormat = depthHandle.valid() ? ctx.getFormat(depthHandle) : vk::Format::Invalid;
        d.cullMode = vk::CullMode::None;
        d.samplesCount = params.pipelineSamples;
        d.topology = topo;
        d.debugName = name;
        return d;
        };

    pointPipeline_ = ctx.createRenderPipeline(makeDesc(vk::Topology::Point, "Im3d Points", pointVS_, pointFS_));
    linePipeline_ = ctx.createRenderPipeline(makeDesc(vk::Topology::Triangle, "Im3d Lines", lineVS_, lineFS_));
    trianglePipeline_ = ctx.createRenderPipeline(makeDesc(vk::Topology::Triangle, "Im3d Triangles", triVS_, triFS_));

    cachedSamples_ = params.pipelineSamples;
}

void Im3dRenderFeature::ExecuteDrawPass(const internal::ExecutionContext& context)
{
    const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
    if (!params.enabled) return;

    EnsurePipelinesCreated(context);
    if (!pointPipeline_.valid() || !linePipeline_.valid() || !trianglePipeline_.valid())
        return;

    if (params.packed.empty()) return;

    auto& cmd = context.GetvkCommandBuffer();
    auto& vkCtx = context.GetvkContext();

    // Ensure SSBO capacity (grow x2 policy)
    const size_t total = params.packed.size();
    size_t current = context.GetBufferSize("Im3dSSBO");
    if (total > current) {
        size_t newSize = current ? std::max(total, current * 2) : std::max<size_t>(total, 256 * 1024);
        context.ResizeBuffer("Im3dSSBO", newSize);
    }
    auto ssbo = context.GetBuffer("Im3dSSBO");

    // Single mapped write + flush
    auto* dst = vkCtx.getMappedPtr(ssbo);
    std::memcpy(dst, params.packed.data(), total);
    vkCtx.flushMappedMemory(ssbo, 0, total);

    // gpuAddress split to uvec2
    const uint64_t baseAddr64 = vkCtx.gpuAddress(ssbo);
    glm::uvec2 baseAddrSplit;
    std::memcpy(&baseAddrSplit, &baseAddr64, sizeof(baseAddrSplit));

    // Common push constants
    struct Push {
        glm::mat4  viewProj;
        glm::vec2  viewportSize; // not used by these paths
        glm::uvec2 baseAddr;
        uint32_t   offPoints;
        uint32_t   offLines;
        uint32_t   offTris;
        uint32_t   mode; // unused
    } pc{};

    const auto& fd = context.GetFrameData();
    pc.viewProj = fd.projMatrix * fd.viewMatrix; // correct order
    pc.viewportSize = { float(fd.screenWidth), float(fd.screenHeight) };
    pc.baseAddr = baseAddrSplit;
    pc.offPoints = params.offPoints;
    pc.offLines = params.offLines;
    pc.offTris = params.offTris;

    // Common state
    const float w = float(fd.screenWidth), h = float(fd.screenHeight);
    cmd.cmdBindViewport({ .x = 0.f, .y = 0.f, .width = w, .height = h });
    cmd.cmdBindScissorRect({ .x = 0u, .y = 0u, .width = (uint32_t)w, .height = (uint32_t)h });
    cmd.cmdBindDepthState({ .compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false });

    // Points
    if (params.numPoints) {
        cmd.cmdBindRenderPipeline(pointPipeline_);
        cmd.cmdPushConstants(pc);
        cmd.cmdDraw(params.numPoints);
    }
    if (params.numLines) {
        cmd.cmdBindRenderPipeline(linePipeline_);   // TriangleStrip pipeline
        cmd.cmdPushConstants(pc);
        cmd.cmdDraw(/*vertexCount=*/6, /*instanceCount=*/params.numLines);
    }
    // Triangles
    if (params.numTris) {
        cmd.cmdBindRenderPipeline(trianglePipeline_);
        cmd.cmdPushConstants(pc);
        cmd.cmdDraw(params.numTris * 3);
    }
}
