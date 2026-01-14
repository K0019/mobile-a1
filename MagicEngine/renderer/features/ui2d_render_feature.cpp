#include "renderer/features/ui2d_render_feature.h"
#include "renderer/hina_context.h"
#include "renderer/gfx_renderer.h"
#include "renderer/linear_color.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>
#include <iostream>

namespace
{
  // HSL shader for UI2D rendering
  static const char* kUi2DShaderHSL = R"(#hina
group Ui2D = 0;

bindings(Ui2D, start=0) {
  texture sampler2D uTexture;
}

push_constant PushConstants {
  vec4 LRTB;
} pc;

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
  vec4 a_color;
};

struct Varyings {
  vec4 color;
  vec2 uv;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;

    float L = pc.LRTB.x;
    float R = pc.LRTB.y;
    float T = pc.LRTB.z;
    float B = pc.LRTB.w;

    // Orthographic projection for Vulkan UI (Y flipped: Y=0 at top, Y=height at bottom)
    mat4 proj = mat4(
        2.0 / (R - L), 0.0, 0.0, 0.0,
        0.0, 2.0 / (T - B), 0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        (R + L) / (L - R), (T + B) / (B - T), 0.0, 1.0
    );

    out.color = in.a_color;
    out.uv = in.a_uv;
    gl_Position = proj * vec4(in.a_position, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;
    out.color = in.color * texture(uTexture, in.uv);
    return out;
}
#hina_end
)";

  constexpr const char* kVertexBufferName = "Ui2DVertexBuffer";
  constexpr const char* kIndexBufferName = "Ui2DIndexBuffer";
} // namespace

void Ui2DRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  gfx::BufferDesc vertexDesc{
    .size = 64 * 1024,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Vertex | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
  };
  gfx::BufferDesc indexDesc{
    .size = 32 * 1024,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Index | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
  };
  PassDeclarationInfo passInfo;
  passInfo.framebufferDebugName = "Ui2DOverlay";
  passInfo.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR, .loadOp = gfx::LoadOp::Load, .storeOp = gfx::StoreOp::Store,
  };
  // No depth attachment - UI2D pipeline doesn't use depth testing/writing
  passBuilder.CreatePass().DeclareTransientResource(kVertexBufferName, vertexDesc).
              DeclareTransientResource(kIndexBufferName, indexDesc).
              UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).
              UseResource(kVertexBufferName, AccessType::ReadWrite).UseResource(kIndexBufferName, AccessType::ReadWrite)
              .SetPriority(internal::RenderPassBuilder::PassPriority::UI)
              .ExecuteAfter("SceneComposite")  // UI2D must run AFTER composite populates SCENE_COLOR
              .AddGraphicsPass(
                "Ui2DRender", passInfo, [this](const internal::ExecutionContext& context)
                {
                  RenderUi(context);
                });
}

void Ui2DRenderFeature::EnsurePipeline(const internal::ExecutionContext& context)
{
  if (resourcesCreated_) return;

  GfxRenderer* gfxRenderer = context.GetGfxRenderer();

  // Compile HSL shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kUi2DShaderHSL, "ui2d_feature_shader", &error);
  if (!module) {
    std::cerr << "[UI2D] Shader compilation failed: " << (error ? error : "Unknown") << std::endl;
    if (error) hslc_free_log(error);
    return;
  }

  // Vertex layout matching PrimitiveVertex: {float x, float y, float u, float v, uint32_t color}
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 1;
  vertex_layout.buffer_strides[0] = sizeof(ui::PrimitiveVertex);
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.attr_count = 3;
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ui::PrimitiveVertex, x), 0, 0 };      // position
  vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ui::PrimitiveVertex, u), 1, 0 };      // uv
  vertex_layout.attrs[2] = { HINA_FORMAT_R8G8B8A8_UNORM, offsetof(ui::PrimitiveVertex, color), 2, 0 }; // color (auto-unpacked to vec4)

  // Pipeline descriptor
  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.primitive_topology = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pip_desc.polygon_mode = HINA_POLYGON_MODE_FILL;
  pip_desc.samples = HINA_SAMPLE_COUNT_1_BIT;

  // Blend state for alpha blending
  pip_desc.blend.enable = true;
  pip_desc.blend.src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
  pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
  pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;

  // Color attachment format: use HDR scene format since we render to SCENE_COLOR
  pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;
  pip_desc.color_attachment_count = 1;

  // No depth testing for UI
  pip_desc.depth.depth_test = false;
  pip_desc.depth.depth_write = false;
  pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

  // Use the shared UI bind group layout
  gfx::BindGroupLayout uiLayout = gfxRenderer->getUIBindGroupLayout();
  if (hina_bind_group_layout_is_valid(uiLayout)) {
    pip_desc.bind_group_layouts[0] = uiLayout;
    pip_desc.bind_group_layout_count = 1;
  }

  pipeline_.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(pipeline_.get())) {
    std::cerr << "[UI2D] Pipeline creation failed" << std::endl;
    return;
  }

  resourcesCreated_ = true;
}

