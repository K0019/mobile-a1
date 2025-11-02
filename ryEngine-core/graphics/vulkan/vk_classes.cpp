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

#include <algorithm>
#include <cstring>
#include <vector>

#define VMA_IMPLEMENTATION
#define VOLK_IMPLEMENTATION

#include "vk_util.h"
#include "vk_classes.h"
#include <glslang/Include/glslang_c_interface.h>
#include <SPIRV-Reflect/spirv_reflect.h>
#ifndef VK_USE_PLATFORM_WIN32_KHR
#include <unistd.h>
#endif
// clang-format off

#if defined(TRACY_ENABLE)
#include "tracy/TracyVulkan.hpp"
#define PROFILER_GPU_ZONE(name, ctx, cmdBuffer, color) TracyVkZoneC(ctx->pimpl_->tracyVkCtx_, cmdBuffer, name, color);
#else
#define PROFILER_GPU_ZONE(name, ctx, cmdBuffer, color)
#endif // TRACY_ENABLE
// clang-format on

#if !defined(__APPLE__)
#include <malloc.h>
#endif

uint32_t vk::VulkanPipelineBuilder::numPipelinesCreated_ = 0;

static_assert(vk::HWDeviceDesc::MAX_PHYSICAL_DEVICE_NAME_SIZE == VK_MAX_PHYSICAL_DEVICE_NAME_SIZE);
static_assert(vk::Swizzle_Default == (uint32_t)VK_COMPONENT_SWIZZLE_IDENTITY);
static_assert(vk::Swizzle_0 == (uint32_t)VK_COMPONENT_SWIZZLE_ZERO);
static_assert(vk::Swizzle_1 == (uint32_t)VK_COMPONENT_SWIZZLE_ONE);
static_assert(vk::Swizzle_R == (uint32_t)VK_COMPONENT_SWIZZLE_R);
static_assert(vk::Swizzle_G == (uint32_t)VK_COMPONENT_SWIZZLE_G);
static_assert(vk::Swizzle_B == (uint32_t)VK_COMPONENT_SWIZZLE_B);
static_assert(vk::Swizzle_A == (uint32_t)VK_COMPONENT_SWIZZLE_A);
static_assert(sizeof(vk::AccelStructInstance) == sizeof(VkAccelerationStructureInstanceKHR));
static_assert(sizeof(vk::mat3x4) == sizeof(VkTransformMatrixKHR));
static_assert(sizeof(vk::ClearColorValue) == sizeof(VkClearColorValue));

namespace
{
  const char* kDefaultValidationLayers[] = {"VK_LAYER_KHRONOS_validation"};

  // These bindings should match GLSL declarations injected into shaders in VulkanContext::createShaderModule().
  enum Bindings
  {
    kBinding_Textures               = 0,
    kBinding_Samplers               = 1,
    kBinding_StorageImages          = 2,
    kBinding_YUVImages              = 3,
    kBinding_AccelerationStructures = 4,
    kBinding_NumBindings            = 5,
  };

  uint32_t getAlignedSize(uint32_t value, uint32_t alignment)
  {
    return (value + alignment - 1) & ~(alignment - 1);
  }

  VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT msgSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT msgType, const VkDebugUtilsMessengerCallbackDataEXT* cbData, void* userData)
  {
    //if (msgSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    //	return VK_FALSE;
    //}

    if (!cbData || !cbData->pMessage)
    {
      return VK_FALSE;
    }

    const bool isError = (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0;

    const size_t len = cbData->pMessage ? strlen(cbData->pMessage) : 128u;

    ASSERT(len < 65536);
    std::vector<char> errorNameBuffer(len + 1);
    char* errorName = errorNameBuffer.data();
    int object = 0;
    void* handle = nullptr;
    void* messageID = nullptr;

    EngineLogLevel level = Info;
    if (isError)
    {
      const vk::VulkanContext* ctx = static_cast<vk::VulkanContext*>(userData);
      level = ctx->config_.terminateOnValidationError ? Critical : Warning;
    }
    else if (msgSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
      level = Warning;
    }

    //if (!isError && !isWarning && cbData->pMessageIdName) {
    //	if (strcmp(cbData->pMessageIdName, "Loader Message") == 0) {
    //		return VK_FALSE;
    //	}
    //}

    if (char typeName[128] = {}; sscanf(cbData->pMessage, "Validation Error: [ %[^]] ] Object %i: handle = %p, type = %127s | MessageID = %p", errorName, &object, &handle, typeName, &messageID) >= 2)
    {
      const char* message = strrchr(cbData->pMessage, '|') + 1;

      LOG_DYNAMIC(level, "{}Validation layer:\n Validation Error: {} \n Object {}: handle = {:#x}, type = {}\n " "MessageID = {:#x} \n{} \n", isError ? "\nERROR:\n" : "", errorName, object, reinterpret_cast<uintptr_t>(handle), typeName, reinterpret_cast<uintptr_t>(messageID), message);
    }
    else
    {
      LOG_DYNAMIC(level, "{}Validation layer: {}", isError ? "\nERROR:\n" : "", cbData->pMessage);
    }

    if (isError)
    {
      vk::VulkanContext* ctx = static_cast<vk::VulkanContext*>(userData);

      if (ctx->config_.shaderModuleErrorCallback != nullptr)
      {
        // retrieve source code references - this is very experimental and depends a lot on the validation layer output
        int line = 0;
        int col = 0;
        const char* substr1 = strstr(cbData->pMessage, "Shader validation error occurred at line ");
        if (substr1 && sscanf(substr1, "Shader validation error occurred at line %d, column %d.", &line, &col) >= 1)
        {
          const char* substr2 = strstr(cbData->pMessage, "Shader Module (Shader Module: ");
          std::vector<char> shaderModuleDebugBuffer(len + 1);
          char* shaderModuleDebugName = shaderModuleDebugBuffer.data();
          VkShaderModule shaderModule = VK_NULL_HANDLE;
#if VK_USE_64_BIT_PTR_DEFINES
          if (substr2 && sscanf(substr2, "Shader Module (Shader Module: %[^)])(%p)", shaderModuleDebugName, &shaderModule) == 2)
          {
#else
					if(substr2 && sscanf(substr2, "Shader Module (Shader Module: %[^)])(%llu)", shaderModuleDebugName, &shaderModule) == 2) {
#endif // VK_USE_64_BIT_PTR_DEFINES
            ctx->invokeShaderModuleErrorCallback(line, col, shaderModuleDebugName, shaderModule);
          }
        }
      }

      if (ctx->config_.terminateOnValidationError)
      {
        ASSERT(false);
        std::terminate();
      }
    }

    return VK_FALSE;
  }

  VkIndexType indexFormatToVkIndexType(vk::IndexFormat fmt)
  {
    switch (fmt)
    {
      case vk::IndexFormat::UI8:
        return VK_INDEX_TYPE_UINT8_EXT;
      case vk::IndexFormat::UI16:
        return VK_INDEX_TYPE_UINT16;
      case vk::IndexFormat::UI32:
        return VK_INDEX_TYPE_UINT32;
    };
    ASSERT(false);
    return VK_INDEX_TYPE_NONE_KHR;
  }

  VkPrimitiveTopology topologyToVkPrimitiveTopology(vk::Topology t)
  {
    switch (t)
    {
      case vk::Topology::Point:
        return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
      case vk::Topology::Line:
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
      case vk::Topology::LineStrip:
        return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
      case vk::Topology::Triangle:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
      case vk::Topology::TriangleStrip:
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
      case vk::Topology::Patch:
        return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
    }
    ASSERT_MSG(false, "Implement Topology = %u", static_cast<uint32_t>(t));
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  }

  VkAttachmentLoadOp loadOpToVkAttachmentLoadOp(vk::LoadOp a)
  {
    switch (a)
    {
      case vk::LoadOp::Invalid: ASSERT(false);
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      case vk::LoadOp::DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      case vk::LoadOp::Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
      case vk::LoadOp::Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
      case vk::LoadOp::None:
        return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
    }
    ASSERT(false);
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }

  VkAttachmentStoreOp storeOpToVkAttachmentStoreOp(vk::StoreOp a)
  {
    switch (a)
    {
      case vk::StoreOp::DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
      case vk::StoreOp::Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
      case vk::StoreOp::MsaaResolve:
        // for MSAA resolve, we have to store data into a special "resolve" attachment
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
      case vk::StoreOp::None:
        return VK_ATTACHMENT_STORE_OP_NONE;
    }
    ASSERT(false);
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  }

  VkShaderStageFlagBits shaderStageToVkShaderStage(vk::ShaderStage stage)
  {
    switch (stage)
    {
      case vk::ShaderStage::Vert:
        return VK_SHADER_STAGE_VERTEX_BIT;
      case vk::ShaderStage::Tesc:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
      case vk::ShaderStage::Tese:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
      case vk::ShaderStage::Geom:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
      case vk::ShaderStage::Frag:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
      case vk::ShaderStage::Comp:
        return VK_SHADER_STAGE_COMPUTE_BIT;
      case vk::ShaderStage::Task:
        return VK_SHADER_STAGE_TASK_BIT_EXT;
      case vk::ShaderStage::Mesh:
        return VK_SHADER_STAGE_MESH_BIT_EXT;
      case vk::ShaderStage::RayGen:
        return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
      case vk::ShaderStage::AnyHit:
        return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
      case vk::ShaderStage::ClosestHit:
        return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
      case vk::ShaderStage::Miss:
        return VK_SHADER_STAGE_MISS_BIT_KHR;
      case vk::ShaderStage::Intersection:
        return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
      case vk::ShaderStage::Callable:
        return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    };
    ASSERT(false);
    return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
  }

  VkMemoryPropertyFlags storageTypeToVkMemoryPropertyFlags(vk::StorageType storage)
  {
    VkMemoryPropertyFlags memFlags{0};

    switch (storage)
    {
      case vk::StorageType::Device:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
      case vk::StorageType::HostVisible:
        memFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
      case vk::StorageType::Memoryless:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        break;
    }
    return memFlags;
  }

  VkBuildAccelerationStructureFlagsKHR buildFlagsToVkBuildAccelerationStructureFlags(uint8_t buildFlags)
  {
    VkBuildAccelerationStructureFlagsKHR flags = 0;

    if (buildFlags & vk::AccelStructBuildFlagBits_AllowUpdate)
    {
      flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    }
    if (buildFlags & vk::AccelStructBuildFlagBits_AllowCompaction)
    {
      flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }
    if (buildFlags & vk::AccelStructBuildFlagBits_PreferFastTrace)
    {
      flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    }
    if (buildFlags & vk::AccelStructBuildFlagBits_PreferFastBuild)
    {
      flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    }
    if (buildFlags & vk::AccelStructBuildFlagBits_LowMemory)
    {
      flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
    }

    return flags;
  }

  VkPolygonMode polygonModeToVkPolygonMode(vk::PolygonMode mode)
  {
    switch (mode)
    {
      case vk::PolygonMode::Fill:
        return VK_POLYGON_MODE_FILL;
      case vk::PolygonMode::Line:
        return VK_POLYGON_MODE_LINE;
    }
    ASSERT_MSG(false, "Implement a missing polygon fill mode");
    return VK_POLYGON_MODE_FILL;
  }

  VkBlendFactor blendFactorToVkBlendFactor(vk::BlendFactor value)
  {
    switch (value)
    {
      case vk::BlendFactor::Zero:
        return VK_BLEND_FACTOR_ZERO;
      case vk::BlendFactor::One:
        return VK_BLEND_FACTOR_ONE;
      case vk::BlendFactor::SrcColor:
        return VK_BLEND_FACTOR_SRC_COLOR;
      case vk::BlendFactor::OneMinusSrcColor:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
      case vk::BlendFactor::DstColor:
        return VK_BLEND_FACTOR_DST_COLOR;
      case vk::BlendFactor::OneMinusDstColor:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
      case vk::BlendFactor::SrcAlpha:
        return VK_BLEND_FACTOR_SRC_ALPHA;
      case vk::BlendFactor::OneMinusSrcAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
      case vk::BlendFactor::DstAlpha:
        return VK_BLEND_FACTOR_DST_ALPHA;
      case vk::BlendFactor::OneMinusDstAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
      case vk::BlendFactor::BlendColor:
        return VK_BLEND_FACTOR_CONSTANT_COLOR;
      case vk::BlendFactor::OneMinusBlendColor:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
      case vk::BlendFactor::BlendAlpha:
        return VK_BLEND_FACTOR_CONSTANT_ALPHA;
      case vk::BlendFactor::OneMinusBlendAlpha:
        return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
      case vk::BlendFactor::SrcAlphaSaturated:
        return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
      case vk::BlendFactor::Src1Color:
        return VK_BLEND_FACTOR_SRC1_COLOR;
      case vk::BlendFactor::OneMinusSrc1Color:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
      case vk::BlendFactor::Src1Alpha:
        return VK_BLEND_FACTOR_SRC1_ALPHA;
      case vk::BlendFactor::OneMinusSrc1Alpha:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
      default: ASSERT(false);
        return VK_BLEND_FACTOR_ONE; // default for unsupported values
    }
  }

  VkBlendOp blendOpToVkBlendOp(vk::BlendOp value)
  {
    switch (value)
    {
      case vk::BlendOp::Add:
        return VK_BLEND_OP_ADD;
      case vk::BlendOp::Subtract:
        return VK_BLEND_OP_SUBTRACT;
      case vk::BlendOp::ReverseSubtract:
        return VK_BLEND_OP_REVERSE_SUBTRACT;
      case vk::BlendOp::Min:
        return VK_BLEND_OP_MIN;
      case vk::BlendOp::Max:
        return VK_BLEND_OP_MAX;
    }

    ASSERT(false);
    return VK_BLEND_OP_ADD;
  }

  VkCullModeFlags cullModeToVkCullMode(vk::CullMode mode)
  {
    switch (mode)
    {
      case vk::CullMode::None:
        return VK_CULL_MODE_NONE;
      case vk::CullMode::Front:
        return VK_CULL_MODE_FRONT_BIT;
      case vk::CullMode::Back:
        return VK_CULL_MODE_BACK_BIT;
    }
    ASSERT_MSG(false, "Implement a missing cull mode");
    return VK_CULL_MODE_NONE;
  }

  VkFrontFace windingModeToVkFrontFace(vk::WindingMode mode)
  {
    switch (mode)
    {
      case vk::WindingMode::CCW:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
      case vk::WindingMode::CW:
        return VK_FRONT_FACE_CLOCKWISE;
    }
    ASSERT_MSG(false, "Wrong winding order (cannot be more than 2)");
    return VK_FRONT_FACE_CLOCKWISE;
  }

  VkStencilOp stencilOpToVkStencilOp(vk::StencilOp op)
  {
    switch (op)
    {
      case vk::StencilOp::Keep:
        return VK_STENCIL_OP_KEEP;
      case vk::StencilOp::Zero:
        return VK_STENCIL_OP_ZERO;
      case vk::StencilOp::Replace:
        return VK_STENCIL_OP_REPLACE;
      case vk::StencilOp::IncrementClamp:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
      case vk::StencilOp::DecrementClamp:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
      case vk::StencilOp::Invert:
        return VK_STENCIL_OP_INVERT;
      case vk::StencilOp::IncrementWrap:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
      case vk::StencilOp::DecrementWrap:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    }
    ASSERT(false);
    return VK_STENCIL_OP_KEEP;
  }

  VkFormat vertexFormatToVkFormat(vk::VertexFormat fmt)
  {
    using vk::VertexFormat;
    switch (fmt)
    {
      case VertexFormat::Invalid: ASSERT(false);
        return VK_FORMAT_UNDEFINED;
      case VertexFormat::Float1:
        return VK_FORMAT_R32_SFLOAT;
      case VertexFormat::Float2:
        return VK_FORMAT_R32G32_SFLOAT;
      case VertexFormat::Float3:
        return VK_FORMAT_R32G32B32_SFLOAT;
      case VertexFormat::Float4:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
      case VertexFormat::Byte1:
        return VK_FORMAT_R8_SINT;
      case VertexFormat::Byte2:
        return VK_FORMAT_R8G8_SINT;
      case VertexFormat::Byte3:
        return VK_FORMAT_R8G8B8_SINT;
      case VertexFormat::Byte4:
        return VK_FORMAT_R8G8B8A8_SINT;
      case VertexFormat::UByte1:
        return VK_FORMAT_R8_UINT;
      case VertexFormat::UByte2:
        return VK_FORMAT_R8G8_UINT;
      case VertexFormat::UByte3:
        return VK_FORMAT_R8G8B8_UINT;
      case VertexFormat::UByte4:
        return VK_FORMAT_R8G8B8A8_UINT;
      case VertexFormat::Short1:
        return VK_FORMAT_R16_SINT;
      case VertexFormat::Short2:
        return VK_FORMAT_R16G16_SINT;
      case VertexFormat::Short3:
        return VK_FORMAT_R16G16B16_SINT;
      case VertexFormat::Short4:
        return VK_FORMAT_R16G16B16A16_SINT;
      case VertexFormat::UShort1:
        return VK_FORMAT_R16_UINT;
      case VertexFormat::UShort2:
        return VK_FORMAT_R16G16_UINT;
      case VertexFormat::UShort3:
        return VK_FORMAT_R16G16B16_UINT;
      case VertexFormat::UShort4:
        return VK_FORMAT_R16G16B16A16_UINT;
      // Normalized variants
      case VertexFormat::Byte2Norm:
        return VK_FORMAT_R8G8_SNORM;
      case VertexFormat::Byte4Norm:
        return VK_FORMAT_R8G8B8A8_SNORM;
      case VertexFormat::UByte2Norm:
        return VK_FORMAT_R8G8_UNORM;
      case VertexFormat::UByte4Norm:
        return VK_FORMAT_R8G8B8A8_UNORM;
      case VertexFormat::Short2Norm:
        return VK_FORMAT_R16G16_SNORM;
      case VertexFormat::Short4Norm:
        return VK_FORMAT_R16G16B16A16_SNORM;
      case VertexFormat::UShort2Norm:
        return VK_FORMAT_R16G16_UNORM;
      case VertexFormat::UShort4Norm:
        return VK_FORMAT_R16G16B16A16_UNORM;
      case VertexFormat::Int1:
        return VK_FORMAT_R32_SINT;
      case VertexFormat::Int2:
        return VK_FORMAT_R32G32_SINT;
      case VertexFormat::Int3:
        return VK_FORMAT_R32G32B32_SINT;
      case VertexFormat::Int4:
        return VK_FORMAT_R32G32B32A32_SINT;
      case VertexFormat::UInt1:
        return VK_FORMAT_R32_UINT;
      case VertexFormat::UInt2:
        return VK_FORMAT_R32G32_UINT;
      case VertexFormat::UInt3:
        return VK_FORMAT_R32G32B32_UINT;
      case VertexFormat::UInt4:
        return VK_FORMAT_R32G32B32A32_UINT;
      case VertexFormat::HalfFloat1:
        return VK_FORMAT_R16_SFLOAT;
      case VertexFormat::HalfFloat2:
        return VK_FORMAT_R16G16_SFLOAT;
      case VertexFormat::HalfFloat3:
        return VK_FORMAT_R16G16B16_SFLOAT;
      case VertexFormat::HalfFloat4:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
      case VertexFormat::Int_2_10_10_10_REV:
        return VK_FORMAT_A2B10G10R10_SNORM_PACK32;
    }
    ASSERT(false);
    return VK_FORMAT_UNDEFINED;
  }

  bool supportsFormat(VkPhysicalDevice physicalDevice, VkFormat format)
  {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
    return properties.bufferFeatures != 0 || properties.linearTilingFeatures != 0 || properties.optimalTilingFeatures != 0;
  }

  std::vector<VkFormat> getCompatibleDepthStencilFormats(vk::Format format)
  {
    switch (format)
    {
      case vk::Format::Z_UN16:
        return {VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
      case vk::Format::Z_UN24:
        return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT};
      case vk::Format::Z_F32:
        return {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
      case vk::Format::Z_UN24_S_UI8:
        return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
      case vk::Format::Z_F32_S_UI8:
        return {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT};
      default: ;
    }
    return {VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
  }

  bool validateImageLimits(VkImageType imageType, VkSampleCountFlagBits samples, const VkExtent3D& extent, const VkPhysicalDeviceLimits& limits, vk::Result* outResult)
  {
    using vk::Result;

    if (samples != VK_SAMPLE_COUNT_1_BIT && !VERIFY(imageType == VK_IMAGE_TYPE_2D))
    {
      Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "Multisampling is supported only for 2D images"));
      return false;
    }

    if (imageType == VK_IMAGE_TYPE_2D && !VERIFY(extent.width <= limits.maxImageDimension2D && extent.height <= limits.maxImageDimension2D))
    {
      Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "2D texture size exceeded"));
      return false;
    }
    if (imageType == VK_IMAGE_TYPE_3D && !VERIFY(extent.width <= limits.maxImageDimension3D && extent.height <= limits.maxImageDimension3D && extent.depth <= limits.maxImageDimension3D))
    {
      Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "3D texture size exceeded"));
      return false;
    }

    return true;
  }

  vk::Result validateRange(const VkExtent3D& ext, uint32_t numLevels, const vk::TextureRangeDesc& range)
  {
    if (!VERIFY(range.dimensions.width > 0 && range.dimensions.height > 0 || range.dimensions.depth > 0 || range.numLayers > 0 || range.numMipLevels > 0))
    {
      return vk::Result{vk::Result::Code::ArgumentOutOfRange, "width, height, depth numLayers, and numMipLevels must be > 0"};
    }
    if (range.mipLevel > numLevels)
    {
      return vk::Result{vk::Result::Code::ArgumentOutOfRange, "range.mipLevel exceeds texture mip-levels"};
    }

    const uint32_t texWidth = (std::max)(ext.width >> range.mipLevel, 1u);
    const uint32_t texHeight = (std::max)(ext.height >> range.mipLevel, 1u);
    const uint32_t texDepth = (std::max)(ext.depth >> range.mipLevel, 1u);

    if (range.dimensions.width > texWidth || range.dimensions.height > texHeight || range.dimensions.depth > texDepth)
    {
      return vk::Result{vk::Result::Code::ArgumentOutOfRange, "range dimensions exceed texture dimensions"};
    }
    if (range.offset.x > static_cast<int32_t>(texWidth - range.dimensions.width)
        || range.offset.y > static_cast<int32_t>(texHeight - range.dimensions.height)
        || range.offset.z > static_cast<int32_t>(texDepth - range.dimensions.depth))
    {
      return vk::Result{vk::Result::Code::ArgumentOutOfRange, "range dimensions exceed texture dimensions"};
    }

    return vk::Result{};
  }

  bool isHostVisibleSingleHeapMemory(VkPhysicalDevice physDev)
  {
    VkPhysicalDeviceMemoryProperties memProperties;

    vkGetPhysicalDeviceMemoryProperties(physDev, &memProperties);

    if (memProperties.memoryHeapCount != 1)
    {
      return false;
    }

    const uint32_t flag = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
      if ((memProperties.memoryTypes[i].propertyFlags & flag) == flag)
      {
        return true;
      }
    }

    return false;
  }

  void getDeviceExtensionProps(VkPhysicalDevice dev, std::vector<VkExtensionProperties>& props, const char* validationLayer = nullptr)
  {
    uint32_t numExtensions = 0;
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> p(numExtensions);
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, p.data());
    props.insert(props.end(), p.begin(), p.end());
  }

  bool hasExtension(const char* ext, const std::vector<VkExtensionProperties>& props)
  {
    for (const VkExtensionProperties& p : props)
    {
      if (strcmp(ext, p.extensionName) == 0)
        return true;
    }
    return false;
  }

  void transitionToColorAttachment(VkCommandBuffer buffer, vk::VulkanImage* colorTex)
  {
    if (!VERIFY(colorTex))
    {
      return;
    }

    if (!VERIFY(!colorTex->isDepthFormat_ && !colorTex->isStencilFormat_))
    {
      ASSERT_MSG(false, "Color attachments cannot have depth/stencil formats");
      return;
    }
    ASSERT_MSG(colorTex->vkImageFormat_ != VK_FORMAT_UNDEFINED, "Invalid color attachment format");
    colorTex->transitionLayout(buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
  }

  bool isDepthOrStencilVkFormat(VkFormat format)
  {
    switch (format)
    {
      case VK_FORMAT_D16_UNORM:
      case VK_FORMAT_X8_D24_UNORM_PACK32:
      case VK_FORMAT_D32_SFLOAT:
      case VK_FORMAT_S8_UINT:
      case VK_FORMAT_D16_UNORM_S8_UINT:
      case VK_FORMAT_D24_UNORM_S8_UINT:
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return true;
      default:
        return false;
    }
    return false;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats, vk::ColorSpace requestedColorSpace, bool hasSwapchainColorspaceExt)
  {
    ASSERT(!formats.empty());

    auto isNativeSwapChainBGR = [](const std::vector<VkSurfaceFormatKHR>& formats) -> bool
    {
      for (const VkSurfaceFormatKHR& fmt : formats)
      {
        // The preferred format should be the one which is closer to the beginning of the formats
        // container. If BGR is encountered earlier, it should be picked as the format of choice. If RGB
        // happens to be earlier, take it.
        if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM || fmt.format == VK_FORMAT_R8G8B8A8_SRGB || fmt.format == VK_FORMAT_A2R10G10B10_UNORM_PACK32)
        {
          return false;
        }
        if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM || fmt.format == VK_FORMAT_B8G8R8A8_SRGB || fmt.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32)
        {
          return true;
        }
      }
      return false;
    };

    auto colorSpaceToVkSurfaceFormat = [](vk::ColorSpace colorSpace, bool isBGR, bool hasSwapchainColorspaceExt) -> VkSurfaceFormatKHR
    {
      switch (colorSpace)
      {
        case vk::ColorSpace::SRGB_LINEAR:
          // the closest thing to sRGB linear
          return VkSurfaceFormatKHR{isBGR ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_BT709_LINEAR_EXT};
        case vk::ColorSpace::SRGB_EXTENDED_LINEAR:
          if (hasSwapchainColorspaceExt)
            return VkSurfaceFormatKHR{VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT};
          [[fallthrough]];
        case vk::ColorSpace::HDR10:
          if (hasSwapchainColorspaceExt)
            return VkSurfaceFormatKHR{isBGR ? VK_FORMAT_A2B10G10R10_UNORM_PACK32 : VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT};
          [[fallthrough]];
        case vk::ColorSpace::SRGB_NONLINEAR: [[fallthrough]];
        default:
          // default to normal sRGB non linear.
          return VkSurfaceFormatKHR{isBGR ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
      }
    };

    const VkSurfaceFormatKHR preferred = colorSpaceToVkSurfaceFormat(requestedColorSpace, isNativeSwapChainBGR(formats), hasSwapchainColorspaceExt);

    for (const VkSurfaceFormatKHR& fmt : formats)
    {
      if (fmt.format == preferred.format && fmt.colorSpace == preferred.colorSpace)
      {
        return fmt;
      }
    }

    // if we can't find a matching format and color space, fallback on matching only format
    for (const VkSurfaceFormatKHR& fmt : formats)
    {
      if (fmt.format == preferred.format)
      {
        return fmt;
      }
    }

    LOG_INFO("Could not find a native swap chain format that matched our designed swapchain format. Defaulting to first supported format.") ;

    return formats[0];
  }

  VkDeviceSize bufferSize(vk::VulkanContext& ctx, const vk::Holder<vk::BufferHandle>& handle)
  {
    vk::VulkanBuffer* buffer = ctx.buffersPool_.get(handle);
    return buffer ? buffer->bufferSize_ : 0;
  }
} // namespace

namespace vk
{
  struct DeferredTask
  {
    std::packaged_task<void()> task_{};
    SubmitHandle handle_{};

    explicit DeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) : task_(std::move(task)), handle_(handle) {}
  };

  struct VulkanContextImpl final
  {
    // Vulkan Memory Allocator
    VmaAllocator vma_ = VK_NULL_HANDLE;

    CommandBuffer currentCommandBuffer_;

    std::vector<DeferredTask> deferredTasks_;

    struct YcbcrConversionData
    {
      VkSamplerYcbcrConversionInfo info;
      Holder<SamplerHandle> sampler;
    };

    YcbcrConversionData ycbcrConversionData_[256]; // indexed by vk::Format
    uint32_t numYcbcrSamplers_ = 0;

#if defined(TRACY_ENABLE)
    TracyVkCtx tracyVkCtx_ = nullptr;
    VkCommandPool tracyCommandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer tracyCommandBuffer_ = VK_NULL_HANDLE;
#endif // TRACY_ENABLE
  };
} // namespace vk

void vk::VulkanBuffer::flushMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size) const
{
  if (!VERIFY(isMapped()))
  {
    return;
  }
  vmaFlushAllocation((VmaAllocator)ctx.getVmaAllocator(), vmaAllocation_, offset, size);
}

void vk::VulkanBuffer::invalidateMappedMemory(const VulkanContext& ctx, VkDeviceSize offset, VkDeviceSize size) const
{
  if (!VERIFY(isMapped()))
  {
    return;
  }
  vmaInvalidateAllocation(static_cast<VmaAllocator>(ctx.getVmaAllocator()), vmaAllocation_, offset, size);
}

void vk::VulkanBuffer::getBufferSubData(const VulkanContext& ctx, size_t offset, size_t size, void* data)
{
  // only host-visible buffers can be downloaded this way
  ASSERT(mappedPtr_);
  if (!mappedPtr_)
  {
    return;
  }
  ASSERT(offset + size <= bufferSize_);
  if (!isCoherentMemory_)
  {
    invalidateMappedMemory(ctx, offset, size);
  }
  const uint8_t* src = static_cast<uint8_t*>(mappedPtr_) + offset;
  memcpy(data, src, size);
}

void vk::VulkanBuffer::bufferSubData(const VulkanContext& ctx, size_t offset, size_t size, const void* data)
{
  // only host-visible buffers can be uploaded this way
  ASSERT(mappedPtr_);

  if (!mappedPtr_)
  {
    return;
  }

  ASSERT(offset + size <= bufferSize_);

  if (data)
  {
    memcpy((uint8_t*)mappedPtr_ + offset, data, size);
  }
  else
  {
    memset((uint8_t*)mappedPtr_ + offset, 0, size);
  }

  if (!isCoherentMemory_)
  {
    flushMappedMemory(ctx, offset, size);
  }
}

VkImageView vk::VulkanImage::createImageView(VkDevice device, VkImageViewType type, VkFormat format, VkImageAspectFlags aspectMask, uint32_t baseLevel, uint32_t numLevels, uint32_t baseLayer, uint32_t numLayers, const VkComponentMapping mapping, const VkSamplerYcbcrConversionInfo* ycbcr, const char* debugName) const
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_CREATE);

  const VkImageViewCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .pNext = ycbcr, .image = vkImage_, .viewType = type, .format = format, .components = mapping, .subresourceRange = {aspectMask, baseLevel, numLevels ? numLevels : numLevels_, baseLayer, numLayers},};
  VkImageView vkView = VK_NULL_HANDLE;
  VK_ASSERT(vkCreateImageView(device, &ci, nullptr, &vkView));
  VK_ASSERT(vk::setDebugObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkView, debugName));

  return vkView;
}

void vk::VulkanImage::transitionLayout(VkCommandBuffer commandBuffer, VkImageLayout newImageLayout, const VkImageSubresourceRange& subresourceRange) const
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_BARRIER);

  const VkImageLayout oldImageLayout = vkImageLayout_ == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL ? (isDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) : vkImageLayout_;

  if (newImageLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
  {
    newImageLayout = isDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }

  StageAccess src = getPipelineStageAccess(oldImageLayout);
  StageAccess dst = getPipelineStageAccess(newImageLayout);

  if (isDepthAttachment() && isResolveAttachment)
  {
    // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#renderpass-resolve-operations
    src.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    dst.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    src.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    dst.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  }

  const VkImageMemoryBarrier2 barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .srcStageMask = src.stage, .srcAccessMask = src.access, .dstStageMask = dst.stage, .dstAccessMask = dst.access, .oldLayout = vkImageLayout_, .newLayout = newImageLayout, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = vkImage_, .subresourceRange = subresourceRange,};

  const VkDependencyInfo depInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier,};

  vkCmdPipelineBarrier2(commandBuffer, &depInfo);

  vkImageLayout_ = newImageLayout;
}

VkImageAspectFlags vk::VulkanImage::getImageAspectFlags() const
{
  VkImageAspectFlags flags = 0;

  flags |= isDepthFormat_ ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
  flags |= isStencilFormat_ ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
  flags |= !(isDepthFormat_ || isStencilFormat_) ? VK_IMAGE_ASPECT_COLOR_BIT : 0;

  return flags;
}

void vk::VulkanImage::generateMipmap(VkCommandBuffer commandBuffer) const
{
  PROFILER_FUNCTION();

  // Check if device supports downscaling for color or depth/stencil buffer based on image format
  {
    const uint32_t formatFeatureMask = (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT);

    const bool hardwareDownscalingSupported = (vkFormatProperties_.optimalTilingFeatures & formatFeatureMask) == formatFeatureMask;

    if (!hardwareDownscalingSupported)
    {
      LOG_INFO("Doesn't support hardware downscaling of this image format: {}", string_VkFormat(vkImageFormat_));
      return;
    }
  }

  // Choose linear filter for color formats if supported by the device, else use nearest filter
  // Choose nearest filter by default for depth/stencil formats
  const VkFilter blitFilter = [](bool isDepthOrStencilFormat, bool imageFilterLinear)
  {
    if (isDepthOrStencilFormat)
    {
      return VK_FILTER_NEAREST;
    }
    if (imageFilterLinear)
    {
      return VK_FILTER_LINEAR;
    }
    return VK_FILTER_NEAREST;
  }(isDepthFormat_ || isStencilFormat_, vkFormatProperties_.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

  const VkImageAspectFlags imageAspectFlags = getImageAspectFlags();

  if (vkCmdBeginDebugUtilsLabelEXT)
  {
    const VkDebugUtilsLabelEXT utilsLabel = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, .pLabelName = "Generate mipmaps", .color = {1.0f, 0.75f, 1.0f, 1.0f},};
    vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &utilsLabel);
  }

  const VkImageLayout originalImageLayout = vkImageLayout_;

  ASSERT(originalImageLayout != VK_IMAGE_LAYOUT_UNDEFINED);

  // 0: Transition the first level and all layers into VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
  transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VkImageSubresourceRange{imageAspectFlags, 0, 1, 0, numLayers_});

  for (uint32_t layer = 0; layer < numLayers_; ++layer)
  {
    int32_t mipWidth = static_cast<int32_t>(vkExtent_.width);
    int32_t mipHeight = static_cast<int32_t>(vkExtent_.height);

    for (uint32_t i = 1; i < numLevels_; ++i)
    {
      // 1: Transition the i-th level to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; it will be copied into from the (i-1)-th layer
      imageMemoryBarrier2(commandBuffer,
                          vkImage_,
                          StageAccess{.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_NONE},
                          StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT},
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          // oldImageLayout
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          // newImageLayout
                          VkImageSubresourceRange{imageAspectFlags, i, 1, layer, 1});

      const int32_t nextLevelWidth = mipWidth > 1 ? mipWidth / 2 : 1;
      const int32_t nextLevelHeight = mipHeight > 1 ? mipHeight / 2 : 1;

      const VkOffset3D srcOffsets[2] = {VkOffset3D{0, 0, 0}, VkOffset3D{mipWidth, mipHeight, 1},};
      const VkOffset3D dstOffsets[2] = {VkOffset3D{0, 0, 0}, VkOffset3D{nextLevelWidth, nextLevelHeight, 1},};

      // 2: Blit the image from the prev mip-level (i-1) (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) to the current mip-level (i)
      // (VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
#if VULKAN_PRINT_COMMANDS
			LOG_INFO("{} vkCmdBlitImage()\n", reinterpret_cast<uintptr_t>(commandBuffer));
#endif // VULKAN_PRINT_COMMANDS
      const VkImageBlit blit = {.srcSubresource = VkImageSubresourceLayers{imageAspectFlags, i - 1, layer, 1}, .srcOffsets = {srcOffsets[0], srcOffsets[1]}, .dstSubresource = VkImageSubresourceLayers{imageAspectFlags, i, layer, 1}, .dstOffsets = {dstOffsets[0], dstOffsets[1]},};
      vkCmdBlitImage(commandBuffer, vkImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, blitFilter);
      // 3: Transition i-th level to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL as it will be read from in the next iteration
      imageMemoryBarrier2(commandBuffer,
                          vkImage_,
                          StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT},
                          StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT},
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          /* oldImageLayout */
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                          /* newImageLayout */
                          VkImageSubresourceRange{imageAspectFlags, i, 1, layer, 1});

      // Compute the size of the next mip-level
      mipWidth = nextLevelWidth;
      mipHeight = nextLevelHeight;
    }
  }

  // 4: Transition all levels and layers (faces) to their final layout
  imageMemoryBarrier2(commandBuffer,
                      vkImage_,
                      StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT},
                      StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT},
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      // oldImageLayout
                      originalImageLayout,
                      // newImageLayout
                      VkImageSubresourceRange{imageAspectFlags, 0, numLevels_, 0, numLayers_});

  if (vkCmdEndDebugUtilsLabelEXT)
  {
    vkCmdEndDebugUtilsLabelEXT(commandBuffer);
  }

  vkImageLayout_ = originalImageLayout;
}

