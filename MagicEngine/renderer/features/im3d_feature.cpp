#include "im3d_feature.h"
#include "../hina_context.h"
#include "../linear_color.h"
#include <hina_vk.h>
#include <cstring>   // memcpy
#include <glm/glm.hpp>
#include <iostream>
// ============================================================================
// Shaders (HSL format, compiled via hslc_compile_hsl_source)
// ============================================================================

// --- Point shader ---
static const char* kPointShaderHSL = R"(#hina
push_constant Push {
  mat4 viewProj;
  vec2 viewportSize;
} PC;

struct VertexIn {
  vec4 aPositionSize;
  uint aColor;
};

struct Varyings {
  vec4 vColor;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
vec4 unpackColor(uint c) {
  return vec4(
    float((c >> 24u) & 0xFFu) / 255.0,
    float((c >> 16u) & 0xFFu) / 255.0,
    float((c >>  8u) & 0xFFu) / 255.0,
    float((c       ) & 0xFFu) / 255.0);
}

Varyings VSMain(VertexIn in) {
  Varyings out;
  vec4 clipPos = PC.viewProj * vec4(in.aPositionSize.xyz, 1.0);
  gl_Position = clipPos;

  float projScale = abs(PC.viewProj[1][1]);
  gl_PointSize = max(1.0, (in.aPositionSize.w * projScale * PC.viewportSize.y * 0.5) / clipPos.w);

  out.vColor = unpackColor(in.aColor);
  return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
  FragOut out;
  vec2 uv = gl_PointCoord * 2.0 - 1.0;
  float d = length(uv);
  if (d > 1.0) discard;
  float alpha = 1.0 - smoothstep(0.9, 1.0, d);
  out.color = vec4(in.vColor.rgb, in.vColor.a * alpha);
  return out;
}
#hina_end
)";

// --- Line shader ---
// Instance-rate: each instance = 2 vertices (line segment endpoints)
// gl_VertexIndex 0-5 expands each segment into a screen-space quad
static const char* kLineShaderHSL = R"(#hina
push_constant Push {
  mat4 viewProj;
  vec2 viewportSize;
} PC;

struct VertexIn {
  vec4 aP0Size;
  uint aC0;
  vec4 aP1Size;
  uint aC1;
};

struct Varyings {
  vec4 vColor;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
vec4 unpackColor(uint c) {
  return vec4(
    float((c >> 24u) & 0xFFu) / 255.0,
    float((c >> 16u) & 0xFFu) / 255.0,
    float((c >>  8u) & 0xFFu) / 255.0,
    float((c       ) & 0xFFu) / 255.0);
}

Varyings VSMain(VertexIn in) {
  Varyings out;
  float sizeWorld = 0.5 * (in.aP0Size.w + in.aP1Size.w);

  vec4 p0 = PC.viewProj * vec4(in.aP0Size.xyz, 1.0);
  vec4 p1 = PC.viewProj * vec4(in.aP1Size.xyz, 1.0);

  const float kNearW = 0.001;
  if (p0.w < kNearW && p1.w < kNearW) {
    gl_Position = vec4(0, 0, 0, 0);
    out.vColor = vec4(0);
    return out;
  }
  if (p0.w < kNearW) {
    float t = (kNearW - p0.w) / (p1.w - p0.w);
    p0 = mix(p0, p1, t);
  }
  if (p1.w < kNearW) {
    float t = (kNearW - p1.w) / (p0.w - p1.w);
    p1 = mix(p1, p0, t);
  }

  vec2 p0_ndc = p0.xy / p0.w;
  vec2 p1_ndc = p1.xy / p1.w;
  vec2 dir_s = p1_ndc - p0_ndc;
  if (dot(dir_s, dir_s) < 0.0000001) {
    gl_Position = vec4(0, 0, 0, 0);
    out.vColor = vec4(0);
    return out;
  }
  dir_s = normalize(dir_s);
  vec2 perp_s = vec2(-dir_s.y, dir_s.x);

  float aspect = PC.viewportSize.x / PC.viewportSize.y;
  float projScaleY = abs(PC.viewProj[1][1]);
  vec2 normal_clip = perp_s * sizeWorld * projScaleY * 0.5;
  normal_clip.x /= aspect;

  uint vid = uint(gl_VertexIndex) % 6u;
  vec4 pos;
  vec2 expansion;
  if      (vid == 0u) { pos = p0; expansion = -normal_clip; }
  else if (vid == 1u) { pos = p0; expansion =  normal_clip; }
  else if (vid == 2u) { pos = p1; expansion =  normal_clip; }
  else if (vid == 3u) { pos = p0; expansion = -normal_clip; }
  else if (vid == 4u) { pos = p1; expansion =  normal_clip; }
  else                { pos = p1; expansion = -normal_clip; }

  gl_Position = pos + vec4(expansion, 0.0, 0.0);

  vec4 c0 = unpackColor(in.aC0);
  vec4 c1 = unpackColor(in.aC1);
  out.vColor = (vid <= 1u || vid == 3u) ? c0 : c1;
  return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
  FragOut out;
  out.color = in.vColor;
  return out;
}
#hina_end
)";

