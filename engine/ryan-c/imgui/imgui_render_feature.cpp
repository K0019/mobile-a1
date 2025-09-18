#include "imgui_render_feature.h"
#include <imgui.h>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <utility>

namespace editor
{
  // ImGui shaders - corrected push constants
  static const char* kVertexShader = R"(
layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_uv;

struct Vertex {
    float x, y;
    float u, v;
    uint rgba;
};

layout(std430, buffer_reference) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(push_constant) uniform PushConstants {
    vec4 LRTB;          // Left, Right, Top, Bottom for orthographic projection
    VertexBuffer vb;        // Fixed: was VertexBuffer, now uint64_t for GPU address
    uint textureId;
    uint samplerId;
} pc;

void main() {
    float L = pc.LRTB.x;
    float R = pc.LRTB.y;
    float T = pc.LRTB.z;
    float B = pc.LRTB.w;
    
    // Orthographic projection matrix
    mat4 proj = mat4(
        2.0 / (R - L),                   0.0,  0.0, 0.0,
        0.0,                   2.0 / (T - B),  0.0, 0.0,
        0.0,                             0.0, -1.0, 0.0,
        (R + L) / (L - R), (T + B) / (B - T),  0.0, 1.0
    );
    
    VertexBuffer vb = VertexBuffer(pc.vb);  // Convert uint64_t to buffer reference
    Vertex v = vb.vertices[gl_VertexIndex];
    out_color = unpackUnorm4x8(v.rgba);
    out_uv = vec2(v.u, v.v);
    gl_Position = proj * vec4(v.x, v.y, 0, 1);
}
)";

  static const char* kFragmentShader = R"(
layout (location = 0) in vec4 in_color;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout (constant_id = 0) const bool kLinearColorSpace = false;

layout(push_constant) uniform PushConstants {
  vec4 LRTB;
  vec2 vb;
  uint textureId;
  uint samplerId;
} pc;