bool vk::VulkanImage::isDepthFormat(VkFormat format)
{
  return (format == VK_FORMAT_D16_UNORM) || (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

bool vk::VulkanImage::isStencilFormat(VkFormat format)
{
  return (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

VkImageView vk::VulkanImage::getOrCreateVkImageViewForFramebuffer(VulkanContext& ctx, uint8_t level, uint16_t layer)
{
  ASSERT(level < MAX_MIP_LEVELS);
  ASSERT(layer < std::size(imageViewForFramebuffer_[0]));

  if (level >= MAX_MIP_LEVELS || layer >= std::size(imageViewForFramebuffer_[0]))
  {
    return VK_NULL_HANDLE;
  }

  if (imageViewForFramebuffer_[level][layer] != VK_NULL_HANDLE)
  {
    return imageViewForFramebuffer_[level][layer];
  }

  char debugNameImageView[320] = {0};
  snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: '%s' imageViewForFramebuffer_[%u][%u]", debugName_, level, layer);

  imageViewForFramebuffer_[level][layer] = createImageView(ctx.getVkDevice(), VK_IMAGE_VIEW_TYPE_2D, vkImageFormat_, getImageAspectFlags(), level, 1u, layer, 1u, {}, nullptr, debugNameImageView);

  return imageViewForFramebuffer_[level][layer];
}

vk::VulkanSwapchain::~VulkanSwapchain()
{
  deinit();
}

vk::Result vk::VulkanSwapchain::init(const InitInfo& info)
{
  PROFILER_FUNCTION();

  ctx_ = &info.ctx;
  device_ = ctx_->getVkDevice();
  physicalDevice_ = ctx_->getVkPhysicalDevice();
  surface_ = ctx_->vkSurface_;
  graphicsQueue_ = ctx_->deviceQueues_.graphicsQueue;
  queueFamilyIndex_ = ctx_->deviceQueues_.graphicsQueueFamilyIndex;

  // Verify queue family supports presentation
  VkBool32 supportsPresent = VK_FALSE;
  VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, queueFamilyIndex_, surface_, &supportsPresent);

  if (result != VK_SUCCESS || supportsPresent != VK_TRUE)
  {
    LOG_WARNING("Queue family {} does not support presentation", queueFamilyIndex_);
    return Result(Result::Code::RuntimeError, "Queue family does not support presentation");
  }

#if defined(__ANDROID__)
  // Capture identity (natural) resolution at startup
  VkSurfaceCapabilitiesKHR caps;
  result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);
  if (result == VK_SUCCESS)
  {
      identityExtent_ = caps.currentExtent;
      preTransform_ = caps.currentTransform;

      // If device starts in 90° or 270° rotation, swap dimensions to get natural resolution
      if (caps.currentTransform & (VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR | VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR))
      {
          std::swap(identityExtent_.width, identityExtent_.height);
      }
  }
#endif

  // Initialize with the requested size
  uint32_t width = info.width;
  uint32_t height = info.height;
  return initResources(width, height, info.vSync);
}

void vk::VulkanSwapchain::deinit()
{
  if (device_ != VK_NULL_HANDLE)
  {
    deinitResources();
  }

  // Reset all state manually since we deleted move assignment
  ctx_ = nullptr;
  device_ = VK_NULL_HANDLE;
  physicalDevice_ = VK_NULL_HANDLE;
  surface_ = VK_NULL_HANDLE;
  graphicsQueue_ = VK_NULL_HANDLE;
  queueFamilyIndex_ = 0;
  swapchain_ = VK_NULL_HANDLE;
  surfaceFormat_ = {.format = VK_FORMAT_UNDEFINED};
  presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
  extent_ = {0, 0};
  needRebuild_ = false;
  nativeHandleReleased_ = false;
  images_.clear();
  frameResources_.clear();
  numSwapchainImages_ = 0;
  currentImageIndex_ = 0;
  currentFrameIndex_ = 0;
  frameCounter_ = 0;
  surfaceCapabilities_ = {};
  surfaceFormats_.clear();
  presentModes_.clear();
  surfaceDataValid_ = false;
}

vk::Result vk::VulkanSwapchain::initResources(uint32_t& outWidth, uint32_t& outHeight, bool vSync)
{
  PROFILER_FUNCTION();

  // Query surface capabilities and supported formats/modes
  Result result = querySwapchainSupport();
  if (!result.isOk())
  {
    return result;
  }

#if defined(__ANDROID__)
  // RECALCULATE identityExtent based on new surface capabilities
  identityExtent_ = surfaceCapabilities_.currentExtent;
  preTransform_ = surfaceCapabilities_.currentTransform;

  // If rotated 90° or 270°, swap dimensions
  if (surfaceCapabilities_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
      surfaceCapabilities_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
      std::swap(identityExtent_.width, identityExtent_.height);
  }
#endif

  // Select optimal swapchain parameters
  surfaceFormat_ = selectSwapSurfaceFormat(surfaceFormats_);
  presentMode_ = selectSwapPresentMode(presentModes_, vSync);
  extent_ = selectSwapExtent(surfaceCapabilities_, outWidth, outHeight);

  // Update output dimensions to actual extent
  outWidth = extent_.width;
  outHeight = extent_.height;

  // Determine number of images
  uint32_t imageCount = surfaceCapabilities_.minImageCount + 1;
  if (surfaceCapabilities_.maxImageCount > 0)
  {
    imageCount = std::min(imageCount, surfaceCapabilities_.maxImageCount);
  }

  // Clamp to our maximum
  imageCount = std::min(imageCount, static_cast<uint32_t>(MAX_SWAPCHAIN_IMAGES));

  // Create swapchain
#if defined(__ANDROID__)
// On Android: use identity extent and current transform for pre-rotation
  preTransform_ = surfaceCapabilities_.currentTransform;
  const VkSwapchainCreateInfoKHR createInfo = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = surface_,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat_.format,
    .imageColorSpace = surfaceFormat_.colorSpace,
    .imageExtent = identityExtent_,  // Use identity extent, not current extent
    .imageArrayLayers = 1,
    .imageUsage = selectUsageFlags(),
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &queueFamilyIndex_,
    .preTransform = preTransform_,  // Use current transform
    .compositeAlpha = selectCompositeAlpha(surfaceCapabilities_),
    .presentMode = presentMode_,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };
#else
  const VkSwapchainCreateInfoKHR createInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = surface_, .minImageCount = imageCount, .imageFormat = surfaceFormat_.format, .imageColorSpace = surfaceFormat_.colorSpace, .imageExtent = extent_, .imageArrayLayers = 1, .imageUsage = selectUsageFlags(), .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 1, .pQueueFamilyIndices = &queueFamilyIndex_,   .preTransform = (surfaceCapabilities_.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) 
                   ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR 
                   : surfaceCapabilities_.currentTransform, .compositeAlpha = selectCompositeAlpha(surfaceCapabilities_), .presentMode = presentMode_, .clipped = VK_TRUE, .oldSwapchain = VK_NULL_HANDLE,};
#endif
  VK_ASSERT(vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_));
  VK_ASSERT(vk::setDebugObjectName(device_, VK_OBJECT_TYPE_SWAPCHAIN_KHR, (uint64_t)swapchain_, "VulkanSwapchain"));

  // Get swapchain images
  VK_ASSERT(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages_, nullptr));

  std::vector<VkImage> swapImages(numSwapchainImages_);
  VK_ASSERT(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages_, swapImages.data()));

  // Create image views and texture handles
  images_.resize(numSwapchainImages_);

  const VkImageViewCreateInfo imageViewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = surfaceFormat_.format, .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY}, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},};

  for (uint32_t i = 0; i < numSwapchainImages_; i++)
  {
    SwapchainImage& swapImg = images_[i];
    swapImg.image = swapImages[i];

    // Create image view
    VkImageViewCreateInfo viewInfo = imageViewCreateInfo;
    viewInfo.image = swapImg.image;
    VK_ASSERT(vkCreateImageView(device_, &viewInfo, nullptr, &swapImg.imageView));

    char debugName[256];
    snprintf(debugName, sizeof(debugName), "Swapchain Image %u", i);
    VK_ASSERT(vk::setDebugObjectName(device_, VK_OBJECT_TYPE_IMAGE, (uint64_t)swapImg.image, debugName));

    snprintf(debugName, sizeof(debugName), "Swapchain ImageView %u", i);
    VK_ASSERT(vk::setDebugObjectName(device_, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)swapImg.imageView, debugName));

    // Create VulkanImage for texture pool
    VulkanImage vulkanImg = {.vkImage_ = swapImg.image, .vkUsageFlags_ = selectUsageFlags(), 
#if defined(__ANDROID__)
        .vkExtent_ = {identityExtent_.width, identityExtent_.height, 1},
#else
        .vkExtent_ = {extent_.width, extent_.height, 1}, 
#endif
        .vkType_ = VK_IMAGE_TYPE_2D, .vkImageFormat_ = surfaceFormat_.format, .vkSamples_ = VK_SAMPLE_COUNT_1_BIT, .isSwapchainImage_ = true, .isOwningVkImage_ = false, .numLevels_ = 1, .numLayers_ = 1, .isDepthFormat_ = VulkanImage::isDepthFormat(surfaceFormat_.format), .isStencilFormat_ = VulkanImage::isStencilFormat(surfaceFormat_.format), .vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED, .imageView_ = swapImg.imageView,};

    snprintf(vulkanImg.debugName_, sizeof(vulkanImg.debugName_), "Swapchain Image %u", i);

    swapImg.handle = ctx_->texturesPool_.create(std::move(vulkanImg));
  }

  // Create synchronization objects
  createSynchronizationObjects();

  // Transition images to present layout
  transitionImagesToPresentLayout();

  needRebuild_ = false;
  nativeHandleReleased_ = false;
  currentImageIndex_ = 0;
  currentFrameIndex_ = 0;
  frameCounter_ = 0;

  return Result();
}

vk::Result vk::VulkanSwapchain::reinitResources(uint32_t& outWidth, uint32_t& outHeight, bool vSync)
{
  PROFILER_FUNCTION();

  // Wait for all operations to complete
  VK_ASSERT(vkQueueWaitIdle(graphicsQueue_));

  // Wait for device idle
  VK_ASSERT(vkDeviceWaitIdle(device_));

  // Store old swapchain for optimization
  VkSwapchainKHR oldSwapchain = swapchain_;

  // Clean up current resources (but keep the old swapchain for now)
  destroySynchronizationObjects();

  for (auto& img : images_)
  {
    if (img.handle.valid())
    {
      ctx_->texturesPool_.destroy(img.handle);
    }
    if (img.imageView != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device_, img.imageView, nullptr);
    }
  }
  images_.clear();

  swapchain_ = VK_NULL_HANDLE;

  // Create new swapchain
  Result result = querySwapchainSupport();
  if (!result.isOk())
  {
    // Clean up old swapchain on failure
    if (oldSwapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(device_, oldSwapchain, nullptr);
    }
    return result;
  }

#if defined(__ANDROID__)
  // RECALCULATE identityExtent based on new surface capabilities
  identityExtent_ = surfaceCapabilities_.currentExtent;
  preTransform_ = surfaceCapabilities_.currentTransform;

  // If rotated 90° or 270°, swap dimensions
  if (surfaceCapabilities_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
      surfaceCapabilities_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
      std::swap(identityExtent_.width, identityExtent_.height);
  }
#endif

  // Select new parameters
  surfaceFormat_ = selectSwapSurfaceFormat(surfaceFormats_);
  presentMode_ = selectSwapPresentMode(presentModes_, vSync);
  extent_ = selectSwapExtent(surfaceCapabilities_, outWidth, outHeight);

  outWidth = extent_.width;
  outHeight = extent_.height;

  uint32_t imageCount = surfaceCapabilities_.minImageCount + 1;
  if (surfaceCapabilities_.maxImageCount > 0)
  {
    imageCount = std::min(imageCount, surfaceCapabilities_.maxImageCount);
  }
  imageCount = std::min(imageCount, static_cast<uint32_t>(MAX_SWAPCHAIN_IMAGES));

#if defined(__ANDROID__)
  preTransform_ = surfaceCapabilities_.currentTransform;
  const VkSwapchainCreateInfoKHR createInfo = { .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = surface_, .minImageCount = imageCount, .imageFormat = surfaceFormat_.format, .imageColorSpace = surfaceFormat_.colorSpace, .imageExtent = identityExtent_, .imageArrayLayers = 1, .imageUsage = selectUsageFlags(), .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 1, .pQueueFamilyIndices = &queueFamilyIndex_, .preTransform = preTransform_, .compositeAlpha = selectCompositeAlpha(surfaceCapabilities_), .presentMode = presentMode_, .clipped = VK_TRUE, .oldSwapchain = oldSwapchain, };
#else    
  const VkSwapchainCreateInfoKHR createInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = surface_, .minImageCount = imageCount, .imageFormat = surfaceFormat_.format, .imageColorSpace = surfaceFormat_.colorSpace, .imageExtent = extent_, .imageArrayLayers = 1, .imageUsage = selectUsageFlags(), .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 1, .pQueueFamilyIndices = &queueFamilyIndex_, .preTransform = surfaceCapabilities_.currentTransform, .compositeAlpha = selectCompositeAlpha(surfaceCapabilities_), .presentMode = presentMode_, .clipped = VK_TRUE, .oldSwapchain = oldSwapchain,};
#endif

  VkResult vkResult = vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);

  // Destroy old swapchain now
  if (oldSwapchain != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(device_, oldSwapchain, nullptr);
  }

  if (vkResult != VK_SUCCESS)
  {
    Result::setResult(&result, Result::Code::RuntimeError, "Failed to create swapchain");
    return result;
  }

  // Get new images and create resources
  VK_ASSERT(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages_, nullptr));

  std::vector<VkImage> swapImages(numSwapchainImages_);
  VK_ASSERT(vkGetSwapchainImagesKHR(device_, swapchain_, &numSwapchainImages_, swapImages.data()));

  images_.resize(numSwapchainImages_);

  const VkImageViewCreateInfo imageViewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = surfaceFormat_.format, .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY, .g = VK_COMPONENT_SWIZZLE_IDENTITY, .b = VK_COMPONENT_SWIZZLE_IDENTITY, .a = VK_COMPONENT_SWIZZLE_IDENTITY}, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1},};

  for (uint32_t i = 0; i < numSwapchainImages_; i++)
  {
    SwapchainImage& swapImg = images_[i];
    swapImg.image = swapImages[i];

    VkImageViewCreateInfo viewInfo = imageViewCreateInfo;
    viewInfo.image = swapImg.image;
    VK_ASSERT(vkCreateImageView(device_, &viewInfo, nullptr, &swapImg.imageView));

#if defined(__ANDROID__)
    VulkanImage vulkanImg = { .vkImage_ = swapImg.image, .vkUsageFlags_ = selectUsageFlags(), .vkExtent_ = {identityExtent_.width, identityExtent_.height, 1}, .vkType_ = VK_IMAGE_TYPE_2D, .vkImageFormat_ = surfaceFormat_.format, .vkSamples_ = VK_SAMPLE_COUNT_1_BIT, .isSwapchainImage_ = true, .isOwningVkImage_ = false, .numLevels_ = 1, .numLayers_ = 1, .isDepthFormat_ = VulkanImage::isDepthFormat(surfaceFormat_.format), .isStencilFormat_ = VulkanImage::isStencilFormat(surfaceFormat_.format), .vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED, .imageView_ = swapImg.imageView, };
#else
    VulkanImage vulkanImg = {.vkImage_ = swapImg.image, .vkUsageFlags_ = selectUsageFlags(), .vkExtent_ = {extent_.width, extent_.height, 1}, .vkType_ = VK_IMAGE_TYPE_2D, .vkImageFormat_ = surfaceFormat_.format, .vkSamples_ = VK_SAMPLE_COUNT_1_BIT, .isSwapchainImage_ = true, .isOwningVkImage_ = false, .numLevels_ = 1, .numLayers_ = 1, .isDepthFormat_ = VulkanImage::isDepthFormat(surfaceFormat_.format), .isStencilFormat_ = VulkanImage::isStencilFormat(surfaceFormat_.format), .vkImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED, .imageView_ = swapImg.imageView,};
#endif
    snprintf(vulkanImg.debugName_, sizeof(vulkanImg.debugName_), "Swapchain Image %u", i);
    swapImg.handle = ctx_->texturesPool_.create(std::move(vulkanImg));
  }

  createSynchronizationObjects();
  transitionImagesToPresentLayout();

  needRebuild_ = false;
  currentImageIndex_ = 0;
  currentFrameIndex_ = 0;

  return Result();
}

void vk::VulkanSwapchain::deinitResources()
{
    if (device_ == VK_NULL_HANDLE)
        return;

    VK_ASSERT(vkDeviceWaitIdle(device_));

  for (auto& img : images_)
  {
    if (img.handle.valid())
    {
      ctx_->texturesPool_.destroy(img.handle);
    }
    if (img.imageView != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device_, img.imageView, nullptr);
    }
  }
  images_.clear();

  destroySynchronizationObjects();
  vkQueueWaitIdle(graphicsQueue_);

  if (swapchain_ != VK_NULL_HANDLE && !nativeHandleReleased_)
  {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  }
  swapchain_ = VK_NULL_HANDLE;

  numSwapchainImages_ = 0;
  surfaceDataValid_ = false;
}

// Completely rewritten VulkanSwapchain::acquireNextImage()
vk::Result vk::VulkanSwapchain::acquireNextImage()
{
  PROFILER_FUNCTION();

  if (needRebuild_)
  {
    return Result(Result::Code::ArgumentOutOfRange, "Swapchain needs rebuild");
  }

  // If we already have an acquired image, don't acquire another
  if (hasAcquiredImage_)
  {
    return Result();
  }

  FrameResources& frame = frameResources_[currentFrameIndex_];

  // Wait for this frame's timeline semaphore if needed
  if (frame.timelineWaitValue > 0)
  {
    const VkSemaphoreWaitInfo waitInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO, .semaphoreCount = 1, .pSemaphores = &ctx_->timelineSemaphore_, .pValues = &frame.timelineWaitValue,};
    VkResult waitResult = vkWaitSemaphores(device_, &waitInfo, 0); // Non-blocking check first
    if (waitResult == VK_TIMEOUT)
    {
      // Still waiting, return early - we'll try again later
      return Result(Result::Code::RuntimeError, "Frame resources not ready");
    }
    VK_ASSERT(waitResult);
    frame.timelineWaitValue = 0;
  }

  // Wait and reset the present fence if it exists and we've used it before
  if (frame.presentFence != VK_NULL_HANDLE && frameCounter_ >= frameResources_.size())
  {
    VkResult fenceResult = vkWaitForFences(device_, 1, &frame.presentFence, VK_TRUE, 0);
    if (fenceResult == VK_SUCCESS)
    {
      VK_ASSERT(vkResetFences(device_, 1, &frame.presentFence));
    }
    else if (fenceResult != VK_TIMEOUT)
    {
      LOG_WARNING("Present fence wait failed: {}", string_VkResult(fenceResult));
    }
  }

  // Now try to acquire the next image
  VkResult result = vkAcquireNextImageKHR(device_,
                                          swapchain_,
                                          UINT64_MAX,
                                          frame.imageAvailableSemaphore,
                                          // This will be signaled when image is available
                                          VK_NULL_HANDLE,
                                          &acquiredImageIndex_ // Use separate tracking variable
  );

  switch (result)
  {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      currentImageIndex_ = acquiredImageIndex_;
      hasAcquiredImage_ = true;
      return Result();

    case VK_ERROR_OUT_OF_DATE_KHR:
      needRebuild_ = true;
      return Result(Result::Code::ArgumentOutOfRange, "Swapchain out of date");

    default: LOG_WARNING("Failed to acquire swapchain image: {}", string_VkResult(result));
      return Result(Result::Code::RuntimeError, "Failed to acquire swapchain image");
  }
}

// Add a method to get the current frame's image available semaphore
VkSemaphore vk::VulkanSwapchain::getCurrentImageAvailableSemaphore() const
{
  if (currentFrameIndex_ < frameResources_.size())
  {
    return frameResources_[currentFrameIndex_].imageAvailableSemaphore;
  }
  return VK_NULL_HANDLE;
}

// Updated VulkanSwapchain::present()
vk::Result vk::VulkanSwapchain::present(VkSemaphore waitSemaphore)
{
  PROFILER_FUNCTION();

  if (needRebuild_ || !hasAcquiredImage_)
  {
    return Result(Result::Code::ArgumentOutOfRange, "No image acquired for present");
  }

  FrameResources& frame = frameResources_[currentFrameIndex_];

  // Setup timeline semaphore for this frame
  const uint64_t signalValue = frameCounter_ + frameResources_.size();
  frame.timelineWaitValue = signalValue;
  ctx_->immediate_->signalSemaphore(ctx_->timelineSemaphore_, signalValue);

  // Present the acquired image
  const VkSwapchainPresentFenceInfoEXT fenceInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT, .swapchainCount = 1, .pFences = &frame.presentFence,};

  const VkPresentInfoKHR presentInfo = {.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = (ctx_->has_EXT_swapchain_maintenance1_ && frame.presentFence) ? &fenceInfo : nullptr,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &waitSemaphore,
    .swapchainCount = 1,
    .pSwapchains = &swapchain_,
    .pImageIndices = &acquiredImageIndex_,
    // Use the image we actually acquired
  };

  VkResult result = vkQueuePresentKHR(graphicsQueue_, &presentInfo);

  // Mark that we no longer have an acquired image
  hasAcquiredImage_ = false;
  acquiredImageIndex_ = UINT32_MAX;

  // Advance to next frame
  frameCounter_++;
  currentFrameIndex_ = (currentFrameIndex_ + 1) % frameResources_.size();

  switch (result)
  {
    case VK_SUCCESS:
      break;
    case VK_SUBOPTIMAL_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
      needRebuild_ = true;
      break;
    default: LOG_WARNING("Failed to present swapchain image: {}", string_VkResult(result));
      return Result(Result::Code::RuntimeError, "Failed to present swapchain image");
  }

  return Result();
}

vk::Result vk::VulkanSwapchain::querySwapchainSupport()
{
  PROFILER_FUNCTION();

  // Get surface capabilities
  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &surfaceCapabilities_);
  if (result != VK_SUCCESS)
  {
    return Result(Result::Code::RuntimeError, "Failed to get surface capabilities");
  }

  // Get surface formats
  uint32_t formatCount;
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
  if (result != VK_SUCCESS || formatCount == 0)
  {
    return Result(Result::Code::RuntimeError, "No surface formats available");
  }

  surfaceFormats_.resize(formatCount);
  result = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, surfaceFormats_.data());
  if (result != VK_SUCCESS)
  {
    return Result(Result::Code::RuntimeError, "Failed to get surface formats");
  }

  // Get present modes
  uint32_t presentModeCount;
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
  if (result != VK_SUCCESS || presentModeCount == 0)
  {
    return Result(Result::Code::RuntimeError, "No present modes available");
  }

  presentModes_.resize(presentModeCount);
  result = vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes_.data());
  if (result != VK_SUCCESS)
  {
    return Result(Result::Code::RuntimeError, "Failed to get present modes");
  }

  surfaceDataValid_ = true;
  return Result();
}

VkSurfaceFormatKHR vk::VulkanSwapchain::selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const
{
  // If there's only one format and it's undefined, we can choose any format
  if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
  {
    return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  }

  // Preferred formats in order of preference
  const std::vector<VkSurfaceFormatKHR> preferredFormats = {{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},};

  // Check if any preferred format is available
  for (const auto& preferred : preferredFormats)
  {
    for (const auto& available : availableFormats)
    {
      if (available.format == preferred.format && available.colorSpace == preferred.colorSpace)
      {
        return available;
      }
    }
  }

  // If no preferred format is found, return the first available
  return availableFormats[0];
}

VkPresentModeKHR vk::VulkanSwapchain::selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool vSync) const
{
#ifdef ANDROID
    // On Android, always use FIFO to ensure compatibility
    return VK_PRESENT_MODE_FIFO_KHR;
#endif

  if (vSync)
  {
    return VK_PRESENT_MODE_FIFO_KHR; // Always available and guarantees vsync
  }

  // For non-vsync, prefer modes in this order:
  // 1. MAILBOX - triple buffering, low latency
  // 2. IMMEDIATE - no buffering, lowest latency but may cause tearing
  // 3. FIFO - fallback (always available)

  bool mailboxSupported = false;
  bool immediateSupported = false;

  for (VkPresentModeKHR mode : availablePresentModes)
  {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      mailboxSupported = true;
    }
    else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
      immediateSupported = true;
    }
  }

  if (mailboxSupported)
  {
    return VK_PRESENT_MODE_MAILBOX_KHR;
  }
  if (immediateSupported)
  {
    return VK_PRESENT_MODE_IMMEDIATE_KHR;
  }

  return VK_PRESENT_MODE_FIFO_KHR; // Fallback
}

VkExtent2D vk::VulkanSwapchain::selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t requestedWidth, uint32_t requestedHeight) const
{
  // If current extent is defined, we must use it
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }

  // Otherwise, we can choose within the allowed range
  VkExtent2D actualExtent = {requestedWidth, requestedHeight};

  actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
  actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

  return actualExtent;
}

VkImageUsageFlags vk::VulkanSwapchain::selectUsageFlags() const
{
  VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  // Check what additional usage flags are supported
  if (surfaceCapabilities_.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
  {
    usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }
  if (surfaceCapabilities_.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT)
  {
    usage |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  return usage;
}

VkCompositeAlphaFlagBitsKHR vk::VulkanSwapchain::selectCompositeAlpha(const VkSurfaceCapabilitiesKHR& capabilities) const
{
  // Prefer opaque if available
  if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
  {
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  }
  if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
  {
    return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  }
  if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
  {
    return VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
  }
  if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
  {
    return VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  }

  // This should never happen as at least one must be supported
  ASSERT_MSG(false, "No supported composite alpha mode found");
  return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
}

void vk::VulkanSwapchain::createSynchronizationObjects()
{
  PROFILER_FUNCTION();

  // Create frame resources for synchronization
  // Use the actual number of swapchain images for optimal performance
  frameResources_.resize(numSwapchainImages_);

  const VkSemaphoreCreateInfo semaphoreInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,};

  for (uint32_t i = 0; i < numSwapchainImages_; i++)
  {
    FrameResources& frame = frameResources_[i];

    // Create semaphores
    VK_ASSERT(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore));

    // Create fence for swapchain maintenance if supported
    if (ctx_->has_EXT_swapchain_maintenance1_)
    {
      const VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,};
      VK_ASSERT(vkCreateFence(device_, &fenceInfo, nullptr, &frame.presentFence));
    }

    // Set debug names
    char debugName[256];
    snprintf(debugName, sizeof(debugName), "Swapchain ImageAvailable Semaphore %u", i);
    VK_ASSERT(vk::setDebugObjectName(device_, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)frame.imageAvailableSemaphore, debugName));
    if (frame.presentFence != VK_NULL_HANDLE)
    {
      snprintf(debugName, sizeof(debugName), "Swapchain Present Fence %u", i);
      VK_ASSERT(vk::setDebugObjectName(device_, VK_OBJECT_TYPE_FENCE, (uint64_t)frame.presentFence, debugName));
    }
  }
}

void vk::VulkanSwapchain::destroySynchronizationObjects()
{
  for (auto& frame : frameResources_)
  {
    if (frame.imageAvailableSemaphore != VK_NULL_HANDLE)
    {
      vkDestroySemaphore(device_, frame.imageAvailableSemaphore, nullptr);
      frame.imageAvailableSemaphore = VK_NULL_HANDLE;
    }
    if (frame.presentFence != VK_NULL_HANDLE)
    {
      vkDestroyFence(device_, frame.presentFence, nullptr);
      frame.presentFence = VK_NULL_HANDLE;
    }
    frame.timelineWaitValue = 0;
  }
  frameResources_.clear();
}

void vk::VulkanSwapchain::transitionImagesToPresentLayout()
{
  PROFILER_FUNCTION();

  // Transition all swapchain images to present layout
  const VulkanImmediateCommands::CommandBufferWrapper& wrapper = ctx_->immediate_->acquire();

  for (uint32_t i = 0; i < numSwapchainImages_; i++)
  {
    const VkImageMemoryBarrier2 barrier = {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .srcAccessMask = VK_ACCESS_2_NONE, .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, .dstAccessMask = VK_ACCESS_2_NONE, .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .image = images_[i].image, .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}};

    const VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier,};

    vkCmdPipelineBarrier2(wrapper.cmdBuf_, &depInfo);

    // Update the VulkanImage layout tracking
    VulkanImage* vulkanImg = ctx_->texturesPool_.get(images_[i].handle);
    if (vulkanImg)
    {
      vulkanImg->vkImageLayout_ = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
  }

  ctx_->immediate_->submit(wrapper);
}

VkImage vk::VulkanSwapchain::getCurrentVkImage() const
{
  if (currentImageIndex_ < images_.size())
  {
    return images_[currentImageIndex_].image;
  }
  return VK_NULL_HANDLE;
}

VkImageView vk::VulkanSwapchain::getCurrentVkImageView() const
{
  if (currentImageIndex_ < images_.size())
  {
    return images_[currentImageIndex_].imageView;
  }
  return VK_NULL_HANDLE;
}

vk::TextureHandle vk::VulkanSwapchain::getCurrentTexture()
{
  if (needRebuild_)
  {
    return {};
  }

  if (currentImageIndex_ < images_.size())
  {
    return images_[currentImageIndex_].handle;
  }
  return {};
}

VkSwapchainKHR vk::VulkanSwapchain::releaseNativeHandle()
{
  nativeHandleReleased_ = true;
  return swapchain_;
}

vk::VulkanImmediateCommands::VulkanImmediateCommands(VkDevice device, uint32_t queueFamilyIndex, const char* debugName) : device_(device), queueFamilyIndex_(queueFamilyIndex), debugName_(debugName)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_CREATE);

  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue_);

  const VkCommandPoolCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, .queueFamilyIndex = queueFamilyIndex,};
  VK_ASSERT(vkCreateCommandPool(device, &ci, nullptr, &commandPool_));
  setDebugObjectName(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)commandPool_, debugName);

  const VkCommandBufferAllocateInfo ai = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = commandPool_, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1,};

  for (uint32_t i = 0; i != kMaxCommandBuffers; i++)
  {
    CommandBufferWrapper& buf = buffers_[i];
    char fenceName[256] = {0};
    char semaphoreName[256] = {0};
    if (debugName)
    {
      snprintf(fenceName, sizeof(fenceName) - 1, "Fence: %s (cmdbuf %u)", debugName, i);
      snprintf(semaphoreName, sizeof(semaphoreName) - 1, "Semaphore: %s (cmdbuf %u)", debugName, i);
    }
    buf.semaphore_ = createSemaphore(device, semaphoreName);
    buf.fence_ = createFence(device, fenceName);
    VK_ASSERT(vkAllocateCommandBuffers(device, &ai, &buf.cmdBufAllocated_));
    buffers_[i].handle_.bufferIndex_ = i;
  }
}

vk::VulkanImmediateCommands::~VulkanImmediateCommands()
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_DESTROY);

  waitAll();

  for (CommandBufferWrapper& buf : buffers_)
  {
    // lifetimes of all VkFence objects are managed explicitly we do not use deferredTask() for them
    vkDestroyFence(device_, buf.fence_, nullptr);
    vkDestroySemaphore(device_, buf.semaphore_, nullptr);
  }

  vkDestroyCommandPool(device_, commandPool_, nullptr);
}

void vk::VulkanImmediateCommands::purge()
{
  PROFILER_FUNCTION();

  const uint32_t numBuffers = static_cast<uint32_t>(std::size(buffers_));

  for (uint32_t i = 0; i != numBuffers; i++)
  {
    // always start checking with the oldest submitted buffer, then wrap around
    CommandBufferWrapper& buf = buffers_[(i + lastSubmitHandle_.bufferIndex_ + 1) % numBuffers];

    if (buf.cmdBuf_ == VK_NULL_HANDLE || buf.isEncoding_)
    {
      continue;
    }

    const VkResult result = vkWaitForFences(device_, 1, &buf.fence_, VK_TRUE, 0);

    if (result == VK_SUCCESS)
    {
      VK_ASSERT(vkResetCommandBuffer(buf.cmdBuf_, VkCommandBufferResetFlags{ 0 }));
      VK_ASSERT(vkResetFences(device_, 1, &buf.fence_));
      buf.cmdBuf_ = VK_NULL_HANDLE;
      numAvailableCommandBuffers_++;
    }
    else
    {
      if (result != VK_TIMEOUT)
      {
        VK_ASSERT(result);
      }
    }
  }
}

const vk::VulkanImmediateCommands::CommandBufferWrapper& vk::VulkanImmediateCommands::acquire()
{
  PROFILER_FUNCTION();

  // Try to purge completed buffers first
  purge();

  if (!numAvailableCommandBuffers_)
  {
    purge(); // Try again
  }

  // If still no buffers, wait for at least one to complete
  while (!numAvailableCommandBuffers_)
  {
    // Find the oldest buffer and wait for it specifically
    uint32_t oldestIndex = UINT32_MAX;
    for (uint32_t i = 0; i < kMaxCommandBuffers; i++)
    {
      if (buffers_[i].cmdBuf_ != VK_NULL_HANDLE && !buffers_[i].isEncoding_)
      {
        oldestIndex = i;
        break;
      }
    }

    if (oldestIndex != UINT32_MAX)
    {
      LOG_INFO_LIMIT_EVERY_N(10, "Waiting for command buffer {}...", oldestIndex);
      VK_ASSERT(vkWaitForFences(device_, 1, &buffers_[oldestIndex].fence_, VK_TRUE, UINT64_MAX));
    }

    purge();
  }

  // Find available buffer
  CommandBufferWrapper* current = nullptr;
  for (CommandBufferWrapper& buf : buffers_)
  {
    if (buf.cmdBuf_ == VK_NULL_HANDLE)
    {
      current = &buf;
      break;
    }
  }

  ASSERT(current);
  ASSERT(numAvailableCommandBuffers_ > 0);
  ASSERT(current->cmdBufAllocated_ != VK_NULL_HANDLE);

  current->handle_.submitId_ = submitCounter_;
  numAvailableCommandBuffers_--;

  current->cmdBuf_ = current->cmdBufAllocated_;
  current->isEncoding_ = true;

  const VkCommandBufferBeginInfo bi = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,};
  VK_ASSERT(vkBeginCommandBuffer(current->cmdBuf_, &bi));

  nextSubmitHandle_ = current->handle_;
  return *current;
}

void vk::VulkanImmediateCommands::wait(const SubmitHandle handle)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_WAIT);

  if (handle.empty())
  {
    vkDeviceWaitIdle(device_);
    return;
  }

  if (isReady(handle))
  {
    return;
  }

  if (!VERIFY(!buffers_[handle.bufferIndex_].isEncoding_))
  {
    // we are waiting for a buffer which has not been submitted - this is probably a logic error somewhere in the calling code
    return;
  }

  VK_ASSERT(vkWaitForFences(device_, 1, &buffers_[handle.bufferIndex_].fence_, VK_TRUE, UINT64_MAX));

  purge();
}

void vk::VulkanImmediateCommands::waitAll()
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_WAIT);

  VkFence fences[kMaxCommandBuffers];

  uint32_t numFences = 0;

  for (const CommandBufferWrapper& buf : buffers_)
  {
    if (buf.cmdBuf_ != VK_NULL_HANDLE && !buf.isEncoding_)
    {
      fences[numFences++] = buf.fence_;
    }
  }

  if (numFences)
  {
    VK_ASSERT(vkWaitForFences(device_, numFences, fences, VK_TRUE, UINT64_MAX));
  }

  purge();
}

bool vk::VulkanImmediateCommands::isReady(const SubmitHandle handle, bool fastCheckNoVulkan) const
{
  ASSERT(handle.bufferIndex_ < kMaxCommandBuffers);

  if (handle.empty())
  {
    // a null handle
    return true;
  }

  const CommandBufferWrapper& buf = buffers_[handle.bufferIndex_];

  if (buf.cmdBuf_ == VK_NULL_HANDLE)
  {
    // already recycled and not yet reused
    return true;
  }

  if (buf.handle_.submitId_ != handle.submitId_)
  {
    // already recycled and reused by another command buffer
    return true;
  }

  if (fastCheckNoVulkan)
  {
    // do not ask the Vulkan API about it, just let it retire naturally (when submitId for this bufferIndex gets incremented)
    return false;
  }

  return vkWaitForFences(device_, 1, &buf.fence_, VK_TRUE, 0) == VK_SUCCESS;
}