// --- Triangle shader ---
static const char* kTriShaderHSL = R"(#hina
push_constant Push {
  mat4 viewProj;
  vec2 viewportSize;
} PC;

struct VertexIn {
  vec4 aPositionSize;
  uint aColor;
};

struct Varyings {
  vec4 vColor;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
vec4 unpackColor(uint c) {
  return vec4(
    float((c >> 24u) & 0xFFu) / 255.0,
    float((c >> 16u) & 0xFFu) / 255.0,
    float((c >>  8u) & 0xFFu) / 255.0,
    float((c       ) & 0xFFu) / 255.0);
}

Varyings VSMain(VertexIn in) {
  Varyings out;
  gl_Position = PC.viewProj * vec4(in.aPositionSize.xyz, 1.0);
  out.vColor = unpackColor(in.aColor);
  return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
  FragOut out;
  out.color = in.vColor;
  return out;
}
#hina_end
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
    .textureName = RenderResources::SCENE_COLOR, .loadOp = gfx::LoadOp::Load, .storeOp = gfx::StoreOp::Store
  };
  passInfo.depthAttachment = {
    .textureName = RenderResources::SCENE_DEPTH, .loadOp = gfx::LoadOp::Load, .storeOp = gfx::StoreOp::Store
  };
  // Single CPU-mapped vertex buffer for all primitives
  gfx::BufferDesc vbDesc{
    .size = 256 * 1024,
    .memory = gfx::BufferMemory::CPU,
    .usage = gfx::BufferUsage::Vertex
  };
  passBuilder.CreatePass().DeclareTransientResource("Im3dVB", vbDesc).UseResource("Im3dVB", AccessType::ReadWrite).
              UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).
              UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite).
              SetPriority(internal::RenderPassBuilder::PassPriority::Transparent).
              ExecuteAfter("SceneComposite").
              AddGraphicsPass(
                "Im3dDraw", passInfo, [this](const internal::ExecutionContext& ctx) { ExecuteDrawPass(ctx); });
}

