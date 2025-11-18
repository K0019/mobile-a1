#include "graphics/features/ui2d_render_feature.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <limits>

namespace
{
  constexpr const char* kUiVertexShader = R"(
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in uint in_color;

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec2 out_uv;

layout(push_constant) uniform PushConstants {
    vec4 LRTB;
    uint textureId;
    uint samplerId;
} pc;

void main() {
    float L = pc.LRTB.x;
    float R = pc.LRTB.y;
    float T = pc.LRTB.z;
    float B = pc.LRTB.w;

    mat4 proj = mat4(
        2.0 / (R - L), 0.0, 0.0, 0.0,
        0.0, 2.0 / (T - B), 0.0, 0.0,
        0.0, 0.0, -1.0, 0.0,
        (R + L) / (L - R), (T + B) / (B - T), 0.0, 1.0
    );

    out_color = unpackUnorm4x8(in_color);
    out_uv = in_uv;
    gl_Position = proj * vec4(in_pos, 0.0, 1.0);
}
)";
  constexpr const char* kUiFragmentShader = R"(
layout (location = 0) in vec4 in_color;
layout (location = 1) in vec2 in_uv;

layout (location = 0) out vec4 out_color;

layout (constant_id = 0) const bool kLinearColorSpace = false;

layout(push_constant) uniform PushConstants {
    vec4 LRTB;
    uint textureId;
    uint samplerId;
} pc;

void main() {
    vec4 sampleColor = texture(nonuniformEXT(sampler2D(kTextures2D[pc.textureId], kSamplers[pc.samplerId])), in_uv);
    vec4 c = in_color * sampleColor;
    out_color = kLinearColorSpace ? vec4(pow(c.rgb, vec3(2.2)), c.a) : c;
}
)";
  constexpr const char* kVertexBufferName = "Ui2DVertexBuffer";
  constexpr const char* kIndexBufferName = "Ui2DIndexBuffer";
} // namespace
void Ui2DRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  vk::BufferDesc vertexDesc{.usage = vk::BufferUsageBits_Vertex, .storage = vk::StorageType::Device, .size = 64 * 1024};
  vk::BufferDesc indexDesc{.usage = vk::BufferUsageBits_Index, .storage = vk::StorageType::Device, .size = 32 * 1024};
  PassDeclarationInfo passInfo;
  passInfo.framebufferDebugName = "Ui2DOverlay";
  passInfo.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR, .loadOp = vk::LoadOp::Load, .storeOp = vk::StoreOp::Store,
  };
  passInfo.depthAttachment = {
    .textureName = RenderResources::SCENE_DEPTH, .loadOp = vk::LoadOp::Load, .storeOp = vk::StoreOp::Store,
  };
  passBuilder.CreatePass().DeclareTransientResource(kVertexBufferName, vertexDesc).
              DeclareTransientResource(kIndexBufferName, indexDesc).
              UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).
              UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite).
              UseResource(kVertexBufferName, AccessType::ReadWrite).UseResource(kIndexBufferName, AccessType::ReadWrite)
              .SetPriority(internal::RenderPassBuilder::PassPriority::UI).AddGraphicsPass(
                "Ui2DRender", passInfo, [this](const internal::ExecutionContext& context)
                {
                  RenderUi(context);
                });
}

