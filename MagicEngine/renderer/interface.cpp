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

// VkFormat enum values from Vulkan spec (used for KTX format conversion)
namespace VkFormatValues {
  constexpr int VK_FORMAT_UNDEFINED = 0;
  constexpr int VK_FORMAT_R8_UNORM = 9;
  constexpr int VK_FORMAT_R16_UNORM = 70;
  constexpr int VK_FORMAT_R16_UINT = 74;
  constexpr int VK_FORMAT_R16_SFLOAT = 76;
  constexpr int VK_FORMAT_R8G8_UNORM = 16;
  constexpr int VK_FORMAT_R16G16_UNORM = 77;
  constexpr int VK_FORMAT_R16G16_UINT = 81;
  constexpr int VK_FORMAT_R16G16_SFLOAT = 83;
  constexpr int VK_FORMAT_R32_SFLOAT = 100;
  constexpr int VK_FORMAT_R32G32_SFLOAT = 103;
  constexpr int VK_FORMAT_R8G8B8A8_UNORM = 37;
  constexpr int VK_FORMAT_R8G8B8A8_SRGB = 43;
  constexpr int VK_FORMAT_B8G8R8A8_UNORM = 44;
  constexpr int VK_FORMAT_B8G8R8A8_SRGB = 50;
  constexpr int VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64;
  constexpr int VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58;
  constexpr int VK_FORMAT_R16G16B16A16_SFLOAT = 97;
  constexpr int VK_FORMAT_R32G32B32A32_UINT = 107;
  constexpr int VK_FORMAT_R32G32B32A32_SFLOAT = 109;
  constexpr int VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147;
  constexpr int VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148;
  constexpr int VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149;
  constexpr int VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150;
  constexpr int VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151;
  constexpr int VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152;
  constexpr int VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131;
  constexpr int VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132;
  constexpr int VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133;
  constexpr int VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134;
  constexpr int VK_FORMAT_BC2_UNORM_BLOCK = 135;
  constexpr int VK_FORMAT_BC2_SRGB_BLOCK = 136;
  constexpr int VK_FORMAT_BC3_UNORM_BLOCK = 137;
  constexpr int VK_FORMAT_BC3_SRGB_BLOCK = 138;
  constexpr int VK_FORMAT_BC4_UNORM_BLOCK = 139;
  constexpr int VK_FORMAT_BC5_UNORM_BLOCK = 141;
  constexpr int VK_FORMAT_BC6H_UFLOAT_BLOCK = 143;
  constexpr int VK_FORMAT_BC6H_SFLOAT_BLOCK = 144;
  constexpr int VK_FORMAT_BC7_UNORM_BLOCK = 145;
  constexpr int VK_FORMAT_BC7_SRGB_BLOCK = 146;
  // ASTC compressed formats (mobile/Android)
  constexpr int VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157;
  constexpr int VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158;
  constexpr int VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159;
  constexpr int VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160;
  constexpr int VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161;
  constexpr int VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162;
  constexpr int VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163;
  constexpr int VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164;
  constexpr int VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165;
  constexpr int VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166;
  constexpr int VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167;
  constexpr int VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168;
  constexpr int VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169;
  constexpr int VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170;
  constexpr int VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171;
  constexpr int VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172;
  constexpr int VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173;
  constexpr int VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174;
  constexpr int VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175;
  constexpr int VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176;
  constexpr int VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177;
  constexpr int VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178;
  constexpr int VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179;
  constexpr int VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180;
  constexpr int VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181;
  constexpr int VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182;
  constexpr int VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183;
  constexpr int VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184;
  constexpr int VK_FORMAT_D16_UNORM = 124;
  constexpr int VK_FORMAT_X8_D24_UNORM_PACK32 = 125;
  constexpr int VK_FORMAT_D24_UNORM_S8_UINT = 129;
  constexpr int VK_FORMAT_D32_SFLOAT = 126;
  constexpr int VK_FORMAT_D32_SFLOAT_S8_UINT = 130;
  constexpr int VK_FORMAT_G8_B8R8_2PLANE_420_UNORM = 1000156003;
  constexpr int VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM = 1000156002;
}

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
    PROPS(ETC2_RGBA1, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),      // 1-bit alpha
    PROPS(ETC2_RGBA1_SRGB, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ETC2_RGBA8, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ETC2_RGBA8_SRGB, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
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
    PROPS(BC7_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    // ASTC compressed formats (mobile/Android) - all 16 bytes per block
    PROPS(ASTC_4x4_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ASTC_4x4_RGBA_SRGB, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ASTC_5x4_RGBA, 16, .blockWidth = 5, .blockHeight = 4, .compressed = true),
    PROPS(ASTC_5x4_RGBA_SRGB, 16, .blockWidth = 5, .blockHeight = 4, .compressed = true),
    PROPS(ASTC_5x5_RGBA, 16, .blockWidth = 5, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_5x5_RGBA_SRGB, 16, .blockWidth = 5, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_6x5_RGBA, 16, .blockWidth = 6, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_6x5_RGBA_SRGB, 16, .blockWidth = 6, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_6x6_RGBA, 16, .blockWidth = 6, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_6x6_RGBA_SRGB, 16, .blockWidth = 6, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_8x5_RGBA, 16, .blockWidth = 8, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_8x5_RGBA_SRGB, 16, .blockWidth = 8, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_8x6_RGBA, 16, .blockWidth = 8, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_8x6_RGBA_SRGB, 16, .blockWidth = 8, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_8x8_RGBA, 16, .blockWidth = 8, .blockHeight = 8, .compressed = true),
    PROPS(ASTC_8x8_RGBA_SRGB, 16, .blockWidth = 8, .blockHeight = 8, .compressed = true),
    PROPS(ASTC_10x5_RGBA, 16, .blockWidth = 10, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_10x5_RGBA_SRGB, 16, .blockWidth = 10, .blockHeight = 5, .compressed = true),
    PROPS(ASTC_10x6_RGBA, 16, .blockWidth = 10, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_10x6_RGBA_SRGB, 16, .blockWidth = 10, .blockHeight = 6, .compressed = true),
    PROPS(ASTC_10x8_RGBA, 16, .blockWidth = 10, .blockHeight = 8, .compressed = true),
    PROPS(ASTC_10x8_RGBA_SRGB, 16, .blockWidth = 10, .blockHeight = 8, .compressed = true),
    PROPS(ASTC_10x10_RGBA, 16, .blockWidth = 10, .blockHeight = 10, .compressed = true),
    PROPS(ASTC_10x10_RGBA_SRGB, 16, .blockWidth = 10, .blockHeight = 10, .compressed = true),
    PROPS(ASTC_12x10_RGBA, 16, .blockWidth = 12, .blockHeight = 10, .compressed = true),
    PROPS(ASTC_12x10_RGBA_SRGB, 16, .blockWidth = 12, .blockHeight = 10, .compressed = true),
    PROPS(ASTC_12x12_RGBA, 16, .blockWidth = 12, .blockHeight = 12, .compressed = true),
    PROPS(ASTC_12x12_RGBA_SRGB, 16, .blockWidth = 12, .blockHeight = 12, .compressed = true),
    PROPS(Z_UN16, 2, .depth = true),
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
  case VertexFormat::R10G10B10A2_UNORM:
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

// Destroy functions - forward to IContext destroy methods
void vk::destroy(IContext* ctx, ComputePipelineHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, RenderPipelineHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, RayTracingPipelineHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, ShaderModuleHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, SamplerHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, BufferHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, TextureHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, QueryPoolHandle handle) { if (ctx) ctx->destroy(handle); }
void vk::destroy(IContext* ctx, AccelStructHandle handle) { if (ctx) ctx->destroy(handle); }

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

// Convert Vulkan VkFormat to internal Format enum (for KTX texture loading)
vk::Format vk::vkFormatToFormat(int vkFormat)
{
  using namespace VkFormatValues;
  switch (vkFormat)
  {
    case VK_FORMAT_UNDEFINED:              return Format::Invalid;
    case VK_FORMAT_R8_UNORM:               return Format::R_UN8;
    case VK_FORMAT_R16_UNORM:              return Format::R_UN16;
    case VK_FORMAT_R16_SFLOAT:             return Format::R_F16;
    case VK_FORMAT_R16_UINT:               return Format::R_UI16;
    case VK_FORMAT_R8G8_UNORM:             return Format::RG_UN8;
    case VK_FORMAT_B8G8R8A8_UNORM:         return Format::BGRA_UN8;
    case VK_FORMAT_R8G8B8A8_UNORM:         return Format::RGBA_UN8;
    case VK_FORMAT_R8G8B8A8_SRGB:          return Format::RGBA_SRGB8;
    case VK_FORMAT_B8G8R8A8_SRGB:          return Format::BGRA_SRGB8;
    case VK_FORMAT_R16G16_UNORM:           return Format::RG_UN16;
    case VK_FORMAT_R16G16_SFLOAT:          return Format::RG_F16;
    case VK_FORMAT_R32G32_SFLOAT:          return Format::RG_F32;
    case VK_FORMAT_R16G16_UINT:            return Format::RG_UI16;
    case VK_FORMAT_R32_SFLOAT:             return Format::R_F32;
    case VK_FORMAT_R16G16B16A16_SFLOAT:    return Format::RGBA_F16;
    case VK_FORMAT_R32G32B32A32_UINT:      return Format::RGBA_UI32;
    case VK_FORMAT_R32G32B32A32_SFLOAT:    return Format::RGBA_F32;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return Format::A2B10G10R10_UN;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return Format::A2R10G10B10_UN;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:   return Format::ETC2_RGB8;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:    return Format::ETC2_SRGB8;
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return Format::ETC2_RGBA1;
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:  return Format::ETC2_RGBA1_SRGB;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return Format::ETC2_RGBA8;
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:  return Format::ETC2_RGBA8_SRGB;
    case VK_FORMAT_D16_UNORM:              return Format::Z_UN16;
    case VK_FORMAT_BC4_UNORM_BLOCK:        return Format::BC4_R;
    case VK_FORMAT_BC5_UNORM_BLOCK:        return Format::BC5_RG;
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:    return Format::BC1_RGB;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK:     return Format::BC1_RGB_SRGB;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:   return Format::BC1_RGBA;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:    return Format::BC1_RGBA_SRGB;
    case VK_FORMAT_BC2_UNORM_BLOCK:        return Format::BC2_RGBA;
    case VK_FORMAT_BC2_SRGB_BLOCK:         return Format::BC2_RGBA_SRGB;
    case VK_FORMAT_BC3_UNORM_BLOCK:        return Format::BC3_RGBA;
    case VK_FORMAT_BC3_SRGB_BLOCK:         return Format::BC3_RGBA_SRGB;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:      return Format::BC6H_RGB_UFLOAT;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:      return Format::BC6H_RGB_SFLOAT;
    case VK_FORMAT_BC7_SRGB_BLOCK:         return Format::BC7_RGBA_SRGB;
    case VK_FORMAT_BC7_UNORM_BLOCK:        return Format::BC7_RGBA;
    // ASTC compressed formats (mobile/Android)
    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:   return Format::ASTC_4x4_RGBA;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:    return Format::ASTC_4x4_RGBA_SRGB;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:   return Format::ASTC_5x4_RGBA;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:    return Format::ASTC_5x4_RGBA_SRGB;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:   return Format::ASTC_5x5_RGBA;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:    return Format::ASTC_5x5_RGBA_SRGB;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:   return Format::ASTC_6x5_RGBA;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:    return Format::ASTC_6x5_RGBA_SRGB;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:   return Format::ASTC_6x6_RGBA;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:    return Format::ASTC_6x6_RGBA_SRGB;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:   return Format::ASTC_8x5_RGBA;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:    return Format::ASTC_8x5_RGBA_SRGB;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:   return Format::ASTC_8x6_RGBA;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:    return Format::ASTC_8x6_RGBA_SRGB;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:   return Format::ASTC_8x8_RGBA;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:    return Format::ASTC_8x8_RGBA_SRGB;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:  return Format::ASTC_10x5_RGBA;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:   return Format::ASTC_10x5_RGBA_SRGB;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:  return Format::ASTC_10x6_RGBA;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:   return Format::ASTC_10x6_RGBA_SRGB;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:  return Format::ASTC_10x8_RGBA;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:   return Format::ASTC_10x8_RGBA_SRGB;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return Format::ASTC_10x10_RGBA;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:  return Format::ASTC_10x10_RGBA_SRGB;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return Format::ASTC_12x10_RGBA;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:  return Format::ASTC_12x10_RGBA_SRGB;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return Format::ASTC_12x12_RGBA;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:  return Format::ASTC_12x12_RGBA_SRGB;
    case VK_FORMAT_X8_D24_UNORM_PACK32:    return Format::Z_UN24;
    case VK_FORMAT_D24_UNORM_S8_UINT:      return Format::Z_UN24_S_UI8;
    case VK_FORMAT_D32_SFLOAT:             return Format::Z_F32;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:     return Format::Z_F32_S_UI8;
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:  return Format::YUV_NV12;
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: return Format::YUV_420p;
    default:
      break;
  }
  ASSERT_MSG(false, "VkFormat value not handled: %d", vkFormat);
  return Format::Invalid;
}