vk::SubmitHandle vk::VulkanImmediateCommands::submit(const CommandBufferWrapper& wrapper)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_SUBMIT);
  ASSERT(wrapper.isEncoding_);
  VK_ASSERT(vkEndCommandBuffer(wrapper.cmdBuf_));

  VkSemaphoreSubmitInfo waitSemaphores[] = {{}, {}};
  uint32_t numWaitSemaphores = 0;
  if (waitSemaphore_.semaphore)
  {
    waitSemaphores[numWaitSemaphores++] = waitSemaphore_;
  }
  if (lastSubmitSemaphore_.semaphore)
  {
    waitSemaphores[numWaitSemaphores++] = lastSubmitSemaphore_;
  }
  VkSemaphoreSubmitInfo signalSemaphores[] = {VkSemaphoreSubmitInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO, .semaphore = wrapper.semaphore_, .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT}, {},};
  uint32_t numSignalSemaphores = 1;
  if (signalSemaphore_.semaphore)
  {
    signalSemaphores[numSignalSemaphores++] = signalSemaphore_;
  }

  PROFILER_ZONE("vkQueueSubmit2()", PROFILER_COLOR_SUBMIT);
#if VULKAN_PRINT_COMMANDS
	LOG_INFO_LIMIT_EVERY_N(1000, "{} vkQueueSubmit2()\n\n", reinterpret_cast<uintptr_t>(wrapper.cmdBuf_));
#endif // VULKAN_PRINT_COMMANDS
    const VkCommandBufferSubmitInfo bufferSI = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO, .commandBuffer = wrapper.cmdBuf_,};
    const VkSubmitInfo2 si = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .waitSemaphoreInfoCount = numWaitSemaphores, .pWaitSemaphoreInfos = waitSemaphores, .commandBufferInfoCount = 1u, .pCommandBufferInfos = &bufferSI, .signalSemaphoreInfoCount = numSignalSemaphores, .pSignalSemaphoreInfos = signalSemaphores,};
    VK_ASSERT(vkQueueSubmit2(queue_, 1u, &si, wrapper.fence_));
  PROFILER_ZONE_END();

  lastSubmitSemaphore_.semaphore = wrapper.semaphore_;
  lastSubmitHandle_ = wrapper.handle_;
  waitSemaphore_.semaphore = VK_NULL_HANDLE;
  signalSemaphore_.semaphore = VK_NULL_HANDLE;

  // reset
  const_cast<CommandBufferWrapper&>(wrapper).isEncoding_ = false;
  submitCounter_++;

  if (!submitCounter_)
  {
    // skip the 0 value - when uint32_t wraps around (null SubmitHandle)
    submitCounter_++;
  }

  return lastSubmitHandle_;
}

void vk::VulkanImmediateCommands::waitSemaphore(VkSemaphore semaphore)
{
  ASSERT(waitSemaphore_.semaphore == VK_NULL_HANDLE);

  waitSemaphore_.semaphore = semaphore;
}

void vk::VulkanImmediateCommands::signalSemaphore(VkSemaphore semaphore, uint64_t signalValue)
{
  ASSERT(signalSemaphore_.semaphore == VK_NULL_HANDLE);

  signalSemaphore_.semaphore = semaphore;
  signalSemaphore_.value = signalValue;
}

VkSemaphore vk::VulkanImmediateCommands::acquireLastSubmitSemaphore()
{
  return std::exchange(lastSubmitSemaphore_.semaphore, VK_NULL_HANDLE);
}

VkFence vk::VulkanImmediateCommands::getVkFence(SubmitHandle handle) const
{
  if (handle.empty())
  {
    return VK_NULL_HANDLE;
  }

  return buffers_[handle.bufferIndex_].fence_;
}

vk::SubmitHandle vk::VulkanImmediateCommands::getLastSubmitHandle() const
{
  return lastSubmitHandle_;
}

vk::SubmitHandle vk::VulkanImmediateCommands::getNextSubmitHandle() const
{
  return nextSubmitHandle_;
}

vk::VulkanPipelineBuilder::VulkanPipelineBuilder() : vertexInputState_(VkPipelineVertexInputStateCreateInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .vertexBindingDescriptionCount = 0, .pVertexBindingDescriptions = nullptr, .vertexAttributeDescriptionCount = 0, .pVertexAttributeDescriptions = nullptr,}), inputAssembly_({.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, .flags = 0, .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, .primitiveRestartEnable = VK_FALSE,}), rasterizationState_({.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, .flags = 0, .depthClampEnable = VK_FALSE, .rasterizerDiscardEnable = VK_FALSE, .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE, .depthBiasEnable = VK_FALSE, .depthBiasConstantFactor = 0.0f, .depthBiasClamp = 0.0f, .depthBiasSlopeFactor = 0.0f, .lineWidth = 1.0f,}), multisampleState_({.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, .sampleShadingEnable = VK_FALSE, .minSampleShading = 0.0f, .pSampleMask = nullptr, .alphaToCoverageEnable = VK_FALSE, .alphaToOneEnable = VK_FALSE,}), depthStencilState_({.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, .pNext = NULL, .flags = 0, .depthTestEnable = VK_FALSE, .depthWriteEnable = VK_FALSE, .depthCompareOp = VK_COMPARE_OP_LESS, .depthBoundsTestEnable = VK_FALSE, .stencilTestEnable = VK_FALSE, .front = {.failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .depthFailOp = VK_STENCIL_OP_KEEP, .compareOp = VK_COMPARE_OP_NEVER, .compareMask = 0, .writeMask = 0, .reference = 0,}, .back = {.failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .depthFailOp = VK_STENCIL_OP_KEEP, .compareOp = VK_COMPARE_OP_NEVER, .compareMask = 0, .writeMask = 0, .reference = 0,}, .minDepthBounds = 0.0f, .maxDepthBounds = 1.0f,}), tessellationState_({.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, .flags = 0, .patchControlPoints = 0,}) {}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::dynamicState(VkDynamicState state)
{
  ASSERT(numDynamicStates_ < MAX_DYNAMIC_STATES);
  dynamicStates_[numDynamicStates_++] = state;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::primitiveTopology(VkPrimitiveTopology topology)
{
  inputAssembly_.topology = topology;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::rasterizationSamples(VkSampleCountFlagBits samples, float minSampleShading)
{
  multisampleState_.rasterizationSamples = samples;
  multisampleState_.sampleShadingEnable = minSampleShading > 0 ? VK_TRUE : VK_FALSE;
  multisampleState_.minSampleShading = minSampleShading;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::cullMode(VkCullModeFlags mode)
{
  rasterizationState_.cullMode = mode;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::frontFace(VkFrontFace mode)
{
  rasterizationState_.frontFace = mode;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::polygonMode(VkPolygonMode mode)
{
  rasterizationState_.polygonMode = mode;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::vertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
  vertexInputState_ = state;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::colorAttachments(const VkPipelineColorBlendAttachmentState* states, const VkFormat* formats, uint32_t numColorAttachments)
{
  ASSERT(states);
  ASSERT(formats);
  ASSERT(numColorAttachments <= std::size(colorBlendAttachmentStates_));
  ASSERT(numColorAttachments <= std::size(colorAttachmentFormats_));
  for (uint32_t i = 0; i != numColorAttachments; i++)
  {
    colorBlendAttachmentStates_[i] = states[i];
    colorAttachmentFormats_[i] = formats[i];
  }
  numColorAttachments_ = numColorAttachments;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::depthAttachmentFormat(VkFormat format)
{
  depthAttachmentFormat_ = format;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::stencilAttachmentFormat(VkFormat format)
{
  stencilAttachmentFormat_ = format;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::patchControlPoints(uint32_t numPoints)
{
  tessellationState_.patchControlPoints = numPoints;
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::shaderStage(VkPipelineShaderStageCreateInfo stage)
{
  if (stage.module != VK_NULL_HANDLE)
  {
    ASSERT(numShaderStages_ < std::size(shaderStages_));
    shaderStages_[numShaderStages_++] = stage;
  }
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::stencilStateOps(VkStencilFaceFlags faceMask, VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp, VkCompareOp compareOp)
{
  depthStencilState_.stencilTestEnable = depthStencilState_.stencilTestEnable == VK_TRUE || failOp != VK_STENCIL_OP_KEEP || passOp != VK_STENCIL_OP_KEEP || depthFailOp != VK_STENCIL_OP_KEEP || compareOp != VK_COMPARE_OP_ALWAYS ? VK_TRUE : VK_FALSE;

  if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
  {
    VkStencilOpState& s = depthStencilState_.front;
    s.failOp = failOp;
    s.passOp = passOp;
    s.depthFailOp = depthFailOp;
    s.compareOp = compareOp;
  }

  if (faceMask & VK_STENCIL_FACE_BACK_BIT)
  {
    VkStencilOpState& s = depthStencilState_.back;
    s.failOp = failOp;
    s.passOp = passOp;
    s.depthFailOp = depthFailOp;
    s.compareOp = compareOp;
  }
  return *this;
}

vk::VulkanPipelineBuilder& vk::VulkanPipelineBuilder::stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask, uint32_t reference)
{
  if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
  {
    VkStencilOpState& s = depthStencilState_.front;
    s.compareMask = compareMask;
    s.writeMask = writeMask;
    s.reference = reference;
  }

  if (faceMask & VK_STENCIL_FACE_BACK_BIT)
  {
    VkStencilOpState& s = depthStencilState_.back;
    s.compareMask = compareMask;
    s.writeMask = writeMask;
    s.reference = reference;
  }
  return *this;
}

VkResult vk::VulkanPipelineBuilder::build(VkDevice device, VkPipelineCache pipelineCache, VkPipelineLayout pipelineLayout, VkPipeline* outPipeline, const char* debugName) noexcept
{
  const VkPipelineDynamicStateCreateInfo dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = numDynamicStates_, .pDynamicStates = dynamicStates_,};
  // viewport and scissor can be NULL if the viewport state is dynamic
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineViewportStateCreateInfo.html
  const VkPipelineViewportStateCreateInfo viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .pViewports = nullptr, .scissorCount = 1, .pScissors = nullptr,};
  const VkPipelineColorBlendStateCreateInfo colorBlendState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .logicOpEnable = VK_FALSE, .logicOp = VK_LOGIC_OP_COPY, .attachmentCount = numColorAttachments_, .pAttachments = colorBlendAttachmentStates_,};
  const VkPipelineRenderingCreateInfo renderingInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR, .pNext = nullptr, .colorAttachmentCount = numColorAttachments_, .pColorAttachmentFormats = colorAttachmentFormats_, .depthAttachmentFormat = depthAttachmentFormat_, .stencilAttachmentFormat = stencilAttachmentFormat_,};

  const VkGraphicsPipelineCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .pNext = &renderingInfo, .flags = 0, .stageCount = numShaderStages_, .pStages = shaderStages_, .pVertexInputState = &vertexInputState_, .pInputAssemblyState = &inputAssembly_, .pTessellationState = &tessellationState_, .pViewportState = &viewportState, .pRasterizationState = &rasterizationState_, .pMultisampleState = &multisampleState_, .pDepthStencilState = &depthStencilState_, .pColorBlendState = &colorBlendState, .pDynamicState = &dynamicState, .layout = pipelineLayout, .renderPass = VK_NULL_HANDLE, .subpass = 0, .basePipelineHandle = VK_NULL_HANDLE, .basePipelineIndex = -1,};

  const VkResult result = vkCreateGraphicsPipelines(device, pipelineCache, 1, &ci, nullptr, outPipeline);

  if (!VERIFY(result == VK_SUCCESS))
  {
    return result;
  }

  numPipelinesCreated_++;

  // set debug name
  return setDebugObjectName(device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)*outPipeline, debugName);
}

vk::CommandBuffer::CommandBuffer(VulkanContext* ctx) : ctx_(ctx), wrapper_(&ctx_->immediate_->acquire()) {}

vk::CommandBuffer::~CommandBuffer()
{
  // did you forget to call cmdEndRendering()?
  ASSERT(!isRendering_);
}

void vk::CommandBuffer::transitionToShaderReadOnly(TextureHandle handle) const
{
  PROFILER_FUNCTION();

  const VulkanImage& img = *ctx_->texturesPool_.get(handle);

  ASSERT(!img.isSwapchainImage_);

  // transition only non-multisampled images - MSAA images cannot be accessed from shaders
  if (img.vkSamples_ == VK_SAMPLE_COUNT_1_BIT)
  {
    const VkImageAspectFlags flags = img.getImageAspectFlags();
    // set the result of the previous render pass
    img.transitionLayout(wrapper_->cmdBuf_, img.isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL, VkImageSubresourceRange{flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
  }
}

void vk::CommandBuffer::cmdBindRayTracingPipeline(RayTracingPipelineHandle handle)
{
  PROFILER_FUNCTION();

  if (!VERIFY(!handle.empty() && ctx_->hasRayTracingPipeline_))
  {
    return;
  }

  currentPipelineGraphics_ = {};
  currentPipelineCompute_ = {};
  currentPipelineRayTracing_ = handle;

  VkPipeline pipeline = ctx_->getVkPipeline(handle);

  const RayTracingPipelineState* rtps = ctx_->rayTracingPipelinesPool_.get(handle);

  ASSERT(rtps);
  ASSERT(pipeline != VK_NULL_HANDLE);

  if (lastPipelineBound_ != pipeline)
  {
    lastPipelineBound_ = pipeline;
    vkCmdBindPipeline(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
    ctx_->checkAndUpdateDescriptorSets();
    ctx_->bindDefaultDescriptorSets(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtps->pipelineLayout_);
  }
}

void vk::CommandBuffer::cmdBindComputePipeline(ComputePipelineHandle handle)
{
  PROFILER_FUNCTION();

  if (!VERIFY(!handle.empty()))
  {
    return;
  }

  currentPipelineGraphics_ = {};
  currentPipelineCompute_ = handle;
  currentPipelineRayTracing_ = {};

  VkPipeline pipeline = ctx_->getVkPipeline(handle);

  const ComputePipelineState* cps = ctx_->computePipelinesPool_.get(handle);

  ASSERT(cps);
  ASSERT(pipeline != VK_NULL_HANDLE);

  if (lastPipelineBound_ != pipeline)
  {
    lastPipelineBound_ = pipeline;
    vkCmdBindPipeline(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    ctx_->checkAndUpdateDescriptorSets();
    ctx_->bindDefaultDescriptorSets(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_COMPUTE, cps->pipelineLayout_);
  }
}

void vk::CommandBuffer::cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDispatchThreadGroups()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DISPATCH);

  ASSERT(!isRendering_);

  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.textures[i]; i++)
  {
    useComputeTexture(deps.textures[i], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
  }
  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.buffers[i]; i++)
  {
    const VulkanBuffer* buf = ctx_->buffersPool_.get(deps.buffers[i]);
    ASSERT_MSG(buf->vkUsageFlags_ & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, "Did you forget to specify BufferUsageBits_Storage on your buffer?");
    bufferBarrier(deps.buffers[i], VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
  }

  vkCmdDispatch(wrapper_->cmdBuf_, threadgroupCount.width, threadgroupCount.height, threadgroupCount.depth);
}

void vk::CommandBuffer::cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA) const
{
  ASSERT(label);

  if (!label || !vkCmdBeginDebugUtilsLabelEXT)
  {
    return;
  }
  const VkDebugUtilsLabelEXT utilsLabel = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, .pNext = nullptr, .pLabelName = label, .color = {float((colorRGBA >> 0) & 0xff) / 255.0f, float((colorRGBA >> 8) & 0xff) / 255.0f, float((colorRGBA >> 16) & 0xff) / 255.0f, float((colorRGBA >> 24) & 0xff) / 255.0f},};
  vkCmdBeginDebugUtilsLabelEXT(wrapper_->cmdBuf_, &utilsLabel);
}

void vk::CommandBuffer::cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA) const
{
  ASSERT(label);

  if (!label || !vkCmdInsertDebugUtilsLabelEXT)
  {
    return;
  }
  const VkDebugUtilsLabelEXT utilsLabel = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT, .pNext = nullptr, .pLabelName = label, .color = {float((colorRGBA >> 0) & 0xff) / 255.0f, float((colorRGBA >> 8) & 0xff) / 255.0f, float((colorRGBA >> 16) & 0xff) / 255.0f, float((colorRGBA >> 24) & 0xff) / 255.0f},};
  vkCmdInsertDebugUtilsLabelEXT(wrapper_->cmdBuf_, &utilsLabel);
}

void vk::CommandBuffer::cmdPopDebugGroupLabel() const
{
  if (!vkCmdEndDebugUtilsLabelEXT)
  {
    return;
  }
  vkCmdEndDebugUtilsLabelEXT(wrapper_->cmdBuf_);
}

void vk::CommandBuffer::useComputeTexture(TextureHandle handle, VkPipelineStageFlags2 dstStage)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_BARRIER);

  ASSERT(!handle.empty());
  VulkanImage& tex = *ctx_->texturesPool_.get(handle);

  (void)dstStage; // TODO: add extra dstStage

  if (!tex.isStorageImage() && !tex.isSampledImage())
  {
    ASSERT_MSG(false, "Did you forget to specify TextureUsageBits::Storage or TextureUsageBits::Sampled on your texture?");
    return;
  }

  tex.transitionLayout(wrapper_->cmdBuf_, tex.isStorageImage() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkImageSubresourceRange{tex.getImageAspectFlags(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
}

void vk::CommandBuffer::bufferBarrier(BufferHandle handle, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_BARRIER);

  VulkanBuffer* buf = ctx_->buffersPool_.get(handle);

  VkBufferMemoryBarrier2 barrier = {.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2, .srcStageMask = srcStage, .srcAccessMask = 0, .dstStageMask = dstStage, .dstAccessMask = 0, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .buffer = buf->vkBuffer_, .offset = 0, .size = VK_WHOLE_SIZE,};

  if (srcStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT)
  {
    barrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }
  else
  {
    barrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  }

  if (dstStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT)
  {
    barrier.dstAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
  }
  else
  {
    barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
  }
  if (dstStage & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT)
  {
    barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
  }
  if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
  {
    barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
  }

  const VkDependencyInfo depInfo = {.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier,};

  vkCmdPipelineBarrier2(wrapper_->cmdBuf_, &depInfo);
}

void vk::CommandBuffer::cmdBeginRendering(const RenderPass& renderPass, const Framebuffer& fb, const Dependencies& deps)
{
  PROFILER_FUNCTION();

  ASSERT(!isRendering_);

  isRendering_ = true;

  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.textures[i]; i++)
  {
    transitionToShaderReadOnly(deps.textures[i]);
  }
  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.buffers[i]; i++)
  {
    VkPipelineStageFlags2 dstStageFlags = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    const VulkanBuffer* buf = ctx_->buffersPool_.get(deps.buffers[i]);
    ASSERT(buf);
    if ((buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) || (buf->vkUsageFlags_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
    {
      dstStageFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    }
    if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    {
      dstStageFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    }
    bufferBarrier(deps.buffers[i], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, dstStageFlags);
  }

  const uint32_t numFbColorAttachments = fb.getNumColorAttachments();
  const uint32_t numPassColorAttachments = renderPass.getNumColorAttachments();

  ASSERT(numPassColorAttachments == numFbColorAttachments);

  framebuffer_ = fb;

  // transition all the color attachments
  for (uint32_t i = 0; i != numFbColorAttachments; i++)
  {
    if (TextureHandle handle = fb.color[i].texture)
    {
      VulkanImage* colorTex = ctx_->texturesPool_.get(handle);
      transitionToColorAttachment(wrapper_->cmdBuf_, colorTex);
    }
    // handle MSAA
    if (TextureHandle handle = fb.color[i].resolveTexture)
    {
      VulkanImage* colorResolveTex = ctx_->texturesPool_.get(handle);
      colorResolveTex->isResolveAttachment = true;
      transitionToColorAttachment(wrapper_->cmdBuf_, colorResolveTex);
    }
  }
  // transition depth-stencil attachment
  TextureHandle depthTex = fb.depthStencil.texture;
  if (depthTex)
  {
    const VulkanImage& depthImg = *ctx_->texturesPool_.get(depthTex);
    ASSERT_MSG(depthImg.vkImageFormat_ != VK_FORMAT_UNDEFINED, "Invalid depth attachment format");
    ASSERT_MSG(depthImg.isDepthFormat_, "Invalid depth attachment format");
    const VkImageAspectFlags flags = depthImg.getImageAspectFlags();
    depthImg.transitionLayout(wrapper_->cmdBuf_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VkImageSubresourceRange{flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
  }
  // handle depth MSAA
  if (TextureHandle handle = fb.depthStencil.resolveTexture)
  {
    VulkanImage& depthResolveImg = *ctx_->texturesPool_.get(handle);
    ASSERT_MSG(depthResolveImg.isDepthFormat_, "Invalid resolve depth attachment format");
    depthResolveImg.isResolveAttachment = true;
    const VkImageAspectFlags flags = depthResolveImg.getImageAspectFlags();
    depthResolveImg.transitionLayout(wrapper_->cmdBuf_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VkImageSubresourceRange{flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
  }

  VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
  uint32_t mipLevel = 0;
  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;

  VkRenderingAttachmentInfo colorAttachments[MAX_COLOR_ATTACHMENTS];

  for (uint32_t i = 0; i != numFbColorAttachments; i++)
  {
    const Framebuffer::AttachmentDesc& attachment = fb.color[i];
    ASSERT(!attachment.texture.empty());

    VulkanImage& colorTexture = *ctx_->texturesPool_.get(attachment.texture);
    const RenderPass::AttachmentDesc& descColor = renderPass.color[i];
    if (mipLevel && descColor.level)
    {
      ASSERT_MSG(descColor.level == mipLevel, "All color attachments should have the same mip-level");
    }
    const VkExtent3D dim = colorTexture.vkExtent_;
    if (fbWidth)
    {
      ASSERT_MSG(dim.width == fbWidth, "All attachments should have the same width");
    }
    if (fbHeight)
    {
      ASSERT_MSG(dim.height == fbHeight, "All attachments should have the same height");
    }
    mipLevel = descColor.level;
    fbWidth = dim.width;
    fbHeight = dim.height;
    samples = colorTexture.vkSamples_;
    colorAttachments[i] = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr, .imageView = colorTexture.getOrCreateVkImageViewForFramebuffer(*ctx_, descColor.level, descColor.layer), .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, .resolveMode = (samples > 1) ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE, .resolveImageView = VK_NULL_HANDLE, .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED, .loadOp = loadOpToVkAttachmentLoadOp(descColor.loadOp), .storeOp = storeOpToVkAttachmentStoreOp(descColor.storeOp), .clearValue = {.color = {.float32 = {descColor.clearColor[0], descColor.clearColor[1], descColor.clearColor[2], descColor.clearColor[3]}}},};
    // handle MSAA
    if (descColor.storeOp == StoreOp::MsaaResolve)
    {
      ASSERT(samples > 1);
      ASSERT_MSG(!attachment.resolveTexture.empty(), "Framebuffer attachment should contain a resolve texture");
      VulkanImage& colorResolveTexture = *ctx_->texturesPool_.get(attachment.resolveTexture);
      colorAttachments[i].resolveImageView = colorResolveTexture.getOrCreateVkImageViewForFramebuffer(*ctx_, descColor.level, descColor.layer);
      colorAttachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  }

  VkRenderingAttachmentInfo depthAttachment = {};

  if (fb.depthStencil.texture)
  {
    VulkanImage& depthTexture = *ctx_->texturesPool_.get(fb.depthStencil.texture);
    const RenderPass::AttachmentDesc& descDepth = renderPass.depth;
    ASSERT_MSG(descDepth.level == mipLevel, "Depth attachment should have the same mip-level as color attachments");
    depthAttachment = {.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .pNext = nullptr, .imageView = depthTexture.getOrCreateVkImageViewForFramebuffer(*ctx_, descDepth.level, descDepth.layer), .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, .resolveMode = VK_RESOLVE_MODE_NONE, .resolveImageView = VK_NULL_HANDLE, .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED, .loadOp = loadOpToVkAttachmentLoadOp(descDepth.loadOp), .storeOp = storeOpToVkAttachmentStoreOp(descDepth.storeOp), .clearValue = {.depthStencil = {.depth = descDepth.clearDepth, .stencil = descDepth.clearStencil}},};
    // handle depth MSAA
    if (descDepth.storeOp == StoreOp::MsaaResolve)
    {
      ASSERT(depthTexture.vkSamples_ == samples);
      const Framebuffer::AttachmentDesc& attachment = fb.depthStencil;
      ASSERT_MSG(!attachment.resolveTexture.empty(), "Framebuffer depth attachment should contain a resolve texture");
      VulkanImage& depthResolveTexture = *ctx_->texturesPool_.get(attachment.resolveTexture);
      depthAttachment.resolveImageView = depthResolveTexture.getOrCreateVkImageViewForFramebuffer(*ctx_, descDepth.level, descDepth.layer);
      depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depthAttachment.resolveMode = ctx_->depthResolveMode_;
    }
    const VkExtent3D dim = depthTexture.vkExtent_;
    if (fbWidth)
    {
      ASSERT_MSG(dim.width == fbWidth, "All attachments should have the same width");
    }
    if (fbHeight)
    {
      ASSERT_MSG(dim.height == fbHeight, "All attachments should have the same height");
    }
    mipLevel = descDepth.level;
    fbWidth = dim.width;
    fbHeight = dim.height;
  }

  const uint32_t width = (std::max)(fbWidth >> mipLevel, 1u);
  const uint32_t height = (std::max)(fbHeight >> mipLevel, 1u);
  const Viewport viewport = {.x = 0.0f, .y = 0.0f, .width = (float)width, .height = (float)height, .minDepth = 0.0f, .maxDepth = +1.0f};
  const ScissorRect scissor = {.x = 0, .y = 0, .width = width, .height = height};

  VkRenderingAttachmentInfo stencilAttachment = depthAttachment;

  const bool isStencilFormat = renderPass.stencil.loadOp != LoadOp::Invalid;

  const VkRenderingInfo renderingInfo = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .pNext = nullptr, .flags = 0, .renderArea = {VkOffset2D{(int32_t)scissor.x, (int32_t)scissor.y}, VkExtent2D{scissor.width, scissor.height}}, .layerCount = 1, .viewMask = 0, .colorAttachmentCount = numFbColorAttachments, .pColorAttachments = colorAttachments, .pDepthAttachment = depthTex ? &depthAttachment : nullptr, .pStencilAttachment = isStencilFormat ? &stencilAttachment : nullptr,};

  cmdBindViewport(viewport);
  cmdBindScissorRect(scissor);
  cmdBindDepthState({});

  ctx_->checkAndUpdateDescriptorSets();

  vkCmdSetDepthCompareOp(wrapper_->cmdBuf_, VK_COMPARE_OP_ALWAYS);
  vkCmdSetDepthBiasEnable(wrapper_->cmdBuf_, VK_FALSE);

  vkCmdBeginRendering(wrapper_->cmdBuf_, &renderingInfo);
}

void vk::CommandBuffer::cmdEndRendering()
{
  ASSERT(isRendering_);

  isRendering_ = false;

  vkCmdEndRendering(wrapper_->cmdBuf_);

  framebuffer_ = {};
}

void vk::CommandBuffer::cmdBindViewport(const Viewport& viewport)
{
  // https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
  const VkViewport vp = {.x = viewport.x,
    // float x;
    .y = viewport.height - viewport.y,
    // float y;
    .width = viewport.width,
    // float width;
    .height = -viewport.height,
    // float height;
    .minDepth = viewport.minDepth,
    // float minDepth;
    .maxDepth = viewport.maxDepth,
    // float maxDepth;
  };
  vkCmdSetViewport(wrapper_->cmdBuf_, 0, 1, &vp);
}

void vk::CommandBuffer::cmdBindScissorRect(const ScissorRect& rect)
{
  const VkRect2D scissor = {VkOffset2D{(int32_t)rect.x, (int32_t)rect.y}, VkExtent2D{rect.width, rect.height},};
  vkCmdSetScissor(wrapper_->cmdBuf_, 0, 1, &scissor);
}

void vk::CommandBuffer::cmdBindRenderPipeline(RenderPipelineHandle handle)
{
  if (!VERIFY(!handle.empty()))
  {
    return;
  }

  currentPipelineGraphics_ = handle;
  currentPipelineCompute_ = {};
  currentPipelineRayTracing_ = {};

  const RenderPipelineState* rps = ctx_->renderPipelinesPool_.get(handle);

  ASSERT(rps);

  const bool hasDepthAttachmentPipeline = rps->desc_.depthFormat != Format::Invalid;
  const bool hasDepthAttachmentPass = !framebuffer_.depthStencil.texture.empty();

  if (hasDepthAttachmentPipeline != hasDepthAttachmentPass)
  {
    ASSERT(false);
    LOG_WARNING("Make sure your render pass and render pipeline both have matching depth attachments");
  }

  VkPipeline pipeline = ctx_->getVkPipeline(handle, viewMask_);

  ASSERT(pipeline != VK_NULL_HANDLE);

  if (lastPipelineBound_ != pipeline)
  {
    lastPipelineBound_ = pipeline;
    vkCmdBindPipeline(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    ctx_->bindDefaultDescriptorSets(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_GRAPHICS, rps->pipelineLayout_);
  }
}

void vk::CommandBuffer::cmdBindDepthState(const DepthState& desc)
{
  PROFILER_FUNCTION();

  const VkCompareOp op = compareOpToVkCompareOp(desc.compareOp);
  vkCmdSetDepthWriteEnable(wrapper_->cmdBuf_, desc.isDepthWriteEnabled ? VK_TRUE : VK_FALSE);
  vkCmdSetDepthTestEnable(wrapper_->cmdBuf_, (op != VK_COMPARE_OP_ALWAYS || desc.isDepthWriteEnabled) ? VK_TRUE : VK_FALSE);
#if defined(__ANDROID__)
	// This is a workaround for the issue.
	// On Android (Mali-G715-Immortalis MC11 v1.r38p1-01eac0.c1a71ccca2acf211eb87c5db5322f569)
	// if depth-stencil texture is not set, call of vkCmdSetDepthCompareOp leads to disappearing of all content.
	if(!framebuffer_.depthStencil.texture) {
		return;
	}
#endif
  vkCmdSetDepthCompareOp(wrapper_->cmdBuf_, op);
}

void vk::CommandBuffer::cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset)
{
  PROFILER_FUNCTION();

  if (!VERIFY(!buffer.empty()))
  {
    return;
  }

  VulkanBuffer* buf = ctx_->buffersPool_.get(buffer);

  ASSERT(buf->vkUsageFlags_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

  vkCmdBindVertexBuffers2(wrapper_->cmdBuf_, index, 1, &buf->vkBuffer_, &bufferOffset, nullptr, nullptr);
}

void vk::CommandBuffer::cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat indexFormat, uint64_t indexBufferOffset)
{
  VulkanBuffer* buf = ctx_->buffersPool_.get(indexBuffer);

  ASSERT(buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

  const VkIndexType type = indexFormatToVkIndexType(indexFormat);
  vkCmdBindIndexBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, indexBufferOffset, type);
}

void vk::CommandBuffer::cmdPushConstants(const void* data, size_t size, size_t offset)
{
  PROFILER_FUNCTION();

  ASSERT(size % 4 == 0); // VUID-vkCmdPushConstants-size-00369: size must be a multiple of 4

  // check push constant size is within max size
  const VkPhysicalDeviceLimits& limits = ctx_->getVkPhysicalDeviceProperties().limits;
  if (!VERIFY(size + offset <= limits.maxPushConstantsSize))
  {
    LOG_WARNING("Push constants size exceeded {} (max {} bytes)", size + offset, limits.maxPushConstantsSize);
  }

  if (currentPipelineGraphics_.empty() && currentPipelineCompute_.empty() && currentPipelineRayTracing_.empty())
  {
    ASSERT_MSG(false, "No pipeline bound - cannot set push constants");
    return;
  }

  const RenderPipelineState* stateGraphics = ctx_->renderPipelinesPool_.get(currentPipelineGraphics_);
  const ComputePipelineState* stateCompute = ctx_->computePipelinesPool_.get(currentPipelineCompute_);
  const RayTracingPipelineState* stateRayTracing = ctx_->rayTracingPipelinesPool_.get(currentPipelineRayTracing_);

  ASSERT(stateGraphics || stateCompute || stateRayTracing);

  VkPipelineLayout layout = stateGraphics ? stateGraphics->pipelineLayout_ : (stateCompute ? stateCompute->pipelineLayout_ : stateRayTracing->pipelineLayout_);
  VkShaderStageFlags shaderStageFlags = stateGraphics ? stateGraphics->shaderStageFlags_ : (stateCompute ? VK_SHADER_STAGE_COMPUTE_BIT : stateRayTracing->shaderStageFlags_);

  vkCmdPushConstants(wrapper_->cmdBuf_, layout, shaderStageFlags, (uint32_t)offset, (uint32_t)size, data);
}

void vk::CommandBuffer::cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data)
{
  PROFILER_FUNCTION();
  ASSERT(buffer.valid());
  ASSERT(size);
  ASSERT(size % 4 == 0);
  ASSERT(bufferOffset % 4 == 0);

  VulkanBuffer* buf = ctx_->buffersPool_.get(buffer);

  bufferBarrier(buffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

  vkCmdFillBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, bufferOffset, size, data);

  VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

  if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
  {
    dstStage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
  }
  if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
  {
    dstStage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
  }

  bufferBarrier(buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstStage);
}

void vk::CommandBuffer::cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data)
{
  PROFILER_FUNCTION();
  ASSERT(buffer.valid());
  ASSERT(data);
  ASSERT(size && size <= 65536);
  ASSERT(size % 4 == 0);
  ASSERT(bufferOffset % 4 == 0);

  VulkanBuffer* buf = ctx_->buffersPool_.get(buffer);

  bufferBarrier(buffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

  vkCmdUpdateBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, bufferOffset, size, data);

  VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

  if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
  {
    dstStage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
  }
  if (buf->vkUsageFlags_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
  {
    dstStage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
  }

  bufferBarrier(buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstStage);
}

void vk::CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDraw()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  if (vertexCount == 0)
  {
    return;
  }

  vkCmdDraw(wrapper_->cmdBuf_, vertexCount, instanceCount, firstVertex, baseInstance);
}

void vk::CommandBuffer::cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t baseInstance)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawIndexed()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  if (indexCount == 0)
  {
    return;
  }

  vkCmdDrawIndexed(wrapper_->cmdBuf_, indexCount, instanceCount, firstIndex, vertexOffset, baseInstance);
}

void vk::CommandBuffer::cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawIndirect()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  VulkanBuffer* bufIndirect = ctx_->buffersPool_.get(indirectBuffer);

  ASSERT(bufIndirect);

  vkCmdDrawIndirect(wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, drawCount, stride ? stride : sizeof(VkDrawIndirectCommand));
}

void vk::CommandBuffer::cmdDrawIndexedIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawIndexedIndirect()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  VulkanBuffer* bufIndirect = ctx_->buffersPool_.get(indirectBuffer);

  ASSERT(bufIndirect);

  vkCmdDrawIndexedIndirect(wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, drawCount, stride ? stride : sizeof(VkDrawIndexedIndirectCommand));
}

void vk::CommandBuffer::cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawIndexedIndirectCount()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  VulkanBuffer* bufIndirect = ctx_->buffersPool_.get(indirectBuffer);
  VulkanBuffer* bufCount = ctx_->buffersPool_.get(countBuffer);

  ASSERT(bufIndirect);
  ASSERT(bufCount);

  vkCmdDrawIndexedIndirectCount(wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, bufCount->vkBuffer_, countBufferOffset, maxDrawCount, stride ? stride : sizeof(VkDrawIndexedIndirectCommand));
}

void vk::CommandBuffer::cmdDrawMeshTasks(const Dimensions& threadgroupCount)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawMeshTasks()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  vkCmdDrawMeshTasksEXT(wrapper_->cmdBuf_, threadgroupCount.width, threadgroupCount.height, threadgroupCount.depth);
}

void vk::CommandBuffer::cmdDrawMeshTasksIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawMeshTasksIndirect()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  VulkanBuffer* bufIndirect = ctx_->buffersPool_.get(indirectBuffer);

  ASSERT(bufIndirect);

  vkCmdDrawMeshTasksIndirectEXT(wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, drawCount, stride ? stride : sizeof(VkDrawMeshTasksIndirectCommandEXT));
}

void vk::CommandBuffer::cmdDrawMeshTasksIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdDrawMeshTasksIndirectCount()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_DRAW);

  VulkanBuffer* bufIndirect = ctx_->buffersPool_.get(indirectBuffer);
  VulkanBuffer* bufCount = ctx_->buffersPool_.get(countBuffer);

  ASSERT(bufIndirect);
  ASSERT(bufCount);

  vkCmdDrawMeshTasksIndirectCountEXT(wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, bufCount->vkBuffer_, countBufferOffset, maxDrawCount, stride ? stride : sizeof(VkDrawMeshTasksIndirectCommandEXT));
}