void Ui2DRenderFeature::EnsurePipeline(const internal::ExecutionContext& context)
{
  if (resourcesCreated_) return;
  vk::IContext& vkContext = context.GetvkContext();
  vertShader_ = vkContext.createShaderModule({kUiVertexShader, vk::ShaderStage::Vert, "Ui2D Vertex Shader"});
  fragShader_ = vkContext.createShaderModule({kUiFragmentShader, vk::ShaderStage::Frag, "Ui2D Fragment Shader"});
  linearSampler_ = vkContext.createSampler({
    .minFilter = vk::SamplerFilter::Linear, .magFilter = vk::SamplerFilter::Linear, .mipMap = vk::SamplerMip::Disabled,
    .wrapU = vk::SamplerWrap::Clamp, .wrapV = vk::SamplerWrap::Clamp, .wrapW = vk::SamplerWrap::Clamp,
    .debugName = "Ui2DLinearSampler"
  });
  nearestSampler_ = vkContext.createSampler({
    .minFilter = vk::SamplerFilter::Nearest, .magFilter = vk::SamplerFilter::Nearest,
    .mipMap = vk::SamplerMip::Disabled, .wrapU = vk::SamplerWrap::Clamp, .wrapV = vk::SamplerWrap::Clamp,
    .wrapW = vk::SamplerWrap::Clamp, .debugName = "Ui2DNearestSampler"
  });
  textSampler_ = vkContext.createSampler({
    .wrapU = vk::SamplerWrap::Clamp, .wrapV = vk::SamplerWrap::Clamp, .wrapW = vk::SamplerWrap::Clamp,
    .debugName = "Ui2DTextSampler"
  });
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
  vk::BufferHandle vertexBuffer = context.GetBuffer(kVertexBufferName);
  vk::BufferHandle indexBuffer = context.GetBuffer(kIndexBufferName);
  vk::IContext& vkContext = context.GetvkContext();
  vkContext.upload(vertexBuffer, params.drawList.vertices.data(), vertexBytes);
  vkContext.upload(indexBuffer, params.drawList.indices.data(), indexBytes);
  if (pipeline_.empty())
  {
    vk::TextureHandle sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
    vk::TextureHandle sceneDepth = context.GetTexture(RenderResources::SCENE_DEPTH);
    const vk::Format depthFormat = sceneDepth.valid() ? vkContext.getFormat(sceneDepth) : vk::Format::Invalid;
    vk::VertexInput vertexInput{};
    vertexInput.attributes[0] = {
      .location = 0, .binding = 0, .format = vk::VertexFormat::Float2, .offset = offsetof(ui::PrimitiveVertex, x)
    };
    vertexInput.attributes[1] = {
      .location = 1, .binding = 0, .format = vk::VertexFormat::Float2, .offset = offsetof(ui::PrimitiveVertex, u)
    };
    vertexInput.attributes[2] = {
      .location = 2, .binding = 0, .format = vk::VertexFormat::UInt1, .offset = offsetof(ui::PrimitiveVertex, color)
    };
    vertexInput.inputBindings[0].stride = sizeof(ui::PrimitiveVertex);
    const bool needsLinearColorSpace = vkContext.getSwapchainColorSpace() == vk::ColorSpace::SRGB_LINEAR;
    const uint32_t linearColorSpaceFlag = needsLinearColorSpace ? 1u : 0u;
    vk::RenderPipelineDesc pipelineDesc{};
    pipelineDesc.vertexInput = vertexInput;
    pipelineDesc.smVert = vertShader_;
    pipelineDesc.smFrag = fragShader_;
    pipelineDesc.specInfo.entries[0] = {.constantId = 0, .offset = 0, .size = sizeof(linearColorSpaceFlag)};
    pipelineDesc.specInfo.data = &linearColorSpaceFlag;
    pipelineDesc.specInfo.dataSize = sizeof(linearColorSpaceFlag);
    pipelineDesc.color[0] = {
      .format = vkContext.getFormat(sceneColor), .blendEnabled = true, .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha,
      .srcAlphaBlendFactor = vk::BlendFactor::One, .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha,
      .dstAlphaBlendFactor = vk::BlendFactor::OneMinusSrcAlpha,
    };
    pipelineDesc.depthFormat = depthFormat;
    pipelineDesc.cullMode = vk::CullMode::None;
    pipelineDesc.debugName = "Ui2DRenderPipeline";
    pipeline_ = vkContext.createRenderPipeline(pipelineDesc, nullptr);
  }
  vk::ICommandBuffer& cmd = context.GetvkCommandBuffer();
  cmd.cmdBindVertexBuffer(0, vertexBuffer, 0);
  cmd.cmdBindIndexBuffer(indexBuffer, vk::IndexFormat::UI32);
  cmd.cmdBindRenderPipeline(pipeline_);
  cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false});
  vk::TextureHandle sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
  vk::Dimensions dims = vkContext.getDimensions(sceneColor);
  const float fbWidth = static_cast<float>(dims.width);
  const float fbHeight = static_cast<float>(dims.height);
  cmd.cmdBindViewport({.x = 0.0f, .y = 0.0f, .width = fbWidth, .height = fbHeight});
  struct PushConstants
  {
    float LRTB[4];
    uint32_t textureId;
    uint32_t samplerId;
  };
  struct FramePush
  {
    float LRTB[4];
  };
  struct DrawPush
  {
    uint32_t textureId;
    uint32_t samplerId;
  };
  FramePush framePc{};
  framePc.LRTB[0] = 0.0f;
  framePc.LRTB[1] = fbWidth;
  framePc.LRTB[2] = 0.0f;
  framePc.LRTB[3] = fbHeight;
  cmd.cmdPushConstants(framePc);
  DrawPush drawPc{};
  const uint32_t samplerLookup[] = {
    linearSampler_.empty() ? 0u : linearSampler_.index(), nearestSampler_.empty() ? 0u : nearestSampler_.index(),
    textSampler_.empty() ? 0u : textSampler_.index()
  };
  constexpr uint32_t samplerCount = sizeof(samplerLookup) / sizeof(samplerLookup[0]);
  uint32_t samplerFallback = 0u;
  for (uint32_t sampler : samplerLookup)
  {
    if (sampler != 0u)
    {
      samplerFallback = sampler;
      break;
    }
  }
  for (const ui::PrimitiveDrawCommand& drawCmd : params.drawList.commands)
  {
    uint32_t textureId = drawCmd.textureId;
    if (textureId == std::numeric_limits<uint32_t>::max() && params.drawList.hasSolidFillFallback())
    {
      textureId = params.drawList.solidFillTextureId;
    }
    if (textureId == std::numeric_limits<uint32_t>::max()) continue;
    drawPc.textureId = textureId;
    uint32_t samplerId = samplerFallback;
    if (drawCmd.samplerIndex < samplerCount && samplerLookup[drawCmd.samplerIndex] != 0u)
    {
      samplerId = samplerLookup[drawCmd.samplerIndex];
    }
    drawPc.samplerId = samplerId;
    cmd.cmdPushConstants(&drawPc, sizeof(drawPc), offsetof(PushConstants, textureId));
    const float clipMinX = std::clamp(drawCmd.clipRect.x, 0.0f, fbWidth);
    const float clipMinY = std::clamp(drawCmd.clipRect.y, 0.0f, fbHeight);
    const float clipMaxX = std::clamp(drawCmd.clipRect.z, 0.0f, fbWidth);
    const float clipMaxY = std::clamp(drawCmd.clipRect.w, 0.0f, fbHeight);
    if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;
    cmd.cmdBindScissorRect({
      .x = static_cast<uint32_t>(clipMinX), .y = static_cast<uint32_t>(clipMinY),
      .width = static_cast<uint32_t>(clipMaxX - clipMinX), .height = static_cast<uint32_t>(clipMaxY - clipMinY)
    });
    // Vertices/indices in the primitive list are absolute, so keep vertex offset at zero.
    cmd.cmdDrawIndexed(drawCmd.indexCount, 1, drawCmd.indexOffset, 0, 0);
  }
}
