#include "OIT.h"
#include "render_graph.h"
#include "shader.h"

OITSystem::OITSystem(vk::IContext& context)
  : m_context(context)
{}

void OITSystem::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  // Get current swapchain dimensions for resource sizing
  vk::TextureHandle swapchainTexture = m_context.getCurrentSwapchainTexture();
  vk::Dimensions screenDims = m_context.getDimensions(swapchainTexture);
  
  // Calculate fragment buffer size based on screen dimensions
  uint32_t maxFragments = screenDims.width * screenDims.height * m_settings.fragmentsPerPixel;

  vk::BufferDesc fragmentBufferDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::Device,
    .size = sizeof(OIT::TransparentFragment) * maxFragments,
    .debugName = "Buffer: transparency lists"
  };

  // Head texture
  vk::TextureDesc headTextureDesc{
    .type = vk::TextureType::Tex2D,
    .format = vk::Format::R_UI32,
    .dimensions = ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS,
    .usage = vk::TextureUsageBits_Storage,
    .debugName = "oitHeads"
  };

  // Atomic counter
  vk::BufferDesc atomicCounterDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::Device,
    .size = sizeof(uint32_t),
    .debugName = "Buffer: atomic counter"
  };

  // NEW: Packed OIT metadata buffer
  vk::BufferDesc oitBufferDesc{
    .usage = vk::BufferUsageBits_Storage,
    .storage = vk::StorageType::HostVisible, // Needs to be updated per frame
    .size = sizeof(OIT::OITBuffer),
    .debugName = "Buffer: OIT metadata"
  };

  CreateOITPipeline();
  // Register OIT Clear Pass - runs early to initialize OIT resources
  passBuilder.CreatePass()
    .DeclareTransientResource(OITResources::FRAGMENT_BUFFER, fragmentBufferDesc)
    .DeclareTransientResource(OITResources::HEAD_TEXTURE, headTextureDesc)  
    .DeclareTransientResource(OITResources::ATOMIC_COUNTER, atomicCounterDesc)
    .DeclareTransientResource(OITResources::OIT_BUFFER, oitBufferDesc)
    .UseResource(OITResources::HEAD_TEXTURE, AccessType::Write)
    .UseResource(OITResources::ATOMIC_COUNTER, AccessType::Write)
    .UseResource(OITResources::OIT_BUFFER, AccessType::Write)
    .SetPriority(internal::RenderPassBuilder::PassPriority::EarlySetup)
    .AddGenericPass("OIT_Clear", [this](internal::ExecutionContext& context)
    {
      vk::TextureHandle headTexture = context.GetTexture(OITResources::HEAD_TEXTURE);
      vk::BufferHandle atomicCounter = context.GetBuffer(OITResources::ATOMIC_COUNTER);
      auto& cmd = context.GetvkCommandBuffer();

      // Clear head texture to 0xFFFFFFFF
      cmd.cmdClearColorImage(headTexture, vk::ClearColorValue{ .uint32 = {0xFFFFFFFF, 0, 0, 0} });
      
      // Reset atomic counter to 0
      cmd.cmdFillBuffer(atomicCounter, 0, sizeof(uint32_t), 0);
      
      // Update OIT metadata buffer
      vk::Dimensions screenDims = context.GetvkContext().getDimensions(headTexture);
      uint32_t maxFragments = screenDims.width * screenDims.height * m_settings.fragmentsPerPixel;
      
      OIT::OITBuffer oitData = {
        .bufferAtomicCounter = context.GetvkContext().gpuAddress(atomicCounter),
        .bufferTransparencyLists = context.GetvkContext().gpuAddress(context.GetBuffer(OITResources::FRAGMENT_BUFFER)),
        .texHeadsOIT = headTexture.index(),
        .maxOITFragments = maxFragments
      };
      vk::BufferHandle oit = context.GetBuffer(OITResources::OIT_BUFFER);

      {
        OIT::OITBuffer* mapped = reinterpret_cast<OIT::OITBuffer*>(context.GetvkContext().getMappedPtr(oit));
        std::memcpy(mapped, &oitData, sizeof(OIT::OITBuffer));
        context.GetvkContext().flushMappedMemory(oit, 0,  sizeof(OIT::OITBuffer));
      }
    });

  // Register OIT Resolve Pass - runs after transparent geometry to sort and blend fragments
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
    .UseResource(OITResources::HEAD_TEXTURE, AccessType::Read)
    .UseResource(OITResources::FRAGMENT_BUFFER, AccessType::Read)
    .SetPriority(internal::RenderPassBuilder::PassPriority::OITResolve)
    .AddGraphicsPass("OIT_Resolve",
      PassDeclarationInfo{
        .colorAttachments = {{
          .textureName = RenderResources::SCENE_COLOR,
          .loadOp = vk::LoadOp::Load,  // Preserve opaque geometry
          .storeOp = vk::StoreOp::Store
        }}
      },
      [this](internal::ExecutionContext& context)
      {
        auto& cmd = context.GetvkCommandBuffer();
        cmd.cmdBindRenderPipeline(m_pipelineOIT);

        struct OITResolvePC
        {
          uint64_t bufferTransparencyLists;
          uint32_t texColor;
          uint32_t texHeadsOIT;
          float time;
          float opacityBoost;
          uint32_t showHeatmap;
        } pc = {
          .bufferTransparencyLists = context.GetvkContext().gpuAddress(context.GetBuffer(OITResources::FRAGMENT_BUFFER)),
          .texColor = context.GetTexture(RenderResources::SCENE_COLOR).index(),
          .texHeadsOIT = context.GetTexture(OITResources::HEAD_TEXTURE).index(),
          .time = static_cast<float>(0),
          .opacityBoost = m_settings.opacityBoost,
          .showHeatmap = m_settings.enableHeatmap ? 1u : 0u
        };

        cmd.cmdPushConstants(pc);
        cmd.cmdBindDepthState({ .compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false });
        cmd.cmdDraw(3);
      });

}