void vk::CommandBuffer::cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth, const Dependencies& deps)
{
  PROFILER_FUNCTION();
  PROFILER_GPU_ZONE("cmdTraceRays()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_RTX);

  RayTracingPipelineState* rtps = ctx_->rayTracingPipelinesPool_.get(currentPipelineRayTracing_);

  if (!VERIFY(rtps))
  {
    return;
  }

  ASSERT(!isRendering_);

  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.textures[i]; i++)
  {
    useComputeTexture(deps.textures[i], VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
  }
  for (uint32_t i = 0; i != Dependencies::MAX_SUBMIT_DEPENDENCIES && deps.buffers[i]; i++)
  {
    bufferBarrier(deps.buffers[i], VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR);
  }

  vkCmdTraceRaysKHR(wrapper_->cmdBuf_, &rtps->sbtEntryRayGen, &rtps->sbtEntryMiss, &rtps->sbtEntryHit, &rtps->sbtEntryCallable, width, height, depth);
}

void vk::CommandBuffer::cmdSetBlendColor(const float color[4])
{
  vkCmdSetBlendConstants(wrapper_->cmdBuf_, color);
}

void vk::CommandBuffer::cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp)
{
  vkCmdSetDepthBias(wrapper_->cmdBuf_, constantFactor, clamp, slopeFactor);
}

void vk::CommandBuffer::cmdSetDepthBiasEnable(bool enable)
{
  vkCmdSetDepthBiasEnable(wrapper_->cmdBuf_, enable ? VK_TRUE : VK_FALSE);
}

void vk::CommandBuffer::cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount)
{
  VkQueryPool vkPool = *ctx_->queriesPool_.get(pool);

  vkCmdResetQueryPool(wrapper_->cmdBuf_, vkPool, firstQuery, queryCount);
}

void vk::CommandBuffer::cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query)
{
  VkQueryPool vkPool = *ctx_->queriesPool_.get(pool);

  vkCmdWriteTimestamp(wrapper_->cmdBuf_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkPool, query);
}

void vk::CommandBuffer::cmdClearColorImage(TextureHandle tex, const ClearColorValue& value, const TextureLayers& layers)
{
  PROFILER_GPU_ZONE("cmdClearColorImage()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_COPY);

  static_assert(sizeof(ClearColorValue) == sizeof(VkClearColorValue));

  VulkanImage* img = ctx_->texturesPool_.get(tex);

  if (!VERIFY(img))
  {
    return;
  }

  const VkImageSubresourceRange range = {.aspectMask = img->getImageAspectFlags(), .baseMipLevel = layers.mipLevel, .levelCount = VK_REMAINING_MIP_LEVELS, .baseArrayLayer = layers.layer, .layerCount = layers.numLayers,};

  imageMemoryBarrier2(wrapper_->cmdBuf_, img->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, img->vkImageLayout_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

  vkCmdClearColorImage(wrapper_->cmdBuf_, img->vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, reinterpret_cast<const VkClearColorValue*>(&value), 1, &range);

  // a ternary cascade...
  const VkImageLayout newLayout = img->vkImageLayout_ == VK_IMAGE_LAYOUT_UNDEFINED ? (img->isAttachment() ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : img->isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL) : img->vkImageLayout_;

  imageMemoryBarrier2(wrapper_->cmdBuf_, img->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout, range);

  img->vkImageLayout_ = newLayout;
}

void vk::CommandBuffer::cmdCopyImage(TextureHandle src, TextureHandle dst, const Dimensions& extent, const Offset3D& srcOffset, const Offset3D& dstOffset, const TextureLayers& srcLayers, const TextureLayers& dstLayers)
{
  PROFILER_GPU_ZONE("cmdCopyImage()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_COPY);

  VulkanImage* imgSrc = ctx_->texturesPool_.get(src);
  VulkanImage* imgDst = ctx_->texturesPool_.get(dst);

  ASSERT(imgSrc && imgDst);
  ASSERT(srcLayers.numLayers == dstLayers.numLayers);

  if (!imgSrc || !imgDst)
  {
    return;
  }

  const VkImageSubresourceRange rangeSrc = {.aspectMask = imgSrc->getImageAspectFlags(), .baseMipLevel = srcLayers.mipLevel, .levelCount = 1, .baseArrayLayer = srcLayers.layer, .layerCount = srcLayers.numLayers,};
  const VkImageSubresourceRange rangeDst = {.aspectMask = imgDst->getImageAspectFlags(), .baseMipLevel = dstLayers.mipLevel, .levelCount = 1, .baseArrayLayer = dstLayers.layer, .layerCount = dstLayers.numLayers,};

  ASSERT(imgSrc->vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);

  const VkExtent3D dstExtent = imgDst->vkExtent_;
  const bool coversFullDstImage = dstExtent.width == extent.width && dstExtent.height == extent.height && dstExtent.depth == extent.depth && dstOffset.x == 0 && dstOffset.y == 0 && dstOffset.z == 0;

  ASSERT(coversFullDstImage || imgDst->vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);

  imageMemoryBarrier2(wrapper_->cmdBuf_, imgSrc->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT}, imgSrc->vkImageLayout_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rangeSrc);
  imageMemoryBarrier2(wrapper_->cmdBuf_, imgDst->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, coversFullDstImage ? VK_IMAGE_LAYOUT_UNDEFINED : imgDst->vkImageLayout_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, rangeDst);

  const VkImageCopy regionCopy = {.srcSubresource = {.aspectMask = imgSrc->getImageAspectFlags(), .mipLevel = srcLayers.mipLevel, .baseArrayLayer = srcLayers.layer, .layerCount = srcLayers.numLayers,}, .srcOffset = {.x = srcOffset.x, .y = srcOffset.y, .z = srcOffset.z}, .dstSubresource = {.aspectMask = imgDst->getImageAspectFlags(), .mipLevel = dstLayers.mipLevel, .baseArrayLayer = dstLayers.layer, .layerCount = dstLayers.numLayers,}, .dstOffset = {.x = dstOffset.x, .y = dstOffset.y, .z = dstOffset.z}, .extent = {.width = extent.width, .height = extent.height, .depth = extent.depth},};
  const VkImageBlit regionBlit = {.srcSubresource = regionCopy.srcSubresource, .srcOffsets = {{}, {.x = int32_t(srcOffset.x + imgSrc->vkExtent_.width), .y = int32_t(srcOffset.y + imgSrc->vkExtent_.height), .z = int32_t(srcOffset.z + imgSrc->vkExtent_.depth)}}, .dstSubresource = regionCopy.dstSubresource, .dstOffsets = {{}, {.x = int32_t(dstOffset.x + imgDst->vkExtent_.width), .y = int32_t(dstOffset.y + imgDst->vkExtent_.height), .z = int32_t(dstOffset.z + imgDst->vkExtent_.depth)}},};

  const bool isCompatible = getBytesPerPixel(imgSrc->vkImageFormat_) == getBytesPerPixel(imgDst->vkImageFormat_);

  isCompatible ? vkCmdCopyImage(wrapper_->cmdBuf_, imgSrc->vkImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imgDst->vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionCopy) : vkCmdBlitImage(wrapper_->cmdBuf_, imgSrc->vkImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imgDst->vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regionBlit, VK_FILTER_LINEAR);

  imageMemoryBarrier2(wrapper_->cmdBuf_, imgSrc->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, imgSrc->vkImageLayout_, rangeSrc);

  // a ternary cascade...
  const VkImageLayout newLayout = imgDst->vkImageLayout_ == VK_IMAGE_LAYOUT_UNDEFINED ? (imgDst->isAttachment() ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL : imgDst->isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL) : imgDst->vkImageLayout_;

  imageMemoryBarrier2(wrapper_->cmdBuf_, imgDst->vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newLayout, rangeDst);

  imgDst->vkImageLayout_ = newLayout;
}

void vk::CommandBuffer::cmdGenerateMipmap(TextureHandle handle)
{
  if (handle.empty())
  {
    return;
  }

  const VulkanImage* tex = ctx_->texturesPool_.get(handle);

  if (tex->numLevels_ <= 1)
  {
    return;
  }

  ASSERT(tex->vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);

  tex->generateMipmap(wrapper_->cmdBuf_);
}

void vk::CommandBuffer::cmdUpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer)
{
  PROFILER_GPU_ZONE("cmdUpdateTLAS()", ctx_, wrapper_->cmdBuf_, PROFILER_COLOR_CMD_RTX);

  if (handle.empty())
  {
    return;
  }

  AccelerationStructure* as = ctx_->accelStructuresPool_.get(handle);

  const VkAccelerationStructureGeometryKHR accelerationStructureGeometry{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR, .geometry = {.instances = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, .arrayOfPointers = VK_FALSE, .data = {.deviceAddress = ctx_->gpuAddress(instancesBuffer)},},}, .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,};

  VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, .geometryCount = 1, .pGeometries = &accelerationStructureGeometry,};
  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,};
  vkGetAccelerationStructureBuildSizesKHR(ctx_->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &as->buildRangeInfo.primitiveCount, &accelerationStructureBuildSizesInfo);

  const uint32_t alignment = ctx_->accelerationStructureProperties_.minAccelerationStructureScratchOffsetAlignment;
  accelerationStructureBuildSizesInfo.accelerationStructureSize += alignment;
  accelerationStructureBuildSizesInfo.updateScratchSize += alignment;
  accelerationStructureBuildSizesInfo.buildScratchSize += alignment;

  if (!as->scratchBuffer.valid() || bufferSize(*ctx_, as->scratchBuffer) < accelerationStructureBuildSizesInfo.updateScratchSize)
  {
    LOG_DEBUG("Recreating scratch buffer for TLAS update");
    as->scratchBuffer = ctx_->createBuffer(BufferDesc{.usage = BufferUsageBits_Storage, .storage = StorageType::Device, .size = accelerationStructureBuildSizesInfo.updateScratchSize, .debugName = "scratchBuffer",}, nullptr, nullptr);
  }

  const VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR, .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR, .srcAccelerationStructure = as->vkHandle, .dstAccelerationStructure = as->vkHandle, .geometryCount = 1, .pGeometries = &accelerationStructureGeometry, .scratchData = {.deviceAddress = ctx_->gpuAddress(as->scratchBuffer)},};

  const VkAccelerationStructureBuildRangeInfoKHR* accelerationBuildStructureRangeInfos[] = {&as->buildRangeInfo};

  {
    const VkBufferMemoryBarrier2 barriers[] = {{.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT, .dstStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, .buffer = getVkBuffer(ctx_, handle), .size = VK_WHOLE_SIZE,}, {.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_HOST_BIT, .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT, .dstStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT, .buffer = getVkBuffer(ctx_, instancesBuffer), .size = VK_WHOLE_SIZE,},};
    const VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = std::size(barriers), .pBufferMemoryBarriers = barriers,};
    vkCmdPipelineBarrier2(wrapper_->cmdBuf_, &dependencyInfo);
  }
  vkCmdBuildAccelerationStructuresKHR(wrapper_->cmdBuf_, 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos);
  {
    const VkBufferMemoryBarrier2 barrier = {.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2, .srcStageMask = VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, .srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT, .dstStageMask = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT, .buffer = getVkBuffer(ctx_, handle), .offset = 0, .size = VK_WHOLE_SIZE,};
    const VkDependencyInfo dependencyInfo{.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO, .bufferMemoryBarrierCount = 1, .pBufferMemoryBarriers = &barrier};
    vkCmdPipelineBarrier2(wrapper_->cmdBuf_, &dependencyInfo);
  }
}

vk::VulkanStagingDevice::VulkanStagingDevice(VulkanContext& ctx) : ctx_(ctx)
{
  PROFILER_FUNCTION();

  const VkPhysicalDeviceLimits& limits = ctx_.getVkPhysicalDeviceProperties().limits;

  // use default value of 128Mb clamped to the max limits
  //maxBufferSize_ = (std::min)(limits.maxStorageBufferRange, 128u * 1024u * 1024u);
  maxBufferSize_ = limits.maxStorageBufferRange;
  ASSERT(minBufferSize_ <= maxBufferSize_);
}

void vk::VulkanStagingDevice::bufferSubData(VulkanBuffer& buffer, size_t dstOffset, size_t size, const void* data)
{
  PROFILER_FUNCTION();

  if (buffer.isMapped())
  {
    buffer.bufferSubData(ctx_, dstOffset, size, data);
    return;
  }

  VulkanBuffer* stagingBuffer = ctx_.buffersPool_.get(stagingBuffer_);

  ASSERT(stagingBuffer);

  while (size)
  {
    // get next staging buffer free offset
    MemoryRegionDesc desc = getNextFreeOffset((uint32_t)size);
    const uint32_t chunkSize = (std::min)((uint32_t)size, desc.size_);

    // copy data into staging buffer
    stagingBuffer->bufferSubData(ctx_, desc.offset_, chunkSize, data);

    // do the transfer
    const VkBufferCopy copy = {.srcOffset = desc.offset_, .dstOffset = dstOffset, .size = chunkSize,};

    const VulkanImmediateCommands::CommandBufferWrapper& wrapper = ctx_.immediate_->acquire();
    vkCmdCopyBuffer(wrapper.cmdBuf_, stagingBuffer->vkBuffer_, buffer.vkBuffer_, 1, &copy);
    VkBufferMemoryBarrier barrier = {.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccessMask = 0, .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, .buffer = buffer.vkBuffer_, .offset = dstOffset, .size = chunkSize,};
    VkPipelineStageFlags dstMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    if (buffer.vkUsageFlags_ & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    {
      dstMask |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
      barrier.dstAccessMask |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if (buffer.vkUsageFlags_ & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
      dstMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (buffer.vkUsageFlags_ & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    {
      dstMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
      barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (buffer.vkUsageFlags_ & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
    {
      dstMask |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
      barrier.dstAccessMask |= VK_ACCESS_MEMORY_READ_BIT;
    }
    vkCmdPipelineBarrier(wrapper.cmdBuf_, VK_PIPELINE_STAGE_TRANSFER_BIT, dstMask, VkDependencyFlags{}, 0, nullptr, 1, &barrier, 0, nullptr);
    desc.handle_ = ctx_.immediate_->submit(wrapper);
    regions_.push_back(desc);

    size -= chunkSize;
    data = (uint8_t*)data + chunkSize;
    dstOffset += chunkSize;
  }
}

void vk::VulkanStagingDevice::imageData2D(VulkanImage& image, const VkRect2D& imageRegion, uint32_t baseMipLevel, uint32_t numMipLevels, uint32_t layer, uint32_t numLayers, VkFormat format, const void* data)
{
  PROFILER_FUNCTION();

  ASSERT(numMipLevels <= MAX_MIP_LEVELS);

  // divide the width and height by 2 until we get to the size of level 'baseMipLevel'
  uint32_t width = image.vkExtent_.width >> baseMipLevel;
  uint32_t height = image.vkExtent_.height >> baseMipLevel;

  const Format texFormat(vkFormatToFormat(format));

  ASSERT_MSG(!imageRegion.offset.x && !imageRegion.offset.y && imageRegion.extent.width == width && imageRegion.extent.height == height, "Uploading mip-levels with an image region that is smaller than the base mip level is not supported");

  // find the storage size for all mip-levels being uploaded
  uint32_t layerStorageSize = 0;
  for (uint32_t i = 0; i < numMipLevels; ++i)
  {
    const uint32_t mipSize = getTextureBytesPerLayer(image.vkExtent_.width, image.vkExtent_.height, texFormat, i);
    layerStorageSize += mipSize;
    width = width <= 1 ? 1 : width >> 1;
    height = height <= 1 ? 1 : height >> 1;
  }
  const uint32_t storageSize = layerStorageSize * numLayers;

  ensureStagingBufferSize(storageSize);

  ASSERT(storageSize <= stagingBufferSize_);

  MemoryRegionDesc desc = getNextFreeOffset(storageSize);
  // No support for copying image in multiple smaller chunk sizes. If we get smaller buffer size than storageSize, we will wait for GPU idle
  // and get bigger chunk.
  if (desc.size_ < storageSize)
  {
    waitAndReset();
    desc = getNextFreeOffset(storageSize);
  }
  ASSERT(desc.size_ >= storageSize);

  const VulkanImmediateCommands::CommandBufferWrapper& wrapper = ctx_.immediate_->acquire();

  VulkanBuffer* stagingBuffer = ctx_.buffersPool_.get(stagingBuffer_);

  stagingBuffer->bufferSubData(ctx_, desc.offset_, storageSize, data);

  uint32_t offset = 0;

  const uint32_t numPlanes = getNumImagePlanes(image.vkImageFormat_);

  if (numPlanes > 1)
  {
    ASSERT(layer == 0 && baseMipLevel == 0);
    ASSERT(numLayers == 1 && numMipLevels == 1);
    ASSERT(imageRegion.offset.x == 0 && imageRegion.offset.y == 0);
    ASSERT(image.vkType_ == VK_IMAGE_TYPE_2D);
    ASSERT(image.vkExtent_.width == imageRegion.extent.width && image.vkExtent_.height == imageRegion.extent.height);
  }

  VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;

  if (numPlanes == 2)
  {
    imageAspect = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT;
  }
  if (numPlanes == 3)
  {
    imageAspect = VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT;
  }

  // https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
  for (uint32_t mipLevel = 0; mipLevel < numMipLevels; ++mipLevel)
  {
    for (uint32_t i = 0; i != numLayers; i++)
    {
      const uint32_t currentMipLevel = baseMipLevel + mipLevel;

      ASSERT(currentMipLevel < image.numLevels_);
      ASSERT(mipLevel < image.numLevels_);

      // 1. Transition initial image layout into TRANSFER_DST_OPTIMAL
      imageMemoryBarrier2(wrapper.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_NONE}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageSubresourceRange{imageAspect, currentMipLevel, 1, i, 1});

#if VULKAN_PRINT_COMMANDS
			LOG_INFO("{} vkCmdCopyBufferToImage()\n", reinterpret_cast<uintptr_t>(wrapper.cmdBuf_));
#endif // VULKAN_PRINT_COMMANDS
      // 2. Copy the pixel data from the staging buffer into the image
      uint32_t planeOffset = 0;
      for (uint32_t plane = 0; plane != numPlanes; plane++)
      {
        const VkExtent2D extent = getImagePlaneExtent({.width = (std::max)(1u, imageRegion.extent.width >> mipLevel), .height = (std::max)(1u, imageRegion.extent.height >> mipLevel),}, vkFormatToFormat(format), plane);
        const VkRect2D region = {.offset = {.x = imageRegion.offset.x >> mipLevel, .y = imageRegion.offset.y >> mipLevel}, .extent = extent,};
        const VkBufferImageCopy copy = {
          // the offset for this level is at the start of all mip-levels plus the size of all previous mip-levels being uploaded
          .bufferOffset = desc.offset_ + offset + planeOffset,
          .bufferRowLength = 0,
          .bufferImageHeight = 0,
          .imageSubresource = VkImageSubresourceLayers{numPlanes > 1 ? VK_IMAGE_ASPECT_PLANE_0_BIT << plane : imageAspect, currentMipLevel, i, 1},
          .imageOffset = {.x = region.offset.x, .y = region.offset.y, .z = 0},
          .imageExtent = {.width = region.extent.width, .height = region.extent.height, .depth = 1u},};
        vkCmdCopyBufferToImage(wrapper.cmdBuf_, stagingBuffer->vkBuffer_, image.vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
        planeOffset += getTextureBytesPerPlane(imageRegion.extent.width, imageRegion.extent.height, vkFormatToFormat(format), plane);
      }

      // 3. Transition TRANSFER_DST_OPTIMAL into SHADER_READ_ONLY_OPTIMAL
      imageMemoryBarrier2(wrapper.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkImageSubresourceRange{imageAspect, currentMipLevel, 1, i, 1});

      offset += getTextureBytesPerLayer(imageRegion.extent.width, imageRegion.extent.height, texFormat, currentMipLevel);
    }
  }

  image.vkImageLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  desc.handle_ = ctx_.immediate_->submit(wrapper);
  regions_.push_back(desc);
}

void vk::VulkanStagingDevice::imageData3D(VulkanImage& image, const VkOffset3D& offset, const VkExtent3D& extent, VkFormat format, const void* data)
{
  PROFILER_FUNCTION();
  ASSERT_MSG(image.numLevels_ == 1, "Can handle only 3D images with exactly 1 mip-level");
  ASSERT_MSG((offset.x == 0) && (offset.y == 0) && (offset.z == 0), "Can upload only full-size 3D images");
  const uint32_t storageSize = extent.width * extent.height * extent.depth * getBytesPerPixel(format);

  ensureStagingBufferSize(storageSize);

  ASSERT_MSG(storageSize <= stagingBufferSize_, "No support for copying image in multiple smaller chunk sizes");

  // get next staging buffer free offset
  MemoryRegionDesc desc = getNextFreeOffset(storageSize);

  // No support for copying image in multiple smaller chunk sizes.
  // If we get smaller buffer size than storageSize, we will wait for GPU idle and get a bigger chunk.
  if (desc.size_ < storageSize)
  {
    waitAndReset();
    desc = getNextFreeOffset(storageSize);
  }

  ASSERT(desc.size_ >= storageSize);

  VulkanBuffer* stagingBuffer = ctx_.buffersPool_.get(stagingBuffer_);

  // 1. Copy the pixel data into the host visible staging buffer
  stagingBuffer->bufferSubData(ctx_, desc.offset_, storageSize, data);

  const VulkanImmediateCommands::CommandBufferWrapper& wrapper = ctx_.immediate_->acquire();

  // 1. Transition initial image layout into TRANSFER_DST_OPTIMAL
  imageMemoryBarrier2(wrapper.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_NONE}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

  // 2. Copy the pixel data from the staging buffer into the image
  const VkBufferImageCopy copy = {.bufferOffset = desc.offset_, .bufferRowLength = 0, .bufferImageHeight = 0, .imageSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, .imageOffset = offset, .imageExtent = extent,};
  vkCmdCopyBufferToImage(wrapper.cmdBuf_, stagingBuffer->vkBuffer_, image.vkImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

  // 3. Transition TRANSFER_DST_OPTIMAL into SHADER_READ_ONLY_OPTIMAL
  imageMemoryBarrier2(wrapper.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

  image.vkImageLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  desc.handle_ = ctx_.immediate_->submit(wrapper);
  regions_.push_back(desc);
}

void vk::VulkanStagingDevice::getImageData(VulkanImage& image, const VkOffset3D& offset, const VkExtent3D& extent, VkImageSubresourceRange range, VkFormat format, void* outData)
{
  ASSERT(image.vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);
  ASSERT(range.layerCount == 1);

  const uint32_t storageSize = extent.width * extent.height * extent.depth * getBytesPerPixel(format);

  ensureStagingBufferSize(storageSize);

  ASSERT(storageSize <= stagingBufferSize_);

  // get next staging buffer free offset
  MemoryRegionDesc desc = getNextFreeOffset(storageSize);

  // No support for copying image in multiple smaller chunk sizes.
  // If we get smaller buffer size than storageSize, we will wait for GPU idle and get a bigger chunk.
  if (desc.size_ < storageSize)
  {
    waitAndReset();
    desc = getNextFreeOffset(storageSize);
  }

  ASSERT(desc.size_ >= storageSize);

  VulkanBuffer* stagingBuffer = ctx_.buffersPool_.get(stagingBuffer_);

  const VulkanImmediateCommands::CommandBufferWrapper& wrapper1 = ctx_.immediate_->acquire();

  // 1. Transition to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
  imageMemoryBarrier2(wrapper1.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT}, image.vkImageLayout_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range);

  // 2.  Copy the pixel data from the image into the staging buffer
  const VkBufferImageCopy copy = {.bufferOffset = desc.offset_, .bufferRowLength = 0, .bufferImageHeight = extent.height, .imageSubresource = VkImageSubresourceLayers{.aspectMask = range.aspectMask, .mipLevel = range.baseMipLevel, .baseArrayLayer = range.baseArrayLayer, .layerCount = range.layerCount,}, .imageOffset = offset, .imageExtent = extent,};
  vkCmdCopyImageToBuffer(wrapper1.cmdBuf_, image.vkImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->vkBuffer_, 1, &copy);

  desc.handle_ = ctx_.immediate_->submit(wrapper1);
  regions_.push_back(desc);

  waitAndReset();

  if (!stagingBuffer->isCoherentMemory_)
  {
    stagingBuffer->invalidateMappedMemory(ctx_, desc.offset_, desc.size_);
  }

  // 3. Copy data from staging buffer into data
  memcpy(outData, stagingBuffer->getMappedPtr() + desc.offset_, storageSize);

  // 4. Transition back to the initial image layout
  const VulkanImmediateCommands::CommandBufferWrapper& wrapper2 = ctx_.immediate_->acquire();

  imageMemoryBarrier2(wrapper2.cmdBuf_, image.vkImage_, StageAccess{.stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT}, StageAccess{.stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT}, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.vkImageLayout_, range);

  ctx_.immediate_->wait(ctx_.immediate_->submit(wrapper2));
}

void vk::VulkanStagingDevice::ensureStagingBufferSize(uint32_t sizeNeeded)
{
  PROFILER_FUNCTION();

  const uint32_t alignedSize = (std::max)(getAlignedSize(sizeNeeded, kStagingBufferAlignment), minBufferSize_);

  sizeNeeded = alignedSize < maxBufferSize_ ? alignedSize : maxBufferSize_;

  if (!stagingBuffer_.empty())
  {
    const bool isEnoughSize = sizeNeeded <= stagingBufferSize_;
    const bool isMaxSize = stagingBufferSize_ == maxBufferSize_;

    if (isEnoughSize || isMaxSize)
    {
      return;
    }
  }

  waitAndReset();

  // deallocate the previous staging buffer
  stagingBuffer_ = nullptr;

  // if the combined size of the new staging buffer and the existing one is larger than the limit imposed by some architectures on buffers
  // that are device and host visible, we need to wait for the current buffer to be destroyed before we can allocate a new one
  if ((sizeNeeded + stagingBufferSize_) > maxBufferSize_)
  {
    ctx_.waitDeferredTasks();
  }

  stagingBufferSize_ = sizeNeeded;

  char debugName[256] = {0};
  snprintf(debugName, sizeof(debugName) - 1, "Buffer: staging buffer %u", stagingBufferCounter_++);

  stagingBuffer_ = {&ctx_, ctx_.createBuffer(stagingBufferSize_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, nullptr, debugName)};
  ASSERT(!stagingBuffer_.empty());

  regions_.clear();
  regions_.push_back({.offset_ = 0, .size_ = stagingBufferSize_, .handle_ = SubmitHandle()});
}

vk::VulkanStagingDevice::MemoryRegionDesc vk::VulkanStagingDevice::getNextFreeOffset(uint32_t size)
{
  PROFILER_FUNCTION();

  const uint32_t requestedAlignedSize = getAlignedSize(size, kStagingBufferAlignment);

  ensureStagingBufferSize(requestedAlignedSize);

  ASSERT(!regions_.empty());

  // if we can't find an available region that is big enough to store requestedAlignedSize, return whatever we could find, which will be
  // stored in bestNextIt
  auto bestNextIt = regions_.begin();

  for (auto it = regions_.begin(); it != regions_.end(); ++it)
  {
    if (ctx_.immediate_->isReady(it->handle_))
    {
      // This region is free, but is it big enough?
      if (it->size_ >= requestedAlignedSize)
      {
        // It is big enough!
        const uint32_t unusedSize = it->size_ - requestedAlignedSize;
        const uint32_t unusedOffset = it->offset_ + requestedAlignedSize;

        // Return this region and add the remaining unused size to the regions_ deque
        SCOPE_EXIT
        {
          regions_.erase(it);
          if (unusedSize > 0)
          {
            regions_.insert(regions_.begin(), {.offset_ = unusedOffset, .size_ = unusedSize, .handle_ = SubmitHandle()});
          }
        };

        return {.offset_ = it->offset_, .size_ = requestedAlignedSize, .handle_ = SubmitHandle()};
      }
      // cache the largest available region that isn't as big as the one we're looking for
      if (it->size_ > bestNextIt->size_)
      {
        bestNextIt = it;
      }
    }
  }

  // we found a region that is available that is smaller than the requested size. It's the best we can do
  if (bestNextIt != regions_.end() && ctx_.immediate_->isReady(bestNextIt->handle_))
  {
    SCOPE_EXIT { regions_.erase(bestNextIt); };

    return {.offset_ = bestNextIt->offset_, .size_ = bestNextIt->size_, .handle_ = SubmitHandle()};
  }

  // nothing was available. Let's wait for the entire staging buffer to become free
  waitAndReset();

  // waitAndReset() adds a region that spans the entire buffer. Since we'll be using part of it, we need to replace it with a used block and
  // an unused portion
  regions_.clear();

  // store the unused size in the deque first...
  const uint32_t unusedSize = stagingBufferSize_ > requestedAlignedSize ? stagingBufferSize_ - requestedAlignedSize : 0;

  if (unusedSize)
  {
    const uint32_t unusedOffset = stagingBufferSize_ - unusedSize;
    regions_.insert(regions_.begin(), {.offset_ = unusedOffset, .size_ = unusedSize, .handle_ = SubmitHandle()});
  }

  // ...and then return the smallest free region that can hold the requested size
  return {.offset_ = 0, .size_ = stagingBufferSize_ - unusedSize, .handle_ = SubmitHandle(),};
}

void vk::VulkanStagingDevice::waitAndReset()
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_WAIT);

  for (const MemoryRegionDesc& r : regions_)
  {
    ctx_.immediate_->wait(r.handle_);
  };

  regions_.clear();
  regions_.push_back({.offset_ = 0, .size_ = stagingBufferSize_, .handle_ = SubmitHandle()});
}

vk::VulkanContext::VulkanContext(const ContextConfig& config) : vkSurface_(VK_NULL_HANDLE), config_(config)
{
  pimpl_ = std::make_unique<VulkanContextImpl>();

  if (volkInitialize() != VK_SUCCESS)
  {
    LOG_CRITICAL("volkInitialize() failed\n");
    exit(255);
  }

  glslang_initialize_process();
  createInstance();

  // Surface will be created explicitly via createSurface()
}

vk::VulkanContext::~VulkanContext()
{
  PROFILER_FUNCTION();

  VK_ASSERT(vkDeviceWaitIdle(vkDevice_));

#if defined(TRACY_ENABLE)
  TracyVkDestroy(pimpl_->tracyVkCtx_);
  if (pimpl_->tracyCommandPool_)
  {
    vkDestroyCommandPool(vkDevice_, pimpl_->tracyCommandPool_, nullptr);
  }
#endif // TRACY_ENABLE
  waitDeferredTasks();

  stagingDevice_.reset(nullptr);
  swapchain_.reset(nullptr); // swapchain has to be destroyed prior to Surface

  vkDestroySemaphore(vkDevice_, timelineSemaphore_, nullptr);

  destroy(dummyTexture_);

  for (VulkanContextImpl::YcbcrConversionData& data : pimpl_->ycbcrConversionData_)
  {
    if (data.info.conversion != VK_NULL_HANDLE)
    {
      vkDestroySamplerYcbcrConversion(vkDevice_, data.info.conversion, nullptr);
      data.sampler.reset();
    }
  }

  if (shaderModulesPool_.numObjects())
  {
    LOG_WARNING("Leaked {} shader modules\n", shaderModulesPool_.numObjects());
  }
  if (renderPipelinesPool_.numObjects())
  {
    LOG_WARNING("Leaked {} render pipelines\n", renderPipelinesPool_.numObjects());
  }
  if (computePipelinesPool_.numObjects())
  {
    LOG_WARNING("Leaked {} compute pipelines\n", computePipelinesPool_.numObjects());
  }
  if (samplersPool_.numObjects() > 1)
  {
    // the dummy value is owned by the context
    LOG_WARNING("Leaked {} samplers\n", samplersPool_.numObjects() - 1);
  }
  if (texturesPool_.numObjects())
  {
    LOG_WARNING("Leaked {} textures\n", texturesPool_.numObjects());
  }
  if (buffersPool_.numObjects())
  {
    LOG_WARNING("Leaked {} buffers\n", buffersPool_.numObjects());
  }

  // manually destroy the dummy sampler
  vkDestroySampler(vkDevice_, samplersPool_.objects_.front().obj_, nullptr);
  samplersPool_.clear();
  computePipelinesPool_.clear();
  renderPipelinesPool_.clear();
  shaderModulesPool_.clear();
  texturesPool_.clear();

  waitDeferredTasks();

  immediate_.reset(nullptr);

  vkDestroyDescriptorSetLayout(vkDevice_, vkDSL_, nullptr);
  vkDestroyDescriptorPool(vkDevice_, vkDPool_, nullptr);
  vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
  vkDestroyPipelineCache(vkDevice_, pipelineCache_, nullptr);

  // Clean up VMA
  vmaDestroyAllocator(pimpl_->vma_);

  // Device has to be destroyed prior to Instance
  vkDestroyDevice(vkDevice_, nullptr);

  if (vkDebugUtilsMessenger_)
  {
    vkDestroyDebugUtilsMessengerEXT(vkInstance_, vkDebugUtilsMessenger_, nullptr);
  }

  vkDestroyInstance(vkInstance_, nullptr);

  glslang_finalize_process();

  LOG_INFO("Vulkan graphics pipelines created: {}\n", VulkanPipelineBuilder::getNumPipelinesCreated());
}

vk::ICommandBuffer& vk::VulkanContext::acquireCommandBuffer()
{
  PROFILER_FUNCTION();

  ASSERT_MSG(!pimpl_->currentCommandBuffer_.ctx_, "Cannot acquire more than 1 command buffer simultaneously");

#if defined(_M_ARM64)
	vkDeviceWaitIdle(vkDevice_); // a temporary workaround for Windows on Snapdragon
#endif

  pimpl_->currentCommandBuffer_ = CommandBuffer(this);

  return pimpl_->currentCommandBuffer_;
}

// Modified VulkanContext::submit()
vk::SubmitHandle vk::VulkanContext::submit(ICommandBuffer& commandBuffer, TextureHandle present)
{
  PROFILER_FUNCTION();

  CommandBuffer* vkCmdBuffer = static_cast<CommandBuffer*>(&commandBuffer);
  ASSERT(vkCmdBuffer && vkCmdBuffer->ctx_ && vkCmdBuffer->wrapper_);

#if defined(TRACY_ENABLE)
  TracyVkCollect(pimpl_->tracyVkCtx_, vkCmdBuffer->wrapper_->cmdBuf_);
#endif

  if (present)
  {
    const VulkanImage& tex = *texturesPool_.get(present);
    ASSERT(tex.isSwapchainImage_);

    tex.transitionLayout(vkCmdBuffer->wrapper_->cmdBuf_, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS});
  }

  const bool shouldPresent = hasSwapchain() && present;

  // If presenting, wait on the current frame's image available semaphore
  if (shouldPresent)
  {
    VkSemaphore imageAvailableSem = swapchain_->getCurrentImageAvailableSemaphore();
    if (imageAvailableSem != VK_NULL_HANDLE)
    {
      immediate_->waitSemaphore(imageAvailableSem);
    }
  }

  vkCmdBuffer->lastSubmitHandle_ = immediate_->submit(*vkCmdBuffer->wrapper_);

  if (shouldPresent)
  {
    Result result = swapchain_->present(immediate_->acquireLastSubmitSemaphore());
    if (!result.isOk())
    {
      if (result.code == Result::Code::ArgumentOutOfRange)
      {
        shouldRecreate_ = true;
      }
      else
      {
        LOG_WARNING("Present failed: {}", result.message);
      }
    }
  }

  processDeferredTasks();
  pimpl_->currentCommandBuffer_ = {};
  return vkCmdBuffer->lastSubmitHandle_;
}

void vk::VulkanContext::wait(SubmitHandle handle)
{
  immediate_->wait(handle);
}

vk::Holder<vk::BufferHandle> vk::VulkanContext::createBuffer(const BufferDesc& requestedDesc, const char* debugName, Result* outResult)
{
  BufferDesc desc = requestedDesc;

  if (debugName && *debugName)
    desc.debugName = debugName;

  if (!useStaging_ && (desc.storage == StorageType::Device))
  {
    desc.storage = StorageType::HostVisible;
  }
  // VMA should handle this.
  // Use staging device to transfer data into the buffer when the storage is private to the device
  VkBufferUsageFlags usageFlags = (desc.storage == StorageType::Device) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
  // VMA should handle this

  if (desc.usage == 0)
  {
    Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "Invalid buffer usage"));
    return {};
  }

  if (desc.usage & BufferUsageBits_Index)
  {
    usageFlags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }
  if (desc.usage & BufferUsageBits_Vertex)
  {
    usageFlags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }
  if (desc.usage & BufferUsageBits_Uniform)
  {
    usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }

  if (desc.usage & BufferUsageBits_Storage)
  {
    usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }

  if (desc.usage & BufferUsageBits_Indirect)
  {
    usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }

  if (desc.usage & BufferUsageBits_ShaderBindingTable)
  {
    usageFlags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }

  if (desc.usage & BufferUsageBits_AccelStructBuildInputReadOnly)
  {
    usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }

  if (desc.usage & BufferUsageBits_AccelStructStorage)
  {
    usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR;
  }
  ASSERT_MSG(usageFlags, "Invalid buffer usage");

  const VkMemoryPropertyFlags memFlags = storageTypeToVkMemoryPropertyFlags(desc.storage);

  Result result;
  BufferHandle handle = createBuffer(desc.size, usageFlags, memFlags, &result, desc.debugName);

  if (!VERIFY(result.isOk()))
  {
    Result::setResult(outResult, result);
    return {};
  }

  if (desc.data)
  {
    upload(handle, desc.data, desc.size, 0);
  }

  Result::setResult(outResult, Result());

  return {this, handle};
}

vk::Holder<vk::QueryPoolHandle> vk::VulkanContext::createQueryPool(uint32_t numQueries, const char* debugName, Result* outResult)
{
  PROFILER_FUNCTION();

  const VkQueryPoolCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, .flags = 0, .queryType = VK_QUERY_TYPE_TIMESTAMP, .queryCount = numQueries, .pipelineStatistics = 0,};

  VkQueryPool queryPool = VK_NULL_HANDLE;
  VK_ASSERT(vkCreateQueryPool(vkDevice_, &createInfo, 0, &queryPool));

  if (!queryPool)
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "Cannot create QueryPool"));
    return {};
  }

  if (debugName && *debugName)
  {
    setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)queryPool, debugName);
  }

  QueryPoolHandle handle = queriesPool_.create(std::move(queryPool));

  return {this, handle};
}

