#include "linear_color.h"
#include "render_graph.h"
#include "shader.h"

namespace LinearColor
{
  bool ShouldUseSRGBFormat(const char* textureName)
  {
    // Check for common albedo/diffuse texture names
    if (strstr(textureName, "albedo") || strstr(textureName, "diffuse") || strstr(textureName, "baseColor") || strstr(
      textureName, "color"))
    {
      return true;
    }
    // Data textures should not use sRGB
    if (strstr(textureName, "normal") || strstr(textureName, "roughness") || strstr(textureName, "metallic") ||
      strstr(textureName, "occlusion") || strstr(textureName, "height") || strstr(textureName, "displacement"))
    {
      return false;
    }
    // Default to false for safety
    return false;
  }

  vk::Format GetTextureFormat(bool isAlbedo, bool preferSRGB)
  {
    if (isAlbedo && preferSRGB)
    {
      return vk::Format::RGBA_SRGB8; // Automatic sRGB->linear conversion
    }
    return vk::Format::RGBA_UN8; // Linear data textures
  }
} // namespace LinearColor
LinearColorSystem::LinearColorSystem(vk::IContext& context)
  : m_context(context)
{
}

void LinearColorSystem::ensureInitialized()
{
  if (m_initialized)
  {
    return;
  }
  // Query context when needed
  if (!m_context.hasSwapchain())
  {
    // Conservative default
    m_requiresLinearWorkflow = true;
    LOG_INFO("LinearColorSystem: No swapchain, assuming linear workflow");
  }
  else
  {
    vk::ColorSpace colorSpace = m_context.getSwapchainColorSpace();
    m_requiresLinearWorkflow = (colorSpace == vk::ColorSpace::SRGB_NONLINEAR);
    LOG_INFO("LinearColorSystem: Linear workflow = {}", m_requiresLinearWorkflow);
  }
  CreateToneMappingPipeline();
  m_initialized = true;
}

void LinearColorSystem::RegisterLinearColorResources(RenderGraph& renderGraph)
{
  // Ensure initialization
  ensureInitialized();
  if (m_requiresLinearWorkflow)
  {
    // Linear workflow: Use HDR intermediate format at fixed internal resolution
    // This avoids recompilation on window resize - only swapchain resources change
    vk::TextureDesc hdrSceneColorDesc{
      .type = vk::TextureType::Tex2D, .format = LinearColor::HDR_SCENE_FORMAT,
      .dimensions = ResourceProperties::INTERNAL_RESOLUTION_DIMENSIONS, .usage = LinearColor::HDR_SCENE_USAGE,
      .debugName = "HDR Scene Color"
    };
    // Register HDR scene color - this replaces the basic SCENE_COLOR
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, hdrSceneColorDesc);
  }
  else
  {
    // Direct rendering: Use swapchain-compatible format at fixed internal resolution
    vk::TextureDesc directSceneColorDesc{
      .type = vk::TextureType::Tex2D, .format = m_context.getSwapchainFormat(),
      .dimensions = ResourceProperties::INTERNAL_RESOLUTION_DIMENSIONS,
      .usage = vk::TextureUsageBits_Attachment | vk::TextureUsageBits_Sampled, .debugName = "Direct Scene Color"
    };
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, directSceneColorDesc);
  }
}

void LinearColorSystem::RegisterToneMappingPass(internal::RenderPassBuilder& builder)
{
  if (!m_requiresLinearWorkflow) return;
  PassDeclarationInfo tonemap;
  tonemap.framebufferDebugName = "OpaqueScene";
  tonemap.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR, .loadOp = vk::LoadOp::Load, // Changed from Clear
    .storeOp = vk::StoreOp::Store
  };
  builder.CreatePass().UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite).SetPriority(
           static_cast<internal::RenderPassBuilder::PassPriority>(599)) // Just before all UI passes.
         .AddGraphicsPass("ToneMapping", tonemap, [this](internal::ExecutionContext& context)
         {
           auto& cmd = context.GetvkCommandBuffer();
           cmd.cmdBindRenderPipeline(m_pipelineToneMap);
           struct ToneMappingPC
           {
             uint32_t hdrSceneColorIndex;
             float exposure;
             int mode;
             float maxWhite;
             float P, a, m, l, c, b;
           } pc = {
               .hdrSceneColorIndex = context.GetTexture(RenderResources::SCENE_COLOR).index(),
               .exposure = m_toneMappingSettings.exposure, .mode = static_cast<int>(m_toneMappingSettings.mode),
               .maxWhite = m_toneMappingSettings.maxWhite, .P = m_toneMappingSettings.P, .a = m_toneMappingSettings.a,
               .m = m_toneMappingSettings.m, .l = m_toneMappingSettings.l, .c = m_toneMappingSettings.c,
               .b = m_toneMappingSettings.b,
             };
           cmd.cmdPushConstants(pc);
           cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false});
           cmd.cmdDraw(3);
         });
}

