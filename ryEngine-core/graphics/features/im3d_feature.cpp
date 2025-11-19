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

struct PointGPU { vec3 p; float sizeWorld; uint abgr; uint pad0; uint pad1; uint pad2; };
layout(buffer_reference, std430) readonly buffer PBuf { PointGPU pts[]; };

layout(push_constant) uniform Push {
  mat4  viewProj;
  vec2  viewportSize;  // needed for world-to-pixel conversion
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

  vec4 clipPos = PC.viewProj * vec4(p.p, 1.0);
  gl_Position = clipPos;

  // Convert world-space size to screen-space pixels
  // The viewProj[1][1] element gives us the vertical projection scale
  // For perspective: screenSize = (worldSize * projScale * viewportHeight) / (2 * clipW)
  float projScale = abs(PC.viewProj[1][1]);
  gl_PointSize = (p.sizeWorld * projScale * PC.viewportSize.y * 0.5) / clipPos.w;

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

struct LineGPU { vec3 p0; float sizeWorld; vec3 p1; uint abgr; };
layout(buffer_reference, std430) readonly buffer LBuf { LineGPU lns[]; };

layout(push_constant) uniform Push {
  mat4  viewProj;
  vec2  viewportSize; 
  uvec2 baseAddr;
  uint  offPoints;
  uint  offLines;
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
  LBuf B = LBuf(addBytes(PC.baseAddr, PC.offLines));
  LineGPU s = B.lns[gl_InstanceIndex];

  // 1. Transform to Clip Space
  vec4 p0 = PC.viewProj * vec4(s.p0, 1.0);
  vec4 p1 = PC.viewProj * vec4(s.p1, 1.0);

  // 2. Manual Near-Plane Clipping
  // If points are behind the camera (w < epsilon), we must clip them.
  // Otherwise, the perspective divide (xyz / w) will invert coordinates 
  // or explode, causing the massive stretching artifacts.
  const float kNearW = 0.001;
  
  // If both are behind, discard the primitive
  if (p0.w < kNearW && p1.w < kNearW) {
    gl_Position = vec4(0,0,0,0); // Degenerate
    return;
  }

  // Clip p0 against Near Plane
  if (p0.w < kNearW) {
    float t = (kNearW - p0.w) / (p1.w - p0.w);
    p0 = mix(p0, p1, t);
  }
  // Clip p1 against Near Plane
  if (p1.w < kNearW) {
    float t = (kNearW - p1.w) / (p0.w - p1.w);
    p1 = mix(p1, p0, t);
  }

  // 3. Screen Space Direction
  vec2 p0_ndc = p0.xy / p0.w;
  vec2 p1_ndc = p1.xy / p1.w;
  
  vec2 dir_s = p1_ndc - p0_ndc;
  
  // Handle zero-length after clipping
  if (dot(dir_s, dir_s) < 0.0000001) {
     gl_Position = vec4(0,0,0,0);
     return;
  }
  
  dir_s = normalize(dir_s);
  vec2 perp_s = vec2(-dir_s.y, dir_s.x);

  // 4. Calculate Offset
  // We want 'sizeWorld' thickness.
  // In NDC, visual width = offset / w. 
  // Therefore, we need a Constant offset in Clip Space to achieve 1/w scaling in NDC.
  
  // Correct for aspect ratio so lines look uniform width
  float aspect = PC.viewportSize.x / PC.viewportSize.y;
  
  // Projection scale (usually 1.0 / tan(fov/2))
  float projScaleY = abs(PC.viewProj[1][1]); 
  
  // Calculate expansion vector in Clip Space
  vec2 normal_clip = perp_s * s.sizeWorld * projScaleY * 0.5;
  normal_clip.x /= aspect; // Correct X for aspect ratio

  // 5. Expand Quad
  uint vid = gl_VertexIndex % 6;
  vec4 pos = (vid < 3) ? p0 : p1; // 0,1,2 -> p0; 3,4,5 -> p1
  
  // Expansion direction pattern for a strip-like quad 
  // (0: -1, 1: +1, 2: +1) for p0 logic, effectively
  float sign = (vid == 0 || vid == 3 || vid == 5) ? -1.0 : 1.0;
  
  vec2 expansion;
  if      (vid == 0) { pos = p0; expansion = -normal_clip; }
  else if (vid == 1) { pos = p0; expansion =  normal_clip; }
  else if (vid == 2) { pos = p1; expansion =  normal_clip; }
  else if (vid == 3) { pos = p0; expansion = -normal_clip; }
  else if (vid == 4) { pos = p1; expansion =  normal_clip; }
  else               { pos = p1; expansion = -normal_clip; }

  // Apply offset directly to Clip Position.
  // When hardware divides by W later, the visual width becomes (expansion / pos.w).
  // This creates the desired world-space size effect.
  gl_Position = pos + vec4(expansion, 0.0, 0.0);
  
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
    .textureName = RenderResources::SCENE_COLOR, .loadOp = vk::LoadOp::Load, .storeOp = vk::StoreOp::Store
  };
  passInfo.depthAttachment = {
    .textureName = RenderResources::SCENE_DEPTH, .loadOp = vk::LoadOp::Load, .storeOp = vk::StoreOp::Store
  };
  // Single SSBO for all primitives
  vk::BufferDesc ssboDesc{
    .usage = vk::BufferUsageBits::BufferUsageBits_Storage, .storage = vk::StorageType::HostVisible, .size = 256 * 1024,
    .debugName = "Im3dSSBO"
  };
  passBuilder.CreatePass().DeclareTransientResource("Im3dSSBO", ssboDesc).UseResource("Im3dSSBO", AccessType::Read).
              UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).
              UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite).
              SetPriority(internal::RenderPassBuilder::PassPriority::Transparent).AddGraphicsPass(
                "Im3dDraw", passInfo, [this](const internal::ExecutionContext& ctx) { ExecuteDrawPass(ctx); });
}