vk::Holder<vk::AccelStructHandle> vk::VulkanContext::createAccelerationStructure(const AccelStructDesc& desc, Result* outResult)
{
  PROFILER_FUNCTION();

  if (!VERIFY(hasAccelerationStructure_))
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "VK_KHR_acceleration_structure is not enabled"));
    return {};
  }

  Result result;

  AccelStructHandle handle;

  switch (desc.type)
  {
    case AccelStructType::BLAS:
      handle = createBLAS(desc, &result);
      break;
    case AccelStructType::TLAS:
      handle = createTLAS(desc, &result);
      break;
    default: ASSERT_MSG(false, "Invalid acceleration structure type");
      Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "Invalid acceleration structure type"));
      return {};
  }

  if (!VERIFY(result.isOk() && handle.valid()))
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "Cannot create AccelerationStructure"));
    return {};
  }

  Result::setResult(outResult, result);

  awaitingCreation_ = true;

  return {this, handle};
}

vk::Holder<vk::SamplerHandle> vk::VulkanContext::createSampler(const SamplerStateDesc& desc, Result* outResult)
{
  PROFILER_FUNCTION();

  Result result;

  const VkSamplerCreateInfo info = samplerStateDescToVkSamplerCreateInfo(desc, getVkPhysicalDeviceProperties().limits);

  SamplerHandle handle = createSampler(info, &result, Format::Invalid, desc.debugName);

  if (!VERIFY(result.isOk()))
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "Cannot create Sampler"));
    return {};
  }

  Result::setResult(outResult, result);

  return {this, handle};
}

vk::Holder<vk::TextureHandle> vk::VulkanContext::createTexture(const TextureDesc& requestedDesc, const char* debugName, Result* outResult)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_CREATE);

  TextureDesc desc(requestedDesc);

  if (debugName && *debugName)
  {
    desc.debugName = debugName;
  }

  auto getClosestDepthStencilFormat = [this](Format desiredFormat) -> VkFormat
  {
    // Get a list of compatible depth formats for a given desired format.
    // The list will contain depth format that are ordered from most to least closest
    const std::vector<VkFormat> compatibleDepthStencilFormatList = getCompatibleDepthStencilFormats(desiredFormat);

    // check if any of the format in compatible list is supported
    for (VkFormat format : compatibleDepthStencilFormatList)
    {
      if (std::find(deviceDepthFormats_.cbegin(), deviceDepthFormats_.cend(), format) != deviceDepthFormats_.cend())
      {
        return format;
      }
    }

    // no matching found, choose the first supported format
    return !deviceDepthFormats_.empty() ? deviceDepthFormats_[0] : VK_FORMAT_D24_UNORM_S8_UINT;
  };

  const VkFormat vkFormat = isDepthOrStencilFormat(desc.format) ? getClosestDepthStencilFormat(desc.format) : formatToVkFormat(desc.format);

  ASSERT_MSG(vkFormat != VK_FORMAT_UNDEFINED, "Invalid VkFormat value");

  const TextureType type = desc.type;
  if (!VERIFY(type == TextureType::Tex2D || type == TextureType::TexCube || type == TextureType::Tex3D))
  {
    ASSERT_MSG(false, "Only 2D, 3D and Cube textures are supported");
    Result::setResult(outResult, Result::Code::RuntimeError);
    return {};
  }

  if (desc.numMipLevels == 0)
  {
    ASSERT_MSG(false, "The number of mip levels specified must be greater than 0");
    desc.numMipLevels = 1;
  }

  if (desc.numSamples > 1 && desc.numMipLevels != 1)
  {
    ASSERT_MSG(false, "The number of mip levels for multisampled images should be 1");
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "The number of mip-levels for multisampled images should be 1");
    return {};
  }

  if (desc.numSamples > 1 && type == TextureType::Tex3D)
  {
    ASSERT_MSG(false, "Multisampled 3D images are not supported");
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Multisampled 3D images are not supported");
    return {};
  }

  if (!VERIFY(desc.numMipLevels <= vk::calcNumMipLevels(desc.dimensions.width, desc.dimensions.height)))
  {
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "The number of specified mip-levels is greater than the maximum possible " "number of mip-levels.");
    return {};
  }

  if (desc.usage == 0)
  {
    ASSERT_MSG(false, "Texture usage flags are not set");
    desc.usage = TextureUsageBits_Sampled;
  }

  /* Use staging device to transfer data into the image when the storage is private to the device */
  VkImageUsageFlags usageFlags = (desc.storage == StorageType::Device) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;

  if (desc.usage & TextureUsageBits_Sampled)
  {
    usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }
  if (desc.usage & TextureUsageBits_Storage)
  {
    ASSERT_MSG(desc.numSamples <= 1, "Storage images cannot be multisampled");
    usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
  }
  if (desc.usage & TextureUsageBits_Attachment)
  {
    usageFlags |= isDepthOrStencilFormat(desc.format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc.storage == StorageType::Memoryless)
    {
      usageFlags |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }
  }

  if (desc.storage != StorageType::Memoryless)
  {
    // For now, always set this flag so we can read it back
    usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  ASSERT_MSG(usageFlags != 0, "Invalid usage flags");

  const VkMemoryPropertyFlags memFlags = storageTypeToVkMemoryPropertyFlags(desc.storage);

  const bool hasDebugName = desc.debugName && *desc.debugName;

  char debugNameImage[256] = {0};
  char debugNameImageView[256] = {0};

  if (hasDebugName)
  {
    snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: %s", desc.debugName);
    snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: %s", desc.debugName);
  }

  VkImageCreateFlags vkCreateFlags = 0;
  VkImageViewType vkImageViewType;
  VkImageType vkImageType;
  VkSampleCountFlagBits vkSamples = VK_SAMPLE_COUNT_1_BIT;
  uint32_t numLayers = desc.numLayers;
  switch (desc.type)
  {
    case TextureType::Tex2D:
      vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
      vkImageType = VK_IMAGE_TYPE_2D;
      vkSamples = getVulkanSampleCountFlags(desc.numSamples, getFramebufferMSAABitMask());
      break;
    case TextureType::Tex3D:
      vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
      vkImageType = VK_IMAGE_TYPE_3D;
      break;
    case TextureType::TexCube:
      vkImageViewType = numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
      vkImageType = VK_IMAGE_TYPE_2D;
      vkCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
      numLayers *= 6;
      break;
    default: ASSERT_MSG(false, "Code should NOT be reached");
      Result::setResult(outResult, Result::Code::RuntimeError, "Unsupported texture type");
      return {};
  }

  const VkExtent3D vkExtent{desc.dimensions.width, desc.dimensions.height, desc.dimensions.depth};
  const uint32_t numLevels = desc.numMipLevels;

  if (!VERIFY(validateImageLimits(vkImageType, vkSamples, vkExtent, getVkPhysicalDeviceProperties().limits, outResult)))
  {
    return {};
  }

  ASSERT_MSG(numLevels > 0, "The image must contain at least one mip-level");
  ASSERT_MSG(numLayers > 0, "The image must contain at least one layer");
  ASSERT_MSG(vkSamples > 0, "The image must contain at least one sample");
  ASSERT(vkExtent.width > 0);
  ASSERT(vkExtent.height > 0);
  ASSERT(vkExtent.depth > 0);

  VulkanImage image = {.vkUsageFlags_ = usageFlags, .vkExtent_ = vkExtent, .vkType_ = vkImageType, .vkImageFormat_ = vkFormat, .vkSamples_ = vkSamples, .numLevels_ = numLevels, .numLayers_ = numLayers, .isDepthFormat_ = VulkanImage::isDepthFormat(vkFormat), .isStencilFormat_ = VulkanImage::isStencilFormat(vkFormat),};

  if (hasDebugName)
  {
    // store debug name
    snprintf(image.debugName_, sizeof(image.debugName_) - 1, "%s", desc.debugName);
  }

  const uint32_t numPlanes = getNumImagePlanes(desc.format);
  const bool isDisjoint = numPlanes > 1;

  if (isDisjoint)
  {
    // some constraints for multiplanar image formats
    ASSERT(vkImageType == VK_IMAGE_TYPE_2D);
    ASSERT(vkSamples == VK_SAMPLE_COUNT_1_BIT);
    ASSERT(numLayers == 1);
    ASSERT(numLevels == 1);
    vkCreateFlags |= VK_IMAGE_CREATE_DISJOINT_BIT | VK_IMAGE_CREATE_ALIAS_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    awaitingNewImmutableSamplers_ = true;
  }

  const VkImageCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .pNext = nullptr, .flags = vkCreateFlags, .imageType = vkImageType, .format = vkFormat, .extent = vkExtent, .mipLevels = numLevels, .arrayLayers = numLayers, .samples = vkSamples, .tiling = VK_IMAGE_TILING_OPTIMAL, .usage = usageFlags, .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 0, .pQueueFamilyIndices = nullptr, .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,};

  if (numPlanes == 1)
  {
    VmaAllocationCreateInfo vmaAllocInfo = {.usage = memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? VMA_MEMORY_USAGE_CPU_TO_GPU : VMA_MEMORY_USAGE_AUTO,};

    VkResult result = vmaCreateImage((VmaAllocator)getVmaAllocator(), &ci, &vmaAllocInfo, &image.vkImage_, &image.vmaAllocation_, nullptr);

    if (!VERIFY(result == VK_SUCCESS))
    {
      LOG_WARNING("Failed: error result: {}, memflags: {},  imageformat: {}\n", string_VkResult(result), string_VkMemoryPropertyFlags(memFlags), string_VkFormat(image.vkImageFormat_));
      Result::setResult(outResult, Result::Code::RuntimeError, "vmaCreateImage() failed");
      return {};
    }
    // handle memory-mapped buffers
    if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
      vmaMapMemory((VmaAllocator)getVmaAllocator(), image.vmaAllocation_, &image.mappedPtr_);
    }
  }

  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_IMAGE, (uint64_t)image.vkImage_, debugNameImage));

  // Get physical device's properties for the image's format
  vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice_, image.vkImageFormat_, &image.vkFormatProperties_);

  VkImageAspectFlags aspect = 0;
  if (image.isDepthFormat_ || image.isStencilFormat_)
  {
    if (image.isDepthFormat_)
    {
      aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (image.isStencilFormat_)
    {
      aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else
  {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  const VkComponentMapping mapping = {.r = VkComponentSwizzle(desc.swizzle.r), .g = VkComponentSwizzle(desc.swizzle.g), .b = VkComponentSwizzle(desc.swizzle.b), .a = VkComponentSwizzle(desc.swizzle.a),};

  const VkSamplerYcbcrConversionInfo* ycbcrInfo = isDisjoint ? getOrCreateYcbcrConversionInfo(desc.format) : nullptr;

  image.imageView_ = image.createImageView(vkDevice_, vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, mapping, ycbcrInfo, debugNameImageView);

  if (image.vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT)
  {
    if (!desc.swizzle.identity())
    {
      // use identity swizzle for storage images
      image.imageViewStorage_ = image.createImageView(vkDevice_, vkImageViewType, vkFormat, aspect, 0, VK_REMAINING_MIP_LEVELS, 0, numLayers, {}, ycbcrInfo, debugNameImageView);
      ASSERT(image.imageViewStorage_ != VK_NULL_HANDLE);
    }
  }

  if (!VERIFY(image.imageView_ != VK_NULL_HANDLE))
  {
    Result::setResult(outResult, Result::Code::RuntimeError, "Cannot create VkImageView");
    return {};
  }

  TextureHandle handle = texturesPool_.create(std::move(image));

  awaitingCreation_ = true;

  if (desc.data)
  {
    ASSERT(desc.type == TextureType::Tex2D || desc.type == TextureType::TexCube);
    ASSERT(desc.dataNumMipLevels <= desc.numMipLevels);
    const uint32_t num_layers = desc.type == TextureType::TexCube ? 6 : 1;
    Result res = upload(handle, {.dimensions = desc.dimensions, .numLayers = num_layers, .numMipLevels = desc.dataNumMipLevels}, desc.data);
    if (!res.isOk())
    {
      Result::setResult(outResult, res);
      return {};
    }
    if (desc.generateMipmaps)
    {
      this->generateMipmap(handle);
    }
  }

  Result::setResult(outResult, Result());

  return {this, handle};
}

vk::Holder<vk::TextureHandle> vk::VulkanContext::createTextureView(TextureHandle texture, const TextureViewDesc& desc, const char* debugName, Result* outResult)
{
  if (!texture)
  {
    ASSERT(texture.valid());
    return {};
  }

  // make a copy and make it non-owning
  VulkanImage image = *texturesPool_.get(texture);
  image.isOwningVkImage_ = false;

  // drop all existing image views - they belong to the base image
  memset(&image.imageViewStorage_, 0, sizeof(image.imageViewStorage_));
  memset(&image.imageViewForFramebuffer_, 0, sizeof(image.imageViewForFramebuffer_));

  VkImageAspectFlags aspect = 0;
  if (image.isDepthFormat_ || image.isStencilFormat_)
  {
    if (image.isDepthFormat_)
    {
      aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (image.isStencilFormat_)
    {
      aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else
  {
    aspect = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  VkImageViewType vkImageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
  switch (desc.type)
  {
    case TextureType::Tex2D:
      vkImageViewType = desc.numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
      break;
    case TextureType::Tex3D:
      vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
      break;
    case TextureType::TexCube:
      vkImageViewType = desc.numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
      break;
    default: ASSERT_MSG(false, "Code should NOT be reached");
      Result::setResult(outResult, Result::Code::RuntimeError, "Unsupported texture view type");
      return {};
  }

  const VkComponentMapping mapping = {.r = VkComponentSwizzle(desc.swizzle.r), .g = VkComponentSwizzle(desc.swizzle.g), .b = VkComponentSwizzle(desc.swizzle.b), .a = VkComponentSwizzle(desc.swizzle.a),};

  ASSERT_MSG(vk::getNumImagePlanes(image.vkImageFormat_) == 1, "Unsupported multiplanar image");

  image.imageView_ = image.createImageView(vkDevice_, vkImageViewType, image.vkImageFormat_, aspect, desc.mipLevel, desc.numMipLevels, desc.layer, desc.numLayers, mapping, nullptr, debugName);

  if (!VERIFY(image.imageView_ != VK_NULL_HANDLE))
  {
    Result::setResult(outResult, Result::Code::RuntimeError, "Cannot create VkImageView");
    return {};
  }

  if (image.vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT)
  {
    if (!desc.swizzle.identity())
    {
      // use identity swizzle for storage images
      image.imageViewStorage_ = image.createImageView(vkDevice_, vkImageViewType, image.vkImageFormat_, aspect, desc.mipLevel, desc.numMipLevels, desc.layer, desc.numLayers, {}, nullptr, debugName);
      ASSERT(image.imageViewStorage_ != VK_NULL_HANDLE);
    }
  }

  TextureHandle handle = texturesPool_.create(std::move(image));

  awaitingCreation_ = true;

  return {this, handle};
}

vk::AccelStructHandle vk::VulkanContext::createBLAS(const AccelStructDesc& desc, Result* outResult)
{
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
  getBuildInfoBLAS(desc, accelerationStructureGeometry, accelerationStructureBuildSizesInfo);

  char debugNameBuffer[256] = {0};
  if (desc.debugName)
  {
    snprintf(debugNameBuffer, sizeof(debugNameBuffer) - 1, "Buffer: %s", desc.debugName);
  }
  AccelerationStructure accelStruct = {.buildRangeInfo = {.primitiveCount = desc.buildRange.primitiveCount, .primitiveOffset = desc.buildRange.primitiveOffset, .firstVertex = desc.buildRange.firstVertex, .transformOffset = desc.buildRange.transformOffset,}, .buffer = createBuffer({.usage = BufferUsageBits_AccelStructStorage, .storage = StorageType::Device, .size = accelerationStructureBuildSizesInfo.accelerationStructureSize, .debugName = debugNameBuffer,}, nullptr, outResult),};
  const VkAccelerationStructureCreateInfoKHR ciAccelerationStructure = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, .buffer = getVkBuffer(this, accelStruct.buffer), .size = accelerationStructureBuildSizesInfo.accelerationStructureSize, .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,};
  VK_ASSERT(vkCreateAccelerationStructureKHR(vkDevice_, &ciAccelerationStructure, nullptr, &accelStruct.vkHandle));

  Holder<BufferHandle> scratchBuffer = createBuffer({.usage = BufferUsageBits_Storage, .storage = StorageType::Device, .size = accelerationStructureBuildSizesInfo.buildScratchSize, .debugName = "Buffer: BLAS scratch",}, nullptr, outResult);

  const VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, .flags = buildFlagsToVkBuildAccelerationStructureFlags(desc.buildFlags), .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, .dstAccelerationStructure = accelStruct.vkHandle, .geometryCount = 1, .pGeometries = &accelerationStructureGeometry, .scratchData = {.deviceAddress = gpuAddress(scratchBuffer)}};

  const VkAccelerationStructureBuildRangeInfoKHR* accelerationBuildStructureRangeInfos[] = {&accelStruct.buildRangeInfo};

  ICommandBuffer& buffer = acquireCommandBuffer();
  vkCmdBuildAccelerationStructuresKHR(getVkCommandBuffer(buffer), 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos);
  wait(submit(buffer, {}));

  const VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, .accelerationStructure = accelStruct.vkHandle,};
  accelStruct.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vkDevice_, &accelerationDeviceAddressInfo);

  return accelStructuresPool_.create(std::move(accelStruct));
}

vk::AccelStructHandle vk::VulkanContext::createTLAS(const AccelStructDesc& desc, Result* outResult)
{
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
  getBuildInfoTLAS(desc, accelerationStructureGeometry, accelerationStructureBuildSizesInfo);

  char debugNameBuffer[256] = {0};
  if (desc.debugName)
  {
    snprintf(debugNameBuffer, sizeof(debugNameBuffer) - 1, "Buffer: %s", desc.debugName);
  }
  AccelerationStructure accelStruct = {.isTLAS = true, .buildRangeInfo = {.primitiveCount = desc.buildRange.primitiveCount, .primitiveOffset = desc.buildRange.primitiveOffset, .firstVertex = desc.buildRange.firstVertex, .transformOffset = desc.buildRange.transformOffset,}, .buffer = createBuffer({.usage = BufferUsageBits_AccelStructStorage, .storage = StorageType::Device, .size = accelerationStructureBuildSizesInfo.accelerationStructureSize, .debugName = debugNameBuffer,}, nullptr, outResult),};

  const VkAccelerationStructureCreateInfoKHR ciAccelerationStructure = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, .buffer = getVkBuffer(this, accelStruct.buffer), .size = accelerationStructureBuildSizesInfo.accelerationStructureSize, .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,};
  vkCreateAccelerationStructureKHR(vkDevice_, &ciAccelerationStructure, nullptr, &accelStruct.vkHandle);

  Holder<BufferHandle> scratchBuffer = createBuffer({.usage = BufferUsageBits_Storage, .storage = StorageType::Device, .size = accelerationStructureBuildSizesInfo.buildScratchSize, .debugName = "Buffer: TLAS scratch",}, nullptr, outResult);

  const VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, .flags = buildFlagsToVkBuildAccelerationStructureFlags(desc.buildFlags), .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR, .dstAccelerationStructure = accelStruct.vkHandle, .geometryCount = 1, .pGeometries = &accelerationStructureGeometry, .scratchData = {.deviceAddress = gpuAddress(scratchBuffer)}};
  if (desc.buildFlags & AccelStructBuildFlagBits_AllowUpdate)
  {
    // Store scratch buffer for future updates
    accelStruct.scratchBuffer = std::move(scratchBuffer);
  }

  const VkAccelerationStructureBuildRangeInfoKHR* accelerationBuildStructureRangeInfos[] = {&accelStruct.buildRangeInfo};

  ICommandBuffer& buffer = acquireCommandBuffer();
  vkCmdBuildAccelerationStructuresKHR(getVkCommandBuffer(buffer), 1, &accelerationBuildGeometryInfo, accelerationBuildStructureRangeInfos);
  wait(submit(buffer, {}));

  const VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, .accelerationStructure = accelStruct.vkHandle,};
  accelStruct.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(vkDevice_, &accelerationDeviceAddressInfo);

  return accelStructuresPool_.create(std::move(accelStruct));
}

static_assert(1 << (sizeof(vk::Format) * 8) <= (sizeof(vk::VulkanContextImpl::ycbcrConversionData_) / sizeof((vk::VulkanContextImpl::ycbcrConversionData_)[0])), "There aren't enough elements in `ycbcrConversionData_` to be accessed by vk::Format");

VkSampler vk::VulkanContext::getOrCreateYcbcrSampler(Format format)
{
  const VkSamplerYcbcrConversionInfo* info = getOrCreateYcbcrConversionInfo(format);

  if (!info)
  {
    return VK_NULL_HANDLE;
  }

  return *samplersPool_.get(pimpl_->ycbcrConversionData_[(uint8_t)format].sampler);
}

const VkSamplerYcbcrConversionInfo* vk::VulkanContext::getOrCreateYcbcrConversionInfo(Format format)
{
  if (pimpl_->ycbcrConversionData_[(uint8_t)format].info.sType)
  {
    return &pimpl_->ycbcrConversionData_[(uint8_t)format].info;
  }

  if (!VERIFY(vkFeatures11_.samplerYcbcrConversion))
  {
    ASSERT_MSG(false, "Ycbcr samplers are not supported");
    return nullptr;
  }

  const VkFormat vkFormat = formatToVkFormat(format);

  VkFormatProperties props;
  vkGetPhysicalDeviceFormatProperties(getVkPhysicalDevice(), vkFormat, &props);

  const bool cosited = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT) != 0;
  const bool midpoint = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT) != 0;

  if (!VERIFY(cosited || midpoint))
  {
    ASSERT_MSG(cosited || midpoint, "Unsupported Ycbcr feature");
    return nullptr;
  }

  const VkSamplerYcbcrConversionCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO, .format = vkFormat, .ycbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709, .ycbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_FULL, .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,}, .xChromaOffset = midpoint ? VK_CHROMA_LOCATION_MIDPOINT : VK_CHROMA_LOCATION_COSITED_EVEN, .yChromaOffset = midpoint ? VK_CHROMA_LOCATION_MIDPOINT : VK_CHROMA_LOCATION_COSITED_EVEN, .chromaFilter = VK_FILTER_LINEAR, .forceExplicitReconstruction = VK_FALSE,};

  VkSamplerYcbcrConversionInfo info = {.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO, .pNext = nullptr,};
  vkCreateSamplerYcbcrConversion(vkDevice_, &ci, nullptr, &info.conversion);

  // check properties
  VkSamplerYcbcrConversionImageFormatProperties samplerYcbcrConversionImageFormatProps = {.sType = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES,};
  VkImageFormatProperties2 imageFormatProps = {.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2, .pNext = &samplerYcbcrConversionImageFormatProps,};
  const VkPhysicalDeviceImageFormatInfo2 imageFormatInfo = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2, .format = vkFormat, .type = VK_IMAGE_TYPE_2D, .tiling = VK_IMAGE_TILING_OPTIMAL, .usage = VK_IMAGE_USAGE_SAMPLED_BIT, .flags = VK_IMAGE_CREATE_DISJOINT_BIT,};
  vkGetPhysicalDeviceImageFormatProperties2(getVkPhysicalDevice(), &imageFormatInfo, &imageFormatProps);

  ASSERT(samplerYcbcrConversionImageFormatProps.combinedImageSamplerDescriptorCount <= 3);

  const VkSamplerCreateInfo cinfo = samplerStateDescToVkSamplerCreateInfo({}, getVkPhysicalDeviceProperties().limits);

  pimpl_->ycbcrConversionData_[(uint8_t)format].info = info;
  pimpl_->ycbcrConversionData_[(uint8_t)format].sampler = {this, this->createSampler(cinfo, nullptr, format, "YUV sampler")};
  pimpl_->numYcbcrSamplers_++;
  awaitingNewImmutableSamplers_ = true;

  return &pimpl_->ycbcrConversionData_[(uint8_t)format].info;
}

VkPipeline vk::VulkanContext::getVkPipeline(RenderPipelineHandle handle, uint32_t viewMask)
{
  RenderPipelineState* rps = renderPipelinesPool_.get(handle);

  if (!rps)
  {
    return VK_NULL_HANDLE;
  }

  if (rps->lastVkDescriptorSetLayout_ != vkDSL_ || rps->viewMask_ != viewMask)
  {
    deferredTask(std::packaged_task<void()>([device = getVkDevice(), pipeline = rps->pipeline_]()
    {
      vkDestroyPipeline(device, pipeline, nullptr);
    }));
    deferredTask(std::packaged_task<void()>([device = getVkDevice(), layout = rps->pipelineLayout_]()
    {
      vkDestroyPipelineLayout(device, layout, nullptr);
    }));
    rps->pipeline_ = VK_NULL_HANDLE;
    rps->lastVkDescriptorSetLayout_ = vkDSL_;
  }

  if (rps->pipeline_ != VK_NULL_HANDLE)
  {
    return rps->pipeline_;
  }

  // build a new Vulkan pipeline

  VkPipelineLayout layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;

  const RenderPipelineDesc& desc = rps->desc_;

  const uint32_t numColorAttachments = rps->desc_.getNumColorAttachments();

  // Not all attachments are valid. We need to create color blend attachments only for active attachments
  VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[MAX_COLOR_ATTACHMENTS] = {};
  VkFormat colorAttachmentFormats[MAX_COLOR_ATTACHMENTS] = {};

  for (uint32_t i = 0; i != numColorAttachments; i++)
  {
    const ColorAttachment& attachment = desc.color[i];
    ASSERT(attachment.format != Format::Invalid);
    colorAttachmentFormats[i] = formatToVkFormat(attachment.format);
    if (!attachment.blendEnabled)
    {
      colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{.blendEnable = VK_FALSE, .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, .colorBlendOp = VK_BLEND_OP_ADD, .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, .alphaBlendOp = VK_BLEND_OP_ADD, .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,};
    }
    else
    {
      colorBlendAttachmentStates[i] = VkPipelineColorBlendAttachmentState{.blendEnable = VK_TRUE, .srcColorBlendFactor = blendFactorToVkBlendFactor(attachment.srcRGBBlendFactor), .dstColorBlendFactor = blendFactorToVkBlendFactor(attachment.dstRGBBlendFactor), .colorBlendOp = blendOpToVkBlendOp(attachment.rgbBlendOp), .srcAlphaBlendFactor = blendFactorToVkBlendFactor(attachment.srcAlphaBlendFactor), .dstAlphaBlendFactor = blendFactorToVkBlendFactor(attachment.dstAlphaBlendFactor), .alphaBlendOp = blendOpToVkBlendOp(attachment.alphaBlendOp), .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,};
    }
  }

  const ShaderModuleState* vertModule = shaderModulesPool_.get(desc.smVert);
  const ShaderModuleState* tescModule = shaderModulesPool_.get(desc.smTesc);
  const ShaderModuleState* teseModule = shaderModulesPool_.get(desc.smTese);
  const ShaderModuleState* geomModule = shaderModulesPool_.get(desc.smGeom);
  const ShaderModuleState* fragModule = shaderModulesPool_.get(desc.smFrag);
  const ShaderModuleState* taskModule = shaderModulesPool_.get(desc.smTask);
  const ShaderModuleState* meshModule = shaderModulesPool_.get(desc.smMesh);

  ASSERT(vertModule || meshModule);
  ASSERT(fragModule);

  if (tescModule || teseModule || desc.patchControlPoints)
  {
    ASSERT_MSG(tescModule && teseModule, "Both tessellation control and evaluation shaders should be provided");
    ASSERT(desc.patchControlPoints > 0 && desc.patchControlPoints <= vkPhysicalDeviceProperties2_.properties.limits.maxTessellationPatchSize);
  }

  const VkPipelineVertexInputStateCreateInfo ciVertexInputState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, .vertexBindingDescriptionCount = rps->numBindings_, .pVertexBindingDescriptions = rps->numBindings_ ? rps->vkBindings_ : nullptr, .vertexAttributeDescriptionCount = rps->numAttributes_, .pVertexAttributeDescriptions = rps->numAttributes_ ? rps->vkAttributes_ : nullptr,};

  VkSpecializationMapEntry entries[SpecializationConstantDesc::SPECIALIZATION_CONSTANTS_MAX] = {};

  const VkSpecializationInfo si = getPipelineShaderStageSpecializationInfo(desc.specInfo, entries);

  // create pipeline layout
  {
#define UPDATE_PUSH_CONSTANT_SIZE(sm, bit)                                    \
    if (sm) {                                                                   \
      pushConstantsSize = (std::max)(pushConstantsSize, sm->pushConstantsSize); \
      rps->shaderStageFlags_ |= bit;                                            \
    }
    rps->shaderStageFlags_ = 0;
    uint32_t pushConstantsSize = 0;
    UPDATE_PUSH_CONSTANT_SIZE(vertModule, VK_SHADER_STAGE_VERTEX_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(tescModule, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(teseModule, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(geomModule, VK_SHADER_STAGE_GEOMETRY_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(fragModule, VK_SHADER_STAGE_FRAGMENT_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(taskModule, VK_SHADER_STAGE_TASK_BIT_EXT);
    UPDATE_PUSH_CONSTANT_SIZE(meshModule, VK_SHADER_STAGE_MESH_BIT_EXT);
#undef UPDATE_PUSH_CONSTANT_SIZE

    // maxPushConstantsSize is guaranteed to be at least 128 bytes
    // https://www.khronos.org/registry/vulkan/specs/1.3/html/vkspec.html#features-limits
    // Table 32. Required Limits
    const VkPhysicalDeviceLimits& limits = getVkPhysicalDeviceProperties().limits;
    if (!VERIFY(pushConstantsSize <= limits.maxPushConstantsSize))
    {
      LOG_WARNING("Push constants size exceeded {} (max {} bytes)", pushConstantsSize, limits.maxPushConstantsSize);
    }

    // duplicate for MoltenVK
    const VkDescriptorSetLayout dsls[] = {vkDSL_, vkDSL_, vkDSL_, vkDSL_};
    const VkPushConstantRange range = {.stageFlags = rps->shaderStageFlags_, .offset = 0, .size = pushConstantsSize,};
    const VkPipelineLayoutCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = (uint32_t)std::size(dsls), .pSetLayouts = dsls, .pushConstantRangeCount = pushConstantsSize ? 1u : 0u, .pPushConstantRanges = pushConstantsSize ? &range : nullptr,};
    VK_ASSERT(vkCreatePipelineLayout(vkDevice_, &ci, nullptr, &layout));
    char pipelineLayoutName[256] = {0};
    if (rps->desc_.debugName)
    {
      snprintf(pipelineLayoutName, sizeof(pipelineLayoutName) - 1, "Pipeline Layout: %s", rps->desc_.debugName);
    }
    VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)layout, pipelineLayoutName));
  }

  VulkanPipelineBuilder()
      // from Vulkan 1.0
     .dynamicState(VK_DYNAMIC_STATE_VIEWPORT).dynamicState(VK_DYNAMIC_STATE_SCISSOR).dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS).dynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
      // from Vulkan 1.3 or VK_EXT_extended_dynamic_state
     .dynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE).dynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE).dynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)
      // from Vulkan 1.3 or VK_EXT_extended_dynamic_state2
     .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE).primitiveTopology(topologyToVkPrimitiveTopology(desc.topology)).rasterizationSamples(getVulkanSampleCountFlags(desc.samplesCount, getFramebufferMSAABitMask()), desc.minSampleShading).polygonMode(polygonModeToVkPolygonMode(desc.polygonMode)).stencilStateOps(VK_STENCIL_FACE_FRONT_BIT, stencilOpToVkStencilOp(desc.frontFaceStencil.stencilFailureOp), stencilOpToVkStencilOp(desc.frontFaceStencil.depthStencilPassOp), stencilOpToVkStencilOp(desc.frontFaceStencil.depthFailureOp), compareOpToVkCompareOp(desc.frontFaceStencil.stencilCompareOp)).stencilStateOps(VK_STENCIL_FACE_BACK_BIT, stencilOpToVkStencilOp(desc.backFaceStencil.stencilFailureOp), stencilOpToVkStencilOp(desc.backFaceStencil.depthStencilPassOp), stencilOpToVkStencilOp(desc.backFaceStencil.depthFailureOp), compareOpToVkCompareOp(desc.backFaceStencil.stencilCompareOp)).stencilMasks(VK_STENCIL_FACE_FRONT_BIT, 0xFF, desc.frontFaceStencil.writeMask, desc.frontFaceStencil.readMask).stencilMasks(VK_STENCIL_FACE_BACK_BIT, 0xFF, desc.backFaceStencil.writeMask, desc.backFaceStencil.readMask).shaderStage(taskModule ? getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, taskModule->sm, desc.entryPointTask, &si) : VkPipelineShaderStageCreateInfo{.module = VK_NULL_HANDLE}).shaderStage(meshModule ? getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT, meshModule->sm, desc.entryPointMesh, &si) : getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertModule->sm, desc.entryPointVert, &si)).shaderStage(getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule->sm, desc.entryPointFrag, &si)).shaderStage(tescModule ? getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, tescModule->sm, desc.entryPointTesc, &si) : VkPipelineShaderStageCreateInfo{.module = VK_NULL_HANDLE}).shaderStage(teseModule ? getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, teseModule->sm, desc.entryPointTese, &si) : VkPipelineShaderStageCreateInfo{.module = VK_NULL_HANDLE}).shaderStage(geomModule ? getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geomModule->sm, desc.entryPointGeom, &si) : VkPipelineShaderStageCreateInfo{.module = VK_NULL_HANDLE}).cullMode(cullModeToVkCullMode(desc.cullMode)).frontFace(windingModeToVkFrontFace(desc.frontFaceWinding)).vertexInputState(ciVertexInputState).colorAttachments(colorBlendAttachmentStates, colorAttachmentFormats, numColorAttachments).depthAttachmentFormat(formatToVkFormat(desc.depthFormat)).stencilAttachmentFormat(formatToVkFormat(desc.stencilFormat)).patchControlPoints(desc.patchControlPoints).build(vkDevice_, pipelineCache_, layout, &pipeline, desc.debugName);

  rps->pipeline_ = pipeline;
  rps->pipelineLayout_ = layout;

  return pipeline;
}

