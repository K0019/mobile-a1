#include "renderer/features/ui2d_render_feature.h"
#include "renderer/hina_context.h"
#include "renderer/gfx_renderer.h"
#include "renderer/linear_color.h"
#include "resource/resource_manager.h"
#include "Engine/VideoPlayer.h"
#include "UI/RectTransform.h"
#include "ECS/ECS.h"
#include "Singleton.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
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

    // Orthographic projection for UI (standard Y: Y=0 at top, Y=height at bottom)
    // No Y-flip here - the final ImGui viewport UV flip handles Vulkan coordinate conversion
    mat4 proj = mat4(
        2.0 / (R - L), 0.0, 0.0, 0.0,
        0.0, 2.0 / (B - T), 0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        (R + L) / (L - R), (T + B) / (T - B), 0.0, 1.0
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
    vec4 texColor = texture(uTexture, in.uv);
    vec4 color = in.color * texColor;
    // Premultiply alpha to avoid fringe artifacts from linear filtering
    out.color = vec4(color.rgb * color.a, color.a);
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
    .memory = gfx::BufferMemory::CPU,
    .usage = gfx::BufferUsage::Vertex
  };
  gfx::BufferDesc indexDesc{
    .size = 32 * 1024,
    .memory = gfx::BufferMemory::CPU,
    .usage = gfx::BufferUsage::Index
  };

  // Video YUV conversion pass - runs BEFORE UI render pass (no framebuffer attachments)
  passBuilder.CreatePass()
              .SetPriority(internal::RenderPassBuilder::PassPriority::UI)
              .ExecuteAfter("SceneComposite")
              .AddGenericPass(
                "VideoYuvConvert", [this](const internal::ExecutionContext& context)
                {
                  ConvertVideoFrames(context);
                });

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
              .ExecuteAfter("VideoYuvConvert")  // UI2D must run AFTER video conversion
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

  // Blend state for premultiplied alpha (eliminates fringe artifacts from linear filtering)
  pip_desc.blend[0].enable = true;
  pip_desc.blend[0].src_color = HINA_BLEND_FACTOR_ONE;  // Color already premultiplied in shader
  pip_desc.blend[0].dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pip_desc.blend[0].color_op = HINA_BLEND_OP_ADD;
  pip_desc.blend[0].src_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  pip_desc.blend[0].alpha_op = HINA_BLEND_OP_ADD;

  // Color attachment format: use HDR scene format since we render to SCENE_COLOR
  pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;

  // No depth testing for UI
  pip_desc.depth.depth_test = false;
  pip_desc.depth.depth_write = false;
  pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

  // Use the shared UI bind group layout
  gfx::BindGroupLayout uiLayout = gfxRenderer->getUIBindGroupLayout();
  if (hina_bind_group_layout_is_valid(uiLayout)) {
    pip_desc.bind_group_layouts[0] = uiLayout;
  }

  pipeline_.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(pipeline_.get())) {
    std::cerr << "[UI2D] Pipeline creation failed" << std::endl;
    return;
  }

  resourcesCreated_ = true;
}

void Ui2DRenderFeature::ConvertVideoFrames(const internal::ExecutionContext& context)
{
  gfx::CommandBuffer& cmd = context.GetCommandBuffer();

  // Upload and convert video frames (same command buffer for proper sync)
  VideoManager* videoMgr = ST<VideoManager>::Get();
  if (videoMgr)
  {
    videoMgr->UploadPendingFrames(cmd);
    videoMgr->ConvertPendingFrames(cmd);
  }
}

