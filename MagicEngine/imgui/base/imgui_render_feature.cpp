#include "imgui_render_feature.h"
#include "renderer/gfx_renderer.h"
#include "logging/log.h"
#include <imgui.h>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <utility>
#include <iostream>

namespace editor
{
  // ImGui shader in HSL format (matching GfxRenderer style)
  static const char* kImGuiShader = R"(#hina
group ImGui = 0;

bindings(ImGui, start=0) {
  texture sampler2D u_texture;
}

push_constant PushConstants {
  vec2 scale;
  vec2 translate;
} pc;

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
  vec4 a_color;
};

struct Varyings {
  vec2 uv;
  vec4 color;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;
    out.uv = in.a_uv;
    out.color = in.a_color;
    gl_Position = vec4(in.a_position * pc.scale + pc.translate, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;
    out.color = in.color * texture(u_texture, in.uv);
    return out;
}
#hina_end
)";

  void ImGuiRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
  {
    gfx::BufferDesc vertexDesc{
      .size = 64 * 1024,
      .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Vertex | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
    };
    gfx::BufferDesc indexDesc{
      .size = 32 * 1024,
      .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Index | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
    };

    // ImGui renders directly to swapchain - it OWNS the swapchain rendering
    // Scene is displayed via VIEW_OUTPUT texture in an ImGui viewport
    PassDeclarationInfo passInfo;
    passInfo.framebufferDebugName = "ImGuiOverlay";
    passInfo.colorAttachments[0] = {
      .textureName = RenderResources::SWAPCHAIN_IMAGE,
      .loadOp = gfx::LoadOp::Clear,  // ImGui clears and owns the swapchain
      .storeOp = gfx::StoreOp::Store,
      .clearColor = {0.1f, 0.1f, 0.1f, 1.0f}  // Dark gray background
    };

    // ImGui rendering pass - renders directly to swapchain
    // Scene viewport display uses VIEW_OUTPUT texture populated by ResolveViewOutput pass
    // Priority 700 (Present) ensures it runs after all scene rendering
    passBuilder.CreatePass().DeclareTransientResource("ImGuiVertexBuffer", vertexDesc).
                DeclareTransientResource("ImGuiIndexBuffer", indexDesc).
                UseResource(RenderResources::SWAPCHAIN_IMAGE, AccessType::ReadWrite).
                UseResource("ImGuiVertexBuffer", AccessType::ReadWrite).
                UseResource("ImGuiIndexBuffer", AccessType::ReadWrite).
                SetPriority(internal::RenderPassBuilder::PassPriority::Present).
                ExecuteAfter("ResolveViewOutput").  // Run after scene is copied to VIEW_OUTPUT
                AddGraphicsPass("ImGuiRender", passInfo, [this](const internal::ExecutionContext& context)
                {
                  RenderImGui(context);
                });
  }

  void ImGuiRenderFeature::EnsurePipelineCreated(const internal::ExecutionContext& context)
  {
    if (resourcesCreated_) return;

    // Create sampler
    {
      gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
      samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
      samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
      samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;
      fontSampler_.reset(gfx::createSampler(samplerDesc));
    }

    std::cout << "[ImGuiRenderFeature] Creating pipeline..." << std::endl;

    // Compile HSL shader
    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(kImGuiShader, "imgui_feature_shader", &error);
    if (!module) {
      std::cout << "[ImGuiRenderFeature] Shader compilation failed: " << (error ? error : "Unknown") << std::endl;
      if (error) hslc_free_log(error);
      return;
    }
    std::cout << "[ImGuiRenderFeature] Shader compiled successfully" << std::endl;

    // Vertex layout matching ImDrawVert
    hina_vertex_layout vertex_layout = {};
    vertex_layout.buffer_count = 1;
    vertex_layout.buffer_strides[0] = sizeof(ImDrawVert);
    vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
    vertex_layout.attr_count = 3;
    vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos), 0, 0 };
    vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv), 1, 0 };
    vertex_layout.attrs[2] = { HINA_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col), 2, 0 };

    // Pipeline descriptor
    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.layout = vertex_layout;
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.blend.enable = true;
    pip_desc.blend.src_color = HINA_BLEND_FACTOR_SRC_ALPHA;
    pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
    pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
    pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Use shared UI bind group layout
    GfxRenderer* gfxRenderer = context.GetGfxRenderer();
    gfx::BindGroupLayout uiLayout = gfxRenderer->getUIBindGroupLayout();
    if (hina_bind_group_layout_is_valid(uiLayout)) {
      pip_desc.bind_group_layouts[0] = uiLayout;
      pip_desc.bind_group_layout_count = 1;
    }

    pipeline_.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(pipeline_.get())) {
      std::cout << "[ImGuiRenderFeature] Pipeline creation failed" << std::endl;
      return;
    }

    std::cout << "[ImGuiRenderFeature] Pipeline created successfully" << std::endl;
    resourcesCreated_ = true;
  }

  void ImGuiRenderFeature::RenderImGui(const internal::ExecutionContext& context)
  {
    const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());
    if (!params.enabled || !params.hasNewFrame) return;
    ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->TotalVtxCount == 0 || drawData->TotalIdxCount == 0 || drawData->CmdListsCount == 0)
    {
      return;
    }
    static_assert(sizeof(ImDrawIdx) == 2, "ImDrawIdx must be 16-bit");
    EnsurePipelineCreated(context);

    gfx::CommandBuffer& cmd = context.GetCommandBuffer();
    const float fbWidth = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    const float fbHeight = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    if (fbWidth <= 0 || fbHeight <= 0) return;

    // Calculate required sizes
    const size_t requiredVertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    const size_t requiredIndexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    // Resize buffers if needed - render graph handles frame isolation automatically
    if (requiredVertexSize > context.GetBufferSize("ImGuiVertexBuffer"))
    {
      context.ResizeBuffer("ImGuiVertexBuffer", requiredVertexSize);
    }
    if (requiredIndexSize > context.GetBufferSize("ImGuiIndexBuffer"))
    {
      context.ResizeBuffer("ImGuiIndexBuffer", requiredIndexSize);
    }

    // Get automatically frame-buffered resources - no manual cycling needed
    gfx::Buffer vertexBuffer = context.GetBuffer("ImGuiVertexBuffer");
    gfx::Buffer indexBuffer = context.GetBuffer("ImGuiIndexBuffer");

    // Upload data using hina buffer operations
    // Note: Buffers are HOST_COHERENT, so no flush is needed
    {
      ImDrawVert* vtx = static_cast<ImDrawVert*>(hina_map_buffer(vertexBuffer));
      uint16_t* idx = static_cast<uint16_t*>(hina_map_buffer(indexBuffer));
      for (int n = 0; n < drawData->CmdListsCount; n++)
      {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        std::memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx += cmdList->VtxBuffer.Size;
        idx += cmdList->IdxBuffer.Size;
      }
    }

    // Check if pipeline was created successfully
    if (!hina_pipeline_is_valid(pipeline_.get())) {
      std::cout << "[ImGuiRenderFeature] Pipeline not valid, skipping render" << std::endl;
      return;
    }

    // Calculate scale and translate for NDC projection
    // These transform vertex positions from ImGui coordinates to NDC [-1, 1]
    const float L = drawData->DisplayPos.x;
    const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float T = drawData->DisplayPos.y;
    const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

    struct PushConstants {
      float scale[2];
      float translate[2];
    } pushConstants = {
      .scale = { 2.0f / (R - L), 2.0f / (B - T) },
      .translate = { -(R + L) / (R - L), -(T + B) / (B - T) }
    };

    // Render setup and commands
    cmd.setViewport({.x = 0.0f, .y = 0.0f, .width = fbWidth, .height = fbHeight});
    const ImVec2 clipOff = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;
    uint32_t idxOffset = 0;
    uint32_t vtxOffset = 0;

    cmd.bindPipeline(pipeline_.get());

    // Bind vertex and index buffers using hina_vertex_input
    hina_vertex_input vertInput = {};
    vertInput.vertex_buffers[0] = vertexBuffer;
    vertInput.vertex_offsets[0] = 0;
    vertInput.index_buffer = indexBuffer;
    vertInput.index_type = HINA_INDEX_UINT16;
    hina_cmd_apply_vertex_input(cmd.get(), &vertInput);

    cmd.pushConstants(pushConstants);

    GfxRenderer* gfxRenderer = context.GetGfxRenderer();

    // Font texture: ID 0 (ImGui default)
    constexpr uint64_t IMGUI_FONT_TEXTURE_ID = 0;
    constexpr uint64_t IMGUI_VIEWOUTPUT_BIT = 1ULL << 63;

    // Render commands
    for (int n = 0; n < drawData->CmdListsCount; n++)
    {
      const ImDrawList* cmdList = drawData->CmdLists[n];
      for (int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
      {
        const ImDrawCmd& drawCmd = cmdList->CmdBuffer[cmdIdx];
        if (drawCmd.UserCallback) continue;
        ImVec2 clipMin((drawCmd.ClipRect.x - clipOff.x) * clipScale.x, (drawCmd.ClipRect.y - clipOff.y) * clipScale.y);
        ImVec2 clipMax((drawCmd.ClipRect.z - clipOff.x) * clipScale.x, (drawCmd.ClipRect.w - clipOff.y) * clipScale.y);
        clipMin.x = std::max(clipMin.x, 0.0f);
        clipMin.y = std::max(clipMin.y, 0.0f);
        clipMax.x = std::min(clipMax.x, fbWidth);
        clipMax.y = std::min(clipMax.y, fbHeight);
        if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y) continue;

        // Bind the texture for this draw call using transient bind groups
        uint64_t textureId = static_cast<uint64_t>(reinterpret_cast<uintptr_t>((void*)drawCmd.TextureId));
        gfx::TextureView texView = {};
        gfx::Sampler sampler = fontSampler_.get();

        if (textureId == IMGUI_FONT_TEXTURE_ID) {
          texView = gfxRenderer->getImGuiFontView();
        }
        // ViewOutput pointer: high bit set
        else if (textureId & IMGUI_VIEWOUTPUT_BIT) {
          const ViewOutput* vo = reinterpret_cast<const ViewOutput*>(textureId & ~IMGUI_VIEWOUTPUT_BIT);
          if (vo && vo->valid) {
            texView = vo->view;
            sampler = vo->sampler;
          }
        }
        // Asset texture: encoded TextureHandle (index in low 16 bits, generation in high 16 bits)
        else {
          gfx::TextureHandle h;
          h.index = static_cast<uint16_t>(textureId & 0xFFFF);
          h.generation = static_cast<uint16_t>((textureId >> 16) & 0xFFFF);
          texView = gfxRenderer->getMaterialSystem().getTextureView(h);
        }

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

        cmd.setScissor({
          .x = static_cast<int32_t>(clipMin.x), .y = static_cast<int32_t>(clipMin.y),
          .width = static_cast<uint32_t>(clipMax.x - clipMin.x), .height = static_cast<uint32_t>(clipMax.y - clipMin.y)
        });
        cmd.drawIndexed(drawCmd.ElemCount, 1u, idxOffset + drawCmd.IdxOffset,
                        static_cast<int32_t>(vtxOffset + drawCmd.VtxOffset));
      }
      idxOffset += cmdList->IdxBuffer.Size;
      vtxOffset += cmdList->VtxBuffer.Size;
    }
  }

  void ImGuiRenderFeature::CopySceneForImGuiView(const internal::ExecutionContext& context)
  {
    gfx::Texture sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
    gfx::Texture sceneView = context.GetTexture("ImGuiSceneView");
    if (!gfx::isValid(sceneColor) || !gfx::isValid(sceneView))
    {
      return;
    }

    gfx::Cmd* cmd = context.GetCmd();

    // Blit the texture (copy with potential format conversion)
    hina_cmd_blit_texture(cmd, sceneColor, sceneView, 0, 0, HINA_FILTER_LINEAR);
  }
} // namespace editor
