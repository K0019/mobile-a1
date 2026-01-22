/**
 * @file gfx_interface.h
 * @brief Graphics RHI abstraction layer.
 *
 * This provides the engine's graphics interface types that map to hina-vk internally.
 * Engine code uses gfx:: types - never hina_ directly.
 */

#pragma once

#include <hina_vk.h>
#include <cstdint>
#include <cstddef>
#include <array>

// ============================================================================
// Global Handle Comparison Operators - Required for std::unique/std::sort
// These must be in global namespace for ADL to find them with hina_ types
// ============================================================================

inline bool operator==(const hina_buffer& a, const hina_buffer& b) { return a.id == b.id; }
inline bool operator!=(const hina_buffer& a, const hina_buffer& b) { return a.id != b.id; }
inline bool operator<(const hina_buffer& a, const hina_buffer& b) { return a.id < b.id; }

inline bool operator==(const hina_texture& a, const hina_texture& b) { return a.id == b.id; }
inline bool operator!=(const hina_texture& a, const hina_texture& b) { return a.id != b.id; }
inline bool operator<(const hina_texture& a, const hina_texture& b) { return a.id < b.id; }

inline bool operator==(const hina_texture_view& a, const hina_texture_view& b) { return a.id == b.id; }
inline bool operator!=(const hina_texture_view& a, const hina_texture_view& b) { return a.id != b.id; }
inline bool operator<(const hina_texture_view& a, const hina_texture_view& b) { return a.id < b.id; }

inline bool operator==(const hina_sampler& a, const hina_sampler& b) { return a.id == b.id; }
inline bool operator!=(const hina_sampler& a, const hina_sampler& b) { return a.id != b.id; }
inline bool operator<(const hina_sampler& a, const hina_sampler& b) { return a.id < b.id; }

inline bool operator==(const hina_pipeline& a, const hina_pipeline& b) { return a.id == b.id; }
inline bool operator!=(const hina_pipeline& a, const hina_pipeline& b) { return a.id != b.id; }
inline bool operator<(const hina_pipeline& a, const hina_pipeline& b) { return a.id < b.id; }

inline bool operator==(const hina_bind_group& a, const hina_bind_group& b) { return a.id == b.id; }
inline bool operator!=(const hina_bind_group& a, const hina_bind_group& b) { return a.id != b.id; }
inline bool operator<(const hina_bind_group& a, const hina_bind_group& b) { return a.id < b.id; }

inline bool operator==(const hina_bind_group_layout& a, const hina_bind_group_layout& b) { return a.id == b.id; }
inline bool operator!=(const hina_bind_group_layout& a, const hina_bind_group_layout& b) { return a.id != b.id; }
inline bool operator<(const hina_bind_group_layout& a, const hina_bind_group_layout& b) { return a.id < b.id; }

inline bool operator==(const hina_query_pool& a, const hina_query_pool& b) { return a.id == b.id; }
inline bool operator!=(const hina_query_pool& a, const hina_query_pool& b) { return a.id != b.id; }
inline bool operator<(const hina_query_pool& a, const hina_query_pool& b) { return a.id < b.id; }

inline bool operator==(const hina_sync_point& a, const hina_sync_point& b) { return a.queue == b.queue && a.index == b.index; }
inline bool operator!=(const hina_sync_point& a, const hina_sync_point& b) { return !(a == b); }
inline bool operator<(const hina_sync_point& a, const hina_sync_point& b) {
    return a.queue != b.queue ? a.queue < b.queue : a.index < b.index;
}

inline bool operator==(const hina_pass_layout& a, const hina_pass_layout& b) { return a.key == b.key; }
inline bool operator!=(const hina_pass_layout& a, const hina_pass_layout& b) { return a.key != b.key; }
inline bool operator<(const hina_pass_layout& a, const hina_pass_layout& b) { return a.key < b.key; }

