/*
 * This file is based on the book "Vulkan 3D Graphics Rendering Cookbook - Second Edition"
 * and code originally published under the MIT License
 * by Sergey Kosarevsky at https://github.com/corporateshark/lightweightvk.
 *
 * The original code has been copied as a starting point and has been
 * significantly modified since.
 *
 * Copyright (c) Sergey Kosarevsky, 2023-2025
 *
 * Licensed under the MIT License. See LICENSE for more details.
 */
#include "interface.h"
#include <cassert>
// Only include GLFW headers on desktop platforms
#if !defined(__ANDROID__)
#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#  if defined(WAYLAND)
#    define GLFW_EXPOSE_NATIVE_WAYLAND
#  else
#    define GLFW_EXPOSE_NATIVE_X11
#  endif
#elif __APPLE__
#  define GLFW_EXPOSE_NATIVE_COCOA
#else
#  error "Unsupported OS for GLFW native access"
#endif
#include "GLFW/glfw3.h"
#ifdef _WIN32
#  include <windows.h>
#  include "GLFW/glfw3native.h"
#elif defined(__linux__)
// to fix later
#endif
#endif // !defined(__ANDROID__)
#include "graphics/vulkan/vk_classes.h"

namespace
{
  struct TextureFormatProperties
  {
    const vk::Format format = vk::Format::Invalid;
    const uint8_t bytesPerBlock : 5 = 1;
    const uint8_t blockWidth : 3 = 1;
    const uint8_t blockHeight : 3 = 1;
    const uint8_t minBlocksX : 2 = 1;
    const uint8_t minBlocksY : 2 = 1;
    const bool depth : 1 = false;
    const bool stencil : 1 = false;
    const bool compressed : 1 = false;
    const uint8_t numPlanes : 2 = 1;
  };

#define PROPS(fmt, bpb, ...) \
  TextureFormatProperties{ .format = vk::Format::fmt, .bytesPerBlock = bpb __VA_OPT__(,) __VA_ARGS__ }
  static constexpr TextureFormatProperties properties[] = {
    PROPS(Invalid, 1), PROPS(R_UN8, 1), PROPS(R_UI16, 2), PROPS(R_UI32, 4), PROPS(R_UN16, 2), PROPS(R_F16, 2),
    PROPS(R_F32, 4), PROPS(RG_UN8, 2), PROPS(RG_UI16, 4), PROPS(RG_UI32, 8), PROPS(RG_UN16, 4), PROPS(RG_F16, 4),
    PROPS(RG_F32, 8), PROPS(RGBA_UN8, 4), PROPS(RGBA_UI32, 16), PROPS(RGBA_F16, 8), PROPS(RGBA_F32, 16),
    PROPS(RGBA_SRGB8, 4), PROPS(BGRA_UN8, 4), PROPS(BGRA_SRGB8, 4), PROPS(A2B10G10R10_UN, 4), PROPS(A2R10G10B10_UN, 4),
    PROPS(ETC2_RGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ETC2_SRGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC1_RGB, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC1_RGB_SRGB, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC1_RGBA, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true), // 1-bit alpha
    PROPS(BC1_RGBA_SRGB, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC2_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC2_RGBA_SRGB, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC3_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC3_RGBA_SRGB, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC6H_RGB_UFLOAT, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC6H_RGB_SFLOAT, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC7_RGBA_SRGB, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC4_R, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC5_RG, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC7_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true), PROPS(Z_UN16, 2, .depth = true),
    PROPS(Z_UN24, 3, .depth = true), PROPS(Z_F32, 4, .depth = true),
    PROPS(Z_UN24_S_UI8, 4, .depth = true, .stencil = true), PROPS(Z_F32_S_UI8, 5, .depth = true, .stencil = true),
    PROPS(YUV_NV12, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 2),
    // Subsampled 420
    PROPS(YUV_420p, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 3),
    // Subsampled 420
  };
} // namespace
static_assert(sizeof(TextureFormatProperties) <= sizeof(uint32_t));
static_assert(std::size(properties) == (uint8_t)vk::Format::YUV_420p + 1);

bool vk::isDepthOrStencilFormat(Format format)
{
  return properties[(uint8_t)format].depth || properties[(uint8_t)format].stencil;
}

uint32_t vk::getNumImagePlanes(Format format) { return properties[(uint8_t)format].numPlanes; }