static const char* codeVS = R"(
//
#version 460

layout (location=0) out vec2 uv;

// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
void main() {
  uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(uv * vec2(2, -2) + vec2(-1, 1), 0.0, 1.0);
}

)";

static const char* codeFS = R"(
// shaders/oit_resolve.frag - Exact copy of demo implementation
layout (set = 0, binding = 2, r32ui) uniform uimage2D kTextures2DIn[];

layout (location=0) in vec2 uv;

layout (location=0) out vec4 out_FragColor;

struct TransparentFragment {
  f16vec4 color;
  float depth;
  uint next;
};

layout(buffer_reference, std430) buffer TransparencyListsBuffer {
  TransparentFragment frags[];
};

layout(push_constant) uniform PushConstants {
  TransparencyListsBuffer oitLists;
  uint texColor;
  uint texHeadsOIT;
  float time;
  float opacityBoost;
  uint showHeatmap;
} pc;

#define MAX_FRAGMENTS 64

void main() {
  TransparentFragment frags[MAX_FRAGMENTS];

  uint numFragments = 0;
  uint idx = imageLoad(kTextures2DIn[pc.texHeadsOIT], ivec2(gl_FragCoord.xy)).r;

  // copy the linked list for this fragment into an array
  while (idx != 0xFFFFFFFF && numFragments < MAX_FRAGMENTS) {
    frags[numFragments] = pc.oitLists.frags[idx];
    numFragments++;
    idx = pc.oitLists.frags[idx].next;
  }

  // sort the array by depth using insertion sort (largest to smallest)
  for (int i = 1; i < numFragments; i++) {
    TransparentFragment toInsert = frags[i];
    uint j = i;
    while (j > 0 && toInsert.depth > frags[j-1].depth) {
      frags[j] = frags[j-1];
      j--;
    }
    frags[j] = toInsert;
  }

  // get the color of the closest non-transparent object from the frame buffer
  vec4 color = textureBindless2D(pc.texColor, 0, uv);

  // traverse the array, and combine the colors using the alpha channel
  for (uint i = 0; i < numFragments; i++) {
    color = mix( color, vec4(frags[i].color), clamp(float(frags[i].color.a+pc.opacityBoost), 0.0, 1.0) );
  }

  if (pc.showHeatmap > 0 && numFragments > 0)
    color = (1.0+sin(5.0*pc.time)) * vec4(vec3(numFragments+1, numFragments+1, 0), 0.0) / 16.0;

  out_FragColor = color;
}
)";


void OITSystem::CreateOITPipeline()
{
  // Load OIT resolve shaders
  /*m_vertOIT = loadShaderModule(m_context, "shaders/fullscreen.vert");
  m_fragOIT = loadShaderModule(m_context, "shaders/oit_resolve.frag");*/
  // hardcode for now to avoid dependency on filesystem
  m_vertOIT = m_context.createShaderModule({codeVS,
      vk::ShaderStage::Vert,
      "Shader Module: OIT (vert)"});
  m_fragOIT = m_context.createShaderModule({codeFS,
      vk::ShaderStage::Frag,
      "Shader Module: OIT (frag)"});

  // Create OIT resolve pipeline - renders to SCENE_COLOR format
  m_pipelineOIT = m_context.createRenderPipeline({
    .smVert = m_vertOIT,
    .smFrag = m_fragOIT,
    .color = {{.format = m_context.getSwapchainFormat()}}, // Will match SCENE_COLOR format
    .debugName = "OIT Resolve Pipeline"
  });
}