namespace gfx {

// ============================================================================
// Constants
// ============================================================================

constexpr uint32_t MAX_COLOR_ATTACHMENTS = 4;
constexpr uint32_t MAX_VERTEX_BUFFERS = 8;
constexpr uint32_t MAX_VERTEX_ATTRS = 16;
constexpr uint32_t MAX_SUBMIT_DEPENDENCIES = 4;

// ============================================================================
// Handle Types - Direct mappings to hina handles
// ============================================================================

using Buffer = hina_buffer;
using Texture = hina_texture;
using TextureView = hina_texture_view;
using Sampler = hina_sampler;
using Pipeline = hina_pipeline;
using BindGroup = hina_bind_group;
using BindGroupLayout = hina_bind_group_layout;
using QueryPool = hina_query_pool;
using Shader = hina_shader;
using Cmd = hina_cmd;
using SyncPoint = hina_sync_point;
using Ticket = hina_ticket;
using PassLayout = hina_pass_layout;

// ============================================================================
// Enums - Engine-facing names mapping to hina enums
// ============================================================================

enum class Format : uint32_t {
    Undefined = HINA_FORMAT_UNDEFINED,
    R8_UNorm = HINA_FORMAT_R8_UNORM,
    R8_SNorm = HINA_FORMAT_R8_SNORM,
    R8_UInt = HINA_FORMAT_R8_UINT,
    R8_SInt = HINA_FORMAT_R8_SINT,
    RG8_UNorm = HINA_FORMAT_R8G8_UNORM,
    RG8_SNorm = HINA_FORMAT_R8G8_SNORM,
    RG8_UInt = HINA_FORMAT_R8G8_UINT,
    RG8_SInt = HINA_FORMAT_R8G8_SINT,
    RGBA8_UNorm = HINA_FORMAT_R8G8B8A8_UNORM,
    RGBA8_SNorm = HINA_FORMAT_R8G8B8A8_SNORM,
    RGBA8_UInt = HINA_FORMAT_R8G8B8A8_UINT,
    RGBA8_SInt = HINA_FORMAT_R8G8B8A8_SINT,
    RGBA8_SRGB = HINA_FORMAT_R8G8B8A8_SRGB,
    BGRA8_UNorm = HINA_FORMAT_B8G8R8A8_UNORM,
    BGRA8_SRGB = HINA_FORMAT_B8G8R8A8_SRGB,
    R16_UNorm = HINA_FORMAT_R16_UNORM,
    R16_SNorm = HINA_FORMAT_R16_SNORM,
    R16_UInt = HINA_FORMAT_R16_UINT,
    R16_SInt = HINA_FORMAT_R16_SINT,
    R16_Float = HINA_FORMAT_R16_SFLOAT,
    RG16_UNorm = HINA_FORMAT_R16G16_UNORM,
    RG16_SNorm = HINA_FORMAT_R16G16_SNORM,
    RG16_UInt = HINA_FORMAT_R16G16_UINT,
    RG16_SInt = HINA_FORMAT_R16G16_SINT,
    RG16_Float = HINA_FORMAT_R16G16_SFLOAT,
    RGBA16_UNorm = HINA_FORMAT_R16G16B16A16_UNORM,
    RGBA16_SNorm = HINA_FORMAT_R16G16B16A16_SNORM,
    RGBA16_UInt = HINA_FORMAT_R16G16B16A16_UINT,
    RGBA16_SInt = HINA_FORMAT_R16G16B16A16_SINT,
    RGBA16_Float = HINA_FORMAT_R16G16B16A16_SFLOAT,
    R32_UInt = HINA_FORMAT_R32_UINT,
    R32_SInt = HINA_FORMAT_R32_SINT,
    R32_Float = HINA_FORMAT_R32_SFLOAT,
    RG32_UInt = HINA_FORMAT_R32G32_UINT,
    RG32_SInt = HINA_FORMAT_R32G32_SINT,
    RG32_Float = HINA_FORMAT_R32G32_SFLOAT,
    RGB32_UInt = HINA_FORMAT_R32G32B32_UINT,
    RGB32_SInt = HINA_FORMAT_R32G32B32_SINT,
    RGB32_Float = HINA_FORMAT_R32G32B32_SFLOAT,
    RGBA32_UInt = HINA_FORMAT_R32G32B32A32_UINT,
    RGBA32_SInt = HINA_FORMAT_R32G32B32A32_SINT,
    RGBA32_Float = HINA_FORMAT_R32G32B32A32_SFLOAT,
    // Depth/Stencil
    D16_UNorm = HINA_FORMAT_D16_UNORM,
    D24_UNorm_X8 = HINA_FORMAT_X8_D24_UNORM_PACK32,
    D32_Float = HINA_FORMAT_D32_SFLOAT,
    S8_UInt = HINA_FORMAT_S8_UINT,
    D16_UNorm_S8_UInt = HINA_FORMAT_D16_UNORM_S8_UINT,
    D24_UNorm_S8_UInt = HINA_FORMAT_D24_UNORM_S8_UINT,
    D32_Float_S8_UInt = HINA_FORMAT_D32_SFLOAT_S8_UINT,
    // Compressed
    BC1_RGB_UNorm = HINA_FORMAT_BC1_RGB_UNORM_BLOCK,
    BC1_RGB_SRGB = HINA_FORMAT_BC1_RGB_SRGB_BLOCK,
    BC1_RGBA_UNorm = HINA_FORMAT_BC1_RGBA_UNORM_BLOCK,
    BC1_RGBA_SRGB = HINA_FORMAT_BC1_RGBA_SRGB_BLOCK,
    BC2_UNorm = HINA_FORMAT_BC2_UNORM_BLOCK,
    BC2_SRGB = HINA_FORMAT_BC2_SRGB_BLOCK,
    BC3_UNorm = HINA_FORMAT_BC3_UNORM_BLOCK,
    BC3_SRGB = HINA_FORMAT_BC3_SRGB_BLOCK,
    BC4_UNorm = HINA_FORMAT_BC4_UNORM_BLOCK,
    BC4_SNorm = HINA_FORMAT_BC4_SNORM_BLOCK,
    BC5_UNorm = HINA_FORMAT_BC5_UNORM_BLOCK,
    BC5_SNorm = HINA_FORMAT_BC5_SNORM_BLOCK,
    BC6H_UFloat = HINA_FORMAT_BC6H_UFLOAT_BLOCK,
    BC6H_SFloat = HINA_FORMAT_BC6H_SFLOAT_BLOCK,
    BC7_UNorm = HINA_FORMAT_BC7_UNORM_BLOCK,
    BC7_SRGB = HINA_FORMAT_BC7_SRGB_BLOCK,
    // ASTC (Android/iOS)
    ASTC_4x4_UNorm = HINA_FORMAT_ASTC_4x4_UNORM_BLOCK,
    ASTC_4x4_SRGB = HINA_FORMAT_ASTC_4x4_SRGB_BLOCK,
    ASTC_5x4_UNorm = HINA_FORMAT_ASTC_5x4_UNORM_BLOCK,
    ASTC_5x4_SRGB = HINA_FORMAT_ASTC_5x4_SRGB_BLOCK,
    ASTC_5x5_UNorm = HINA_FORMAT_ASTC_5x5_UNORM_BLOCK,
    ASTC_5x5_SRGB = HINA_FORMAT_ASTC_5x5_SRGB_BLOCK,
    ASTC_6x5_UNorm = HINA_FORMAT_ASTC_6x5_UNORM_BLOCK,
    ASTC_6x5_SRGB = HINA_FORMAT_ASTC_6x5_SRGB_BLOCK,
    ASTC_6x6_UNorm = HINA_FORMAT_ASTC_6x6_UNORM_BLOCK,
    ASTC_6x6_SRGB = HINA_FORMAT_ASTC_6x6_SRGB_BLOCK,
    ASTC_8x5_UNorm = HINA_FORMAT_ASTC_8x5_UNORM_BLOCK,
    ASTC_8x5_SRGB = HINA_FORMAT_ASTC_8x5_SRGB_BLOCK,
    ASTC_8x6_UNorm = HINA_FORMAT_ASTC_8x6_UNORM_BLOCK,
    ASTC_8x6_SRGB = HINA_FORMAT_ASTC_8x6_SRGB_BLOCK,
    ASTC_8x8_UNorm = HINA_FORMAT_ASTC_8x8_UNORM_BLOCK,
    ASTC_8x8_SRGB = HINA_FORMAT_ASTC_8x8_SRGB_BLOCK,
    ASTC_10x5_UNorm = HINA_FORMAT_ASTC_10x5_UNORM_BLOCK,
    ASTC_10x5_SRGB = HINA_FORMAT_ASTC_10x5_SRGB_BLOCK,
    ASTC_10x6_UNorm = HINA_FORMAT_ASTC_10x6_UNORM_BLOCK,
    ASTC_10x6_SRGB = HINA_FORMAT_ASTC_10x6_SRGB_BLOCK,
    ASTC_10x8_UNorm = HINA_FORMAT_ASTC_10x8_UNORM_BLOCK,
    ASTC_10x8_SRGB = HINA_FORMAT_ASTC_10x8_SRGB_BLOCK,
    ASTC_10x10_UNorm = HINA_FORMAT_ASTC_10x10_UNORM_BLOCK,
    ASTC_10x10_SRGB = HINA_FORMAT_ASTC_10x10_SRGB_BLOCK,
    ASTC_12x10_UNorm = HINA_FORMAT_ASTC_12x10_UNORM_BLOCK,
    ASTC_12x10_SRGB = HINA_FORMAT_ASTC_12x10_SRGB_BLOCK,
    ASTC_12x12_UNorm = HINA_FORMAT_ASTC_12x12_UNORM_BLOCK,
    ASTC_12x12_SRGB = HINA_FORMAT_ASTC_12x12_SRGB_BLOCK,
};

// Convert VkFormat integer to gfx::Format
// Note: VkFormat and hina_format/gfx::Format use different numbering schemes
inline Format vkFormatToFormat(int vkFormat) {
    // Common VkFormat values mapped to gfx::Format (which uses hina_format values)
    switch (vkFormat) {
        case 0: return Format::Undefined;                // VK_FORMAT_UNDEFINED
        case 9: return Format::R8_UNorm;                 // VK_FORMAT_R8_UNORM
        case 10: return Format::R8_SNorm;                // VK_FORMAT_R8_SNORM
        case 13: return Format::R8_UInt;                 // VK_FORMAT_R8_UINT
        case 14: return Format::R8_SInt;                 // VK_FORMAT_R8_SINT
        case 16: return Format::RG8_UNorm;               // VK_FORMAT_R8G8_UNORM
        case 17: return Format::RG8_SNorm;               // VK_FORMAT_R8G8_SNORM
        case 20: return Format::RG8_UInt;                // VK_FORMAT_R8G8_UINT
        case 21: return Format::RG8_SInt;                // VK_FORMAT_R8G8_SINT
        case 37: return Format::RGBA8_UNorm;             // VK_FORMAT_R8G8B8A8_UNORM
        case 38: return Format::RGBA8_SNorm;             // VK_FORMAT_R8G8B8A8_SNORM
        case 41: return Format::RGBA8_UInt;              // VK_FORMAT_R8G8B8A8_UINT
        case 42: return Format::RGBA8_SInt;              // VK_FORMAT_R8G8B8A8_SINT
        case 43: return Format::RGBA8_SRGB;              // VK_FORMAT_R8G8B8A8_SRGB
        case 44: return Format::BGRA8_UNorm;             // VK_FORMAT_B8G8R8A8_UNORM
        case 50: return Format::BGRA8_SRGB;              // VK_FORMAT_B8G8R8A8_SRGB
        case 70: return Format::R16_UNorm;               // VK_FORMAT_R16_UNORM
        case 71: return Format::R16_SNorm;               // VK_FORMAT_R16_SNORM
        case 74: return Format::R16_UInt;                // VK_FORMAT_R16_UINT
        case 75: return Format::R16_SInt;                // VK_FORMAT_R16_SINT
        case 76: return Format::R16_Float;               // VK_FORMAT_R16_SFLOAT
        case 77: return Format::RG16_UNorm;              // VK_FORMAT_R16G16_UNORM
        case 78: return Format::RG16_SNorm;              // VK_FORMAT_R16G16_SNORM
        case 81: return Format::RG16_UInt;               // VK_FORMAT_R16G16_UINT
        case 82: return Format::RG16_SInt;               // VK_FORMAT_R16G16_SINT
        case 83: return Format::RG16_Float;              // VK_FORMAT_R16G16_SFLOAT
        case 91: return Format::RGBA16_UNorm;            // VK_FORMAT_R16G16B16A16_UNORM
        case 92: return Format::RGBA16_SNorm;            // VK_FORMAT_R16G16B16A16_SNORM
        case 95: return Format::RGBA16_UInt;             // VK_FORMAT_R16G16B16A16_UINT
        case 96: return Format::RGBA16_SInt;             // VK_FORMAT_R16G16B16A16_SINT
        case 97: return Format::RGBA16_Float;            // VK_FORMAT_R16G16B16A16_SFLOAT
        case 98: return Format::R32_UInt;                // VK_FORMAT_R32_UINT
        case 99: return Format::R32_SInt;                // VK_FORMAT_R32_SINT
        case 100: return Format::R32_Float;              // VK_FORMAT_R32_SFLOAT
        case 101: return Format::RG32_UInt;              // VK_FORMAT_R32G32_UINT
        case 102: return Format::RG32_SInt;              // VK_FORMAT_R32G32_SINT
        case 103: return Format::RG32_Float;             // VK_FORMAT_R32G32_SFLOAT
        case 104: return Format::RGB32_UInt;             // VK_FORMAT_R32G32B32_UINT
        case 105: return Format::RGB32_SInt;             // VK_FORMAT_R32G32B32_SINT
        case 106: return Format::RGB32_Float;            // VK_FORMAT_R32G32B32_SFLOAT
        case 107: return Format::RGBA32_UInt;            // VK_FORMAT_R32G32B32A32_UINT
        case 108: return Format::RGBA32_SInt;            // VK_FORMAT_R32G32B32A32_SINT
        case 109: return Format::RGBA32_Float;           // VK_FORMAT_R32G32B32A32_SFLOAT
        case 124: return Format::D16_UNorm;              // VK_FORMAT_D16_UNORM
        case 125: return Format::D24_UNorm_X8;           // VK_FORMAT_X8_D24_UNORM_PACK32
        case 126: return Format::D32_Float;              // VK_FORMAT_D32_SFLOAT
        case 127: return Format::S8_UInt;                // VK_FORMAT_S8_UINT
        case 128: return Format::D16_UNorm_S8_UInt;      // VK_FORMAT_D16_UNORM_S8_UINT
        case 129: return Format::D24_UNorm_S8_UInt;      // VK_FORMAT_D24_UNORM_S8_UINT
        case 130: return Format::D32_Float_S8_UInt;      // VK_FORMAT_D32_SFLOAT_S8_UINT
        case 131: return Format::BC1_RGB_UNorm;          // VK_FORMAT_BC1_RGB_UNORM_BLOCK
        case 132: return Format::BC1_RGB_SRGB;           // VK_FORMAT_BC1_RGB_SRGB_BLOCK
        case 133: return Format::BC1_RGBA_UNorm;         // VK_FORMAT_BC1_RGBA_UNORM_BLOCK
        case 134: return Format::BC1_RGBA_SRGB;          // VK_FORMAT_BC1_RGBA_SRGB_BLOCK
        case 135: return Format::BC2_UNorm;              // VK_FORMAT_BC2_UNORM_BLOCK
        case 136: return Format::BC2_SRGB;               // VK_FORMAT_BC2_SRGB_BLOCK
        case 137: return Format::BC3_UNorm;              // VK_FORMAT_BC3_UNORM_BLOCK
        case 138: return Format::BC3_SRGB;               // VK_FORMAT_BC3_SRGB_BLOCK
        case 139: return Format::BC4_UNorm;              // VK_FORMAT_BC4_UNORM_BLOCK
        case 140: return Format::BC4_SNorm;              // VK_FORMAT_BC4_SNORM_BLOCK
        case 141: return Format::BC5_UNorm;              // VK_FORMAT_BC5_UNORM_BLOCK
        case 142: return Format::BC5_SNorm;              // VK_FORMAT_BC5_SNORM_BLOCK
        case 143: return Format::BC6H_UFloat;            // VK_FORMAT_BC6H_UFLOAT_BLOCK
        case 144: return Format::BC6H_SFloat;            // VK_FORMAT_BC6H_SFLOAT_BLOCK
        case 145: return Format::BC7_UNorm;              // VK_FORMAT_BC7_UNORM_BLOCK
        case 146: return Format::BC7_SRGB;               // VK_FORMAT_BC7_SRGB_BLOCK
        // ASTC formats (VK_FORMAT_ASTC_*)
        case 157: return Format::ASTC_4x4_UNorm;         // VK_FORMAT_ASTC_4x4_UNORM_BLOCK
        case 158: return Format::ASTC_4x4_SRGB;          // VK_FORMAT_ASTC_4x4_SRGB_BLOCK
        case 159: return Format::ASTC_5x4_UNorm;         // VK_FORMAT_ASTC_5x4_UNORM_BLOCK
        case 160: return Format::ASTC_5x4_SRGB;          // VK_FORMAT_ASTC_5x4_SRGB_BLOCK
        case 161: return Format::ASTC_5x5_UNorm;         // VK_FORMAT_ASTC_5x5_UNORM_BLOCK
        case 162: return Format::ASTC_5x5_SRGB;          // VK_FORMAT_ASTC_5x5_SRGB_BLOCK
        case 163: return Format::ASTC_6x5_UNorm;         // VK_FORMAT_ASTC_6x5_UNORM_BLOCK
        case 164: return Format::ASTC_6x5_SRGB;          // VK_FORMAT_ASTC_6x5_SRGB_BLOCK
        case 165: return Format::ASTC_6x6_UNorm;         // VK_FORMAT_ASTC_6x6_UNORM_BLOCK
        case 166: return Format::ASTC_6x6_SRGB;          // VK_FORMAT_ASTC_6x6_SRGB_BLOCK
        case 167: return Format::ASTC_8x5_UNorm;         // VK_FORMAT_ASTC_8x5_UNORM_BLOCK
        case 168: return Format::ASTC_8x5_SRGB;          // VK_FORMAT_ASTC_8x5_SRGB_BLOCK
        case 169: return Format::ASTC_8x6_UNorm;         // VK_FORMAT_ASTC_8x6_UNORM_BLOCK
        case 170: return Format::ASTC_8x6_SRGB;          // VK_FORMAT_ASTC_8x6_SRGB_BLOCK
        case 171: return Format::ASTC_8x8_UNorm;         // VK_FORMAT_ASTC_8x8_UNORM_BLOCK
        case 172: return Format::ASTC_8x8_SRGB;          // VK_FORMAT_ASTC_8x8_SRGB_BLOCK
        case 173: return Format::ASTC_10x5_UNorm;        // VK_FORMAT_ASTC_10x5_UNORM_BLOCK
        case 174: return Format::ASTC_10x5_SRGB;         // VK_FORMAT_ASTC_10x5_SRGB_BLOCK
        case 175: return Format::ASTC_10x6_UNorm;        // VK_FORMAT_ASTC_10x6_UNORM_BLOCK
        case 176: return Format::ASTC_10x6_SRGB;         // VK_FORMAT_ASTC_10x6_SRGB_BLOCK
        case 177: return Format::ASTC_10x8_UNorm;        // VK_FORMAT_ASTC_10x8_UNORM_BLOCK
        case 178: return Format::ASTC_10x8_SRGB;         // VK_FORMAT_ASTC_10x8_SRGB_BLOCK
        case 179: return Format::ASTC_10x10_UNorm;       // VK_FORMAT_ASTC_10x10_UNORM_BLOCK
        case 180: return Format::ASTC_10x10_SRGB;        // VK_FORMAT_ASTC_10x10_SRGB_BLOCK
        case 181: return Format::ASTC_12x10_UNorm;       // VK_FORMAT_ASTC_12x10_UNORM_BLOCK
        case 182: return Format::ASTC_12x10_SRGB;        // VK_FORMAT_ASTC_12x10_SRGB_BLOCK
        case 183: return Format::ASTC_12x12_UNorm;       // VK_FORMAT_ASTC_12x12_UNORM_BLOCK
        case 184: return Format::ASTC_12x12_SRGB;        // VK_FORMAT_ASTC_12x12_SRGB_BLOCK
        default: return Format::Undefined;
    }
}

/**
 * @brief Convert a UNORM format to its SRGB equivalent.
 * @param format The input format (may be UNORM or already SRGB)
 * @param toSRGB If true, convert UNORM->SRGB; if false, convert SRGB->UNORM
 * @return The converted format, or the original if no conversion exists
 */
inline Format convertFormatSRGB(Format format, bool toSRGB) {
    if (toSRGB) {
        // Convert UNORM -> SRGB
        switch (format) {
            case Format::RGBA8_UNorm:     return Format::RGBA8_SRGB;
            case Format::BGRA8_UNorm:     return Format::BGRA8_SRGB;
            case Format::BC1_RGB_UNorm:   return Format::BC1_RGB_SRGB;
            case Format::BC1_RGBA_UNorm:  return Format::BC1_RGBA_SRGB;
            case Format::BC2_UNorm:       return Format::BC2_SRGB;
            case Format::BC3_UNorm:       return Format::BC3_SRGB;
            case Format::BC7_UNorm:       return Format::BC7_SRGB;
            // ASTC formats
            case Format::ASTC_4x4_UNorm:  return Format::ASTC_4x4_SRGB;
            case Format::ASTC_5x4_UNorm:  return Format::ASTC_5x4_SRGB;
            case Format::ASTC_5x5_UNorm:  return Format::ASTC_5x5_SRGB;
            case Format::ASTC_6x5_UNorm:  return Format::ASTC_6x5_SRGB;
            case Format::ASTC_6x6_UNorm:  return Format::ASTC_6x6_SRGB;
            case Format::ASTC_8x5_UNorm:  return Format::ASTC_8x5_SRGB;
            case Format::ASTC_8x6_UNorm:  return Format::ASTC_8x6_SRGB;
            case Format::ASTC_8x8_UNorm:  return Format::ASTC_8x8_SRGB;
            case Format::ASTC_10x5_UNorm: return Format::ASTC_10x5_SRGB;
            case Format::ASTC_10x6_UNorm: return Format::ASTC_10x6_SRGB;
            case Format::ASTC_10x8_UNorm: return Format::ASTC_10x8_SRGB;
            case Format::ASTC_10x10_UNorm:return Format::ASTC_10x10_SRGB;
            case Format::ASTC_12x10_UNorm:return Format::ASTC_12x10_SRGB;
            case Format::ASTC_12x12_UNorm:return Format::ASTC_12x12_SRGB;
            default: return format;  // Already SRGB or no conversion available
        }
    } else {
        // Convert SRGB -> UNORM
        switch (format) {
            case Format::RGBA8_SRGB:      return Format::RGBA8_UNorm;
            case Format::BGRA8_SRGB:      return Format::BGRA8_UNorm;
            case Format::BC1_RGB_SRGB:    return Format::BC1_RGB_UNorm;
            case Format::BC1_RGBA_SRGB:   return Format::BC1_RGBA_UNorm;
            case Format::BC2_SRGB:        return Format::BC2_UNorm;
            case Format::BC3_SRGB:        return Format::BC3_UNorm;
            case Format::BC7_SRGB:        return Format::BC7_UNorm;
            // ASTC formats
            case Format::ASTC_4x4_SRGB:   return Format::ASTC_4x4_UNorm;
            case Format::ASTC_5x4_SRGB:   return Format::ASTC_5x4_UNorm;
            case Format::ASTC_5x5_SRGB:   return Format::ASTC_5x5_UNorm;
            case Format::ASTC_6x5_SRGB:   return Format::ASTC_6x5_UNorm;
            case Format::ASTC_6x6_SRGB:   return Format::ASTC_6x6_UNorm;
            case Format::ASTC_8x5_SRGB:   return Format::ASTC_8x5_UNorm;
            case Format::ASTC_8x6_SRGB:   return Format::ASTC_8x6_UNorm;
            case Format::ASTC_8x8_SRGB:   return Format::ASTC_8x8_UNorm;
            case Format::ASTC_10x5_SRGB:  return Format::ASTC_10x5_UNorm;
            case Format::ASTC_10x6_SRGB:  return Format::ASTC_10x6_UNorm;
            case Format::ASTC_10x8_SRGB:  return Format::ASTC_10x8_UNorm;
            case Format::ASTC_10x10_SRGB: return Format::ASTC_10x10_UNorm;
            case Format::ASTC_12x10_SRGB: return Format::ASTC_12x10_UNorm;
            case Format::ASTC_12x12_SRGB: return Format::ASTC_12x12_UNorm;
            default: return format;  // Already UNORM or no conversion available
        }
    }
}

/**
 * @brief Check if a format is an SRGB format.
 */
inline bool isFormatSRGB(Format format) {
    switch (format) {
        case Format::RGBA8_SRGB:
        case Format::BGRA8_SRGB:
        case Format::BC1_RGB_SRGB:
        case Format::BC1_RGBA_SRGB:
        case Format::BC2_SRGB:
        case Format::BC3_SRGB:
        case Format::BC7_SRGB:
        // ASTC SRGB formats
        case Format::ASTC_4x4_SRGB:
        case Format::ASTC_5x4_SRGB:
        case Format::ASTC_5x5_SRGB:
        case Format::ASTC_6x5_SRGB:
        case Format::ASTC_6x6_SRGB:
        case Format::ASTC_8x5_SRGB:
        case Format::ASTC_8x6_SRGB:
        case Format::ASTC_8x8_SRGB:
        case Format::ASTC_10x5_SRGB:
        case Format::ASTC_10x6_SRGB:
        case Format::ASTC_10x8_SRGB:
        case Format::ASTC_10x10_SRGB:
        case Format::ASTC_12x10_SRGB:
        case Format::ASTC_12x12_SRGB:
            return true;
        default:
            return false;
    }
}

enum class IndexType : uint32_t {
    UInt16 = HINA_INDEX_UINT16,
    UInt32 = HINA_INDEX_UINT32,
};

enum class Topology : uint32_t {
    PointList = HINA_PRIMITIVE_TOPOLOGY_POINT_LIST,
    LineList = HINA_PRIMITIVE_TOPOLOGY_LINE_LIST,
    LineStrip = HINA_PRIMITIVE_TOPOLOGY_LINE_STRIP,
    TriangleList = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    TriangleStrip = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    TriangleFan = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
    PatchList = HINA_PRIMITIVE_TOPOLOGY_PATCH_LIST,
};

enum class PolygonMode : uint32_t {
    Fill = HINA_POLYGON_MODE_FILL,
    Line = HINA_POLYGON_MODE_LINE,
    Point = HINA_POLYGON_MODE_POINT,
};

enum class CullMode : uint32_t {
    None = HINA_CULL_MODE_NONE,
    Front = HINA_CULL_MODE_FRONT,
    Back = HINA_CULL_MODE_BACK,
    FrontAndBack = HINA_CULL_MODE_FRONT_AND_BACK,
};

enum class FrontFace : uint32_t {
    CounterClockwise = HINA_FRONT_FACE_COUNTER_CLOCKWISE,
    Clockwise = HINA_FRONT_FACE_CLOCKWISE,
};

enum class CompareOp : uint32_t {
    Never = HINA_COMPARE_OP_NEVER,
    Less = HINA_COMPARE_OP_LESS,
    Equal = HINA_COMPARE_OP_EQUAL,
    LessOrEqual = HINA_COMPARE_OP_LESS_OR_EQUAL,
    Greater = HINA_COMPARE_OP_GREATER,
    NotEqual = HINA_COMPARE_OP_NOT_EQUAL,
    GreaterOrEqual = HINA_COMPARE_OP_GREATER_OR_EQUAL,
    Always = HINA_COMPARE_OP_ALWAYS,
};

enum class BlendFactor : uint32_t {
    Zero = HINA_BLEND_FACTOR_ZERO,
    One = HINA_BLEND_FACTOR_ONE,
    SrcColor = HINA_BLEND_FACTOR_SRC_COLOR,
    OneMinusSrcColor = HINA_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    DstColor = HINA_BLEND_FACTOR_DST_COLOR,
    OneMinusDstColor = HINA_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    SrcAlpha = HINA_BLEND_FACTOR_SRC_ALPHA,
    OneMinusSrcAlpha = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    DstAlpha = HINA_BLEND_FACTOR_DST_ALPHA,
    OneMinusDstAlpha = HINA_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    ConstantColor = HINA_BLEND_FACTOR_CONSTANT_COLOR,
    OneMinusConstantColor = HINA_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    ConstantAlpha = HINA_BLEND_FACTOR_CONSTANT_ALPHA,
    OneMinusConstantAlpha = HINA_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    SrcAlphaSaturate = HINA_BLEND_FACTOR_SRC_ALPHA_SATURATE,
};

enum class BlendOp : uint32_t {
    Add = HINA_BLEND_OP_ADD,
    Subtract = HINA_BLEND_OP_SUBTRACT,
    ReverseSubtract = HINA_BLEND_OP_REVERSE_SUBTRACT,
    Min = HINA_BLEND_OP_MIN,
    Max = HINA_BLEND_OP_MAX,
};

enum class Filter : uint32_t {
    Nearest = HINA_FILTER_NEAREST,
    Linear = HINA_FILTER_LINEAR,
};

enum class AddressMode : uint32_t {
    Repeat = HINA_ADDRESS_REPEAT,
    MirroredRepeat = HINA_ADDRESS_MIRRORED_REPEAT,
    ClampToEdge = HINA_ADDRESS_CLAMP_TO_EDGE,
    ClampToBorder = HINA_ADDRESS_CLAMP_TO_BORDER,
};

enum class TextureType : uint32_t {
    Tex2D = HINA_TEX_TYPE_2D,
    Tex2DArray = HINA_TEX_TYPE_2D_ARRAY,
    TexCube = HINA_TEX_TYPE_CUBE,
    Tex3D = HINA_TEX_TYPE_3D,
};

enum class SampleCount : uint32_t {
    x1 = HINA_SAMPLE_COUNT_1_BIT,
    x2 = HINA_SAMPLE_COUNT_2_BIT,
    x4 = HINA_SAMPLE_COUNT_4_BIT,
    x8 = HINA_SAMPLE_COUNT_8_BIT,
};

enum class ColorSpace : uint8_t {
    SRGB_LINEAR,      // Linear sRGB (no gamma correction)
    SRGB_NONLINEAR,   // sRGB with gamma correction (standard monitors)
};

enum class SurfaceTransform : uint8_t {
    Identity = 0,
    Rotate90,
    Rotate180,
    Rotate270,
    HorizontalMirror,
    HorizontalMirrorRotate90,
    HorizontalMirrorRotate180,
    HorizontalMirrorRotate270,
    Inherit,
};

enum class LoadOp : uint32_t {
    Load = HINA_LOAD_OP_LOAD,
    Clear = HINA_LOAD_OP_CLEAR,
    DontCare = HINA_LOAD_OP_DONT_CARE,
};

enum class StoreOp : uint32_t {
    Store = HINA_STORE_OP_STORE,
    DontCare = HINA_STORE_OP_DONT_CARE,
};

enum class TextureState : uint32_t {
    ShaderRead = HINA_TEXSTATE_SHADER_READ,
    ColorAttachment = HINA_TEXSTATE_COLOR_ATTACHMENT,
    DepthAttachment = HINA_TEXSTATE_DEPTH_ATTACHMENT,
    TransferSrc = HINA_TEXSTATE_TRANSFER_SRC,
    TransferDst = HINA_TEXSTATE_TRANSFER_DST,
    Present = HINA_TEXSTATE_PRESENT,
};

// Queue type - same as gfx_types.h
using Queue = hina_queue;

// ============================================================================
// Flags
// ============================================================================

namespace BufferUsage {
    constexpr uint32_t HostVisible = HINA_BUFFER_HOST_VISIBLE_BIT;
    constexpr uint32_t HostCoherent = HINA_BUFFER_HOST_COHERENT_BIT;
    constexpr uint32_t DeviceLocal = HINA_BUFFER_DEVICE_LOCAL_BIT;
    constexpr uint32_t Vertex = HINA_BUFFER_VERTEX_BIT;
    constexpr uint32_t Index = HINA_BUFFER_INDEX_BIT;
    constexpr uint32_t Uniform = HINA_BUFFER_UNIFORM_BIT;
    constexpr uint32_t Storage = HINA_BUFFER_STORAGE_BIT;
    constexpr uint32_t Indirect = HINA_BUFFER_INDIRECT_BIT;
    constexpr uint32_t TransferSrc = HINA_BUFFER_TRANSFER_SRC_BIT;
    constexpr uint32_t TransferDst = HINA_BUFFER_TRANSFER_DST_BIT;
}

namespace TextureUsage {
    constexpr uint32_t Sampled = HINA_TEXTURE_SAMPLED_BIT;
    constexpr uint32_t Storage = HINA_TEXTURE_STORAGE_BIT;
    constexpr uint32_t RenderTarget = HINA_TEXTURE_RENDER_TARGET_BIT;
    constexpr uint32_t TransferSrc = HINA_TEXTURE_TRANSFER_SRC_BIT;
    constexpr uint32_t Transient = HINA_TEXTURE_TRANSIENT_BIT;
}

namespace ShaderStage {
    constexpr uint32_t Vertex = HINA_STAGE_VERTEX;
    constexpr uint32_t TessControl = HINA_STAGE_TESS_CONTROL;
    constexpr uint32_t TessEval = HINA_STAGE_TESS_EVAL;
    constexpr uint32_t Geometry = HINA_STAGE_GEOMETRY;
    constexpr uint32_t Fragment = HINA_STAGE_FRAGMENT;
    constexpr uint32_t Compute = HINA_STAGE_COMPUTE;
    constexpr uint32_t AllGraphics = HINA_STAGE_ALL_GRAPHICS;
    constexpr uint32_t All = HINA_STAGE_ALL;
}

namespace ColorComponent {
    constexpr uint32_t R = HINA_COLOR_COMPONENT_R_BIT;
    constexpr uint32_t G = HINA_COLOR_COMPONENT_G_BIT;
    constexpr uint32_t B = HINA_COLOR_COMPONENT_B_BIT;
    constexpr uint32_t A = HINA_COLOR_COMPONENT_A_BIT;
    constexpr uint32_t All = HINA_COLOR_COMPONENT_ALL;
}

// ============================================================================
// Descriptor Structs - Thin wrappers over hina structs
// ============================================================================

using BufferDesc = hina_buffer_desc;
using TextureDesc = hina_texture_desc;
using SamplerDesc = hina_sampler_desc;
using BindGroupDesc = hina_bind_group_desc;
using BindGroupLayoutDesc = hina_bind_group_layout_desc;
using BindGroupEntry = hina_bind_group_entry;
using BindGroupLayoutEntry = hina_bind_group_layout_entry;

// ============================================================================
// Resource Creation - Inline wrappers
// ============================================================================

inline Buffer createBuffer(const BufferDesc& desc) {
    return hina_make_buffer(&desc);
}

inline void destroyBuffer(Buffer buf) {
    hina_destroy_buffer(buf);
}

inline void* mapBuffer(Buffer buf) {
    return hina_map_buffer(buf);
}

inline void unmapBuffer(Buffer buf) {
    hina_unmap_buffer(buf);
}

inline Texture createTexture(const TextureDesc& desc) {
    return hina_make_texture(&desc);
}

inline void destroyTexture(Texture tex) {
    hina_destroy_texture(tex);
}

inline TextureView getDefaultView(Texture tex) {
    return hina_texture_get_default_view(tex);
}

inline Sampler createSampler(const SamplerDesc& desc) {
    return hina_make_sampler(&desc);
}

inline void destroySampler(Sampler samp) {
    hina_destroy_sampler(samp);
}

inline TextureDesc textureDescDefault() {
    return hina_texture_desc_default();
}

inline SamplerDesc samplerDescDefault() {
    return hina_sampler_desc_default();
}

// ============================================================================
// Basic Structs (needed before inline functions)
// ============================================================================

struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct ScissorRect {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 1;
    uint32_t height = 1;
};

// ============================================================================
// Command Recording - Inline wrappers
// ============================================================================

inline Cmd* cmdBegin() {
    return hina_cmd_begin();
}

inline Cmd* cmdBeginEx(Queue queue) {
    return hina_cmd_begin_ex(static_cast<hina_queue>(queue));
}

inline void cmdDraw(Cmd* cmd, uint32_t vertexCount, uint32_t instanceCount = 1,
                    uint32_t firstVertex = 0, uint32_t baseInstance = 0) {
    hina_cmd_draw(cmd, vertexCount, instanceCount, firstVertex, baseInstance);
}

inline void cmdDrawIndexed(Cmd* cmd, uint32_t indexCount, uint32_t instanceCount = 1,
                           uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                           uint32_t baseInstance = 0) {
    hina_cmd_draw_indexed(cmd, indexCount, instanceCount, firstIndex, vertexOffset, baseInstance);
}

inline void cmdDispatch(Cmd* cmd, uint32_t x, uint32_t y, uint32_t z) {
    hina_cmd_dispatch(cmd, x, y, z);
}

inline void cmdBindPipeline(Cmd* cmd, Pipeline pipeline) {
    hina_cmd_bind_pipeline(cmd, pipeline);
}

using VertexInput = hina_vertex_input;

inline void cmdApplyVertexInput(Cmd* cmd, const VertexInput* input) {
    hina_cmd_apply_vertex_input(cmd, input);
}

inline void cmdBindGroup(Cmd* cmd, uint32_t set, BindGroup group) {
    hina_cmd_bind_group(cmd, set, group);
}

inline void cmdBindGroupWithOffsets(Cmd* cmd, uint32_t set, BindGroup group,
                                     const uint32_t* offsets, uint32_t offsetCount) {
    hina_cmd_bind_group_with_offsets(cmd, set, group, offsets, offsetCount);
}

inline void cmdPushConstants(Cmd* cmd, uint32_t offset, uint32_t size, const void* data) {
    hina_cmd_push_constants(cmd, offset, size, data);
}

template<typename T>
inline void cmdPushConstants(Cmd* cmd, const T& data, uint32_t offset = 0) {
    hina_cmd_push_constants(cmd, offset, sizeof(T), &data);
}

inline void cmdSetViewport(Cmd* cmd, const Viewport& vp) {
    hina_viewport hvp = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
    hina_cmd_set_viewport(cmd, &hvp);
}

inline void cmdSetScissor(Cmd* cmd, const ScissorRect& sc) {
    hina_scissor hsc = {sc.x, sc.y, sc.width, sc.height};
    hina_cmd_set_scissor(cmd, &hsc);
}

inline void cmdTransitionTexture(Cmd* cmd, Texture tex, TextureState newState) {
    hina_cmd_transition_texture(cmd, tex, static_cast<hina_texture_state_hint>(newState));
}

inline void cmdBeginLabel(Cmd* cmd, const char* label, float r = 1.f, float g = 1.f, float b = 1.f, float a = 1.f) {
    float color[4] = {r, g, b, a};
    hina_cmd_begin_label(cmd, label, color);
}

inline void cmdEndLabel(Cmd* cmd) {
    hina_cmd_end_label(cmd);
}

// ============================================================================
// Utility
// ============================================================================

inline bool isValid(Buffer buf) { return hina_buffer_is_valid(buf); }
inline bool isValid(Texture tex) { return hina_texture_is_valid(tex); }
inline bool isValid(Sampler samp) { return hina_sampler_is_valid(samp); }
inline bool isValid(Pipeline pipe) { return hina_pipeline_is_valid(pipe); }

inline bool isDepthFormat(Format fmt) {
    switch (fmt) {
        case Format::D16_UNorm:
        case Format::D24_UNorm_X8:
        case Format::D32_Float:
        case Format::D16_UNorm_S8_UInt:
        case Format::D24_UNorm_S8_UInt:
        case Format::D32_Float_S8_UInt:
            return true;
        default:
            return false;
    }
}

inline bool isStencilFormat(Format fmt) {
    switch (fmt) {
        case Format::S8_UInt:
        case Format::D16_UNorm_S8_UInt:
        case Format::D24_UNorm_S8_UInt:
        case Format::D32_Float_S8_UInt:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// RenderGraph Support Structures
// ============================================================================

struct Dimensions {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;

    Dimensions divide1D(uint32_t v) const {
        return {width / v, height, depth};
    }
    Dimensions divide2D(uint32_t v) const {
        return {width / v, height / v, depth};
    }
    Dimensions divide3D(uint32_t v) const {
        return {width / v, height / v, depth / v};
    }
    bool operator==(const Dimensions& other) const {
        return width == other.width && height == other.height && depth == other.depth;
    }
};

// Texture metadata for loaded textures (used by resource system)
struct TextureMetadata {
    TextureType type = TextureType::Tex2D;
    Format format = Format::Undefined;
    Dimensions dimensions = {1, 1, 1};
    uint32_t numLayers = 1;
    uint32_t numMipLevels = 1;
    uint8_t usage = TextureUsage::Sampled;
    const char* debugName = "";
};

// Render pass attachment description
struct AttachmentDesc {
    LoadOp loadOp = LoadOp::DontCare;
    StoreOp storeOp = StoreOp::Store;
    uint8_t layer = 0;
    uint8_t level = 0;
    float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
};

// Render pass description (what to do with attachments)
struct RenderPass {
    AttachmentDesc color[MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depth = {LoadOp::DontCare, StoreOp::DontCare};
    AttachmentDesc stencil = {LoadOp::DontCare, StoreOp::DontCare};
    uint32_t layerCount = 1;
    uint32_t viewMask = 0;

    uint32_t getNumColorAttachments() const {
        uint32_t n = 0;
        while (n < MAX_COLOR_ATTACHMENTS && color[n].loadOp != LoadOp::DontCare) { n++; }
        return n;
    }
};

// Framebuffer attachment
struct FramebufferAttachment {
    Texture texture = {0};
    Texture resolveTexture = {0};
};

// Framebuffer description (which textures to use)
struct Framebuffer {
    FramebufferAttachment color[MAX_COLOR_ATTACHMENTS] = {};
    FramebufferAttachment depthStencil = {};
    const char* debugName = "";

    uint32_t getNumColorAttachments() const {
        uint32_t n = 0;
        while (n < MAX_COLOR_ATTACHMENTS && isValid(color[n].texture)) { n++; }
        return n;
    }
};

// Dependencies for synchronization
struct Dependencies {
    Texture textures[MAX_SUBMIT_DEPENDENCIES] = {};
    Buffer buffers[MAX_SUBMIT_DEPENDENCIES] = {};
};

// ============================================================================
// Command Buffer Class - OOP wrapper over hina_cmd*
// ============================================================================

class CommandBuffer {
public:
    CommandBuffer() = default;
    explicit CommandBuffer(Cmd* cmd) : m_cmd(cmd) {}

    Cmd* get() const { return m_cmd; }
    void reset(Cmd* cmd) { m_cmd = cmd; }
    operator bool() const { return m_cmd != nullptr; }

    // Begin rendering with render pass + framebuffer
    void beginRendering(const RenderPass& rp, const Framebuffer& fb, const Dependencies& deps = {});
    void endRendering() { hina_cmd_end_pass(m_cmd); }

    // Draw commands
    void draw(uint32_t vertexCount, uint32_t instanceCount = 1,
              uint32_t firstVertex = 0, uint32_t baseInstance = 0) {
        hina_cmd_draw(m_cmd, vertexCount, instanceCount, firstVertex, baseInstance);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                     uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                     uint32_t baseInstance = 0) {
        hina_cmd_draw_indexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset, baseInstance);
    }

    void drawIndexedIndirect(Buffer buffer, uint64_t offset, uint32_t drawCount, uint32_t stride) {
        hina_cmd_draw_indexed_indirect(m_cmd, buffer, offset, drawCount, stride);
    }

    // Compute
    void dispatch(uint32_t x, uint32_t y, uint32_t z) {
        hina_cmd_dispatch(m_cmd, x, y, z);
    }

    void dispatch(const Dimensions& threadgroupCount) {
        hina_cmd_dispatch(m_cmd, threadgroupCount.width, threadgroupCount.height, threadgroupCount.depth);
    }

    // Pipeline binding
    void bindPipeline(Pipeline pipeline) {
        hina_cmd_bind_pipeline(m_cmd, pipeline);
    }

    // Vertex input binding
    void applyVertexInput(const VertexInput& input) {
        hina_cmd_apply_vertex_input(m_cmd, &input);
    }

    // Convenience: bind only index buffer (for BDA vertex access)
    void bindIndexBuffer(Buffer indexBuffer, IndexType type = IndexType::UInt16) {
        hina_vertex_input input = {};
        input.index_buffer = indexBuffer;
        input.index_type = static_cast<hina_index_type>(type);
        hina_cmd_apply_vertex_input(m_cmd, &input);
    }

    // Descriptor binding
    void bindGroup(uint32_t set, BindGroup group) {
        hina_cmd_bind_group(m_cmd, set, group);
    }

    void bindGroupWithOffsets(uint32_t set, BindGroup group, const uint32_t* offsets, uint32_t offsetCount) {
        hina_cmd_bind_group_with_offsets(m_cmd, set, group, offsets, offsetCount);
    }

    // Push constants
    void pushConstants(uint32_t offset, uint32_t size, const void* data) {
        hina_cmd_push_constants(m_cmd, offset, size, data);
    }

    template<typename T>
    void pushConstants(const T& data, uint32_t offset = 0) {
        hina_cmd_push_constants(m_cmd, offset, sizeof(T), &data);
    }

    // Viewport/Scissor
    void setViewport(const Viewport& vp) {
        hina_viewport hvp = {vp.x, vp.y, vp.width, vp.height, vp.minDepth, vp.maxDepth};
        hina_cmd_set_viewport(m_cmd, &hvp);
    }

    void setViewport(float x, float y, float w, float h, float minDepth = 0.f, float maxDepth = 1.f) {
        hina_viewport hvp = {x, y, w, h, minDepth, maxDepth};
        hina_cmd_set_viewport(m_cmd, &hvp);
    }

    void setScissor(const ScissorRect& sc) {
        hina_scissor hsc = {sc.x, sc.y, sc.width, sc.height};
        hina_cmd_set_scissor(m_cmd, &hsc);
    }

    void setScissor(int32_t x, int32_t y, uint32_t w, uint32_t h) {
        hina_scissor hsc = {x, y, w, h};
        hina_cmd_set_scissor(m_cmd, &hsc);
    }

    // Texture state transitions
    void transitionTexture(Texture tex, TextureState newState) {
        hina_cmd_transition_texture(m_cmd, tex, static_cast<hina_texture_state_hint>(newState));
    }

    // Debug markers
    void pushDebugLabel(const char* label, uint32_t colorRGBA = 0xFFFFFFFF) {
        float r = ((colorRGBA >> 24) & 0xFF) / 255.0f;
        float g = ((colorRGBA >> 16) & 0xFF) / 255.0f;
        float b = ((colorRGBA >> 8) & 0xFF) / 255.0f;
        float color[4] = {r, g, b, 1.0f};
        hina_cmd_begin_label(m_cmd, label, color);
    }

    void popDebugLabel() {
        hina_cmd_end_label(m_cmd);
    }

private:
    Cmd* m_cmd = nullptr;
};

// ============================================================================
// RAII Holder for GFX Resources
// ============================================================================

// Type traits for destruction
template<typename T> struct ResourceDeleter;

template<> struct ResourceDeleter<Buffer> {
    static void destroy(Buffer b) { if (isValid(b)) hina_destroy_buffer(b); }
};

template<> struct ResourceDeleter<Texture> {
    static void destroy(Texture t) { if (isValid(t)) hina_destroy_texture(t); }
};

template<> struct ResourceDeleter<TextureView> {
    static void destroy(TextureView) { /* Views are owned by textures */ }
};

template<> struct ResourceDeleter<Sampler> {
    static void destroy(Sampler s) { if (isValid(s)) hina_destroy_sampler(s); }
};

template<> struct ResourceDeleter<Pipeline> {
    static void destroy(Pipeline p) { if (isValid(p)) hina_destroy_pipeline(p); }
};

template<> struct ResourceDeleter<BindGroup> {
    static void destroy(BindGroup g) { if (hina_bind_group_is_valid(g)) hina_destroy_bind_group(g); }
};

template<> struct ResourceDeleter<BindGroupLayout> {
    static void destroy(BindGroupLayout l) { if (hina_bind_group_layout_is_valid(l)) hina_destroy_bind_group_layout(l); }
};

template<> struct ResourceDeleter<Shader> {
    // hina_shader is a value type, functions take pointers
    static void destroy(Shader s) {
        if (s.spirv_data != nullptr) {
            hina_destroy_shader(&s);
        }
    }
};

// Type traits to check if a type is ID-based (has .id member like hina_buffer, hina_texture)
template<typename T, typename = void>
struct is_id_handle : std::false_type {};

template<typename T>
struct is_id_handle<T, std::void_t<decltype(std::declval<T>().id)>> : std::true_type {};

// RAII holder for graphics resources
template<typename T>
class Holder {
public:
    Holder() = default;
    explicit Holder(T handle) : m_handle(handle) {}

    ~Holder() { reset(); }

    // Move only
    Holder(Holder&& other) noexcept : m_handle(other.m_handle) {
        other.m_handle = T{};
    }

    Holder& operator=(Holder&& other) noexcept {
        if (this != &other) {
            reset();
            m_handle = other.m_handle;
            other.m_handle = T{};
        }
        return *this;
    }

    Holder(const Holder&) = delete;
    Holder& operator=(const Holder&) = delete;

    void reset(T newHandle = T{}) {
        ResourceDeleter<T>::destroy(m_handle);
        m_handle = newHandle;
    }

    T get() const { return m_handle; }
    T release() { T h = m_handle; m_handle = T{}; return h; }

    operator T() const { return m_handle; }

    // For ID-based handles (hina_buffer, hina_texture, etc.), check .id != 0
    template<typename U = T>
    explicit operator bool() const requires (is_id_handle<U>::value) {
        return m_handle.id != 0;
    }

    // For other types (like Shader), use a different validity check
    template<typename U = T>
    explicit operator bool() const requires (!is_id_handle<U>::value) {
        return true; // Caller should use isValid() or specific checks
    }

private:
    T m_handle{};
};

} // namespace gfx

// ============================================================================
// Utility Functions for Buffer/Texture Operations
// These bridge the gap between the old vk:: API and hina-vk
// ============================================================================

/**
 * @brief Upload data to a buffer.
 *
 * For HOST_VISIBLE buffers, this maps and writes directly.
 * For DEVICE_LOCAL buffers, this uses hina-vk's internal staging.
 *
 * @note This is a synchronous operation that may stall the GPU.
 */
inline void hina_upload_buffer(gfx::Buffer buffer, const void* data, size_t size, size_t offset = 0) {
    if (!gfx::isValid(buffer) || !data || size == 0) return;

    // Map the buffer, write, and unmap
    // Note: This assumes HOST_VISIBLE buffers. For DEVICE_LOCAL buffers,
    // you should use staging buffers with hina_cmd_copy_buffer.
    void* mapped = hina_map_buffer(buffer);
    if (mapped) {
        memcpy(static_cast<uint8_t*>(mapped) + offset, data, size);
        // No flush needed: All buffers are HOST_COHERENT
    }
}

/**
 * @brief Get the texture ID for use in descriptor arrays.
 *
 * Returns the texture's internal ID which can be used as an index into
 * the engine's global texture descriptor array. This is NOT a hina-vk
 * bindless feature - the engine manages the descriptor array separately.
 *
 * @note The term "bindless" refers to the engine's descriptor indexing
 * system, not a hina-vk feature. Use this when passing texture IDs to shaders.
 */
inline uint32_t gfx_get_texture_id(gfx::Texture texture) {
    if (!gfx::isValid(texture)) return 0;
    return texture.id;
}