VkPipeline vk::VulkanContext::getVkPipeline(RayTracingPipelineHandle handle)
{
  RayTracingPipelineState* rtps = rayTracingPipelinesPool_.get(handle);

  if (!rtps)
  {
    return VK_NULL_HANDLE;
  }

  if (rtps->lastVkDescriptorSetLayout_ != vkDSL_)
  {
    deferredTask(std::packaged_task<void()>([device = vkDevice_, pipeline = rtps->pipeline_]()
    {
      vkDestroyPipeline(device, pipeline, nullptr);
    }));
    deferredTask(std::packaged_task<void()>([device = vkDevice_, layout = rtps->pipelineLayout_]()
    {
      vkDestroyPipelineLayout(device, layout, nullptr);
    }));
    rtps->pipeline_ = VK_NULL_HANDLE;
    rtps->pipelineLayout_ = VK_NULL_HANDLE;
    rtps->lastVkDescriptorSetLayout_ = vkDSL_;
  }

  if (rtps->pipeline_)
  {
    return rtps->pipeline_;
  }

  checkAndUpdateDescriptorSets();

  // build a new Vulkan ray tracing pipeline
  const RayTracingPipelineDesc& desc = rtps->desc_;

  const ShaderModuleState* moduleRGen[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  const ShaderModuleState* moduleAHit[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  const ShaderModuleState* moduleCHit[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  const ShaderModuleState* moduleMiss[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  const ShaderModuleState* moduleIntr[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  const ShaderModuleState* moduleCall[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i)
  {
    if (desc.smRayGen[i])
    {
      moduleRGen[i] = shaderModulesPool_.get(desc.smRayGen[i]);
    }
    if (desc.smAnyHit[i])
    {
      moduleAHit[i] = shaderModulesPool_.get(desc.smAnyHit[i]);
    }
    if (desc.smClosestHit[i])
    {
      moduleCHit[i] = shaderModulesPool_.get(desc.smClosestHit[i]);
    }
    if (desc.smMiss[i])
    {
      moduleMiss[i] = shaderModulesPool_.get(desc.smMiss[i]);
    }
    if (desc.smIntersection[i])
    {
      moduleIntr[i] = shaderModulesPool_.get(desc.smIntersection[i]);
    }
    if (desc.smCallable[i])
    {
      moduleCall[i] = shaderModulesPool_.get(desc.smCallable[i]);
    }
  }

  ASSERT(moduleRGen);

  // create pipeline layout
  {
#define UPDATE_PUSH_CONSTANT_SIZE(sm, bit)                                       \
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i) {              \
    if (sm[i] && sm[i]->pushConstantsSize) {                                     \
      pushConstantsSize = (std::max)(pushConstantsSize, sm[i]->pushConstantsSize); \
      rtps->shaderStageFlags_ |= bit;                                            \
    }                                                                            \
  }
    rtps->shaderStageFlags_ = 0;
    uint32_t pushConstantsSize = 0;
    UPDATE_PUSH_CONSTANT_SIZE(moduleRGen, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    UPDATE_PUSH_CONSTANT_SIZE(moduleAHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
    UPDATE_PUSH_CONSTANT_SIZE(moduleCHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
    UPDATE_PUSH_CONSTANT_SIZE(moduleMiss, VK_SHADER_STAGE_MISS_BIT_KHR);
    UPDATE_PUSH_CONSTANT_SIZE(moduleIntr, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
    UPDATE_PUSH_CONSTANT_SIZE(moduleCall, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
#undef UPDATE_PUSH_CONSTANT_SIZE

    // maxPushConstantsSize is guaranteed to be at least 128 bytes
    // https://www.khronos.org/registry/vulkan/specs/1.3/html/vkspec.html#features-limits
    // Table 32. Required Limits
    const VkPhysicalDeviceLimits& limits = getVkPhysicalDeviceProperties().limits;
    if (!VERIFY(pushConstantsSize <= limits.maxPushConstantsSize))
    {
      LOG_WARNING("Push constants size exceeded {} (max {} bytes)", pushConstantsSize, limits.maxPushConstantsSize);
    }

    const VkDescriptorSetLayout dsls[] = {vkDSL_, vkDSL_, vkDSL_, vkDSL_};
    const VkPushConstantRange range = {.stageFlags = rtps->shaderStageFlags_, .size = pushConstantsSize,};

    const VkPipelineLayoutCreateInfo ciPipelineLayout = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = std::size(dsls), .pSetLayouts = dsls, .pushConstantRangeCount = 1, .pPushConstantRanges = &range,};
    VK_ASSERT(vkCreatePipelineLayout(vkDevice_, &ciPipelineLayout, nullptr, &rtps->pipelineLayout_));
    char pipelineLayoutName[256] = {0};
    if (rtps->desc_.debugName)
    {
      snprintf(pipelineLayoutName, sizeof(pipelineLayoutName) - 1, "Pipeline Layout: %s", rtps->desc_.debugName);
    }
    VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)rtps->pipelineLayout_, pipelineLayoutName));
  }

  VkSpecializationMapEntry entries[SpecializationConstantDesc::SPECIALIZATION_CONSTANTS_MAX] = {};

  const VkSpecializationInfo siComp = getPipelineShaderStageSpecializationInfo(rtps->desc_.specInfo, entries);

  const uint32_t kMaxRayTracingShaderStages = 6 * MAX_RAY_TRACING_SHADER_GROUP_SIZE;
  VkPipelineShaderStageCreateInfo ciShaderStages[kMaxRayTracingShaderStages];
  uint32_t numShaderStages = 0;
#define ADD_STAGE(shaderModule, vkStageFlag)                                                                                        \
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i) {                                                                 \
    if (shaderModule[i])                                                                                                            \
      ciShaderStages[numShaderStages++] = vk::getPipelineShaderStageCreateInfo(vkStageFlag, shaderModule[i]->sm, "main", &siComp); \
  }
  ADD_STAGE(moduleRGen, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
  ADD_STAGE(moduleMiss, VK_SHADER_STAGE_MISS_BIT_KHR);
  ADD_STAGE(moduleCHit, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
  ADD_STAGE(moduleAHit, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
  ADD_STAGE(moduleIntr, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
  ADD_STAGE(moduleCall, VK_SHADER_STAGE_CALLABLE_BIT_KHR);
#undef ADD_STAGE

  const uint32_t kMaxShaderGroups = 4;
  VkRayTracingShaderGroupCreateInfoKHR shaderGroups[kMaxShaderGroups];
  uint32_t numShaderGroups = 0;
  uint32_t numShaders = 0;
  uint32_t idxMiss = 0;
  uint32_t idxHit = 0;
  uint32_t idxCallable = 0;
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i)
  {
    if (moduleRGen[i])
    {
      // ray generation group
      shaderGroups[numShaderGroups++] = VkRayTracingShaderGroupCreateInfoKHR{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, .generalShader = numShaders++, .closestHitShader = VK_SHADER_UNUSED_KHR, .anyHitShader = VK_SHADER_UNUSED_KHR, .intersectionShader = VK_SHADER_UNUSED_KHR,};
    }
  }
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i)
  {
    if (moduleMiss[i])
    {
      // miss group
      if (i == 0)
        idxMiss = numShaders;
      shaderGroups[numShaderGroups++] = VkRayTracingShaderGroupCreateInfoKHR{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, .generalShader = numShaders++, .closestHitShader = VK_SHADER_UNUSED_KHR, .anyHitShader = VK_SHADER_UNUSED_KHR, .intersectionShader = VK_SHADER_UNUSED_KHR,};
    }
  }
  // hit group
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i)
  {
    if (moduleAHit[i] || moduleCHit[i] || moduleIntr[i])
    {
      if (i == 0)
        idxHit = numShaders;
      shaderGroups[numShaderGroups++] = VkRayTracingShaderGroupCreateInfoKHR{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR, .generalShader = VK_SHADER_UNUSED_KHR, .closestHitShader = moduleCHit[i] ? numShaders++ : VK_SHADER_UNUSED_KHR, .anyHitShader = moduleAHit[i] ? numShaders++ : VK_SHADER_UNUSED_KHR, .intersectionShader = moduleIntr[i] ? numShaders++ : VK_SHADER_UNUSED_KHR,};
    }
  }
  // callable group
  for (int i = 0; i < MAX_RAY_TRACING_SHADER_GROUP_SIZE; ++i)
  {
    if (moduleCall[i])
    {
      if (i == 0)
        idxCallable = numShaders;
      shaderGroups[numShaderGroups++] = VkRayTracingShaderGroupCreateInfoKHR{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR, .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR, .generalShader = numShaders++, .closestHitShader = VK_SHADER_UNUSED_KHR, .anyHitShader = VK_SHADER_UNUSED_KHR, .intersectionShader = VK_SHADER_UNUSED_KHR,};
    }
  }

  const VkRayTracingPipelineCreateInfoKHR ciRayTracingPipeline = {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR, .stageCount = numShaderStages, .pStages = ciShaderStages, .groupCount = numShaderGroups, .pGroups = shaderGroups, .maxPipelineRayRecursionDepth = rayTracingPipelineProperties_.maxRayRecursionDepth, .layout = rtps->pipelineLayout_,};
  VK_ASSERT(vkCreateRayTracingPipelinesKHR(vkDevice_, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &ciRayTracingPipeline, nullptr, &rtps ->pipeline_));

  // shader binding table
  const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& props = rayTracingPipelineProperties_;
  const uint32_t handleSize = props.shaderGroupHandleSize;
  const uint32_t handleSizeAligned = getAlignedSize(props.shaderGroupHandleSize, props.shaderGroupHandleAlignment);
  const uint32_t sbtSize = numShaderGroups * handleSizeAligned;

  ASSERT(sbtSize);

  std::vector<uint8_t> shaderHandleStorage(sbtSize);
  VK_ASSERT(vkGetRayTracingShaderGroupHandlesKHR(vkDevice_, rtps->pipeline_, 0, numShaderGroups, sbtSize, shaderHandleStorage. data()));

  const uint32_t sbtEntrySizeAligned = getAlignedSize(handleSizeAligned, props.shaderGroupBaseAlignment);
  const uint32_t sbtBufferSize = numShaderGroups * sbtEntrySizeAligned;

  // repack SBT respecting `shaderGroupBaseAlignment`
  std::vector<uint8_t> sbtStorage(sbtBufferSize);
  for (uint32_t i = 0; i != numShaderGroups; i++)
  {
    memcpy(sbtStorage.data() + i * sbtEntrySizeAligned, shaderHandleStorage.data() + i * handleSizeAligned, handleSize);
  }

  rtps->sbt = createBuffer({.usage = BufferUsageBits_ShaderBindingTable, .storage = StorageType::Device, .size = sbtBufferSize, .data = sbtStorage.data(), .debugName = "Buffer: SBT",}, nullptr, nullptr);
  // generate SBT entries
  rtps->sbtEntryRayGen = {.deviceAddress = gpuAddress(rtps->sbt), .stride = handleSizeAligned, .size = handleSizeAligned,};
  rtps->sbtEntryMiss = {.deviceAddress = idxMiss ? gpuAddress(rtps->sbt, idxMiss * sbtEntrySizeAligned) : 0, .stride = handleSizeAligned, .size = handleSizeAligned,};
  rtps->sbtEntryHit = {.deviceAddress = idxHit ? gpuAddress(rtps->sbt, idxHit * sbtEntrySizeAligned) : 0, .stride = handleSizeAligned, .size = handleSizeAligned,};
  rtps->sbtEntryCallable = {.deviceAddress = idxCallable ? gpuAddress(rtps->sbt, idxCallable * sbtEntrySizeAligned) : 0, .stride = handleSizeAligned, .size = handleSizeAligned,};

  return rtps->pipeline_;
}

VkPipeline vk::VulkanContext::getVkPipeline(ComputePipelineHandle handle)
{
  ComputePipelineState* cps = computePipelinesPool_.get(handle);

  if (!cps)
  {
    return VK_NULL_HANDLE;
  }

  checkAndUpdateDescriptorSets();

  if (cps->lastVkDescriptorSetLayout_ != vkDSL_)
  {
    deferredTask(std::packaged_task<void()>([device = vkDevice_, pipeline = cps->pipeline_]()
    {
      vkDestroyPipeline(device, pipeline, nullptr);
    }));
    deferredTask(std::packaged_task<void()>([device = vkDevice_, layout = cps->pipelineLayout_]()
    {
      vkDestroyPipelineLayout(device, layout, nullptr);
    }));
    cps->pipeline_ = VK_NULL_HANDLE;
    cps->pipelineLayout_ = VK_NULL_HANDLE;
    cps->lastVkDescriptorSetLayout_ = vkDSL_;
  }

  if (cps->pipeline_ == VK_NULL_HANDLE)
  {
    const ShaderModuleState* sm = shaderModulesPool_.get(cps->desc_.smComp);

    ASSERT(sm);

    VkSpecializationMapEntry entries[SpecializationConstantDesc::SPECIALIZATION_CONSTANTS_MAX] = {};

    const VkSpecializationInfo siComp = getPipelineShaderStageSpecializationInfo(cps->desc_.specInfo, entries);

    // create pipeline layout
    {
      // duplicate for MoltenVK
      const VkDescriptorSetLayout dsls[] = {vkDSL_, vkDSL_, vkDSL_, vkDSL_};
      const VkPushConstantRange range = {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, 
        .offset = 0, 
        .size = sm->pushConstantsSize
      };
      const VkPipelineLayoutCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, 
        .setLayoutCount = (uint32_t)std::size(dsls), 
        .pSetLayouts = dsls, 
        .pushConstantRangeCount = sm->pushConstantsSize ? 1u : 0u,  // FIX: Only set if size > 0
        .pPushConstantRanges = sm->pushConstantsSize ? &range : nullptr  // FIX: nullptr when no push constants
      };
      VK_ASSERT(vkCreatePipelineLayout(vkDevice_, &ci, nullptr, &cps->pipelineLayout_));
      char pipelineLayoutName[256] = {0};
      if (cps->desc_.debugName)
      {
        snprintf(pipelineLayoutName, sizeof(pipelineLayoutName) - 1, "Pipeline Layout: %s", cps->desc_.debugName);
      }
      VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_PIPELINE_LAYOUT, (uint64_t)cps->pipelineLayout_, pipelineLayoutName));
    }

    const VkComputePipelineCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, .flags = 0, .stage = getPipelineShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, sm->sm, cps->desc_.entryPoint, &siComp), .layout = cps->pipelineLayout_, .basePipelineHandle = VK_NULL_HANDLE, .basePipelineIndex = -1,};
    VK_ASSERT(vkCreateComputePipelines(vkDevice_, pipelineCache_, 1, &ci, nullptr, &cps->pipeline_));
    VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_PIPELINE, (uint64_t)cps->pipeline_, cps->desc_.debugName));
  }

  return cps->pipeline_;
}

vk::Holder<vk::ComputePipelineHandle> vk::VulkanContext::createComputePipeline(const ComputePipelineDesc& desc, Result* outResult)
{
  if (!VERIFY(desc.smComp.valid()))
  {
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Missing compute shader");
    return {};
  }

  ComputePipelineState cps{.desc_ = desc};

  if (desc.specInfo.data && desc.specInfo.dataSize)
  {
    // copy into a local storage
    cps.specConstantDataStorage_ = malloc(desc.specInfo.dataSize);
    memcpy(cps.specConstantDataStorage_, desc.specInfo.data, desc.specInfo.dataSize);
    cps.desc_.specInfo.data = cps.specConstantDataStorage_;
  }

  return {this, computePipelinesPool_.create(std::move(cps))};
}

vk::Holder<vk::RayTracingPipelineHandle> vk::VulkanContext::createRayTracingPipeline(const RayTracingPipelineDesc& desc, Result* outResult)
{
  PROFILER_FUNCTION();

  if (!VERIFY(hasRayTracingPipeline_))
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "VK_KHR_ray_tracing_pipeline is not enabled"));
    return {};
  }

  RayTracingPipelineState rtps{.desc_ = desc};

  if (desc.specInfo.data && desc.specInfo.dataSize)
  {
    // copy into a local storage
    rtps.specConstantDataStorage_ = malloc(desc.specInfo.dataSize);
    memcpy(rtps.specConstantDataStorage_, desc.specInfo.data, desc.specInfo.dataSize);
    rtps.desc_.specInfo.data = rtps.specConstantDataStorage_;
  }

  return {this, rayTracingPipelinesPool_.create(std::move(rtps))};
}

vk::Holder<vk::RenderPipelineHandle> vk::VulkanContext::createRenderPipeline(const RenderPipelineDesc& desc, Result* outResult)
{
  const bool hasColorAttachments = desc.getNumColorAttachments() > 0;
  const bool hasDepthAttachment = desc.depthFormat != Format::Invalid;
  const bool hasAnyAttachments = hasColorAttachments || hasDepthAttachment;
  if (!VERIFY(hasAnyAttachments))
  {
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Need at least one attachment");
    return {};
  }

  if (desc.smMesh.valid())
  {
    if (!VERIFY(!desc.vertexInput.getNumAttributes() && !desc.vertexInput.getNumInputBindings()))
    {
      Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Cannot have vertexInput with mesh shaders");
      return {};
    }
    if (!VERIFY(!desc.smVert.valid()))
    {
      Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Cannot have both vertex and mesh shaders");
      return {};
    }
    if (!VERIFY(!desc.smTesc.valid() && !desc.smTese.valid()))
    {
      Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Cannot have both tessellation and mesh shaders");
      return {};
    }
    if (!VERIFY(!desc.smGeom.valid()))
    {
      Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Cannot have both geometry and mesh shaders");
      return {};
    }
  }
  else
  {
    if (!VERIFY(desc.smVert.valid()))
    {
      Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Missing vertex shader");
      return {};
    }
  }

  if (!VERIFY(desc.smFrag.valid()))
  {
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Missing fragment shader");
    return {};
  }

  RenderPipelineState rps = {.desc_ = desc};

  // Iterate and cache vertex input bindings and attributes
  const VertexInput& vstate = rps.desc_.vertexInput;

  bool bufferAlreadyBound[VertexInput::VERTEX_BUFFER_MAX] = {};

  rps.numAttributes_ = vstate.getNumAttributes();

  for (uint32_t i = 0; i != rps.numAttributes_; i++)
  {
    const VertexInput::VertexAttribute& attr = vstate.attributes[i];

    rps.vkAttributes_[i] = {.location = attr.location, .binding = attr.binding, .format = vertexFormatToVkFormat(attr.format), .offset = (uint32_t)attr.offset};

    if (!bufferAlreadyBound[attr.binding])
    {
      bufferAlreadyBound[attr.binding] = true;
      rps.vkBindings_[rps.numBindings_++] = {.binding = attr.binding, .stride = vstate.inputBindings[attr.binding].stride, .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
    }
  }

  if (desc.specInfo.data && desc.specInfo.dataSize)
  {
    // copy into a local storage
    rps.specConstantDataStorage_ = malloc(desc.specInfo.dataSize);
    memcpy(rps.specConstantDataStorage_, desc.specInfo.data, desc.specInfo.dataSize);
    rps.desc_.specInfo.data = rps.specConstantDataStorage_;
  }

  return {this, renderPipelinesPool_.create(std::move(rps))};
}

void vk::VulkanContext::destroy(RayTracingPipelineHandle handle)
{
  RayTracingPipelineState* rtps = rayTracingPipelinesPool_.get(handle);

  if (!rtps)
  {
    return;
  }

  free(rtps->specConstantDataStorage_);

  deferredTask(std::packaged_task<void()>([device = getVkDevice(), pipeline = rtps->pipeline_]()
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }));
  deferredTask(std::packaged_task<void()>([device = getVkDevice(), layout = rtps->pipelineLayout_]()
  {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }));

  rayTracingPipelinesPool_.destroy(handle);
}

void vk::VulkanContext::destroy(ComputePipelineHandle handle)
{
  ComputePipelineState* cps = computePipelinesPool_.get(handle);

  if (!cps)
  {
    return;
  }

  free(cps->specConstantDataStorage_);

  deferredTask(std::packaged_task<void()>([device = getVkDevice(), pipeline = cps->pipeline_]()
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }));
  deferredTask(std::packaged_task<void()>([device = getVkDevice(), layout = cps->pipelineLayout_]()
  {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }));

  computePipelinesPool_.destroy(handle);
}

void vk::VulkanContext::destroy(RenderPipelineHandle handle)
{
  RenderPipelineState* rps = renderPipelinesPool_.get(handle);

  if (!rps)
  {
    return;
  }

  free(rps->specConstantDataStorage_);

  deferredTask(std::packaged_task<void()>([device = getVkDevice(), pipeline = rps->pipeline_]()
  {
    vkDestroyPipeline(device, pipeline, nullptr);
  }));
  deferredTask(std::packaged_task<void()>([device = getVkDevice(), layout = rps->pipelineLayout_]()
  {
    vkDestroyPipelineLayout(device, layout, nullptr);
  }));

  renderPipelinesPool_.destroy(handle);
}

void vk::VulkanContext::destroy(ShaderModuleHandle handle)
{
  const ShaderModuleState* state = shaderModulesPool_.get(handle);

  if (!state)
  {
    return;
  }

  if (state->sm != VK_NULL_HANDLE)
  {
    // a shader module can be destroyed while pipelines created using its shaders are still in use
    // https://registry.khronos.org/vulkan/specs/1.3/html/chap9.html#vkDestroyShaderModule
    vkDestroyShaderModule(getVkDevice(), state->sm, nullptr);
  }

  shaderModulesPool_.destroy(handle);
}

void vk::VulkanContext::destroy(SamplerHandle handle)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_DESTROY);

  VkSampler sampler = *samplersPool_.get(handle);

  samplersPool_.destroy(handle);

  deferredTask(std::packaged_task<void()>([device = vkDevice_, sampler = sampler]()
  {
    vkDestroySampler(device, sampler, nullptr);
  }));
}

void vk::VulkanContext::destroy(BufferHandle handle)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_DESTROY);

  SCOPE_EXIT { buffersPool_.destroy(handle); };

  VulkanBuffer* buf = buffersPool_.get(handle);

  if (!buf)
  {
    return;
  }

  if (buf->mappedPtr_)
  {
    vmaUnmapMemory((VmaAllocator)getVmaAllocator(), buf->vmaAllocation_);
  }
  deferredTask(std::packaged_task<void()>([vma = getVmaAllocator(), buffer = buf->vkBuffer_, allocation = buf->vmaAllocation_]()
  {
    vmaDestroyBuffer((VmaAllocator)vma, buffer, allocation);
  }));
}

void vk::VulkanContext::destroy(TextureHandle handle)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_DESTROY);

  SCOPE_EXIT
  {
    texturesPool_.destroy(handle);
    awaitingCreation_ = true; // make the validation layers happy
  };

  VulkanImage* tex = texturesPool_.get(handle);

  if (!tex)
  {
    return;
  }

  deferredTask(std::packaged_task<void()>([device = getVkDevice(), imageView = tex->imageView_]()
  {
    vkDestroyImageView(device, imageView, nullptr);
  }));
  if (tex->imageViewStorage_)
  {
    deferredTask(std::packaged_task<void()>([device = getVkDevice(), imageView = tex->imageViewStorage_]()
    {
      vkDestroyImageView(device, imageView, nullptr);
    }));
  }

  for (size_t i = 0; i != MAX_MIP_LEVELS; i++)
  {
    for (size_t j = 0; j != std::size(tex->imageViewForFramebuffer_[0]); j++)
    {
      VkImageView v = tex->imageViewForFramebuffer_[i][j];
      if (v != VK_NULL_HANDLE)
      {
        deferredTask(std::packaged_task<void()>([device = getVkDevice(), imageView = v]()
        {
          vkDestroyImageView(device, imageView, nullptr);
        }));
      }
    }
  }

  if (!tex->isOwningVkImage_)
  {
    return;
  }

  if (tex->mappedPtr_)
  {
    vmaUnmapMemory((VmaAllocator)getVmaAllocator(), tex->vmaAllocation_);
  }
  deferredTask(std::packaged_task<void()>([vma = getVmaAllocator(), image = tex->vkImage_, allocation = tex->vmaAllocation_]()
  {
    vmaDestroyImage((VmaAllocator)vma, image, allocation);
  }));
}

void vk::VulkanContext::destroy(QueryPoolHandle handle)
{
  VkQueryPool pool = *queriesPool_.get(handle);

  queriesPool_.destroy(handle);

  deferredTask(std::packaged_task<void()>([device = vkDevice_, pool = pool]()
  {
    vkDestroyQueryPool(device, pool, nullptr);
  }));
}

void vk::VulkanContext::destroy(AccelStructHandle handle)
{
  AccelerationStructure* accelStruct = accelStructuresPool_.get(handle);

  SCOPE_EXIT { accelStructuresPool_.destroy(handle); };

  deferredTask(std::packaged_task<void()>([device = vkDevice_, as = accelStruct->vkHandle]()
  {
    vkDestroyAccelerationStructureKHR(device, as, nullptr);
  }));
}

void vk::VulkanContext::destroy(Framebuffer& fb)
{
  auto destroyFbTexture = [this](TextureHandle& handle)
  {
    {
      if (handle.empty())
        return;
      VulkanImage* tex = texturesPool_.get(handle);
      if (!tex || !tex->isOwningVkImage_)
        return;
      destroy(handle);
      handle = {};
    }
  };

  for (Framebuffer::AttachmentDesc& a : fb.color)
  {
    destroyFbTexture(a.texture);
    destroyFbTexture(a.resolveTexture);
  }
  destroyFbTexture(fb.depthStencil.texture);
  destroyFbTexture(fb.depthStencil.resolveTexture);
}

uint64_t vk::VulkanContext::gpuAddress(AccelStructHandle handle) const
{
  const AccelerationStructure* as = accelStructuresPool_.get(handle);

  ASSERT(as && as->deviceAddress);

  return as ? (uint64_t)as->deviceAddress : 0u;
}

vk::Result vk::VulkanContext::upload(BufferHandle handle, const void* data, size_t size, size_t offset)
{
  PROFILER_FUNCTION();

  if (!VERIFY(data))
  {
    return Result();
  }

  ASSERT_MSG(size, "Data size should be non-zero");

  VulkanBuffer* buf = buffersPool_.get(handle);

  if (!VERIFY(buf))
  {
    return Result();
  }

  if (!VERIFY(offset + size <= buf->bufferSize_))
  {
    return Result(Result::Code::ArgumentOutOfRange, "Out of range");
  }

  stagingDevice_->bufferSubData(*buf, offset, size, data);

  return Result();
}

vk::Result vk::VulkanContext::download(BufferHandle handle, void* data, size_t size, size_t offset)
{
  PROFILER_FUNCTION();

  if (!VERIFY(data))
  {
    return Result();
  }

  ASSERT_MSG(size, "Data size should be non-zero");

  VulkanBuffer* buf = buffersPool_.get(handle);

  if (!VERIFY(buf))
  {
    return Result();
  }

  if (!VERIFY(offset + size <= buf->bufferSize_))
  {
    return Result(Result::Code::ArgumentOutOfRange, "Out of range");
  }

  buf->getBufferSubData(*this, offset, size, data);

  return Result();
}

uint8_t* vk::VulkanContext::getMappedPtr(BufferHandle handle) const
{
  const VulkanBuffer* buf = buffersPool_.get(handle);

  ASSERT(buf);

  return buf->isMapped() ? buf->getMappedPtr() : nullptr;
}

uint64_t vk::VulkanContext::gpuAddress(BufferHandle handle, size_t offset) const
{
  ASSERT_MSG((offset & 7) == 0, "Buffer offset must be 8 bytes aligned as per GLSL_EXT_buffer_reference spec.");

  const VulkanBuffer* buf = buffersPool_.get(handle);

  ASSERT(buf && buf->vkDeviceAddress_);

  return buf ? (uint64_t)buf->vkDeviceAddress_ + offset : 0u;
}

void vk::VulkanContext::flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const
{
  const VulkanBuffer* buf = buffersPool_.get(handle);

  ASSERT(buf);

  buf->flushMappedMemory(*this, offset, size);
}

vk::Result vk::VulkanContext::download(TextureHandle handle, const TextureRangeDesc& range, void* outData)
{
  if (!outData)
  {
    return Result(Result::Code::ArgumentOutOfRange);
  }

  VulkanImage* texture = texturesPool_.get(handle);

  ASSERT(texture);

  if (!texture)
  {
    return Result(Result::Code::RuntimeError);
  }

  const Result result = validateRange(texture->vkExtent_, texture->numLevels_, range);

  if (!VERIFY(result.isOk()))
  {
    return result;
  }

  stagingDevice_->getImageData(*texture, VkOffset3D{range.offset.x, range.offset.y, range.offset.z}, VkExtent3D{range.dimensions.width, range.dimensions.height, range.dimensions.depth}, VkImageSubresourceRange{.aspectMask = texture->getImageAspectFlags(), .baseMipLevel = range.mipLevel, .levelCount = range.numMipLevels, .baseArrayLayer = range.layer, .layerCount = range.numLayers,}, texture->vkImageFormat_, outData);

  return Result();
}

vk::Result vk::VulkanContext::upload(TextureHandle handle, const TextureRangeDesc& range, const void* data)
{
  if (!data)
  {
    return Result(Result::Code::ArgumentOutOfRange);
  }

  VulkanImage* texture = texturesPool_.get(handle);

  if (!texture)
  {
    return Result(Result::Code::RuntimeError);
  }

  const Result result = validateRange(texture->vkExtent_, texture->numLevels_, range);

  if (!VERIFY(result.isOk()))
  {
    return result;
  }

  //const uint32_t numLayers = (std::max)(range.numLayers, 1u);

  VkFormat vkFormat = texture->vkImageFormat_;

  if (texture->vkType_ == VK_IMAGE_TYPE_3D)
  {
    stagingDevice_->imageData3D(*texture, VkOffset3D{range.offset.x, range.offset.y, range.offset.z}, VkExtent3D{range.dimensions.width, range.dimensions.height, range.dimensions.depth}, vkFormat, data);
  }
  else
  {
    const VkRect2D imageRegion = {.offset = {.x = range.offset.x, .y = range.offset.y}, .extent = {.width = range.dimensions.width, .height = range.dimensions.height},};
    stagingDevice_->imageData2D(*texture, imageRegion, range.mipLevel, range.numMipLevels, range.layer, range.numLayers, vkFormat, data);
  }

  return Result();
}

vk::Dimensions vk::VulkanContext::getDimensions(TextureHandle handle) const
{
  if (!handle)
  {
    return {};
  }

  const VulkanImage* tex = texturesPool_.get(handle);

  ASSERT(tex);

  if (!tex)
  {
    return {};
  }

  return {.width = tex->vkExtent_.width, .height = tex->vkExtent_.height, .depth = tex->vkExtent_.depth,};
}

float vk::VulkanContext::getAspectRatio(TextureHandle handle) const
{
  if (!handle)
  {
    return 1.0f;
  }

  const VulkanImage* tex = texturesPool_.get(handle);

  ASSERT(tex);

  if (!tex)
  {
    return 1.0f;
  }

  return static_cast<float>(tex->vkExtent_.width) / static_cast<float>(tex->vkExtent_.height);
}

void vk::VulkanContext::generateMipmap(TextureHandle handle) const
{
  if (handle.empty())
  {
    return;
  }

  const VulkanImage* tex = texturesPool_.get(handle);

  if (tex->numLevels_ <= 1)
  {
    return;
  }

  ASSERT(tex->vkImageLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);
  const VulkanImmediateCommands::CommandBufferWrapper& wrapper = immediate_->acquire();
  tex->generateMipmap(wrapper.cmdBuf_);
  immediate_->submit(wrapper);
}

vk::Format vk::VulkanContext::getFormat(TextureHandle handle) const
{
  if (handle.empty())
  {
    return Format::Invalid;
  }

  return vkFormatToFormat(texturesPool_.get(handle)->vkImageFormat_);
}

vk::Holder<vk::ShaderModuleHandle> vk::VulkanContext::createShaderModule(const ShaderModuleDesc& desc, Result* outResult)
{
  Result result;
  ShaderModuleState sm = desc.dataSize ? createShaderModuleFromSPIRV(desc.data, desc.dataSize, desc.debugName, &result) // binary
                           : createShaderModuleFromGLSL(desc.stage, desc.data, desc.debugName, &result);                // text

  if (!result.isOk())
  {
    Result::setResult(outResult, result);
    return {};
  }
  Result::setResult(outResult, result);

  return {this, shaderModulesPool_.create(std::move(sm))};
}

vk::ShaderModuleState vk::VulkanContext::createShaderModuleFromSPIRV(const void* spirv, size_t numBytes, const char* debugName, Result* outResult) const
{
  VkShaderModule vkShaderModule = VK_NULL_HANDLE;

  const VkShaderModuleCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = numBytes, .pCode = (const uint32_t*)spirv,};

  {
    const VkResult result = vkCreateShaderModule(vkDevice_, &ci, nullptr, &vkShaderModule);

    setResultFrom(outResult, result);

    if (result != VK_SUCCESS)
    {
      return {.sm = VK_NULL_HANDLE};
    }
  }

  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)vkShaderModule, debugName));

  ASSERT(vkShaderModule != VK_NULL_HANDLE);

  SpvReflectShaderModule mdl;
  SpvReflectResult result = spvReflectCreateShaderModule(numBytes, spirv, &mdl);
  ASSERT(result == SPV_REFLECT_RESULT_SUCCESS);
  SCOPE_EXIT { spvReflectDestroyShaderModule(&mdl); };

  uint32_t pushConstantsSize = 0;

  for (uint32_t i = 0; i < mdl.push_constant_block_count; ++i)
  {
    const SpvReflectBlockVariable& block = mdl.push_constant_blocks[i];
    pushConstantsSize = (std::max)(pushConstantsSize, block.offset + block.size);
  }

  return {.sm = vkShaderModule, .pushConstantsSize = pushConstantsSize,};
}

vk::ShaderModuleState vk::VulkanContext::createShaderModuleFromGLSL(ShaderStage stage, const char* source, const char* debugName, Result* outResult) const
{
  const VkShaderStageFlagBits vkStage = shaderStageToVkShaderStage(stage);
  ASSERT(vkStage != VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);
  ASSERT(source);

  std::string sourcePatched;

  if (!source || !*source)
  {
    Result::setResult(outResult, Result::Code::ArgumentOutOfRange, "Shader source is empty");
    return {};
  }

  if (strstr(source, "#version ") == nullptr)
  {
    if (vkStage == VK_SHADER_STAGE_TASK_BIT_EXT || vkStage == VK_SHADER_STAGE_MESH_BIT_EXT)
    {
      sourcePatched += R"(
      #version 460
      #extension GL_EXT_buffer_reference : require
      #extension GL_EXT_buffer_reference_uvec2 : require
      #extension GL_EXT_debug_printf : enable
      #extension GL_EXT_nonuniform_qualifier : require
      #extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
      #extension GL_EXT_mesh_shader : require
      )";
    }
    if (vkStage == VK_SHADER_STAGE_VERTEX_BIT || vkStage == VK_SHADER_STAGE_COMPUTE_BIT || vkStage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT || vkStage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
    {
      sourcePatched += R"(
      #version 460
      #extension GL_EXT_buffer_reference : require
      #extension GL_EXT_buffer_reference_uvec2 : require
      #extension GL_EXT_debug_printf : enable
      #extension GL_EXT_nonuniform_qualifier : require
      #extension GL_EXT_samplerless_texture_functions : require
      #extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
      )";
    }
    if (vkStage == VK_SHADER_STAGE_FRAGMENT_BIT)
    {
      const bool bInjectTLAS = strstr(source, "kTLAS[") != nullptr;
      // Note how nonuniformEXT() should be used:
      // https://github.com/KhronosGroup/Vulkan-Samples/blob/main/shaders/descriptor_indexing/nonuniform-quads.frag#L33-L39
      sourcePatched += R"(
      #version 460
      #extension GL_EXT_buffer_reference_uvec2 : require
      #extension GL_EXT_debug_printf : enable
      #extension GL_EXT_nonuniform_qualifier : require
      #extension GL_EXT_samplerless_texture_functions : require
      #extension GL_EXT_shader_explicit_arithmetic_types_float16 : require
      )";
      if (bInjectTLAS)
      {
        sourcePatched += R"(
      #extension GL_EXT_buffer_reference : require
      #extension GL_EXT_ray_query : require

      layout(set = 0, binding = 4) uniform accelerationStructureEXT kTLAS[];
      )";
      }
      sourcePatched += R"(
      layout (set = 0, binding = 0) uniform texture2D kTextures2D[];
      layout (set = 1, binding = 0) uniform texture3D kTextures3D[];
      layout (set = 2, binding = 0) uniform textureCube kTexturesCube[];
      layout (set = 3, binding = 0) uniform texture2D kTextures2DShadow[];
      layout (set = 0, binding = 1) uniform sampler kSamplers[];
      layout (set = 3, binding = 1) uniform samplerShadow kSamplersShadow[];

      layout (set = 0, binding = 3) uniform sampler2D kSamplerYUV[];

      vec4 textureBindless2D(uint textureid, uint samplerid, vec2 uv) {
        return texture(nonuniformEXT(sampler2D(kTextures2D[textureid], kSamplers[samplerid])), uv);
      }
      vec4 textureBindless2DLod(uint textureid, uint samplerid, vec2 uv, float lod) {
        return textureLod(nonuniformEXT(sampler2D(kTextures2D[textureid], kSamplers[samplerid])), uv, lod);
      }
      float textureBindless2DShadow(uint textureid, uint samplerid, vec3 uvw) {
        return texture(nonuniformEXT(sampler2DShadow(kTextures2DShadow[textureid], kSamplersShadow[samplerid])), uvw);
      }
      ivec2 textureBindlessSize2D(uint textureid) {
        return textureSize(nonuniformEXT(kTextures2D[textureid]), 0);
      }
      vec4 textureBindlessCube(uint textureid, uint samplerid, vec3 uvw) {
        return texture(nonuniformEXT(samplerCube(kTexturesCube[textureid], kSamplers[samplerid])), uvw);
      }
      vec4 textureBindlessCubeLod(uint textureid, uint samplerid, vec3 uvw, float lod) {
        return textureLod(nonuniformEXT(samplerCube(kTexturesCube[textureid], kSamplers[samplerid])), uvw, lod);
      }
      int textureBindlessQueryLevels2D(uint textureid) {
        return textureQueryLevels(nonuniformEXT(kTextures2D[textureid]));
      }
      int textureBindlessQueryLevelsCube(uint textureid) {
        return textureQueryLevels(nonuniformEXT(kTexturesCube[textureid]));
      }
      )";
    }
    sourcePatched += source;
    source = sourcePatched.c_str();
  }

  const glslang_resource_t glslangResource = getGlslangResource(getVkPhysicalDeviceProperties().limits);

  std::vector<uint8_t> spirv;
  const Result result = compileShader(vkStage, source, &spirv, &glslangResource);

  return createShaderModuleFromSPIRV(spirv.data(), spirv.size(), debugName, outResult);
}