vk::Format LinearColorSystem::GetSceneColorFormat() const
{
  return m_requiresLinearWorkflow ? LinearColor::HDR_SCENE_FORMAT : m_context.getSwapchainFormat();
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
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform ToneMappingData {
    uint hdrSceneColorIndex;
    float exposure;
    int mode;
    float maxWhite;
    float P, a, m, l, c, b; // Uchimura parameters
} pc;

// Corrected ACES implementation
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Corrected Reinhard implementation
vec3 Reinhard(vec3 color) {
    // Formula: color / (1 + color / maxWhite)
    return color / (1.0 + color / pc.maxWhite);
}

// Corrected Uchimura implementation - simplified but accurate
vec3 Uchimura(vec3 x) {
    float P = pc.P;  // max display brightness
    float a = pc.a;  // contrast
    float m = pc.m;  // linear section start
    float l = pc.l;  // linear section length
    float c = pc.c;  // black tightness
    float b = pc.b;  // pedestal
    
    // Pre-compute constants
    float l0 = ((P - m) * l) / a;
    float L0 = m - (m / a);
    float L1 = m + ((1.0 - m) / a);
    float S0 = m + l0;
    float S1 = m + (a * l0);
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    vec3 result;
    for(int i = 0; i < 3; i++) {
        float val = x[i];
        
        if(val < m) {
            // Toe section
            result[i] = m * pow(val / m, c) + b;
        }
        else if(val < m + l0) {
            // Linear section
            result[i] = m + a * (val - m);
        }
        else {
            // Shoulder section
            result[i] = P - (P - S1) * exp(CP * (val - S0));
        }
    }
    
    return result;
}

vec3 Uncharted2(vec3 x) {
    const float A = 0.15; // Shoulder strength
    const float B = 0.50; // Linear strength  
    const float C = 0.10; // Linear angle
    const float D = 0.20; // Toe strength
    const float E = 0.02; // Toe numerator
    const float F = 0.30; // Toe denominator
    
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Unreal(vec3 x) {
    return x / (x + 0.155) * 1.019;
}

// Corrected Khronos PBR Neutral implementation (from official spec)
vec3 KhronosPBR(vec3 color) {
    const float startCompression = 0.76; // 0.8 - 0.04
    const float desaturation = 0.15;
    
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;
    
    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;
    
    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;
    
    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return mix(color, vec3(newPeak), g);
}

void main() {
    // Sample HDR color
    vec3 hdrColor = textureBindless2D(pc.hdrSceneColorIndex, 0, uv).rgb;
    
    // Apply exposure
    hdrColor *= pc.exposure;
    
    // Apply tone mapping
    vec3 toneMapped;
    switch (pc.mode) {
        case 1: // Reinhard
            toneMapped = Reinhard(hdrColor);
            break;
        case 2: // Uchimura
            toneMapped = Uchimura(hdrColor);
            break;
        case 3: // ACES
            toneMapped = ACESFilm(hdrColor);
            break;
        case 4: // Khronos PBR
            toneMapped = KhronosPBR(hdrColor);
            break;
        case 5: // Uncharted 2
            toneMapped = Uncharted2(hdrColor);
            break;
        case 6: // Unreal
            toneMapped = Unreal(hdrColor);
            // Skip gamma correction for Unreal as it's baked in
            outColor = vec4(toneMapped, 1.0);
            return;
        default: // None
            toneMapped = clamp(hdrColor, 0.0, 1.0);
            break;
    }
    
    // Gamma correction (sRGB conversion)
    toneMapped = pow(toneMapped, vec3(1.0 / 2.2));
    
    outColor = vec4(toneMapped, 1.0);
}
)";

void LinearColorSystem::CreateToneMappingPipeline()
{
  // Only create tone mapping pipeline if linear workflow is active
  if (!m_requiresLinearWorkflow) return;
  /*
  m_vertToneMap = loadShaderModule(m_context, "shaders/fullscreen.vert");
  m_fragToneMap = loadShaderModule(m_context, "shaders/tonemap.frag");*/
  // hardcode for now to avoid dependency on filesystem
  m_vertToneMap = m_context.createShaderModule({codeVS, vk::ShaderStage::Vert, "Shader Module: Tonemap (vert)"});
  m_fragToneMap = m_context.createShaderModule({codeFS, vk::ShaderStage::Frag, "Shader Module: Tonemap (frag)"});
  m_pipelineToneMap = m_context.createRenderPipeline({
    .smVert = m_vertToneMap, .smFrag = m_fragToneMap, .color = {{.format = m_context.getSwapchainFormat()}},
    .debugName = "Tone Mapping Pipeline"
  });
}