void Im3dRenderFeature::EnsurePipelinesCreated(const internal::ExecutionContext& context)
{
  const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
  auto& ctx = context.GetvkContext();
  auto colorHandle = context.GetTexture(RenderResources::SCENE_COLOR);
  auto depthHandle = context.GetTexture(RenderResources::SCENE_DEPTH);
  if (pointPipeline_.valid() && linePipeline_.valid() && trianglePipeline_.valid() && cachedSamples_ == params.
    pipelineSamples) return;
  // Compile shaders
  pointVS_ = ctx.createShaderModule({kPointVS, vk::ShaderStage::Vert, "Im3d Point VS"});
  pointFS_ = ctx.createShaderModule({kPointFS, vk::ShaderStage::Frag, "Im3d Point FS"});
  lineVS_ = ctx.createShaderModule({kLineVS, vk::ShaderStage::Vert, "Im3d Line VS"});
  lineFS_ = ctx.createShaderModule({kLineFS, vk::ShaderStage::Frag, "Im3d Line FS"});
  triVS_ = ctx.createShaderModule({kTriVS, vk::ShaderStage::Vert, "Im3d Tri VS"});
  triFS_ = ctx.createShaderModule({kTriFS, vk::ShaderStage::Frag, "Im3d Tri FS"});
  auto makeDesc = [&](vk::Topology topo, const char* name, vk::ShaderModuleHandle vs, vk::ShaderModuleHandle fs)
  {
    vk::RenderPipelineDesc d{};
    d.smVert = vs;
    d.smFrag = fs;
    if (colorHandle.valid())
    {
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
  if (!pointPipeline_.valid() || !linePipeline_.valid() || !trianglePipeline_.valid()) return;
  if (params.packed.empty()) return;
  auto& cmd = context.GetvkCommandBuffer();
  auto& vkCtx = context.GetvkContext();
  // Ensure SSBO capacity (grow x2 policy)
  const size_t total = params.packed.size();
  size_t current = context.GetBufferSize("Im3dSSBO");
  if (total > current)
  {
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
  struct Push
  {
    glm::mat4 viewProj;
    glm::vec2 viewportSize; // not used by these paths
    glm::uvec2 baseAddr;
    uint32_t offPoints;
    uint32_t offLines;
    uint32_t offTris;
    uint32_t mode; // unused
  } pc{};
  const auto& fd = context.GetFrameData();
  pc.viewProj = fd.projMatrix * fd.viewMatrix; // correct order
  pc.viewportSize = {float(fd.screenWidth), float(fd.screenHeight)};
  pc.baseAddr = baseAddrSplit;
  pc.offPoints = params.offPoints;
  pc.offLines = params.offLines;
  pc.offTris = params.offTris;
  // Common state
  const float w = float(fd.screenWidth), h = float(fd.screenHeight);
  cmd.cmdBindViewport({.x = 0.f, .y = 0.f, .width = w, .height = h});
  cmd.cmdBindScissorRect({.x = 0u, .y = 0u, .width = (uint32_t)w, .height = (uint32_t)h});
  cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false});
  // Points
  if (params.numPoints)
  {
    cmd.cmdBindRenderPipeline(pointPipeline_);
    cmd.cmdPushConstants(pc);
    cmd.cmdDraw(params.numPoints);
  }
  if (params.numLines)
  {
    cmd.cmdBindRenderPipeline(linePipeline_); // TriangleStrip pipeline
    cmd.cmdPushConstants(pc);
    cmd.cmdDraw(/*vertexCount=*/6, /*instanceCount=*/params.numLines);
  }
  // Triangles
  if (params.numTris)
  {
    cmd.cmdBindRenderPipeline(trianglePipeline_);
    cmd.cmdPushConstants(pc);
    cmd.cmdDraw(params.numTris * 3);
  }
}