uint32_t vk::getVertexFormatSize(VertexFormat format)
{
  // clang-format off
#define SIZE4(LVKBaseType, BaseType)           \
  case VertexFormat::LVKBaseType##1: return sizeof(BaseType) * 1u; \
  case VertexFormat::LVKBaseType##2: return sizeof(BaseType) * 2u; \
  case VertexFormat::LVKBaseType##3: return sizeof(BaseType) * 3u; \
  case VertexFormat::LVKBaseType##4: return sizeof(BaseType) * 4u;
#define SIZE2_4_NORM(LVKBaseType, BaseType)           \
  case VertexFormat::LVKBaseType##2Norm: return sizeof(BaseType) * 2u; \
  case VertexFormat::LVKBaseType##4Norm: return sizeof(BaseType) * 4u;
#define SIZE1_2_3_4_NORM(LVKBaseType, BaseType)           \
  case VertexFormat::LVKBaseType##1Norm: return sizeof(BaseType) * 1u; \
  case VertexFormat::LVKBaseType##2Norm: return sizeof(BaseType) * 2u; \
  case VertexFormat::LVKBaseType##3Norm: return sizeof(BaseType) * 3u; \
  case VertexFormat::LVKBaseType##4Norm: return sizeof(BaseType) * 4u;

  // clang-format on
  switch (format)
  {
  SIZE4(Float, float);
  SIZE4(Byte, uint8_t);
  SIZE4(UByte, uint8_t);
  SIZE4(Short, uint16_t);
  SIZE4(UShort, uint16_t);
  SIZE2_4_NORM(Byte, uint8_t);
  SIZE2_4_NORM(UByte, uint8_t);
  SIZE1_2_3_4_NORM(Short, uint16_t);
  SIZE1_2_3_4_NORM(UShort, uint16_t);
  SIZE4(Int, uint32_t);
  SIZE4(UInt, uint32_t);
  SIZE4(HalfFloat, uint16_t);
  case VertexFormat::R10G10B10A2_SNORM:
    return sizeof(uint32_t);
  default: assert(false);
    return 0;
  }
#undef SIZE4
}

uint32_t vk::getTextureBytesPerLayer(uint32_t width, uint32_t height, Format format, uint32_t level)
{
  const uint32_t levelWidth = (std::max)(width >> level, 1u);
  const uint32_t levelHeight = (std::max)(height >> level, 1u);
  const TextureFormatProperties props = properties[(uint8_t)format];
  if (!props.compressed) { return props.bytesPerBlock * levelWidth * levelHeight; }
  //const uint32_t blockWidth = (std::max)((uint32_t)props.blockWidth, 1u);
  //const uint32_t blockHeight = (std::max)((uint32_t)props.blockHeight, 1u);
  const uint32_t widthInBlocks = (levelWidth + props.blockWidth - 1) / props.blockWidth;
  const uint32_t heightInBlocks = (levelHeight + props.blockHeight - 1) / props.blockHeight;
  return widthInBlocks * heightInBlocks * props.bytesPerBlock;
}

uint32_t vk::getTextureBytesPerPlane(uint32_t width, uint32_t height, Format format, uint32_t plane)
{
  const TextureFormatProperties props = properties[(uint8_t)format];
  ASSERT(plane < props.numPlanes);
  switch (format)
  {
  case Format::YUV_NV12:
    return width * height / (plane + 1);
  case Format::YUV_420p:
    return width * height / (plane ? 4 : 1);
  default: ;
  }
  return getTextureBytesPerLayer(width, height, format, 0);
}

void vk::destroy(IContext* ctx, ComputePipelineHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, RenderPipelineHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, RayTracingPipelineHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, ShaderModuleHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, SamplerHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, BufferHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, TextureHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, QueryPoolHandle handle) { if (ctx) { ctx->destroy(handle); } }

void vk::destroy(IContext* ctx, AccelStructHandle handle) { if (ctx) { ctx->destroy(handle); } }

// Logs GLSL shaders with line numbers annotation
void vk::logShaderSource(const char* text)
{
  if (!text)
  {
    LOG_INFO("( Shader source is NULL )");
    return;
  }
  uint32_t line_num = 0;
  const char* line_start = text;
  const char* ptr = text;
  while (true)
  {
    while (*ptr != '\n' && *ptr != '\0') { ptr++; }
    int32_t length = static_cast<int32_t>(ptr - line_start);
    if (length > 0 && *(ptr - 1) == '\r') { length--; }
    line_num++;
    if (length >= 0)
    {
      std::string_view line_content(line_start, static_cast<size_t>(length));
      LOG_INFO("({:3}) {}", line_num, line_content);
    }
    else { LOG_INFO("({:3})", line_num); }
    if (*ptr == '\0') { break; }
    ptr++;
    line_start = ptr;
  }
  if (line_num == 0 && *text == '\0') { LOG_INFO("( Shader source is empty )"); }
}