vk::Format vk::VulkanContext::getSwapchainFormat() const
{
  if (!hasSwapchain())
  {
    return Format::Invalid;
  }

  return vkFormatToFormat(swapchain_->getSurfaceFormat().format);
}

vk::ColorSpace vk::VulkanContext::getSwapchainColorSpace() const
{
  if (!hasSwapchain())
  {
    return ColorSpace::SRGB_NONLINEAR;
  }

  return vkColorSpaceToColorSpace(swapchain_->getSurfaceFormat().colorSpace);
}

uint32_t vk::VulkanContext::getNumSwapchainImages() const
{
  return hasSwapchain() ? swapchain_->getNumSwapchainImages() : 0;
}

// Updated VulkanContext::getCurrentSwapchainTexture()
vk::TextureHandle vk::VulkanContext::getCurrentSwapchainTexture()
{
  PROFILER_FUNCTION();

  if (!hasSwapchain())
  {
    return {};
  }

  // Handle swapchain recreation if needed
  if (shouldRecreate_ || swapchain_->needsRebuild())
  {
    uint32_t actualWidth = newWidth_ > 0 ? newWidth_ : 1920;
    uint32_t actualHeight = newHeight_ > 0 ? newHeight_ : 1080;

    Result result = swapchain_->reinitResources(actualWidth, actualHeight, !config_.disableVSync);
    if (!result.isOk())
    {
      LOG_WARNING("Failed to recreate swapchain: {}", result.message);
      return {};
    }
    shouldRecreate_ = false;
  }

  // Try to acquire next image - this may fail if frame resources aren't ready
  Result result = swapchain_->acquireNextImage();
  if (!result.isOk())
  {
    // Don't log warnings for normal "not ready" cases
    if (result.code == Result::Code::ArgumentOutOfRange)
    {
      shouldRecreate_ = true;
    }
    return {};
  }

  return swapchain_->getCurrentTexture();
}

uint32_t vk::VulkanContext::getSwapchainCurrentImageIndex() const
{
  if (hasSwapchain())
  {
    // make sure we do not use a stale image
    (void)swapchain_->getCurrentTexture();
  }

  return hasSwapchain() ? swapchain_->getSwapchainCurrentImageIndex() : 0;
}

void vk::VulkanContext::recreateSwapchain(int newWidth, int newHeight)
{
  newWidth_ = newWidth;
  newHeight_ = newHeight;

  if (!swapchain_)
  {
    // No existing swapchain, create new one
    initSwapchain(newWidth_, newHeight_);
    return;
  }

  if (!newWidth_ || !newHeight_)
  {
    // Zero size means destroy swapchain
    initSwapchain(0, 0);
    return;
  }

  // Mark for recreation on next frame
  shouldRecreate_ = true;
}

uint32_t vk::VulkanContext::getFramebufferMSAABitMask() const
{
  const VkPhysicalDeviceLimits& limits = getVkPhysicalDeviceProperties().limits;
  return limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
}

double vk::VulkanContext::getTimestampPeriodToMs() const
{
  return double(getVkPhysicalDeviceProperties().limits.timestampPeriod) * 1e-6;
}

bool vk::VulkanContext::getQueryPoolResults(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* outData, size_t stride) const
{
  VkQueryPool vkPool = *queriesPool_.get(pool);

  VK_ASSERT(vkGetQueryPoolResults( vkDevice_, vkPool, firstQuery, queryCount, dataSize, outData, stride, VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT));

  return true;
}

vk::AccelStructSizes vk::VulkanContext::getAccelStructSizes(const AccelStructDesc& desc, Result* outResult) const
{
  PROFILER_FUNCTION();

  if (!VERIFY(hasAccelerationStructure_))
  {
    Result::setResult(outResult, Result(Result::Code::RuntimeError, "VK_KHR_acceleration_structure is not enabled"));
    return {};
  }

  Result result;
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
  switch (desc.type)
  {
    case AccelStructType::BLAS:
      getBuildInfoBLAS(desc, accelerationStructureGeometry, accelerationStructureBuildSizesInfo);
      break;
    case AccelStructType::TLAS:
      getBuildInfoTLAS(desc, accelerationStructureGeometry, accelerationStructureBuildSizesInfo);
      break;
    default: ASSERT_MSG(false, "Invalid acceleration structure type");
      Result::setResult(outResult, Result(Result::Code::ArgumentOutOfRange, "Invalid acceleration structure type"));
      return {};
  }

  Result::setResult(outResult, Result::Code::Ok);

  return AccelStructSizes{.accelerationStructureSize = accelerationStructureBuildSizesInfo.accelerationStructureSize, .updateScratchSize = accelerationStructureBuildSizesInfo.updateScratchSize, .buildScratchSize = accelerationStructureBuildSizesInfo.buildScratchSize,};
}

void vk::VulkanContext::createInstance()
{
  vkInstance_ = VK_NULL_HANDLE;

  // check if we have validation layers in the system
  {
    uint32_t numLayerProperties = 0;
    vkEnumerateInstanceLayerProperties(&numLayerProperties, nullptr);
    std::vector<VkLayerProperties> layerProperties(numLayerProperties);
    vkEnumerateInstanceLayerProperties(&numLayerProperties, layerProperties.data());

    [this, &layerProperties]() -> void
    {
      for (const VkLayerProperties& props : layerProperties)
      {
        for (const char* layer : kDefaultValidationLayers)
        {
          if (!strcmp(props.layerName, layer))
            return;
        }
      }
      config_.enableValidation = false; // no validation layers available
    }();
  }

  std::vector<VkExtensionProperties> allInstanceExtensions;
  {
    uint32_t count = 0;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    allInstanceExtensions.resize(count);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(nullptr, &count, allInstanceExtensions.data()));
  }
  // collect instance extensions from all validation layers
  if (config_.enableValidation)
  {
    for (const char* layer : kDefaultValidationLayers)
    {
      uint32_t count = 0;
      VK_ASSERT(vkEnumerateInstanceExtensionProperties(layer, &count, nullptr));
      if (count > 0)
      {
        const size_t sz = allInstanceExtensions.size();
        allInstanceExtensions.resize(sz + count);
        VK_ASSERT(vkEnumerateInstanceExtensionProperties(layer, &count, allInstanceExtensions.data() + sz));
      }
    }
  }
  std::vector<const char*> instanceExtensionNames = {VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
			VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif defined(__linux__)
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
			VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#else
			VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#endif
#elif defined(__APPLE__)
			VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
			VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#endif
#if defined(LVK_WITH_VULKAN_PORTABILITY)
			VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
  }; // check if we have the VK_EXT_debug_utils extension
  const bool hasDebugUtils = hasExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, allInstanceExtensions);

  if (hasDebugUtils)
  {
    instanceExtensionNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  if (config_.enableValidation)
  {
    instanceExtensionNames.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME); // enabled only for validation
  }

  if (config_.enableHeadlessSurface)
  {
    instanceExtensionNames.push_back(VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME);
  }
  if (hasExtension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME, allInstanceExtensions))
  {
    // required by the device extension VK_EXT_swapchain_maintenance1
    instanceExtensionNames.push_back(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME);
  }
  if (hasExtension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, allInstanceExtensions))
  {
    // required by the instance extension VK_EXT_surface_maintenance1
    instanceExtensionNames.push_back(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME);
  }

  for (const char* ext : config_.extensionsInstance)
  {
    if (ext)
    {
      instanceExtensionNames.push_back(ext);
    }
  }

#if !defined(__ANDROID__)
  // GPU Assisted Validation doesn't work on Android.
  // It implicitly requires vertexPipelineStoresAndAtomics feature that's not supported even on high-end devices.
  const VkValidationFeatureEnableEXT validationFeaturesEnabled[] = {VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT, VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,};
#endif // ANDROID

#if defined(__APPLE__)
	// Shader validation doesn't work in MoltenVK for SPIR-V 1.6 under Vulkan 1.3:
	// "Invalid SPIR-V binary version 1.6 for target environment SPIR-V 1.5 (under Vulkan 1.2 semantics)."
	const VkValidationFeatureDisableEXT validationFeaturesDisabled[] = {
		VK_VALIDATION_FEATURE_DISABLE_SHADERS_EXT,
		VK_VALIDATION_FEATURE_DISABLE_SHADER_VALIDATION_CACHE_EXT,
	};
#endif // __APPLE__

  const VkValidationFeaturesEXT features = {.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    .pNext = nullptr,
#if !defined(__ANDROID__)
    .enabledValidationFeatureCount = config_.enableValidation ? (uint32_t)std::size(validationFeaturesEnabled) : 0u,
    .pEnabledValidationFeatures = config_.enableValidation ? validationFeaturesEnabled : nullptr,
#endif
#if defined(__APPLE__)
			.disabledValidationFeatureCount = config_.enableValidation ? (uint32_t)std::size(validationFeaturesDisabled) : 0u,
			.pDisabledValidationFeatures = config_.enableValidation ? validationFeaturesDisabled : nullptr,
#endif
  };

#if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
  // https://github.com/KhronosGroup/MoltenVK/blob/main/Docs/MoltenVK_Configuration_Parameters.md
  const int useMetalArgumentBuffers = 1;
  const VkBool32 gpuav_descriptor_checks = VK_FALSE;
  // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8688
  const VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;
  // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/8579
  const VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE;
  // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/9222
#define LAYER_SETTINGS_BOOL32(name, var)                                                                                        \
  VkLayerSettingEXT {                                                                                                           \
    .pLayerName = kDefaultValidationLayers[0], .pSettingName = name, .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, .valueCount = 1, \
    .pValues = var,                                                                                                             \
  }
  const VkLayerSettingEXT settings[] = {LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks), LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers), LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing", &gpuav_post_process_descriptor_indexing), {"MoltenVK", "MVK_CONFIG_USE_METAL_ARGUMENT_BUFFERS", VK_LAYER_SETTING_TYPE_INT32_EXT, 1, &useMetalArgumentBuffers},};
#undef LAYER_SETTINGS_BOOL32
  const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT, .pNext = config_.enableValidation ? &features : nullptr, .settingCount = (uint32_t)std::size(settings), .pSettings = settings,};
#endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings

  const VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = nullptr,
    .pApplicationName = "Vulkan",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "Vulkan",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
#if defined(VK_API_VERSION_1_4)
    .apiVersion = config_.vulkanVersion == VulkanVersion_1_3 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_4,
#else
			.apiVersion = VK_API_VERSION_1_3,
#endif // VK_API_VERSION_1_4
  };

  VkInstanceCreateFlags flags = 0;
  const VkInstanceCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#if defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
    .pNext = &layerSettingsCreateInfo,
#else
			.pNext = config_.enableValidation ? &features : nullptr,
#endif // defined(VK_EXT_layer_settings) && VK_EXT_layer_settings
    .flags = flags,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = config_.enableValidation ? (uint32_t)std::size(kDefaultValidationLayers) : 0,
    .ppEnabledLayerNames = config_.enableValidation ? kDefaultValidationLayers : nullptr,
    .enabledExtensionCount = (uint32_t)instanceExtensionNames.size(),
    .ppEnabledExtensionNames = instanceExtensionNames.data(),};

  VK_ASSERT(vkCreateInstance(&ci, nullptr, &vkInstance_));

  volkLoadInstance(vkInstance_);

  // debug messenger
  if (hasDebugUtils)
  {
    const VkDebugUtilsMessengerCreateInfoEXT cdi = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, .pfnUserCallback = &vulkanDebugCallback, .pUserData = this,};
    VK_ASSERT(vkCreateDebugUtilsMessengerEXT(vkInstance_, &cdi, nullptr, &vkDebugUtilsMessenger_));
  }

  LOG_INFO("{} layer version: {}.{}.{}\n", kDefaultValidationLayers[0], VK_VERSION_MAJOR(khronosValidationVersion_), VK_VERSION_MINOR(khronosValidationVersion_), VK_VERSION_PATCH(khronosValidationVersion_));

  // log available instance extensions
  LOG_INFO("Vulkan instance extensions:");

  for (const VkExtensionProperties& extension : allInstanceExtensions)
  {
    LOG_INFO("{}", extension.extensionName);
  }
}

void vk::VulkanContext::createHeadlessSurface()
{
  const VkHeadlessSurfaceCreateInfoEXT ci = {.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT, .pNext = nullptr, .flags = 0,};
  ASSERT(vkCreateHeadlessSurfaceEXT(vkInstance_, &ci, nullptr, &vkSurface_));
}

uint32_t vk::VulkanContext::queryDevices(HWDeviceType deviceType, HWDeviceDesc* outDevices, uint32_t maxOutDevices)
{
  // Physical devices
  uint32_t deviceCount = 0;
  VK_ASSERT(vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, nullptr));
  std::vector<VkPhysicalDevice> vkDevices(deviceCount);
  VK_ASSERT(vkEnumeratePhysicalDevices(vkInstance_, &deviceCount, vkDevices.data()));

  auto convertVulkanDeviceTypeToVK = [](VkPhysicalDeviceType vkDeviceType) -> HWDeviceType
  {
    switch (vkDeviceType)
    {
      case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        return HWDeviceType::Integrated;
      case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        return HWDeviceType::Discrete;
      case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        return HWDeviceType::External;
      case VK_PHYSICAL_DEVICE_TYPE_CPU:
        return HWDeviceType::Software;
      default:
        return HWDeviceType::Software;
    }
  };

  const HWDeviceType desiredDeviceType = deviceType;

  uint32_t numCompatibleDevices = 0;

  for (uint32_t i = 0; i < deviceCount; ++i)
  {
    VkPhysicalDevice physicalDevice = vkDevices[i];
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    const HWDeviceType hw_device = convertVulkanDeviceTypeToVK(deviceProperties.deviceType);

    // filter non-suitable hardware devices
    if (desiredDeviceType != HWDeviceType::Software && desiredDeviceType != hw_device)
    {
      continue;
    }

    if (outDevices && numCompatibleDevices < maxOutDevices)
    {
      outDevices[numCompatibleDevices] = {.guid = (uintptr_t)vkDevices[i], .type = hw_device};
      strncpy(outDevices[numCompatibleDevices].name, deviceProperties.deviceName, strlen(deviceProperties.deviceName));
      numCompatibleDevices++;
    }
  }

  return numCompatibleDevices;
}

void vk::VulkanContext::addNextPhysicalDeviceProperties(void* properties)
{
  if (!properties)
    return;

  std::launder(reinterpret_cast<VkBaseOutStructure*>(properties))->pNext = std::launder(reinterpret_cast<VkBaseOutStructure*>(vkPhysicalDeviceProperties2_.pNext));

  vkPhysicalDeviceProperties2_.pNext = properties;
}

void vk::VulkanContext::getBuildInfoBLAS(const AccelStructDesc& desc, VkAccelerationStructureGeometryKHR& outGeometry, VkAccelerationStructureBuildSizesInfoKHR& outSizesInfo) const
{
  ASSERT(desc.type == AccelStructType::BLAS);
  ASSERT(desc.geometryType == AccelStructGeomType::Triangles);
  ASSERT(desc.numVertices);
  ASSERT(desc.indexBuffer.valid());
  ASSERT(desc.vertexBuffer.valid());
  ASSERT(desc.transformBuffer.valid());
  ASSERT(desc.buildRange.primitiveCount);

  ASSERT(buffersPool_.get(desc.indexBuffer)->vkUsageFlags_ & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
  ASSERT(buffersPool_.get(desc.vertexBuffer)->vkUsageFlags_ & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
  ASSERT(buffersPool_.get(desc.transformBuffer)->vkUsageFlags_ & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

  VkGeometryFlagsKHR geometryFlags = 0;

  if (desc.geometryFlags & AccelStructGeometryFlagBits_Opaque)
  {
    geometryFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
  }
  if (desc.geometryFlags & AccelStructGeometryFlagBits_NoDuplicateAnyHit)
  {
    geometryFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  }

  outGeometry = VkAccelerationStructureGeometryKHR{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR, .geometry = {.triangles = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR, .vertexFormat = vertexFormatToVkFormat(desc.vertexFormat), .vertexData = {.deviceAddress = gpuAddress(desc.vertexBuffer)}, .vertexStride = desc.vertexStride ? desc.vertexStride : getVertexFormatSize(desc.vertexFormat), .maxVertex = desc.numVertices - 1, .indexType = VK_INDEX_TYPE_UINT32, .indexData = {.deviceAddress = gpuAddress(desc.indexBuffer)}, .transformData = {.deviceAddress = gpuAddress(desc.transformBuffer)},},}, .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,};

  const VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR, .geometryCount = 1, .pGeometries = &outGeometry,};

  outSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  vkGetAccelerationStructureBuildSizesKHR(vkDevice_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationBuildGeometryInfo, &desc.buildRange.primitiveCount, &outSizesInfo);
  const uint32_t alignment = accelerationStructureProperties_.minAccelerationStructureScratchOffsetAlignment;
  outSizesInfo.accelerationStructureSize += alignment;
  outSizesInfo.updateScratchSize += alignment;
  outSizesInfo.buildScratchSize += alignment;
}

void vk::VulkanContext::getBuildInfoTLAS(const AccelStructDesc& desc, VkAccelerationStructureGeometryKHR& outGeometry, VkAccelerationStructureBuildSizesInfoKHR& outSizesInfo) const
{
  ASSERT(desc.type == AccelStructType::TLAS);
  ASSERT(desc.geometryType == AccelStructGeomType::Instances);
  ASSERT(desc.numVertices == 0);
  ASSERT(desc.instancesBuffer.valid());
  ASSERT(desc.buildRange.primitiveCount);
  ASSERT(buffersPool_.get(desc.instancesBuffer)->vkUsageFlags_ & VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);

  outGeometry = VkAccelerationStructureGeometryKHR{.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR, .geometry = {.instances = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, .arrayOfPointers = VK_FALSE, .data = {.deviceAddress = gpuAddress(desc.instancesBuffer)},},}, .flags = VK_GEOMETRY_OPAQUE_BIT_KHR,};

  const VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, .flags = buildFlagsToVkBuildAccelerationStructureFlags(desc.buildFlags), .geometryCount = 1, .pGeometries = &outGeometry,};

  outSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR, vkGetAccelerationStructureBuildSizesKHR(vkDevice_, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &desc.buildRange.primitiveCount, &outSizesInfo);
  const uint32_t alignment = accelerationStructureProperties_.minAccelerationStructureScratchOffsetAlignment;
  outSizesInfo.accelerationStructureSize += alignment;
  outSizesInfo.updateScratchSize += alignment;
  outSizesInfo.buildScratchSize += alignment;
}

vk::Result vk::VulkanContext::initContext(const HWDeviceDesc& desc)
{
  if (desc.guid == 0UL)
  {
    LOG_WARNING("Invalid hardwareGuid({})", desc.guid);
    return Result(Result::Code::RuntimeError, "Vulkan is not supported");
  }
  vkPhysicalDevice_ = (VkPhysicalDevice)desc.guid;
  useStaging_ = !isHostVisibleSingleHeapMemory(vkPhysicalDevice_);
  std::vector<VkExtensionProperties> allDeviceExtensions;
  getDeviceExtensionProps(vkPhysicalDevice_, allDeviceExtensions);
  if (config_.enableValidation)
  {
    for (const char* layer : kDefaultValidationLayers)
    {
      getDeviceExtensionProps(vkPhysicalDevice_, allDeviceExtensions, layer);
    }
  }
  if (hasExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, allDeviceExtensions))
  {
    addNextPhysicalDeviceProperties(&accelerationStructureProperties_);
  }
  if (hasExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, allDeviceExtensions))
  {
    addNextPhysicalDeviceProperties(&rayTracingPipelineProperties_);
  }

#if defined(VK_API_VERSION_1_4)
  if (config_.vulkanVersion >= VulkanVersion_1_4)
  {
    addNextPhysicalDeviceProperties(&vkPhysicalDeviceVulkan14Properties_);
    vkFeatures13_.pNext = &vkFeatures14_;
  }
#else
	ASSERT_MSG(config_.vulkanVersion == VulkanVersion_1_3, "Only Vulkan 1.3 is supported on this platform");
#endif // VK_API_VERSION_1_4

  if (hasExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, allDeviceExtensions))
  {
    vkRobustness2Features_.pNext = vkFeatures10_.pNext;
    vkFeatures10_.pNext = &vkRobustness2Features_;
  }

  if (hasExtension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, allDeviceExtensions))
  {
    vkSwapchainMaintenance1Features_.pNext = vkFeatures10_.pNext;
    vkFeatures10_.pNext = &vkSwapchainMaintenance1Features_;
  }

  vkGetPhysicalDeviceFeatures2(vkPhysicalDevice_, &vkFeatures10_);
  vkGetPhysicalDeviceProperties2(vkPhysicalDevice_, &vkPhysicalDeviceProperties2_);

  const uint32_t apiVersion = vkPhysicalDeviceProperties2_.properties.apiVersion;

  LOG_INFO("Vulkan physical device: {}\n", vkPhysicalDeviceProperties2_.properties.deviceName);
  LOG_INFO("API version: {}.{}.{}.{}\n", VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion), VK_API_VERSION_VARIANT(apiVersion));
  LOG_INFO("Driver info: {} {}\n", vkPhysicalDeviceDriverProperties_.driverName, vkPhysicalDeviceDriverProperties_.driverInfo);

  LOG_INFO("Vulkan physical device extensions:\n");
  // log available physical device extensions
  for (const VkExtensionProperties& ext : allDeviceExtensions)
  {
    LOG_INFO("{}", ext.extensionName);
  }

  deviceQueues_.graphicsQueueFamilyIndex = findQueueFamilyIndex(vkPhysicalDevice_, VK_QUEUE_GRAPHICS_BIT);
  deviceQueues_.computeQueueFamilyIndex = findQueueFamilyIndex(vkPhysicalDevice_, VK_QUEUE_COMPUTE_BIT);

  if (deviceQueues_.graphicsQueueFamilyIndex == DeviceQueues::INVALID)
  {
    LOG_WARNING("VK_QUEUE_GRAPHICS_BIT is not supported");
    return Result(Result::Code::RuntimeError, "VK_QUEUE_GRAPHICS_BIT is not supported");
  }

  if (deviceQueues_.computeQueueFamilyIndex == DeviceQueues::INVALID)
  {
    LOG_WARNING("VK_QUEUE_COMPUTE_BIT is not supported");
    return Result(Result::Code::RuntimeError, "VK_QUEUE_COMPUTE_BIT is not supported");
  }

  const float queuePriority = 1.0f;

  const VkDeviceQueueCreateInfo ciQueue[2] = {{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = deviceQueues_.graphicsQueueFamilyIndex, .queueCount = 1, .pQueuePriorities = &queuePriority,}, {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = deviceQueues_.computeQueueFamilyIndex, .queueCount = 1, .pQueuePriorities = &queuePriority,},};
  const uint32_t numQueues = ciQueue[0].queueFamilyIndex == ciQueue[1].queueFamilyIndex ? 1 : 2;
  std::vector deviceExtensionNames = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  for (const char* ext : config_.extensionsDevice)
  {
    if (ext)
    {
      deviceExtensionNames.push_back(ext);
    }
  }

  VkPhysicalDeviceFeatures deviceFeatures10 = {.robustBufferAccess = vkFeatures10_.features.robustBufferAccess,
    .geometryShader = vkFeatures10_.features.geometryShader,
    // enable if supported
    .tessellationShader = vkFeatures10_.features.tessellationShader,
    // enable if supported
    .sampleRateShading = VK_TRUE,
    .multiDrawIndirect = VK_TRUE,
    .drawIndirectFirstInstance = VK_TRUE,
    .depthBiasClamp = VK_TRUE,
    .fillModeNonSolid = vkFeatures10_.features.fillModeNonSolid,
    // enable if supported
    .samplerAnisotropy = VK_TRUE,
    .textureCompressionBC = vkFeatures10_.features.textureCompressionBC,
    // enable if supported
    .vertexPipelineStoresAndAtomics = vkFeatures10_.features.vertexPipelineStoresAndAtomics,
    // enable if supported
    .fragmentStoresAndAtomics = VK_TRUE,
    .shaderImageGatherExtended = VK_TRUE,
    .shaderInt64 = vkFeatures10_.features.shaderInt64,
    // enable if supported
    .shaderInt16 = vkFeatures10_.features.shaderInt16,
    // enable if supported
  };
  VkPhysicalDeviceVulkan11Features deviceFeatures11 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .pNext = config_.extensionsDeviceFeatures,
    .storageBuffer16BitAccess = VK_TRUE,
    .multiview = vkFeatures11_.multiview,
    // enable if supported
    .samplerYcbcrConversion = vkFeatures11_.samplerYcbcrConversion,
    // enable if supported
    .shaderDrawParameters = VK_TRUE,};
  VkPhysicalDeviceVulkan12Features deviceFeatures12 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext = &deviceFeatures11,
    .drawIndirectCount = vkFeatures12_.drawIndirectCount,
    // enable if supported
    .storageBuffer8BitAccess = vkFeatures12_.storageBuffer8BitAccess,
    // enable if supported
    .uniformAndStorageBuffer8BitAccess = vkFeatures12_.uniformAndStorageBuffer8BitAccess,
    // enable if supported
    .shaderFloat16 = vkFeatures12_.shaderFloat16,
    // enable if supported
    .shaderInt8 = vkFeatures12_.shaderInt8,
    // enable if supported
    .descriptorIndexing = VK_TRUE,
    .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
    .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
    .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
    .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
    .descriptorBindingPartiallyBound = VK_TRUE,
    .descriptorBindingVariableDescriptorCount = VK_TRUE,
    .runtimeDescriptorArray = VK_TRUE,
    .scalarBlockLayout = VK_TRUE,
    .uniformBufferStandardLayout = VK_TRUE,
    .hostQueryReset = vkFeatures12_.hostQueryReset,
    // enable if supported
    .timelineSemaphore = VK_TRUE,
    .bufferDeviceAddress = VK_TRUE,
    .vulkanMemoryModel = vkFeatures12_.vulkanMemoryModel,
    // enable if supported
    .vulkanMemoryModelDeviceScope = vkFeatures12_.vulkanMemoryModelDeviceScope,
    // enable if supported
  };
  VkPhysicalDeviceVulkan13Features deviceFeatures13 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES, .pNext = &deviceFeatures12, .subgroupSizeControl = VK_TRUE, .synchronization2 = VK_TRUE, .dynamicRendering = VK_TRUE, .maintenance4 = VK_TRUE,};
#if defined(VK_API_VERSION_1_4)
  VkPhysicalDeviceVulkan14Features deviceFeatures14 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES, .pNext = &deviceFeatures13, .indexTypeUint8 = vkFeatures14_.indexTypeUint8, .dynamicRenderingLocalRead = vkFeatures14_.dynamicRenderingLocalRead,};
#endif // VK_API_VERSION_1_4

#if defined(VK_API_VERSION_1_4)
  void* createInfoNext = config_.vulkanVersion >= VulkanVersion_1_4 ? static_cast<void*>(&deviceFeatures14) : static_cast<void*>(&deviceFeatures13);
#else
	void* createInfoNext = &deviceFeatures13;
#endif
  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, .accelerationStructure = VK_TRUE, .accelerationStructureCaptureReplay = VK_FALSE, .accelerationStructureIndirectBuild = VK_FALSE, .accelerationStructureHostCommands = VK_FALSE, .descriptorBindingAccelerationStructureUpdateAfterBind = VK_TRUE,};
  VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, .rayTracingPipeline = VK_TRUE, .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE, .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE, .rayTracingPipelineTraceRaysIndirect = VK_TRUE, .rayTraversalPrimitiveCulling = VK_FALSE,};
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, .rayQuery = VK_TRUE,};
  VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT, .swapchainMaintenance1 = VK_TRUE,};
  VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT, .indexTypeUint8 = VK_TRUE,};

  auto addOptionalExtension = [&allDeviceExtensions, &deviceExtensionNames, &createInfoNext](const char* name, bool& enabled, void* features = nullptr) mutable -> bool
  {
    if (!hasExtension(name, allDeviceExtensions))
    {
      enabled = false;
      return false; // Don't add features if extension isn't available
    }
    enabled = true;
    deviceExtensionNames.push_back(name);
    if (features)
    {
      std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext = std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
      createInfoNext = features;
    }
    return true;
  };
  auto addOptionalExtensions = [&allDeviceExtensions, &deviceExtensionNames, &createInfoNext](const char* name1, const char* name2, bool& enabled, void* features = nullptr) mutable
  {
    if (!hasExtension(name1, allDeviceExtensions) || !hasExtension(name2, allDeviceExtensions))
      return;
    enabled = true;
    deviceExtensionNames.push_back(name1);
    deviceExtensionNames.push_back(name2);
    if (features)
    {
      std::launder(reinterpret_cast<VkBaseOutStructure*>(features))->pNext = std::launder(reinterpret_cast<VkBaseOutStructure*>(createInfoNext));
      createInfoNext = features;
    }
  };
  VkPhysicalDeviceRobustness2FeaturesEXT robustness2Features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT, .pNext = nullptr, .robustBufferAccess2 = vkRobustness2Features_.robustBufferAccess2, .robustImageAccess2 = vkRobustness2Features_.robustImageAccess2, .nullDescriptor = VK_TRUE};

  // 4. Add after the existing addOptionalExtension calls:
  addOptionalExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME, hasRobustness2_, &robustness2Features);
#if defined(TRACY_ENABLE)
  addOptionalExtension(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME, hasCalibratedTimestamps_, nullptr);
#endif
  addOptionalExtensions(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, hasAccelerationStructure_, &accelerationStructureFeatures);
  addOptionalExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, hasRayQuery_, &rayQueryFeatures);
  addOptionalExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, hasRayTracingPipeline_, &rayTracingFeatures);
  addOptionalExtension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME, has_EXT_swapchain_maintenance1_, &vkSwapchainMaintenance1Features_);
#if defined(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME)
  if (!addOptionalExtension(VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME, has8BitIndices_, &indexTypeUint8Features))