void Ui2DRenderFeature::RenderUi(const internal::ExecutionContext& context)
{
  const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
  if (!params.enabled || params.drawList.commands.empty()) return;
  const size_t vertexCount = params.drawList.vertices.size();
  const size_t indexCount = params.drawList.indices.size();
  if (vertexCount == 0 || indexCount == 0) return;

  EnsurePipeline(context);
  if (!hina_pipeline_is_valid(pipeline_.get())) return;

  const size_t vertexBytes = vertexCount * sizeof(ui::PrimitiveVertex);
  const size_t indexBytes = indexCount * sizeof(uint32_t);
  const auto ensureCapacity = [&](const char* bufferName, size_t requiredBytes, size_t minimumGrowth)
  {
    const size_t currentSize = context.GetBufferSize(bufferName);
    if (requiredBytes <= currentSize) return;
    const size_t doubled = currentSize == 0 ? 0 : currentSize * 2;
    const size_t newSize = std::max({requiredBytes, doubled, minimumGrowth});
    context.ResizeBuffer(bufferName, newSize);
  };
  ensureCapacity(kVertexBufferName, vertexBytes, static_cast<size_t>(128u * 1024u));
  ensureCapacity(kIndexBufferName, indexBytes, static_cast<size_t>(64u * 1024u));

  gfx::Buffer vertexBuffer = context.GetBuffer(kVertexBufferName);
  gfx::Buffer indexBuffer = context.GetBuffer(kIndexBufferName);
  hina_upload_buffer(vertexBuffer, params.drawList.vertices.data(), vertexBytes);
  hina_upload_buffer(indexBuffer, params.drawList.indices.data(), indexBytes);

  gfx::CommandBuffer& cmd = context.GetCommandBuffer();
  HinaContext* hinaCtx = context.GetHinaContext();
  GfxRenderer* gfxRenderer = context.GetGfxRenderer();

  gfx::Texture sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
  gfx::Dimensions dims = hinaCtx->getDimensions(sceneColor);
  const float fbWidth = static_cast<float>(dims.width);
  const float fbHeight = static_cast<float>(dims.height);

  cmd.setViewport({.x = 0.0f, .y = 0.0f, .width = fbWidth, .height = fbHeight});
  cmd.bindPipeline(pipeline_.get());

  // Bind vertex and index buffers using hina_vertex_input
  hina_vertex_input vertInput = {};
  vertInput.vertex_buffers[0] = vertexBuffer;
  vertInput.vertex_offsets[0] = 0;
  vertInput.index_buffer = indexBuffer;
  vertInput.index_type = HINA_INDEX_UINT32;
  hina_cmd_apply_vertex_input(cmd.get(), &vertInput);

  // Sort commands by layer, then by sortOrder
  std::vector<const ui::PrimitiveDrawCommand*> sortedCommands;
  sortedCommands.reserve(params.drawList.commands.size());
  for (const auto& drawCmd : params.drawList.commands)
    sortedCommands.push_back(&drawCmd);

  std::sort(sortedCommands.begin(), sortedCommands.end(),
    [](const ui::PrimitiveDrawCommand* a, const ui::PrimitiveDrawCommand* b) {
      if (a->layer != b->layer) return a->layer < b->layer;
      return a->sortOrder < b->sortOrder;
    });

  struct Ui2DPushConstants
  {
    float LRTB[4];
  } pushData = {
    .LRTB = {0.0f, fbWidth, 0.0f, fbHeight},
  };

  for (const ui::PrimitiveDrawCommand* drawCmdPtr : sortedCommands)
  {
    const ui::PrimitiveDrawCommand& drawCmd = *drawCmdPtr;
    uint32_t textureId = drawCmd.textureId;
    if (textureId == std::numeric_limits<uint32_t>::max() && params.drawList.hasSolidFillFallback())
    {
      textureId = params.drawList.solidFillTextureId;
    }
    if (textureId == std::numeric_limits<uint32_t>::max()) continue;

    // Decode texture handle and create transient bind group
    gfx::TextureView texView = {};
    gfx::Sampler sampler = gfxRenderer->getDefaultSampler();

    // Decode TextureHandle from ID (index in low 16 bits, generation in high 16 bits)
    gfx::TextureHandle h;
    h.index = static_cast<uint16_t>(textureId & 0xFFFF);
    h.generation = static_cast<uint16_t>((textureId >> 16) & 0xFFFF);
    texView = gfxRenderer->getMaterialSystem().getTextureView(h);

    // Fallback to default white texture if invalid
    if (!hina_texture_view_is_valid(texView)) {
      texView = gfxRenderer->getMaterialSystem().getTextureView(
          gfxRenderer->getMaterialSystem().getDefaultWhiteTexture());
      if (!hina_texture_view_is_valid(texView)) continue;
    }

    // Create transient bind group using the shared UI layout
    hina_transient_bind_group tbg = hina_alloc_transient_bind_group(gfxRenderer->getUIBindGroupLayout());
    hina_transient_write_combined_image(&tbg, 0, texView, sampler);
    hina_cmd_bind_transient_group(cmd.get(), 0, tbg);

    cmd.pushConstants(pushData);

    const float clipMinX = std::clamp(drawCmd.clipRect.x, 0.0f, fbWidth);
    const float clipMinY = std::clamp(drawCmd.clipRect.y, 0.0f, fbHeight);
    const float clipMaxX = std::clamp(drawCmd.clipRect.z, 0.0f, fbWidth);
    const float clipMaxY = std::clamp(drawCmd.clipRect.w, 0.0f, fbHeight);
    if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;

    cmd.setScissor({
      .x = static_cast<int32_t>(clipMinX), .y = static_cast<int32_t>(clipMinY),
      .width = static_cast<uint32_t>(clipMaxX - clipMinX), .height = static_cast<uint32_t>(clipMaxY - clipMinY)
    });

    // Vertices/indices in the primitive list are absolute, so keep vertex offset at zero.
    cmd.drawIndexed(drawCmd.indexCount, 1, drawCmd.indexOffset, 0, 0);
  }
}