void Im3dRenderFeature::EnsurePipelinesCreated(const internal::ExecutionContext& context)
{
  const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
  HinaContext* hinaCtx = context.GetHinaContext();
  if (!hinaCtx) return;

  auto depthHandle = context.GetTexture(RenderResources::SCENE_DEPTH);

  bool pointValid = gfx::isValid(pointPipeline_.get());
  bool lineValid = gfx::isValid(linePipeline_.get());
  bool triValid = gfx::isValid(trianglePipeline_.get());
  bool samplesMatch = cachedSamples_ == params.pipelineSamples;

  if (pointValid && lineValid && triValid && samplesMatch)
    return;

  // Formats
  hina_format colorFormat = LinearColor::HDR_SCENE_FORMAT;
  hina_format depthFormat = gfx::isValid(depthHandle) ? HINA_FORMAT_D32_SFLOAT : HINA_FORMAT_UNDEFINED;

  // sizeof(Im3d::VertexData) = 20 bytes: Vec4(16) + Color(4)
  constexpr uint32_t kVertexStride = 20; // sizeof(Im3d::VertexData)

  // Common pipeline state setup
  auto setupCommonState = [&](hina_hsl_pipeline_desc& pipDesc) {
    pipDesc.cull_mode = HINA_CULL_MODE_NONE;
    pipDesc.polygon_mode = HINA_POLYGON_MODE_FILL;
    pipDesc.samples = static_cast<hina_sample_count>(params.pipelineSamples);

    // Color attachment with alpha blending
    pipDesc.color_formats[0] = colorFormat;
    pipDesc.blend[0].enable = true;
    pipDesc.blend[0].src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
    pipDesc.blend[0].dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipDesc.blend[0].src_alpha = HINA_BLEND_FACTOR_ONE;
    pipDesc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pipDesc.blend[0].color_op = HINA_BLEND_OP_ADD;
    pipDesc.blend[0].alpha_op = HINA_BLEND_OP_ADD;

    // Depth (always pass, no write - for debug overlay)
    pipDesc.depth_format = depthFormat;
    pipDesc.depth.depth_test = (depthFormat != HINA_FORMAT_UNDEFINED);
    pipDesc.depth.depth_write = false;
    pipDesc.depth.depth_compare = HINA_COMPARE_OP_ALWAYS;
  };

  // Helper to compile HSL and create pipeline
  auto compileHSL = [](const char* hsl, const char* name) -> hina_hsl_module* {
    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(hsl, name, &error);
    if (!module) {
      std::cout << "[Im3D] HSL compile failed for " << name;
      if (error) { std::cout << ": " << error; hslc_free_log(error); }
      std::cout << std::endl;
    }
    return module;
  };

  // --- Point pipeline ---
  {
    hina_hsl_module* module = compileHSL(kPointShaderHSL, "im3d_point");
    if (module) {
      hina_hsl_pipeline_desc pipDesc = hina_hsl_pipeline_desc_default();
      pipDesc.primitive_topology = HINA_PRIMITIVE_TOPOLOGY_POINT_LIST;
      setupCommonState(pipDesc);

      pipDesc.layout.buffer_count = 1;
      pipDesc.layout.buffer_strides[0] = kVertexStride;
      pipDesc.layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
      pipDesc.layout.attr_count = 2;
      pipDesc.layout.attrs[0] = { HINA_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0 };
      pipDesc.layout.attrs[1] = { HINA_FORMAT_R32_UINT, 16, 1, 0 };

      pointPipeline_.reset(hina_make_pipeline_from_module(module, &pipDesc, nullptr));
      hslc_hsl_module_free(module);
    }
  }

  // --- Line pipeline ---
  {
    hina_hsl_module* module = compileHSL(kLineShaderHSL, "im3d_line");
    if (module) {
      hina_hsl_pipeline_desc pipDesc = hina_hsl_pipeline_desc_default();
      pipDesc.primitive_topology = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      setupCommonState(pipDesc);

      // Instance-rate: each instance = a line segment (2 vertices, stride=40)
      pipDesc.layout.buffer_count = 1;
      pipDesc.layout.buffer_strides[0] = kVertexStride * 2;
      pipDesc.layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_INSTANCE;
      pipDesc.layout.attr_count = 4;
      pipDesc.layout.attrs[0] = { HINA_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0 };
      pipDesc.layout.attrs[1] = { HINA_FORMAT_R32_UINT, 16, 1, 0 };
      pipDesc.layout.attrs[2] = { HINA_FORMAT_R32G32B32A32_SFLOAT, 20, 2, 0 };
      pipDesc.layout.attrs[3] = { HINA_FORMAT_R32_UINT, 36, 3, 0 };

      linePipeline_.reset(hina_make_pipeline_from_module(module, &pipDesc, nullptr));
      hslc_hsl_module_free(module);
    }
  }

  // --- Triangle pipeline ---
  {
    hina_hsl_module* module = compileHSL(kTriShaderHSL, "im3d_tri");
    if (module) {
      hina_hsl_pipeline_desc pipDesc = hina_hsl_pipeline_desc_default();
      pipDesc.primitive_topology = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      setupCommonState(pipDesc);

      pipDesc.layout.buffer_count = 1;
      pipDesc.layout.buffer_strides[0] = kVertexStride;
      pipDesc.layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
      pipDesc.layout.attr_count = 2;
      pipDesc.layout.attrs[0] = { HINA_FORMAT_R32G32B32A32_SFLOAT, 0, 0, 0 };
      pipDesc.layout.attrs[1] = { HINA_FORMAT_R32_UINT, 16, 1, 0 };

      trianglePipeline_.reset(hina_make_pipeline_from_module(module, &pipDesc, nullptr));
      hslc_hsl_module_free(module);
    }
  }

  cachedSamples_ = params.pipelineSamples;
}