#endif // VK_KHR_INDEX_TYPE_UINT8_EXTENSION_NAME
  {
    addOptionalExtension(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, has8BitIndices_, &indexTypeUint8Features);
  }

  // check extensions
  {
    std::string missingExtensions;
    for (const char* ext : deviceExtensionNames)
    {
      if (!hasExtension(ext, allDeviceExtensions))
        missingExtensions += "\n   " + std::string(ext);
    }
    if (!missingExtensions.empty())
    {
      LOG_CRITICAL("Missing Vulkan device extensions: {}\n", missingExtensions.c_str());
      assert(false);
      return Result(Result::Code::RuntimeError);
    }
  }

  // check features
  {
    std::string missingFeatures;
#define CHECK_VULKAN_FEATURE(reqFeatures, availFeatures, feature, version)     \
  if ((reqFeatures.feature == VK_TRUE) && (availFeatures.feature == VK_FALSE)) \
    missingFeatures.append("\n   " version " ." #feature);
#define CHECK_FEATURE_1_0(feature) CHECK_VULKAN_FEATURE(deviceFeatures10, vkFeatures10_.features, feature, "1.0 ");
    CHECK_FEATURE_1_0(robustBufferAccess);
    CHECK_FEATURE_1_0(fullDrawIndexUint32);
    CHECK_FEATURE_1_0(imageCubeArray);
    CHECK_FEATURE_1_0(independentBlend);
    CHECK_FEATURE_1_0(geometryShader);
    CHECK_FEATURE_1_0(tessellationShader);
    CHECK_FEATURE_1_0(sampleRateShading);
    CHECK_FEATURE_1_0(dualSrcBlend);
    CHECK_FEATURE_1_0(logicOp);
    CHECK_FEATURE_1_0(multiDrawIndirect);
    CHECK_FEATURE_1_0(drawIndirectFirstInstance);
    CHECK_FEATURE_1_0(depthClamp);
    CHECK_FEATURE_1_0(depthBiasClamp);
    CHECK_FEATURE_1_0(fillModeNonSolid);
    CHECK_FEATURE_1_0(depthBounds);
    CHECK_FEATURE_1_0(wideLines);
    CHECK_FEATURE_1_0(largePoints);
    CHECK_FEATURE_1_0(alphaToOne);
    CHECK_FEATURE_1_0(multiViewport);
    CHECK_FEATURE_1_0(samplerAnisotropy);
    CHECK_FEATURE_1_0(textureCompressionETC2);
    CHECK_FEATURE_1_0(textureCompressionASTC_LDR);
    CHECK_FEATURE_1_0(textureCompressionBC);
    CHECK_FEATURE_1_0(occlusionQueryPrecise);
    CHECK_FEATURE_1_0(pipelineStatisticsQuery);
    CHECK_FEATURE_1_0(vertexPipelineStoresAndAtomics);
    CHECK_FEATURE_1_0(fragmentStoresAndAtomics);
    CHECK_FEATURE_1_0(shaderTessellationAndGeometryPointSize);
    CHECK_FEATURE_1_0(shaderImageGatherExtended);
    CHECK_FEATURE_1_0(shaderStorageImageExtendedFormats);
    CHECK_FEATURE_1_0(shaderStorageImageMultisample);
    CHECK_FEATURE_1_0(shaderStorageImageReadWithoutFormat);
    CHECK_FEATURE_1_0(shaderStorageImageWriteWithoutFormat);
    CHECK_FEATURE_1_0(shaderUniformBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderSampledImageArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderStorageBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderStorageImageArrayDynamicIndexing);
    CHECK_FEATURE_1_0(shaderClipDistance);
    CHECK_FEATURE_1_0(shaderCullDistance);
    CHECK_FEATURE_1_0(shaderFloat64);
    CHECK_FEATURE_1_0(shaderInt64);
    CHECK_FEATURE_1_0(shaderInt16);
    CHECK_FEATURE_1_0(shaderResourceResidency);
    CHECK_FEATURE_1_0(shaderResourceMinLod);
    CHECK_FEATURE_1_0(sparseBinding);
    CHECK_FEATURE_1_0(sparseResidencyBuffer);
    CHECK_FEATURE_1_0(sparseResidencyImage2D);
    CHECK_FEATURE_1_0(sparseResidencyImage3D);
    CHECK_FEATURE_1_0(sparseResidency2Samples);
    CHECK_FEATURE_1_0(sparseResidency4Samples);
    CHECK_FEATURE_1_0(sparseResidency8Samples);
    CHECK_FEATURE_1_0(sparseResidency16Samples);
    CHECK_FEATURE_1_0(sparseResidencyAliased);
    CHECK_FEATURE_1_0(variableMultisampleRate);
    CHECK_FEATURE_1_0(inheritedQueries);
#undef CHECK_FEATURE_1_0
#define CHECK_FEATURE_1_1(feature) CHECK_VULKAN_FEATURE(deviceFeatures11, vkFeatures11_, feature, "1.1 ");
    CHECK_FEATURE_1_1(storageBuffer16BitAccess);
    CHECK_FEATURE_1_1(uniformAndStorageBuffer16BitAccess);
    CHECK_FEATURE_1_1(storagePushConstant16);
    CHECK_FEATURE_1_1(storageInputOutput16);
    CHECK_FEATURE_1_1(multiview);
    CHECK_FEATURE_1_1(multiviewGeometryShader);
    CHECK_FEATURE_1_1(multiviewTessellationShader);
    CHECK_FEATURE_1_1(variablePointersStorageBuffer);
    CHECK_FEATURE_1_1(variablePointers);
    CHECK_FEATURE_1_1(protectedMemory);
    CHECK_FEATURE_1_1(samplerYcbcrConversion);
    CHECK_FEATURE_1_1(shaderDrawParameters);
#undef CHECK_FEATURE_1_1
#define CHECK_FEATURE_1_2(feature) CHECK_VULKAN_FEATURE(deviceFeatures12, vkFeatures12_, feature, "1.2 ");
    CHECK_FEATURE_1_2(samplerMirrorClampToEdge);
    CHECK_FEATURE_1_2(drawIndirectCount);
    CHECK_FEATURE_1_2(storageBuffer8BitAccess);
    CHECK_FEATURE_1_2(uniformAndStorageBuffer8BitAccess);
    CHECK_FEATURE_1_2(storagePushConstant8);
    CHECK_FEATURE_1_2(shaderBufferInt64Atomics);
    CHECK_FEATURE_1_2(shaderSharedInt64Atomics);
    CHECK_FEATURE_1_2(shaderFloat16);
    CHECK_FEATURE_1_2(shaderInt8);
    CHECK_FEATURE_1_2(descriptorIndexing);
    CHECK_FEATURE_1_2(shaderInputAttachmentArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayDynamicIndexing);
    CHECK_FEATURE_1_2(shaderUniformBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderSampledImageArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageImageArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderInputAttachmentArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderUniformTexelBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(shaderStorageTexelBufferArrayNonUniformIndexing);
    CHECK_FEATURE_1_2(descriptorBindingUniformBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingSampledImageUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageImageUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingUniformTexelBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingStorageTexelBufferUpdateAfterBind);
    CHECK_FEATURE_1_2(descriptorBindingUpdateUnusedWhilePending);
    CHECK_FEATURE_1_2(descriptorBindingPartiallyBound);
    CHECK_FEATURE_1_2(descriptorBindingVariableDescriptorCount);
    CHECK_FEATURE_1_2(runtimeDescriptorArray);
    CHECK_FEATURE_1_2(samplerFilterMinmax);
    CHECK_FEATURE_1_2(scalarBlockLayout);
    CHECK_FEATURE_1_2(imagelessFramebuffer);
    CHECK_FEATURE_1_2(uniformBufferStandardLayout);
    CHECK_FEATURE_1_2(shaderSubgroupExtendedTypes);
    CHECK_FEATURE_1_2(separateDepthStencilLayouts);
    CHECK_FEATURE_1_2(hostQueryReset);
    CHECK_FEATURE_1_2(timelineSemaphore);
    CHECK_FEATURE_1_2(bufferDeviceAddress);
    CHECK_FEATURE_1_2(bufferDeviceAddressCaptureReplay);
    CHECK_FEATURE_1_2(bufferDeviceAddressMultiDevice);
    CHECK_FEATURE_1_2(vulkanMemoryModel);
    CHECK_FEATURE_1_2(vulkanMemoryModelDeviceScope);
    CHECK_FEATURE_1_2(vulkanMemoryModelAvailabilityVisibilityChains);
    CHECK_FEATURE_1_2(shaderOutputViewportIndex);
    CHECK_FEATURE_1_2(shaderOutputLayer);
    CHECK_FEATURE_1_2(subgroupBroadcastDynamicId);
#undef CHECK_FEATURE_1_2
#define CHECK_FEATURE_1_3(feature) CHECK_VULKAN_FEATURE(deviceFeatures13, vkFeatures13_, feature, "1.3 ");
    CHECK_FEATURE_1_3(robustImageAccess);
    CHECK_FEATURE_1_3(inlineUniformBlock);
    CHECK_FEATURE_1_3(descriptorBindingInlineUniformBlockUpdateAfterBind);
    CHECK_FEATURE_1_3(pipelineCreationCacheControl);
    CHECK_FEATURE_1_3(privateData);
    CHECK_FEATURE_1_3(shaderDemoteToHelperInvocation);
    CHECK_FEATURE_1_3(shaderTerminateInvocation);
    CHECK_FEATURE_1_3(subgroupSizeControl);
    CHECK_FEATURE_1_3(computeFullSubgroups);
    CHECK_FEATURE_1_3(synchronization2);
    CHECK_FEATURE_1_3(textureCompressionASTC_HDR);
    CHECK_FEATURE_1_3(shaderZeroInitializeWorkgroupMemory);
    CHECK_FEATURE_1_3(dynamicRendering);
    CHECK_FEATURE_1_3(shaderIntegerDotProduct);
    CHECK_FEATURE_1_3(maintenance4);
#undef CHECK_FEATURE_1_3
    if (!missingFeatures.empty())
    {
      LOG_WARNING("Missing Vulkan features: {}\n", missingFeatures.c_str());
    }
  }

  const VkDeviceCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = createInfoNext, .queueCreateInfoCount = numQueues, .pQueueCreateInfos = ciQueue, .enabledExtensionCount = (uint32_t)deviceExtensionNames.size(), .ppEnabledExtensionNames = deviceExtensionNames.data(), .pEnabledFeatures = &deviceFeatures10,};
  VK_ASSERT_RETURN(vkCreateDevice(vkPhysicalDevice_, &ci, nullptr, &vkDevice_));

  volkLoadDevice(vkDevice_);

  vkGetDeviceQueue(vkDevice_, deviceQueues_.graphicsQueueFamilyIndex, 0, &deviceQueues_.graphicsQueue);
  vkGetDeviceQueue(vkDevice_, deviceQueues_.computeQueueFamilyIndex, 0, &deviceQueues_.computeQueue);

  std::string deviceName = vkPhysicalDeviceProperties2_.properties.deviceName;

  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_DEVICE, (uint64_t)vkDevice_, "Device: VulkanContext::vkDevice_" ));
  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_INSTANCE, (uint64_t)vkInstance_, "Instance: VulkanContext::vkInstance_"));
  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_PHYSICAL_DEVICE, (uint64_t)vkPhysicalDevice_, (std::string( "Physical Device: VulkanContext::vkPhysicalDevice_ (") + deviceName + ")").c_str()));
  depthResolveMode_ = [this]() -> VkResolveModeFlagBits
  {
    const VkResolveModeFlags modes = vkPhysicalDeviceDepthStencilResolveProperties_.supportedDepthResolveModes;
    if (modes & VK_RESOLVE_MODE_AVERAGE_BIT)
      return VK_RESOLVE_MODE_AVERAGE_BIT;
    // this mode is always available
    return VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
  }();

  immediate_ = std::make_unique<VulkanImmediateCommands>(vkDevice_, deviceQueues_.graphicsQueueFamilyIndex, "VulkanContext::immediate_");

  // create Vulkan pipeline cache
  {
    const VkPipelineCacheCreateInfo cache_create_info = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, nullptr, VkPipelineCacheCreateFlags(0), config_.pipelineCacheDataSize, config_.pipelineCacheData,};
    vkCreatePipelineCache(vkDevice_, &cache_create_info, nullptr, &pipelineCache_);
  }

  pimpl_->vma_ = createVmaAllocator(vkPhysicalDevice_, vkDevice_, vkInstance_, apiVersion > VK_API_VERSION_1_3 ? VK_API_VERSION_1_3 : apiVersion);
  ASSERT(pimpl_->vma_ != VK_NULL_HANDLE);

  stagingDevice_ = std::make_unique<VulkanStagingDevice>(*this);

  // default texture
  {
    const uint32_t texWidth = 32;
    const uint32_t texHeight = 32;
    std::vector<uint32_t> pixels(texWidth * texHeight);
    const uint32_t squareSize = 8;
    const uint32_t color1 = 0xFFFF00FF; // Magenta
    const uint32_t color2 = 0xFF000000; // Black
    for (uint32_t y = 0; y != texHeight; y++)
    {
      for (uint32_t x = 0; x != texWidth; x++)
      {
        uint32_t x_square = x / squareSize;
        uint32_t y_square = y / squareSize;
        if ((x_square + y_square) % 2 == 0)
        {
          pixels[y * texWidth + x] = color1;
        }
        else
        {
          pixels[y * texWidth + x] = color2;
        }
      }
    }
    Result result;
    dummyTexture_ = this->createTexture({.format = Format::BGRA_UN8, .dimensions = {.width = texWidth, .height = texHeight}, .usage = TextureUsageBits_Sampled | TextureUsageBits_Storage, .data = pixels.data(),}, "DummyTexture", &result).release();
    if (!VERIFY(result.isOk()))
    {
      return result;
    }
    ASSERT(texturesPool_.numObjects() == 1);
  }

  // default sampler
  ASSERT(samplersPool_.numObjects() == 0);
  createSampler({.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr, .flags = 0, .magFilter = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR, .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR, .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT, .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT, .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT, .mipLodBias = 0.0f, .anisotropyEnable = VK_FALSE, .maxAnisotropy = 0.0f, .compareEnable = VK_FALSE, .compareOp = VK_COMPARE_OP_ALWAYS, .minLod = 0.0f, .maxLod = static_cast<float>(MAX_MIP_LEVELS - 1), .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK, .unnormalizedCoordinates = VK_FALSE,}, nullptr, Format::Invalid, "Sampler: default");

  growDescriptorPool(currentMaxTextures_, currentMaxSamplers_, currentMaxAccelStructs_);

  querySurfaceCapabilities();

#if defined(TRACY_ENABLE)
  std::vector<VkTimeDomainEXT> timeDomains;

  if (hasCalibratedTimestamps_)
  {
    uint32_t numTimeDomains = 0;
    VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(vkPhysicalDevice_, &numTimeDomains, nullptr));
    timeDomains.resize(numTimeDomains);
    VK_ASSERT(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(vkPhysicalDevice_, &numTimeDomains, timeDomains.data()));
  }

  const bool hasHostQuery = vkFeatures12_.hostQueryReset && [&timeDomains]() -> bool
  {
    for (VkTimeDomainEXT domain : timeDomains)
      if (domain == VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT || domain == VK_TIME_DOMAIN_QUERY_PERFORMANCE_COUNTER_EXT)
        return true;
    return false;
  }();

  if (hasHostQuery)
  {
    pimpl_->tracyVkCtx_ = TracyVkContextHostCalibrated(vkPhysicalDevice_, vkDevice_, vkResetQueryPool, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
  }
  else
  {
    const VkCommandPoolCreateInfo ciCommandPool = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, .queueFamilyIndex = deviceQueues_.graphicsQueueFamilyIndex,};
    VK_ASSERT(vkCreateCommandPool(vkDevice_, &ciCommandPool, nullptr, &pimpl_->tracyCommandPool_));
    setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t)pimpl_->tracyCommandPool_, "Command Pool: VulkanContextImpl::tracyCommandPool_");
    const VkCommandBufferAllocateInfo aiCommandBuffer = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = pimpl_->tracyCommandPool_, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1,};
    VK_ASSERT(vkAllocateCommandBuffers(vkDevice_, &aiCommandBuffer, &pimpl_->tracyCommandBuffer_));
    if (hasCalibratedTimestamps_)
    {
      pimpl_->tracyVkCtx_ = TracyVkContextCalibrated(vkPhysicalDevice_, vkDevice_, deviceQueues_.graphicsQueue, pimpl_->tracyCommandBuffer_, vkGetPhysicalDeviceCalibrateableTimeDomainsEXT, vkGetCalibratedTimestampsEXT);
    }
    else
    {
      pimpl_->tracyVkCtx_ = TracyVkContext(vkPhysicalDevice_, vkDevice_, deviceQueues_.graphicsQueue, pimpl_->tracyCommandBuffer_);
    };
  }
  ASSERT(pimpl_->tracyVkCtx_);
#endif // TRACY_ENABLE

  return Result();
}

void vk::VulkanContext::createSurface(void* nativeWindow, void* display)
{
  // Check for existing surface
  if (vkSurface_ != VK_NULL_HANDLE)
  {
    LOG_WARNING("Surface already exists, destroying old surface first");
    destroySurface();
  }

  if (!nativeWindow)
  {
    LOG_ERROR("Invalid native window handle");
    return;
  }

  // Create platform-specific surface
#if defined(VK_USE_PLATFORM_WIN32_KHR)
  const VkWin32SurfaceCreateInfoKHR ci = {.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, .hinstance = GetModuleHandle(nullptr), .hwnd = (HWND)nativeWindow,};
  VK_ASSERT(vkCreateWin32SurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
  const VkAndroidSurfaceCreateInfoKHR ci = {
    .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR, .pNext = nullptr, .flags = 0, .window = (ANativeWindow*)nativeWindow };
  VK_ASSERT(vkCreateAndroidSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  const VkXlibSurfaceCreateInfoKHR ci = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .flags = 0,
    .dpy = (Display*)display,
    .window = (Window)nativeWindow,
  };
  VK_ASSERT(vkCreateXlibSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
  const VkWaylandSurfaceCreateInfoKHR ci = {
    .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
    .flags = 0,
    .display = (wl_display*)display,
    .surface = (wl_surface*)nativeWindow,
  };
  VK_ASSERT(vkCreateWaylandSurfaceKHR(vkInstance_, &ci, nullptr, &vkSurface_));
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
  const VkMacOSSurfaceCreateInfoMVK ci = {
    .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
    .flags = 0,
    .pView = nativeWindow,
  };
  VK_ASSERT(vkCreateMacOSSurfaceMVK(vkInstance_, &ci, nullptr, &vkSurface_));
#else
#error Implement for other platforms
#endif

  // Verify surface compatibility with physical device
  if (vkPhysicalDevice_ != VK_NULL_HANDLE)
  {
    VkBool32 supported = VK_FALSE;
    VK_ASSERT(vkGetPhysicalDeviceSurfaceSupportKHR(
      vkPhysicalDevice_,
      deviceQueues_.graphicsQueueFamilyIndex,
      vkSurface_,
      &supported
    ));

    if (!supported)
    {
      LOG_ERROR("Physical device does not support presentation to this surface");
      vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
      vkSurface_ = VK_NULL_HANDLE;
    }
  }
}

void vk::VulkanContext::destroySurface()
{
  // CRITICAL: Wait for GPU to be idle
  if (vkDevice_ != VK_NULL_HANDLE)
  {
    VK_ASSERT(vkDeviceWaitIdle(vkDevice_));
  }

  // Destroy swapchain first
  if (swapchain_)
  {
    swapchain_.reset();
    timelineSemaphore_ = VK_NULL_HANDLE;
  }

  // Destroy surface
  if (vkSurface_ != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(vkInstance_, vkSurface_, nullptr);
    vkSurface_ = VK_NULL_HANDLE;
  }

  // Reset state
  shouldRecreate_ = false;
  newWidth_ = 0;
  newHeight_ = 0;
}

vk::Result vk::VulkanContext::initSwapchain(uint32_t width, uint32_t height)
{
  if (!vkDevice_ || !immediate_)
  {
    LOG_WARNING("Call initContext() first");
    return Result(Result::Code::RuntimeError, "Call initContext() first");
  }

  // Check for surface
  if (vkSurface_ == VK_NULL_HANDLE)
  {
    LOG_ERROR("No surface available. Call createSurface() first.");
    return Result(Result::Code::RuntimeError, "No surface available");
  }

  if (!width || !height)
  {
    // Destroy existing swapchain
    swapchain_.reset();
    return Result();
  }

  if (swapchain_)
  {
    // Reinitialize existing swapchain
    uint32_t actualWidth = width;
    uint32_t actualHeight = height;
    return swapchain_->reinitResources(actualWidth, actualHeight, !config_.disableVSync);
  }
  else
  {
    // Create new swapchain
    swapchain_ = std::make_unique<VulkanSwapchain>();

    VulkanSwapchain::InitInfo info = {.ctx = *this, .width = width, .height = height, .vSync = !config_.disableVSync, .oldSwapchain = VK_NULL_HANDLE};

    Result result = swapchain_->init(info);
    if (!result.isOk())
    {
      swapchain_.reset();
      return result;
    }

    // Create timeline semaphore for synchronization
    timelineSemaphore_ = createSemaphoreTimeline(vkDevice_, swapchain_->getNumSwapchainImages() - 1, "Semaphore: timelineSemaphore_");
  }

  shouldRecreate_ = false;
  return Result();
}

vk::Result vk::VulkanContext::growDescriptorPool(uint32_t maxTextures, uint32_t maxSamplers, uint32_t maxAccelStructs)
{
  currentMaxTextures_ = maxTextures;
  currentMaxSamplers_ = maxSamplers;
  currentMaxAccelStructs_ = maxAccelStructs;

#if VULKAN_PRINT_COMMANDS
	LOG_INFO("growDescriptorPool({}}, {})\n", maxTextures, maxSamplers);
#endif // VULKAN_PRINT_COMMANDS

  if (!VERIFY(maxTextures <= vkPhysicalDeviceVulkan12Properties_.maxDescriptorSetUpdateAfterBindSampledImages))
  {
    LOG_WARNING("Max Textures exceeded: {} (max {})", maxTextures, vkPhysicalDeviceVulkan12Properties_.maxDescriptorSetUpdateAfterBindSampledImages);
  }

  if (!VERIFY(maxSamplers <= vkPhysicalDeviceVulkan12Properties_.maxDescriptorSetUpdateAfterBindSamplers))
  {
    LOG_WARNING("Max Samplers exceeded {} (max {})", maxSamplers, vkPhysicalDeviceVulkan12Properties_.maxDescriptorSetUpdateAfterBindSamplers);
  }

  if (vkDSL_ != VK_NULL_HANDLE)
  {
    deferredTask(std::packaged_task<void()>([device = vkDevice_, dsl = vkDSL_]()
    {
      vkDestroyDescriptorSetLayout(device, dsl, nullptr);
    }));
  }
  if (vkDPool_ != VK_NULL_HANDLE)
  {
    deferredTask(std::packaged_task<void()>([device = vkDevice_, dp = vkDPool_]()
    {
      vkDestroyDescriptorPool(device, dp, nullptr);
    }));
  }

  bool hasYUVImages = false;

  // check if we have any YUV images
  for (const auto& obj : texturesPool_.objects_)
  {
    const VulkanImage* img = &obj.obj_;
    // multisampled images cannot be directly accessed from shaders
    const bool isTextureAvailable = (img->vkSamples_ & VK_SAMPLE_COUNT_1_BIT) == VK_SAMPLE_COUNT_1_BIT;
    hasYUVImages = isTextureAvailable && img->isSampledImage() && getNumImagePlanes(img->vkImageFormat_) > 1;
    if (hasYUVImages)
    {
      break;
    }
  }

  std::vector<VkSampler> immutableSamplers;
  const VkSampler* immutableSamplersData = nullptr;

  if (hasYUVImages)
  {
    VkSampler dummySampler = samplersPool_.objects_[0].obj_;
    immutableSamplers.reserve(texturesPool_.objects_.size());
    for (const auto& obj : texturesPool_.objects_)
    {
      const VulkanImage* img = &obj.obj_;
      // multisampled images cannot be directly accessed from shaders
      const bool isTextureAvailable = (img->vkSamples_ & VK_SAMPLE_COUNT_1_BIT) == VK_SAMPLE_COUNT_1_BIT;
      const bool isYUVImage = isTextureAvailable && img->isSampledImage() && getNumImagePlanes(img->vkImageFormat_) > 1;
      immutableSamplers.push_back(isYUVImage ? getOrCreateYcbcrSampler(vkFormatToFormat(img->vkImageFormat_)) : dummySampler);
    }
    immutableSamplersData = immutableSamplers.data();
  }

  // create default descriptor set layout which is going to be shared by graphics pipelines
  VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;
  if (hasRayTracingPipeline_)
  {
    stageFlags |= (VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_INTERSECTION_BIT_KHR | VK_SHADER_STAGE_CALLABLE_BIT_KHR);
  }
  const VkDescriptorSetLayoutBinding bindings[kBinding_NumBindings] = {getDSLBinding(kBinding_Textures, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures, stageFlags), getDSLBinding(kBinding_Samplers, VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers, stageFlags), getDSLBinding(kBinding_StorageImages, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures, stageFlags), getDSLBinding(kBinding_YUVImages, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, (uint32_t)immutableSamplers.size(), stageFlags, immutableSamplersData), getDSLBinding(kBinding_AccelerationStructures, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxAccelStructs, stageFlags),};
  const uint32_t flags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
  VkDescriptorBindingFlags bindingFlags[kBinding_NumBindings];
  for (int i = 0; i < kBinding_NumBindings; ++i)
  {
    bindingFlags[i] = flags;
  }
  const VkDescriptorSetLayoutBindingFlagsCreateInfo setLayoutBindingFlagsCI = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, .bindingCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1), .pBindingFlags = bindingFlags,};
  const VkDescriptorSetLayoutCreateInfo dslci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .pNext = &setLayoutBindingFlagsCI, .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT, .bindingCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1), .pBindings = bindings,};
  VK_ASSERT(vkCreateDescriptorSetLayout(vkDevice_, &dslci, nullptr, &vkDSL_));
  VK_ASSERT(vk::setDebugObjectName( vkDevice_, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)vkDSL_, "Descriptor Set Layout: VulkanContext::vkDSL_"));

  {
    // create default descriptor pool and allocate 1 descriptor set
    const VkDescriptorPoolSize poolSizes[kBinding_NumBindings]{VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures}, VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, maxAccelStructs},};
    const VkDescriptorPoolCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT, .maxSets = 1, .poolSizeCount = uint32_t(hasAccelerationStructure_ ? kBinding_NumBindings : kBinding_NumBindings - 1), .pPoolSizes = poolSizes,};
    VK_ASSERT_RETURN(vkCreateDescriptorPool(vkDevice_, &ci, nullptr, &vkDPool_));
    const VkDescriptorSetAllocateInfo ai = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = vkDPool_, .descriptorSetCount = 1, .pSetLayouts = &vkDSL_,};
    VK_ASSERT_RETURN(vkAllocateDescriptorSets(vkDevice_, &ai, &vkDSet_));
  }

  awaitingNewImmutableSamplers_ = false;

  return Result();
}

vk::BufferHandle vk::VulkanContext::createBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memFlags, Result* outResult, const char* debugName)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_CREATE);

  ASSERT(bufferSize > 0);

#define ENSURE_BUFFER_SIZE(flag, maxSize)                                                             \
  if (usageFlags & flag) {                                                                            \
    if (!VERIFY(bufferSize <= maxSize)) {                                                         \
      Result::setResult(outResult, Result(Result::Code::RuntimeError, "Buffer size exceeded" #flag)); \
      return {};                                                                                      \
    }                                                                                                 \
  }

  const VkPhysicalDeviceLimits& limits = getVkPhysicalDeviceProperties().limits;
  ENSURE_BUFFER_SIZE(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, limits.maxUniformBufferRange);
  // any buffer
  ENSURE_BUFFER_SIZE(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM, limits.maxStorageBufferRange);
#undef ENSURE_BUFFER_SIZE

  VulkanBuffer buf = {.bufferSize_ = bufferSize, .vkUsageFlags_ = usageFlags, .vkMemFlags_ = memFlags,};

  const VkBufferCreateInfo ci = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = nullptr, .flags = 0, .size = bufferSize, .usage = usageFlags, .sharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 0, .pQueueFamilyIndices = nullptr,};

  VmaAllocationCreateInfo vmaAllocInfo = {};

  // Initialize VmaAllocation Info
  if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  {
    vmaAllocInfo = {.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT, .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, .preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,};
  }

  if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  {
    // Check if coherent buffer is available.
    VK_ASSERT(vkCreateBuffer(vkDevice_, &ci, nullptr, &buf.vkBuffer_));
    VkMemoryRequirements requirements = {};
    vkGetBufferMemoryRequirements(vkDevice_, buf.vkBuffer_, &requirements);
    vkDestroyBuffer(vkDevice_, buf.vkBuffer_, nullptr);
    buf.vkBuffer_ = VK_NULL_HANDLE;

    if (requirements.memoryTypeBits & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
    {
      vmaAllocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      buf.isCoherentMemory_ = true;
    }
  }

  vmaAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  vmaCreateBufferWithAlignment((VmaAllocator)getVmaAllocator(), &ci, &vmaAllocInfo, 16, &buf.vkBuffer_, &buf.vmaAllocation_, nullptr);

  // handle memory-mapped buffers
  if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
  {
    vmaMapMemory((VmaAllocator)getVmaAllocator(), buf.vmaAllocation_, &buf.mappedPtr_);
  }

  ASSERT(buf.vkBuffer_ != VK_NULL_HANDLE);

  // set debug name
  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_BUFFER, (uint64_t)buf.vkBuffer_, debugName));

  // handle shader access
  if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
  {
    const VkBufferDeviceAddressInfo ai = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buf.vkBuffer_,};
    buf.vkDeviceAddress_ = vkGetBufferDeviceAddress(vkDevice_, &ai);
    ASSERT(buf.vkDeviceAddress_);
  }

  return buffersPool_.create(std::move(buf));
}

void vk::VulkanContext::bindDefaultDescriptorSets(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint, VkPipelineLayout layout) const
{
  PROFILER_FUNCTION();
  const VkDescriptorSet dsets[4] = {vkDSet_, vkDSet_, vkDSet_, vkDSet_};
  vkCmdBindDescriptorSets(cmdBuf, bindPoint, layout, 0, (uint32_t)std::size(dsets), dsets, 0, nullptr);
}

void vk::VulkanContext::checkAndUpdateDescriptorSets()
{
  if (!awaitingCreation_)
  {
    // nothing to update here
    return;
  }

  // newly created resources can be used immediately - make sure they are put into descriptor sets
  PROFILER_FUNCTION();

  // update Vulkan descriptor set here

  // make sure the guard values are always there
  ASSERT(texturesPool_.numObjects() >= 1);
  ASSERT(samplersPool_.numObjects() >= 1);

  uint32_t newMaxTextures = currentMaxTextures_;
  uint32_t newMaxSamplers = currentMaxSamplers_;
  uint32_t newMaxAccelStructs = currentMaxAccelStructs_;

  while (texturesPool_.objects_.size() > newMaxTextures)
  {
    newMaxTextures *= 2;
  }
  while (samplersPool_.objects_.size() > newMaxSamplers)
  {
    newMaxSamplers *= 2;
  }
  while (accelStructuresPool_.objects_.size() > newMaxAccelStructs)
  {
    newMaxAccelStructs *= 2;
  }
  if (newMaxTextures != currentMaxTextures_ || newMaxSamplers != currentMaxSamplers_ || awaitingNewImmutableSamplers_ || newMaxAccelStructs != currentMaxAccelStructs_)
  {
    growDescriptorPool(newMaxTextures, newMaxSamplers, newMaxAccelStructs);
  }

  // 1. Sampled and storage images
  std::vector<VkDescriptorImageInfo> infoSampledImages;
  std::vector<VkDescriptorImageInfo> infoStorageImages;
  std::vector<VkDescriptorImageInfo> infoYUVImages;

  infoSampledImages.reserve(texturesPool_.numObjects());
  infoStorageImages.reserve(texturesPool_.numObjects());

  const bool hasYcbcrSamplers = pimpl_->numYcbcrSamplers_ > 0;

  if (hasYcbcrSamplers)
  {
    infoYUVImages.reserve(texturesPool_.numObjects());
  }

  // use the dummy texture to avoid sparse array
  VkImageView dummyImageView = texturesPool_.objects_[0].obj_.imageView_;

  for (const auto& obj : texturesPool_.objects_)
  {
    const VulkanImage& img = obj.obj_;
    const VkImageView view = obj.obj_.imageView_;
    const VkImageView storageView = obj.obj_.imageViewStorage_ ? obj.obj_.imageViewStorage_ : view;
    // multisampled images cannot be directly accessed from shaders
    const bool isTextureAvailable = (img.vkSamples_ & VK_SAMPLE_COUNT_1_BIT) == VK_SAMPLE_COUNT_1_BIT;
    const bool isYUVImage = isTextureAvailable && img.isSampledImage() && getNumImagePlanes(img.vkImageFormat_) > 1;
    const bool isSampledImage = isTextureAvailable && img.isSampledImage() && !isYUVImage;
    const bool isStorageImage = isTextureAvailable && img.isStorageImage();
    infoSampledImages.push_back(VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE, .imageView = isSampledImage ? view : dummyImageView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,});
    ASSERT(infoSampledImages.back().imageView != VK_NULL_HANDLE);
    infoStorageImages.push_back(VkDescriptorImageInfo{.sampler = VK_NULL_HANDLE, .imageView = isStorageImage ? storageView : dummyImageView, .imageLayout = VK_IMAGE_LAYOUT_GENERAL,});
    if (hasYcbcrSamplers)
    {
      // we don't need to update this if there're no YUV samplers
      infoYUVImages.push_back(VkDescriptorImageInfo{.imageView = isYUVImage ? view : dummyImageView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,});
    }
  }

  // 2. Samplers
  std::vector<VkDescriptorImageInfo> infoSamplers;
  infoSamplers.reserve(samplersPool_.objects_.size());

  for (const auto& sampler : samplersPool_.objects_)
  {
    infoSamplers.push_back({.sampler = sampler.obj_ ? sampler.obj_ : samplersPool_.objects_[0].obj_, .imageView = VK_NULL_HANDLE, .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,});
  }

  // 3. Acceleration structures
  std::vector<VkAccelerationStructureKHR> handlesAccelStructs;
  handlesAccelStructs.reserve(accelStructuresPool_.objects_.size());
  // Only add valid (non-null) acceleration structure handles
  for (const auto& as : accelStructuresPool_.objects_)
  {
    if (as.obj_.vkHandle != VK_NULL_HANDLE)
    {
      handlesAccelStructs.push_back(as.obj_.vkHandle);
    }
  }

  VkWriteDescriptorSet write[kBinding_NumBindings] = {};
  uint32_t numWrites = 0;

  // Only create the descriptor write if we have valid acceleration structures
  if (!handlesAccelStructs.empty())
  {
    VkWriteDescriptorSetAccelerationStructureKHR writeAccelStruct = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, .accelerationStructureCount = (uint32_t)handlesAccelStructs.size(), .pAccelerationStructures = handlesAccelStructs.data(),};

    write[numWrites++] = VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = &writeAccelStruct, .dstSet = vkDSet_, .dstBinding = kBinding_AccelerationStructures, .dstArrayElement = 0, .descriptorCount = (uint32_t)handlesAccelStructs.size(), .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,};
  }

  if (!infoSampledImages.empty())
  {
    write[numWrites++] = VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vkDSet_, .dstBinding = kBinding_Textures, .dstArrayElement = 0, .descriptorCount = (uint32_t)infoSampledImages.size(), .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .pImageInfo = infoSampledImages.data(),};
  }

  if (!infoSamplers.empty())
  {
    write[numWrites++] = VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vkDSet_, .dstBinding = kBinding_Samplers, .dstArrayElement = 0, .descriptorCount = (uint32_t)infoSamplers.size(), .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER, .pImageInfo = infoSamplers.data(),};
  }

  if (!infoStorageImages.empty())
  {
    write[numWrites++] = VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vkDSet_, .dstBinding = kBinding_StorageImages, .dstArrayElement = 0, .descriptorCount = (uint32_t)infoStorageImages.size(), .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .pImageInfo = infoStorageImages.data(),};
  }

  if (!infoYUVImages.empty())
  {
    write[numWrites++] = VkWriteDescriptorSet{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = vkDSet_, .dstBinding = kBinding_YUVImages, .dstArrayElement = 0, .descriptorCount = (uint32_t)infoYUVImages.size(), .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = infoYUVImages.data(),};
  }

  // do not switch to the next descriptor set if there is nothing to update
  if (numWrites)
  {
#if VULKAN_PRINT_COMMANDS
		LOG_INFO("vkUpdateDescriptorSets()\n");
#endif // VULKAN_PRINT_COMMANDS
    immediate_->wait(immediate_->getLastSubmitHandle());
    PROFILER_ZONE("vkUpdateDescriptorSets()", PROFILER_COLOR_PRESENT);
      vkUpdateDescriptorSets(vkDevice_, numWrites, write, 0, nullptr);
    PROFILER_ZONE_END();
  }

  awaitingCreation_ = false;
}

vk::SamplerHandle vk::VulkanContext::createSampler(const VkSamplerCreateInfo& ci, [[maybe_unused]] Result* outResult, Format yuvFormat, const char* debugName)
{
  PROFILER_FUNCTION_COLOR(PROFILER_COLOR_CREATE);

  VkSamplerCreateInfo cinfo = ci;

  if (yuvFormat != Format::Invalid)
  {
    cinfo.pNext = getOrCreateYcbcrConversionInfo(yuvFormat);
    // must be CLAMP_TO_EDGE
    // https://vulkan.lunarg.com/doc/view/1.3.268.0/windows/1.3-extensions/vkspec.html#VUID-VkSamplerCreateInfo-addressModeU-01646
    cinfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cinfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cinfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    cinfo.anisotropyEnable = VK_FALSE;
    cinfo.unnormalizedCoordinates = VK_FALSE;
  }

  VkSampler sampler = VK_NULL_HANDLE;
  VK_ASSERT(vkCreateSampler(vkDevice_, &cinfo, nullptr, &sampler));
  VK_ASSERT(vk::setDebugObjectName(vkDevice_, VK_OBJECT_TYPE_SAMPLER, (uint64_t)sampler, debugName));

  SamplerHandle handle = samplersPool_.create(VkSampler(sampler));

  awaitingCreation_ = true;

  return handle;
}

void vk::VulkanContext::querySurfaceCapabilities()
{
  // enumerate only the formats we are using
  const VkFormat depthFormats[] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM};
  for (const VkFormat& depthFormat : depthFormats)
  {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(vkPhysicalDevice_, depthFormat, &formatProps);

    if (formatProps.optimalTilingFeatures)
    {
      deviceDepthFormats_.push_back(depthFormat);
    }
  }

  if (vkSurface_ == VK_NULL_HANDLE)
  {
    return;
  }

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice_, vkSurface_, &deviceSurfaceCaps_);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_, vkSurface_, &formatCount, nullptr);

  if (formatCount)
  {
    deviceSurfaceFormats_.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice_, vkSurface_, &formatCount, deviceSurfaceFormats_.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_, vkSurface_, &presentModeCount, nullptr);

  if (presentModeCount)
  {
    devicePresentModes_.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice_, vkSurface_, &presentModeCount, devicePresentModes_.data());
  }
}

std::vector<uint8_t> vk::VulkanContext::getPipelineCacheData() const
{
  size_t size = 0;
  vkGetPipelineCacheData(vkDevice_, pipelineCache_, &size, nullptr);

  std::vector<uint8_t> data(size);

  if (size)
  {
    vkGetPipelineCacheData(vkDevice_, pipelineCache_, &size, data.data());
  }

  return data;
}

void vk::VulkanContext::deferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) const
{
  if (handle.empty())
  {
    handle = immediate_->getNextSubmitHandle();
  }
  pimpl_->deferredTasks_.emplace_back(std::move(task), handle);
}

void* vk::VulkanContext::getVmaAllocator() const
{
  return pimpl_->vma_;
}

void vk::VulkanContext::processDeferredTasks() const
{
  std::vector<DeferredTask>::iterator it = pimpl_->deferredTasks_.begin();

  while (it != pimpl_->deferredTasks_.end() && immediate_->isReady(it->handle_, true))
  {
    (it++)->task_();
  }

  pimpl_->deferredTasks_.erase(pimpl_->deferredTasks_.begin(), it);
}

void vk::VulkanContext::waitDeferredTasks()
{
  for (auto& task : pimpl_->deferredTasks_)
  {
    immediate_->wait(task.handle_);
    task.task_();
  }
  pimpl_->deferredTasks_.clear();
}

void vk::VulkanContext::invokeShaderModuleErrorCallback(int line, int col, const char* debugName, VkShaderModule sm)
{
  if (!config_.shaderModuleErrorCallback)
  {
    return;
  }

  ShaderModuleHandle handle;

  for (uint32_t i = 0; i != shaderModulesPool_.objects_.size(); i++)
  {
    if (shaderModulesPool_.objects_[i].obj_.sm == sm)
    {
      handle = shaderModulesPool_.getHandle(i);
    }
  }

  if (!handle.empty())
  {
    config_.shaderModuleErrorCallback(this, handle, line, col, debugName);
  }
}

uint32_t vk::VulkanContext::getMaxStorageBufferRange() const
{
  return vkPhysicalDeviceProperties2_.properties.limits.maxStorageBufferRange;
}

#if defined(__ANDROID__)
SurfaceTransform vk::VulkanContext::getSwapchainPreTransform() const
{
    if (!swapchain_)
    {
        return SurfaceTransform::Identity;
    }

    VkSurfaceTransformFlagBitsKHR vkTransform = swapchain_->getPreTransform();

    switch (vkTransform)
    {
    case VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR:
        return SurfaceTransform::Identity;
    case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
        return SurfaceTransform::Rotate90;
    case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
        return SurfaceTransform::Rotate180;
    case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
        return SurfaceTransform::Rotate270;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR:
        return SurfaceTransform::HorizontalMirror;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR:
        return SurfaceTransform::HorizontalMirrorRotate90;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR:
        return SurfaceTransform::HorizontalMirrorRotate180;
    case VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR:
        return SurfaceTransform::HorizontalMirrorRotate270;
    case VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR:
        return SurfaceTransform::Inherit;
    default:
        return SurfaceTransform::Identity;
    }
}
#endif