void main() {
    vec4 c = in_color * texture(nonuniformEXT(sampler2D(kTextures2D[pc.textureId], kSamplers[pc.samplerId])), in_uv);
    out_color = kLinearColorSpace ? vec4(pow(c.rgb, vec3(2.2)), c.a) : c;
}
)";

  using namespace Render;

  void ImGuiRenderFeature::SetupPasses(Render::internal::RenderPassBuilder& passBuilder)
  {
    vk::BufferDesc vertexDesc{ .usage = vk::BufferUsageBits_Storage,
      .storage = vk::StorageType::HostVisible,
      .size = 64 * 1024,
    };

    vk::BufferDesc indexDesc{ .usage = vk::BufferUsageBits_Index,
      .storage = vk::StorageType::HostVisible,
      .size = 32 * 1024,
    };

    PassDeclarationInfo passInfo;
    passInfo.framebufferDebugName = "ImGuiOverlay";
    passInfo.colorAttachments[0] = { .textureName = RenderResources::SCENE_COLOR, .loadOp = vk::LoadOp::Load, .storeOp = vk::StoreOp::Store, };

    passBuilder.CreatePass()
      .DeclareTransientResource("ImGuiSceneView", vk::TextureDesc{
          .type = vk::TextureType::Tex2D,
          .format = vk::Format::Invalid,
          .dimensions = ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS,
          .usage = vk::TextureUsageBits_Sampled })
          .UseResource(RenderResources::SCENE_COLOR, AccessType::Read)
      .UseResource("ImGuiSceneView", AccessType::Write)
      .SetPriority(Render::internal::RenderPassBuilder::PassPriority::UI)
      .AddGenericPass("CopySceneForImGui", [this](const Render::internal::ExecutionContext& context) {
      CopySceneForImGuiView(context);
    });
    passBuilder.CreatePass().
      DeclareTransientResource("ImGuiVertexBuffer", vertexDesc).
      DeclareTransientResource("ImGuiIndexBuffer", indexDesc).
      UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).
      UseResource("ImGuiVertexBuffer", AccessType::ReadWrite).
      UseResource("ImGuiIndexBuffer", AccessType::ReadWrite).
      SetPriority(Render::internal::RenderPassBuilder::PassPriority::UI).
      ExecuteAfter("CopySceneForImGui").
      AddGraphicsPass("ImGuiRender",
                      passInfo,
                      [this](const Render::internal::ExecutionContext& context)
    {
      RenderImGui(context);
    });
  }

  void ImGuiRenderFeature::EnsurePipelineCreated(const Render::internal::ExecutionContext& context)
  {
    if(resourcesCreated_)
      return;

    vk::IContext& vkContext = context.GetvkContext();

    vertShader_ = vkContext.createShaderModule({ kVertexShader, vk::ShaderStage::Vert, "ImGui Vertex Shader" });
    fragShader_ = vkContext.createShaderModule({ kFragmentShader, vk::ShaderStage::Frag, "ImGui Fragment Shader" });

    fontSampler_ = vkContext.createSampler({
    .wrapU = vk::SamplerWrap::Clamp,
    .wrapV = vk::SamplerWrap::Clamp,
    .wrapW = vk::SamplerWrap::Clamp,
    .debugName = "ImGui Font Sampler"
});

    resourcesCreated_ = true;
  }

  void ImGuiRenderFeature::RenderImGui(const Render::internal::ExecutionContext& context)
  {
    const Parameters& params = *static_cast<const Parameters*>(GetParameterBlock_RT());

    if(!params.enabled || !params.hasNewFrame)
      return;

    ImDrawData* drawData = ImGui::GetDrawData();
    if(!drawData || drawData->TotalVtxCount == 0 || drawData->TotalIdxCount == 0 || drawData->CmdListsCount == 0)
    {
      return;
    }

    static_assert(sizeof(ImDrawIdx) == 2, "ImDrawIdx must be 16-bit");

    EnsurePipelineCreated(context);

    vk::ICommandBuffer& cmd = context.GetvkCommandBuffer();
    vk::IContext& vkContext = context.GetvkContext();

    const float fbWidth = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    const float fbHeight = drawData->DisplaySize.y * drawData->FramebufferScale.y;

    if(fbWidth <= 0 || fbHeight <= 0)
      return;
    // Calculate required sizes
    const size_t requiredVertexSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    const size_t requiredIndexSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

    // Resize buffers if needed - render graph handles frame isolation automatically
    if(requiredVertexSize > context.GetBufferSize("ImGuiVertexBuffer"))
    {
      context.ResizeBuffer("ImGuiVertexBuffer", requiredVertexSize);
    }
    if(requiredIndexSize > context.GetBufferSize("ImGuiIndexBuffer"))
    {
      context.ResizeBuffer("ImGuiIndexBuffer", requiredIndexSize);
    }

    // Get automatically frame-buffered resources - no manual cycling needed
    vk::BufferHandle vertexBuffer = context.GetBuffer("ImGuiVertexBuffer");
    vk::BufferHandle indexBuffer = context.GetBuffer("ImGuiIndexBuffer");

    // Upload data - same as before but using render graph buffers
    {
      ImDrawVert* vtx = reinterpret_cast<ImDrawVert*>(vkContext.getMappedPtr(vertexBuffer));
      uint16_t* idx = reinterpret_cast<uint16_t*>(vkContext.getMappedPtr(indexBuffer));

      for(int n = 0; n < drawData->CmdListsCount; n++)
      {
        const ImDrawList* cmdList = drawData->CmdLists[n];
        std::memcpy(vtx, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(idx, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vtx += cmdList->VtxBuffer.Size;
        idx += cmdList->IdxBuffer.Size;
      }

      vkContext.flushMappedMemory(vertexBuffer, 0, requiredVertexSize);
      vkContext.flushMappedMemory(indexBuffer, 0, requiredIndexSize);
    }

    // Create pipeline if needed - same as before
    if(pipeline_.empty())
    {
      vk::TextureHandle sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
      const bool needsLinearColorSpace = vkContext.getSwapchainColorSpace() == vk::ColorSpace::SRGB_LINEAR;
      const uint32_t linearColorSpaceFlag = needsLinearColorSpace ? 1u : 0u;
      pipeline_ = vkContext.createRenderPipeline({ .smVert = vertShader_, .smFrag = fragShader_, .specInfo = {.entries = {{.constantId = 0, .size = sizeof(linearColorSpaceFlag)}}, .data = &linearColorSpaceFlag, .dataSize = sizeof(linearColorSpaceFlag)}, .color = {{.format = vkContext.getFormat(sceneColor), .blendEnabled = true, .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha, .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha,}}, .depthFormat = vk::Format::Invalid, .cullMode = vk::CullMode::None, .debugName = "ImGui Render Pipeline" }, nullptr);
    }

    // Render setup and commands - unchanged except using render graph buffers
    cmd.cmdBindDepthState({});
    cmd.cmdBindViewport({ .x = 0.0f, .y = 0.0f, .width = fbWidth, .height = fbHeight, });

    const float L = drawData->DisplayPos.x;
    const float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    const float T = drawData->DisplayPos.y;
    const float B = drawData->DisplayPos.y + drawData->DisplaySize.y;

    const ImVec2 clipOff = drawData->DisplayPos;
    const ImVec2 clipScale = drawData->FramebufferScale;

    uint32_t idxOffset = 0;
    uint32_t vtxOffset = 0;

    cmd.cmdBindIndexBuffer(indexBuffer, vk::IndexFormat::UI16);
    cmd.cmdBindRenderPipeline(pipeline_);

    // Render commands - same as before
    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
      const ImDrawList* cmdList = drawData->CmdLists[n];

      for(int cmdIdx = 0; cmdIdx < cmdList->CmdBuffer.Size; cmdIdx++)
      {
        const ImDrawCmd& drawCmd = cmdList->CmdBuffer[cmdIdx];

        if(drawCmd.UserCallback)
          continue;

        ImVec2 clipMin((drawCmd.ClipRect.x - clipOff.x) * clipScale.x, (drawCmd.ClipRect.y - clipOff.y) * clipScale.y);
        ImVec2 clipMax((drawCmd.ClipRect.z - clipOff.x) * clipScale.x, (drawCmd.ClipRect.w - clipOff.y) * clipScale.y);

        clipMin.x = std::max(clipMin.x, 0.0f);
        clipMin.y = std::max(clipMin.y, 0.0f);
        clipMax.x = std::min(clipMax.x, fbWidth);
        clipMax.y = std::min(clipMax.y, fbHeight);

        if(clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
          continue;

        struct VulkanImguiBindData
        {
          float LRTB[4];
          uint64_t vb = 0;
          uint32_t textureId = 0;
          uint32_t samplerId = 0;
        } bindData = { .LRTB = {L, R, T, B},
          .vb = vkContext.gpuAddress(vertexBuffer),
          // Using render graph buffer
          .textureId = static_cast<uint32_t>(drawCmd.TextureId),
          .samplerId = fontSampler_.index(), };

        cmd.cmdPushConstants(bindData);
        cmd.cmdBindScissorRect({ .x = static_cast<uint32_t>(clipMin.x), .y = static_cast<uint32_t>(clipMin.y), .width = static_cast<uint32_t>(clipMax.x - clipMin.x), .height = static_cast<uint32_t>(clipMax.y - clipMin.y) });

        cmd.cmdDrawIndexed(drawCmd.ElemCount, 1u, idxOffset + drawCmd.IdxOffset, static_cast<int32_t>(vtxOffset + drawCmd.VtxOffset));
      }

      idxOffset += cmdList->IdxBuffer.Size;
      vtxOffset += cmdList->VtxBuffer.Size;
    }
  }

  void ImGuiRenderFeature::CopySceneForImGuiView(
    const Render::internal::ExecutionContext& context)
  {
    vk::TextureHandle sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
    vk::TextureHandle sceneView = context.GetTexture("ImGuiSceneView");

    if(!sceneColor.valid() || !sceneView.valid())
    {
      return;
    }

    vk::ICommandBuffer& cmd = context.GetvkCommandBuffer();
    vk::IContext& vkContext = context.GetvkContext();

    vk::Dimensions sceneDims = vkContext.getDimensions(sceneColor);
    vk::Dimensions viewDims = vkContext.getDimensions(sceneView);

    vk::Dimensions copyExtent = {
        .width = std::min(sceneDims.width, viewDims.width),
        .height = std::min(sceneDims.height, viewDims.height),
        .depth = 1
    };

    cmd.cmdCopyImage(sceneColor, sceneView, copyExtent,
                     vk::Offset3D{ 0, 0, 0 }, vk::Offset3D{ 0, 0, 0 },
                     vk::TextureLayers{ 0, 0, 1 }, vk::TextureLayers{ 0, 0, 1 });
  }
} // namespace editor