void Im3dRenderFeature::ExecuteDrawPass(const internal::ExecutionContext& context)
{
  const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
  if (!params.enabled) return;
  EnsurePipelinesCreated(context);
  if (!gfx::isValid(pointPipeline_.get()) || !gfx::isValid(linePipeline_.get()) ||
      !gfx::isValid(trianglePipeline_.get()))
    return;

  const uint32_t numPoints = (uint32_t)params.pointVertices.size();
  const uint32_t numLineVerts = (uint32_t)params.lineVertices.size();
  const uint32_t numTriVerts = (uint32_t)params.triVertices.size();

  if (numPoints == 0 && numLineVerts == 0 && numTriVerts == 0) return;

  constexpr size_t vertSize = sizeof(Im3d::VertexData); // 20 bytes

  // Calculate buffer layout: [points | lines | triangles]
  const size_t bytesPoints = numPoints * vertSize;
  const size_t bytesLines = numLineVerts * vertSize;
  const size_t bytesTris = numTriVerts * vertSize;
  const size_t totalBytes = bytesPoints + bytesLines + bytesTris;

  // Ensure vertex buffer capacity (grow x2 policy)
  size_t currentSize = context.GetBufferSize("Im3dVB");
  if (totalBytes > currentSize)
  {
    size_t newSize = currentSize ? std::max(totalBytes, currentSize * 2) : std::max<size_t>(totalBytes, 256 * 1024);
    context.ResizeBuffer("Im3dVB", newSize);
  }

  auto vb = context.GetBuffer("Im3dVB");

  // Build a contiguous upload buffer, then upload in one shot
  std::vector<uint8_t> uploadBuf(totalBytes);
  size_t off = 0;
  if (bytesPoints > 0) { std::memcpy(uploadBuf.data() + off, params.pointVertices.data(), bytesPoints); off += bytesPoints; }
  if (bytesLines > 0) { std::memcpy(uploadBuf.data() + off, params.lineVertices.data(), bytesLines); off += bytesLines; }
  if (bytesTris > 0) { std::memcpy(uploadBuf.data() + off, params.triVertices.data(), bytesTris); off += bytesTris; }
  hina_upload_buffer(vb, uploadBuf.data(), totalBytes);

  // Push constants
  struct Push
  {
    glm::mat4 viewProj;
    glm::vec2 viewportSize;
  } pc{};
  const auto& fd = context.GetFrameData();
  pc.viewProj = fd.projMatrix * fd.viewMatrix;
  pc.viewportSize = { float(fd.screenWidth), float(fd.screenHeight) };


  auto& cmd = context.GetCommandBuffer();
  const float w = float(fd.screenWidth), h = float(fd.screenHeight);
  cmd.setViewport({ .x = 0.f, .y = 0.f, .width = w, .height = h });
  cmd.setScissor({ .x = 0, .y = 0, .width = (uint32_t)w, .height = (uint32_t)h });


  // Points
  if (numPoints > 0)
  {
    cmd.bindPipeline(pointPipeline_.get());
    cmd.pushConstants(pc);
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = vb;
    vertInput.vertex_offsets[0] = 0; // points start at offset 0
    cmd.applyVertexInput(vertInput);
    cmd.draw(numPoints);
  }

  // Lines (instanced: 6 vertices per instance, each instance = 2 consecutive VertexData)
  if (numLineVerts > 0)
  {
    const uint32_t numLines = numLineVerts / 2;
    cmd.bindPipeline(linePipeline_.get());
    cmd.pushConstants(pc);
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = vb;
    vertInput.vertex_offsets[0] = bytesPoints; // lines start after points
    cmd.applyVertexInput(vertInput);
    cmd.draw(/*vertexCount=*/6, /*instanceCount=*/numLines);
  }

  // Triangles
  if (numTriVerts > 0)
  {
    cmd.bindPipeline(trianglePipeline_.get());
    cmd.pushConstants(pc);
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = vb;
    vertInput.vertex_offsets[0] = bytesPoints + bytesLines; // tris start after lines
    cmd.applyVertexInput(vertInput);
    cmd.draw(numTriVerts);
  }
}