vk::ShaderStage vk::ShaderStageFromFileName(const char* fileName)
{
  if (endsWith(fileName, ".vert")) return ShaderStage::Vert;
  if (endsWith(fileName, ".frag")) return ShaderStage::Frag;
  if (endsWith(fileName, ".geom")) return ShaderStage::Geom;
  if (endsWith(fileName, ".comp")) return ShaderStage::Comp;
  if (endsWith(fileName, ".tesc")) return ShaderStage::Tesc;
  if (endsWith(fileName, ".tese")) return ShaderStage::Tese;
  return ShaderStage::Vert;
}

uint32_t vk::VertexInput::getVertexSize() const
{
  uint32_t vertexSize = 0;
  for (uint32_t i = 0; i < VERTEX_ATTRIBUTES_MAX && attributes[i].format != VertexFormat::Invalid; i++)
  {
    ASSERT_MSG(attributes[i].offset == vertexSize, "Unsupported vertex attributes format");
    vertexSize += getVertexFormatSize(attributes[i].format);
  }
  return vertexSize;
}

std::unique_ptr<vk::IContext> vk::createContext(const ContextConfig& cfg, HWDeviceType preferredDeviceType)
{
  // 1. Create headless context (Instance only - no surface)
  auto ctx = std::make_unique<VulkanContext>(cfg);
  // 2. Query and select physical device
  HWDeviceDesc devices[8];
  uint32_t numDevices = ctx->queryDevices(preferredDeviceType, devices, std::size(devices));
  if (!numDevices)
  {
    if (preferredDeviceType == HWDeviceType::Discrete)
    {
      numDevices = ctx->queryDevices(HWDeviceType::Integrated, devices, std::size(devices));
    }
    else if (preferredDeviceType == HWDeviceType::Integrated)
    {
      numDevices = ctx->queryDevices(HWDeviceType::Discrete, devices, std::size(devices));
    }
  }
  if (!numDevices)
  {
    LOG_ERROR("No suitable Vulkan devices found");
    return nullptr;
  }
  // 3. Initialize device and queues (no swapchain)
  Result result = ctx->initContext(devices[0]);
  if (!result.isOk())
  {
    LOG_ERROR("Failed to initialize Vulkan context: {}", result.message);
    return nullptr;
  }
  return ctx;
}

std::unique_ptr<vk::IContext> vk::createVulkanContextWithSwapchain(window* window, uint32_t width, uint32_t height,
                                                                   const ContextConfig& cfg,
                                                                   HWDeviceType preferredDeviceType)
{
  // Use new headless factory
  auto ctx = createContext(cfg, preferredDeviceType);
  if (!ctx)
  {
    return nullptr;
  }
  // Extract native window handle (platform-specific)
  void* nativeWindow = nullptr;
  void* display = nullptr;
#if defined(_WIN32)
  nativeWindow = (void*)glfwGetWin32Window(window);
#elif defined(__ANDROID__)
  nativeWindow = (void*)window;
#elif defined(__linux__)
#if defined(LVK_WITH_WAYLAND)
  wl_surface* waylandWindow = glfwGetWaylandWindow(window); if (!waylandWindow)
  {
    ASSERT_MSG(false, "Wayland window not found");
    return nullptr;
  } nativeWindow = (void*)waylandWindow; display = (void*)glfwGetWaylandDisplay();
#else
  nativeWindow = (void*)glfwGetX11Window(window); display = (void*)glfwGetX11Display();
#endif
#elif defined(__APPLE__)
  nativeWindow = createCocoaWindowView(window);
#else
#error Unsupported OS
#endif
  // Create surface through IContext interface
  ctx->createSurface(nativeWindow, display);
  // Initialize swapchain - need to cast to access initSwapchain which is not in IContext
  auto* vkCtx = static_cast<VulkanContext*>(ctx.get());
  if (width > 0 && height > 0)
  {
    Result result = vkCtx->initSwapchain(width, height);
    if (!result.isOk())
    {
      LOG_ERROR("Failed to create swapchain: {}", result.message);
      return nullptr;
    }
  }
  return ctx;
}