void Ui2DRenderFeature::RenderUi(const internal::ExecutionContext& context)
{
  const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
  gfx::CommandBuffer& cmd = context.GetCommandBuffer();

  // Video frames already converted in ConvertVideoFrames pass
  VideoManager* videoMgr = ST<VideoManager>::Get();

  // Check if we have any UI content
  bool hasUiContent = params.enabled && !params.drawList.commands.empty() &&
                      params.drawList.vertices.size() > 0 && params.drawList.indices.size() > 0;

  // Check if we have any videos with valid textures to render (regardless of playback state)
  bool hasActiveVideos = false;
  if (videoMgr)
  {
    for (auto it = ecs::GetCompsActiveBegin<VideoPlayerComponent>();
         it != ecs::GetCompsEnd<VideoPlayerComponent>(); ++it)
    {
      VideoPlayerComponent& player = *it;
      uint32_t handle = player.GetDecoderHandle();
      if (handle != 0 && hina_texture_view_is_valid(videoMgr->GetTextureView(handle)))
      {
        hasActiveVideos = true;
        break;
      }
    }
  }

  // Early return only if nothing to render
  if (!hasUiContent && !hasActiveVideos) return;

  // Get UI vertex/index counts (may be 0 if only rendering video)
  const size_t vertexCount = params.drawList.vertices.size();
  const size_t indexCount = params.drawList.indices.size();

  EnsurePipeline(context);
  if (!hina_pipeline_is_valid(pipeline_.get())) return;

  HinaContext* hinaCtx = context.GetHinaContext();
  GfxRenderer* gfxRenderer = context.GetGfxRenderer();

  // Framebuffer dimensions (SCENE_COLOR is always internal resolution)
  gfx::Texture sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
  gfx::Dimensions dims = hinaCtx->getDimensions(sceneColor);
  const float fbWidth = static_cast<float>(dims.width);
  const float fbHeight = static_cast<float>(dims.height);

  // UI viewport dimensions (dynamic based on window aspect ratio)
  const float uiWidth = params.viewportWidth;
  const float uiHeight = params.viewportHeight;

  // Collect video quads from active VideoPlayerComponents
  struct VideoQuad {
    gfx::TextureView view;
    float minX, minY, maxX, maxY;  // UI coordinates
    uint16_t layer;
  };
  std::vector<VideoQuad> videoQuads;
  std::vector<ui::PrimitiveVertex> videoVertices;
  std::vector<uint32_t> videoIndices;

  if (videoMgr)
  {
    for (auto it = ecs::GetCompsActiveBegin<VideoPlayerComponent>();
         it != ecs::GetCompsEnd<VideoPlayerComponent>(); ++it)
    {
      VideoPlayerComponent& player = *it;
      // Render video if we have a valid decoder (shows first frame even when stopped)
      uint32_t handle = player.GetDecoderHandle();
      if (handle == 0)
        continue;
      gfx::TextureView view = videoMgr->GetTextureView(handle);
      if (!hina_texture_view_is_valid(view))
        continue;

      uint32_t videoWidth = 0, videoHeight = 0;
      if (!videoMgr->GetVideoDimensions(handle, videoWidth, videoHeight))
        continue;

      // Get entity and RectTransform once for position, scale, and layer
      ecs::Entity* entity = ecs::GetEntity(&player);
      RectTransformComponent* rectTransform = entity ? entity->GetComp<RectTransformComponent>() : nullptr;

      // Calculate video display rect
      float minX = 0.0f, minY = 0.0f, maxX = uiWidth, maxY = uiHeight;
      const float videoAspect = static_cast<float>(videoWidth) / static_cast<float>(videoHeight);
      const float uiAspect = uiWidth / uiHeight;

      if (player.GetFullscreen())
      {
        // Fullscreen: Scale to fill viewport while maintaining aspect ratio (letterbox/pillarbox)
        if (videoAspect > uiAspect)
        {
          // Video is wider than viewport - letterbox (black bars top/bottom)
          const float scaledHeight = uiWidth / videoAspect;
          const float offset = (uiHeight - scaledHeight) * 0.5f;
          minY = offset;
          maxY = offset + scaledHeight;
        }
        else
        {
          // Video is narrower than viewport - pillarbox (black bars left/right)
          const float scaledWidth = uiHeight * videoAspect;
          const float offset = (uiWidth - scaledWidth) * 0.5f;
          minX = offset;
          maxX = offset + scaledWidth;
        }
      }
      else
      {
        // Non-fullscreen: Use RectTransform for position and scale (like other UI components)
        // Base display size is video's native size
        float displayWidth = static_cast<float>(videoWidth);
        float displayHeight = static_cast<float>(videoHeight);

        // Apply RectTransform scale if available
        Vec2 scale = rectTransform ? rectTransform->GetWorldScale() : Vec2{1.0f, 1.0f};
        displayWidth *= scale.x;
        displayHeight *= scale.y;

        // Get position from RectTransform (or center if not available)
        Vec2 position = rectTransform ? rectTransform->GetWorldPosition() : Vec2{uiWidth * 0.5f, uiHeight * 0.5f};

        // Position is the center of the video quad
        minX = position.x - displayWidth * 0.5f;
        minY = position.y - displayHeight * 0.5f;
        maxX = position.x + displayWidth * 0.5f;
        maxY = position.y + displayHeight * 0.5f;
      }

      // Create quad vertices (4 vertices per video)
      uint32_t baseVertex = static_cast<uint32_t>(videoVertices.size());
      uint32_t white = 0xFFFFFFFF;

      videoVertices.push_back({minX, minY, 0.0f, 0.0f, white});  // top-left
      videoVertices.push_back({maxX, minY, 1.0f, 0.0f, white});  // top-right
      videoVertices.push_back({maxX, maxY, 1.0f, 1.0f, white});  // bottom-right
      videoVertices.push_back({minX, maxY, 0.0f, 1.0f, white});  // bottom-left

      // Create quad indices (2 triangles)
      videoIndices.push_back(baseVertex + 0);
      videoIndices.push_back(baseVertex + 1);
      videoIndices.push_back(baseVertex + 2);
      videoIndices.push_back(baseVertex + 0);
      videoIndices.push_back(baseVertex + 2);
      videoIndices.push_back(baseVertex + 3);

      // Use RectTransform layer (auto-attached via IUIComponent)
      uint16_t layer = rectTransform ? rectTransform->GetLayer() : 0;
      videoQuads.push_back({view, minX, minY, maxX, maxY, layer});
    }
  }

  // Calculate total vertex/index bytes
  const size_t totalVertexCount = vertexCount + videoVertices.size();
  const size_t totalIndexCount = indexCount + videoIndices.size();
  const size_t totalVertexBytes = totalVertexCount * sizeof(ui::PrimitiveVertex);
  const size_t totalIndexBytes = totalIndexCount * sizeof(uint32_t);

  const auto ensureCapacity = [&](const char* bufferName, size_t requiredBytes, size_t minimumGrowth)
  {
    const size_t currentSize = context.GetBufferSize(bufferName);
    if (requiredBytes <= currentSize) return;
    const size_t doubled = currentSize == 0 ? 0 : currentSize * 2;
    const size_t newSize = std::max({requiredBytes, doubled, minimumGrowth});
    context.ResizeBuffer(bufferName, newSize);
  };
  ensureCapacity(kVertexBufferName, totalVertexBytes, static_cast<size_t>(128u * 1024u));
  ensureCapacity(kIndexBufferName, totalIndexBytes, static_cast<size_t>(64u * 1024u));

  gfx::Buffer vertexBuffer = context.GetBuffer(kVertexBufferName);
  gfx::Buffer indexBuffer = context.GetBuffer(kIndexBufferName);

  // Upload UI vertices/indices
  hina_upload_buffer(vertexBuffer, params.drawList.vertices.data(), vertexCount * sizeof(ui::PrimitiveVertex));
  hina_upload_buffer(indexBuffer, params.drawList.indices.data(), indexCount * sizeof(uint32_t));

  // Upload video vertices/indices after UI data
  if (!videoVertices.empty())
  {
    hina_upload_buffer(vertexBuffer, videoVertices.data(),
                       videoVertices.size() * sizeof(ui::PrimitiveVertex),
                       vertexCount * sizeof(ui::PrimitiveVertex));
    hina_upload_buffer(indexBuffer, videoIndices.data(),
                       videoIndices.size() * sizeof(uint32_t),
                       indexCount * sizeof(uint32_t));
  }

  cmd.setViewport({.x = 0.0f, .y = 0.0f, .width = fbWidth, .height = fbHeight});
  cmd.bindPipeline(pipeline_.get());

  // Bind vertex and index buffers using hina_vertex_input
  hina_vertex_input vertInput = {};
  vertInput.vertex_buffers[0] = vertexBuffer;
  vertInput.vertex_offsets[0] = 0;
  vertInput.index_buffer = indexBuffer;
  vertInput.index_type = HINA_INDEX_UINT32;
  hina_cmd_apply_vertex_input(cmd.get(), &vertInput);

  // Create draw commands for videos (to be merged with UI commands)
  std::vector<ui::PrimitiveDrawCommand> videoDrawCommands;
  uint32_t videoIndexOffset = static_cast<uint32_t>(indexCount);
  for (size_t i = 0; i < videoQuads.size(); ++i)
  {
    const VideoQuad& vq = videoQuads[i];
    ui::PrimitiveDrawCommand drawCmd = {};
    drawCmd.indexOffset = videoIndexOffset + static_cast<uint32_t>(i * 6);
    drawCmd.indexCount = 6;
    drawCmd.textureId = 0;  // Not used when directView is set
    drawCmd.directView = vq.view;
    drawCmd.clipRect = {0.0f, 0.0f, uiWidth, uiHeight};  // Full screen clip
    drawCmd.layer = vq.layer;
    drawCmd.sortOrder = 0;
    videoDrawCommands.push_back(drawCmd);
  }

  // Sort commands by layer, then by sortOrder (merge UI and video commands)
  std::vector<const ui::PrimitiveDrawCommand*> sortedCommands;
  sortedCommands.reserve(params.drawList.commands.size() + videoDrawCommands.size());
  for (const auto& drawCmd : params.drawList.commands)
    sortedCommands.push_back(&drawCmd);
  for (const auto& drawCmd : videoDrawCommands)
    sortedCommands.push_back(&drawCmd);

  std::sort(sortedCommands.begin(), sortedCommands.end(),
    [](const ui::PrimitiveDrawCommand* a, const ui::PrimitiveDrawCommand* b) {
      if (a->layer != b->layer) return a->layer < b->layer;
      return a->sortOrder < b->sortOrder;
    });

  // Ortho projection uses UI viewport dimensions (dynamic based on aspect ratio)
  // This ensures UI coordinates map correctly to NDC space
  struct Ui2DPushConstants
  {
    float LRTB[4];
  } pushData = {
    .LRTB = {0.0f, uiWidth, 0.0f, uiHeight},
  };

  for (const ui::PrimitiveDrawCommand* drawCmdPtr : sortedCommands)
  {
    const ui::PrimitiveDrawCommand& drawCmd = *drawCmdPtr;

    // Resolve texture view - check directView first (runtime textures like video frames)
    gfx::TextureView texView = {};
    gfx::Sampler sampler = gfxRenderer->getDefaultSampler();

    if (hina_texture_view_is_valid(drawCmd.directView)) {
      // Direct view provided - use it directly (no resolution needed)
      texView = drawCmd.directView;
    } else {
      // Resolve textureId through ResourceManager
      uint64_t textureId = drawCmd.textureId;
      if (textureId == 0 && params.drawList.hasSolidFillFallback()) {
        textureId = params.drawList.solidFillTextureId;
      }
      if (textureId == 0) continue;

      Resource::ResourceManager* resourceMngr = gfxRenderer->getResourceManager();
      if (resourceMngr) {
        ::TextureHandle engineHandle = ::TextureHandle::fromOpaqueValue(textureId);
        texView = resourceMngr->resolveTextureView(engineHandle);
      } else {
        texView = gfxRenderer->getMaterialSystem().getTextureView(
            gfxRenderer->getMaterialSystem().getDefaultWhiteTexture());
      }
    }

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

    // Clip rects are in UI coordinate space, convert to framebuffer pixel space
    // Scale factor: framebuffer / UI viewport
    const float scaleX = fbWidth / uiWidth;
    const float scaleY = fbHeight / uiHeight;

    const float clipMinX = std::clamp(drawCmd.clipRect.x * scaleX, 0.0f, fbWidth);
    const float clipMinY = std::clamp(drawCmd.clipRect.y * scaleY, 0.0f, fbHeight);
    const float clipMaxX = std::clamp(drawCmd.clipRect.z * scaleX, 0.0f, fbWidth);
    const float clipMaxY = std::clamp(drawCmd.clipRect.w * scaleY, 0.0f, fbHeight);
    if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;

    cmd.setScissor({
      .x = static_cast<int32_t>(clipMinX), .y = static_cast<int32_t>(clipMinY),
      .width = static_cast<uint32_t>(clipMaxX - clipMinX), .height = static_cast<uint32_t>(clipMaxY - clipMinY)
    });

    // Vertices/indices in the primitive list are absolute, so keep vertex offset at zero.
    cmd.drawIndexed(drawCmd.indexCount, 1, drawCmd.indexOffset, 0, 0);
  }
}
