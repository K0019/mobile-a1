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

// =============================================================================
// DEPRECATED - This file contains the old vk:: abstraction layer
//
// The vk:: namespace (Handle<T>, Holder<T>, IContext, ICommandBuffer) is being
// phased out in favor of direct hina-vk usage through gfx_interface.h.
//
// Still in use:
// - vk::Format, vk::ColorSpace - used by HinaContext for swapchain queries
//
// For new code:
// - Use gfx:: types from gfx_interface.h
// - Use gfx::Holder<T> for RAII resource management
// - Create pipelines directly with hina_make_pipeline_ex()
//
// See grid_feature.cpp and im3d_feature.cpp for migration examples.
// =============================================================================

#pragma once

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

#include "core_utils/util.h"
#include "logging/log.h"
#include "logging/profiler.h"

#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

// Only include GLFW on non-Android platforms
#if !defined(__ANDROID__)
typedef struct GLFWwindow GLFWwindow;
#endif

#if defined(__ANDROID__)
  using window = ANativeWindow;
#else
  using window = GLFWwindow;
#endif

// Platform-independent wrapper for Vulkan surface transform flags
// Used on Android for display rotation handling, always Identity on desktop
enum class SurfaceTransform : uint8_t
{
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

// Forward declaration for hina-vk backend
class HinaContext;

namespace vk
{
  class IContext;

  // Non-ref counted handles; based on:
  // https://enginearchitecture.realtimerendering.com/downloads/reac2023_modern_mobile_rendering_at_hypehype.pdf
  template <typename ObjectType>
  class Handle final
  {
    public:
      Handle() = default;

      [[nodiscard]] bool empty() const { return gen_ == 0; }

      [[nodiscard]] bool valid() const { return gen_ != 0; }

      [[nodiscard]] uint32_t index() const { return index_; }

      [[nodiscard]] uint32_t gen() const { return gen_; }

      [[nodiscard]] void* indexAsVoid() const { return reinterpret_cast<void*>(static_cast<ptrdiff_t>(index_)); }

      bool operator==(const Handle& other) const { return index_ == other.index_ && gen_ == other.gen_; }

      bool operator!=(const Handle& other) const { return index_ != other.index_ || gen_ != other.gen_; }

      bool operator>(const Handle& other) const
      {
        if (index_ != other.index_) { return index_ > other.index_; }
        return gen_ > other.gen_;
      }
      bool operator<(const Handle& other) const
      {
        if (index_ != other.index_) { return index_ < other.index_; }
        return gen_ < other.gen_;
      }
      explicit operator bool() const { return gen_ != 0; }

    private:
      Handle(uint32_t index, uint32_t gen) : index_(index), gen_(gen) {}

      template <typename ObjectType_, typename ImplObjectType>
      friend class Pool;

      // Allow HinaContext to create handles for the hina-vk backend
      friend class ::HinaContext;

      uint32_t index_ = 0;
      uint32_t gen_ = 0;
  };

  static_assert(sizeof(Handle<class Foo>) == sizeof(uint64_t));

  // specialized with dummy structs for type safety
  using ComputePipelineHandle = Handle<struct ComputePipeline>;
  using RenderPipelineHandle = Handle<struct RenderPipeline>;
  using RayTracingPipelineHandle = Handle<struct RayTracingPipeline>;
  using ShaderModuleHandle = Handle<struct ShaderModule>;
  using SamplerHandle = Handle<struct Sampler>;
  using BufferHandle = Handle<struct Buffer>;
  using TextureHandle = Handle<struct Texture>;
  using QueryPoolHandle = Handle<struct QueryPool>;
  using AccelStructHandle = Handle<struct AccelerationStructure>;

  // forward declarations to access incomplete type IContext
  void destroy(IContext* ctx, ComputePipelineHandle handle);

  void destroy(IContext* ctx, RenderPipelineHandle handle);

  void destroy(IContext* ctx, RayTracingPipelineHandle handle);

  void destroy(IContext* ctx, ShaderModuleHandle handle);

  void destroy(IContext* ctx, SamplerHandle handle);

  void destroy(IContext* ctx, BufferHandle handle);

  void destroy(IContext* ctx, TextureHandle handle);

  void destroy(IContext* ctx, QueryPoolHandle handle);

  void destroy(IContext* ctx, AccelStructHandle handle);

  template <typename HandleType>
  class Holder final
  {
    public:
      Holder() = default;

      Holder(IContext* ctx, HandleType handle) : ctx_(ctx), handle_(handle) {}

      ~Holder() { vk::destroy(ctx_, handle_); }

      Holder(const Holder&) = delete;

      Holder(Holder&& other) noexcept : ctx_(other.ctx_), handle_(other.handle_)
      {
        other.ctx_ = nullptr;
        other.handle_ = HandleType{};
      }

      Holder& operator=(const Holder&) = delete;

      Holder& operator=(Holder&& other) noexcept
      {
        std::swap(ctx_, other.ctx_);
        std::swap(handle_, other.handle_);
        return *this;
      }

      Holder& operator=(std::nullptr_t)
      {
        this->reset();
        return *this;
      }

      operator HandleType() const { return handle_; }

      [[nodiscard]] bool valid() const { return handle_.valid(); }

      [[nodiscard]] bool empty() const { return handle_.empty(); }

      void reset()
      {
        vk::destroy(ctx_, handle_);
        ctx_ = nullptr;
        handle_ = HandleType{};
      }

      HandleType release()
      {
        ctx_ = nullptr;
        return std::exchange(handle_, HandleType{});
      }

      [[nodiscard]] uint32_t gen() const { return handle_.gen(); }

      [[nodiscard]] uint32_t index() const { return handle_.index(); }

      [[nodiscard]] void* indexAsVoid() const { return handle_.indexAsVoid(); }

    private:
      IContext* ctx_ = nullptr;
      HandleType handle_ = {};
  };

  constexpr int MAX_COLOR_ATTACHMENTS = 8;
  constexpr int MAX_MIP_LEVELS = 16;
  constexpr int MAX_RAY_TRACING_SHADER_GROUP_SIZE = 4;

  enum class IndexFormat : uint8_t
  {
    UI8,
    UI16,
    UI32,
  };

  enum class Topology : uint8_t
  {
    Point,
    Line,
    LineStrip,
    Triangle,
    TriangleStrip,
    Patch,
  };

  enum class ColorSpace : uint8_t
  {
    SRGB_LINEAR,
    SRGB_NONLINEAR,
    SRGB_EXTENDED_LINEAR,
    HDR10,
  };

  enum class TextureType : uint8_t
  {
    Tex2D,
    Tex3D,
    TexCube,
  };

  enum class SamplerFilter : uint8_t
  {
    Nearest = 0,
    Linear
  };

  enum class SamplerMip : uint8_t
  {
    Disabled = 0,
    Nearest,
    Linear
  };

  enum class SamplerWrap : uint8_t
  {
    Repeat = 0,
    Clamp,
    MirrorRepeat
  };

  enum class HWDeviceType : uint8_t
  {
    Discrete   = 1,
    External   = 2,
    Integrated = 3,
    Software   = 4,
  };

  struct HWDeviceDesc
  {
    static constexpr int MAX_PHYSICAL_DEVICE_NAME_SIZE = 256;
    uintptr_t guid = 0;
    HWDeviceType type = HWDeviceType::Software;
    char name[MAX_PHYSICAL_DEVICE_NAME_SIZE] = {0};
  };

  enum class StorageType : uint8_t
  {
    Device,
    HostVisible,
    Memoryless
  };

  enum class CullMode : uint8_t
  {
    None,
    Front,
    Back
  };

  enum class WindingMode : uint8_t
  {
    CCW,
    CW
  };

  struct Result
  {
    enum class Code : uint8_t
    {
      Ok,
      ArgumentOutOfRange,
      RuntimeError,
    };

    Code code = Code::Ok;
    const char* message = "";

    explicit Result() = default;

    explicit Result(Code code, const char* message = "") : code(code), message(message) {}

    [[nodiscard]] bool isOk() const { return code == Code::Ok; }

    static void setResult(Result* outResult, Code code, const char* message = "")
    {
      if (outResult)
      {
        outResult->code = code;
        outResult->message = message;
      }
    }

    static void setResult(Result* outResult, const Result& sourceResult)
    {
      if (outResult) { *outResult = sourceResult; }
    }
  };

  struct ScissorRect
  {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
  };

  struct Dimensions
  {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;

    [[nodiscard]] Dimensions divide1D(uint32_t v) const
    {
      return {.width = width / v, .height = height, .depth = depth};
    }

    [[nodiscard]] Dimensions divide2D(uint32_t v) const
    {
      return {.width = width / v, .height = height / v, .depth = depth};
    }

    [[nodiscard]] Dimensions divide3D(uint32_t v) const
    {
      return {.width = width / v, .height = height / v, .depth = depth / v};
    }

    bool operator==(const Dimensions& other) const
    {
      return width == other.width && height == other.height && depth == other.depth;
    }
  };

  struct Viewport
  {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
  };

  enum class CompareOp : uint8_t
  {
    Never = 0,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
  };

  enum class StencilOp : uint8_t
  {
    Keep = 0,
    Zero,
    Replace,
    IncrementClamp,
    DecrementClamp,
    Invert,
    IncrementWrap,
    DecrementWrap
  };

  enum class BlendOp : uint8_t
  {
    Add = 0,
    Subtract,
    ReverseSubtract,
    Min,
    Max
  };

  enum class BlendFactor : uint8_t
  {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstColor,
    OneMinusDstColor,
    DstAlpha,
    OneMinusDstAlpha,
    SrcAlphaSaturated,
    BlendColor,
    OneMinusBlendColor,
    BlendAlpha,
    OneMinusBlendAlpha,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
  };

  struct SamplerStateDesc
  {
    SamplerFilter minFilter = SamplerFilter::Linear;
    SamplerFilter magFilter = SamplerFilter::Linear;
    SamplerMip mipMap = SamplerMip::Disabled;
    SamplerWrap wrapU = SamplerWrap::Repeat;
    SamplerWrap wrapV = SamplerWrap::Repeat;
    SamplerWrap wrapW = SamplerWrap::Repeat;
    CompareOp depthCompareOp = CompareOp::LessEqual;
    uint8_t mipLodMin = 0;
    uint8_t mipLodMax = 15;
    uint8_t maxAnisotropic = 1;
    bool depthCompareEnabled = false;
    const char* debugName = "";
  };

  struct StencilState
  {
    StencilOp stencilFailureOp = StencilOp::Keep;
    StencilOp depthFailureOp = StencilOp::Keep;
    StencilOp depthStencilPassOp = StencilOp::Keep;
    CompareOp stencilCompareOp = CompareOp::Always;
    uint32_t readMask = (uint32_t)~0;
    uint32_t writeMask = (uint32_t)~0;
  };

  struct DepthState
  {
    CompareOp compareOp = CompareOp::Always;
    bool isDepthWriteEnabled = false;
  };

  enum class PolygonMode : uint8_t
  {
    Fill = 0,
    Line = 1,
  };

  enum class VertexFormat
  {
    Invalid = 0,

    Float1,
    Float2,
    Float3,
    Float4,

    Byte1,
    Byte2,
    Byte3,
    Byte4,

    UByte1,
    UByte2,
    UByte3,
    UByte4,

    Short1,
    Short2,
    Short3,
    Short4,

    UShort1,
    UShort2,
    UShort3,
    UShort4,

    Byte2Norm,
    Byte4Norm,

    UByte2Norm,
    UByte4Norm,

    Short1Norm,
    Short2Norm,
    Short3Norm,
    Short4Norm,

    UShort1Norm,
    UShort2Norm,
    UShort3Norm,
    UShort4Norm,

    Int1,
    Int2,
    Int3,
    Int4,

    UInt1,
    UInt2,
    UInt3,
    UInt4,

    HalfFloat1,
    HalfFloat2,
    HalfFloat3,
    HalfFloat4,

    R10G10B10A2_SNORM,
    R10G10B10A2_UNORM,
  };

  enum class Format : uint8_t
  {
    Invalid = 0,

    R_UN8,
    R_UI16,
    R_UI32,
    R_UN16,
    R_F16,
    R_F32,

    RG_UN8,
    RG_UI16,
    RG_UI32,
    RG_UN16,
    RG_F16,
    RG_F32,

    RGBA_UN8,
    RGBA_UI32,
    RGBA_F16,
    RGBA_F32,
    RGBA_SRGB8,

    BGRA_UN8,
    BGRA_SRGB8,

    A2B10G10R10_UN,
    A2R10G10B10_UN,

    ETC2_RGB8,
    ETC2_SRGB8,
    ETC2_RGBA1,      // 1-bit alpha
    ETC2_RGBA1_SRGB,
    ETC2_RGBA8,
    ETC2_RGBA8_SRGB,
    BC1_RGB,
    BC1_RGB_SRGB,
    BC1_RGBA,
    BC1_RGBA_SRGB,

    BC2_RGBA,
    BC2_RGBA_SRGB,

    BC3_RGBA,
    BC3_RGBA_SRGB,

    BC4_R,
    BC5_RG,

    BC6H_RGB_UFLOAT,
    BC6H_RGB_SFLOAT,

    BC7_RGBA_SRGB,
    BC7_RGBA,

    // ASTC compressed formats (mobile/Android)
    ASTC_4x4_RGBA,
    ASTC_4x4_RGBA_SRGB,
    ASTC_5x4_RGBA,
    ASTC_5x4_RGBA_SRGB,
    ASTC_5x5_RGBA,
    ASTC_5x5_RGBA_SRGB,
    ASTC_6x5_RGBA,
    ASTC_6x5_RGBA_SRGB,
    ASTC_6x6_RGBA,
    ASTC_6x6_RGBA_SRGB,
    ASTC_8x5_RGBA,
    ASTC_8x5_RGBA_SRGB,
    ASTC_8x6_RGBA,
    ASTC_8x6_RGBA_SRGB,
    ASTC_8x8_RGBA,
    ASTC_8x8_RGBA_SRGB,
    ASTC_10x5_RGBA,
    ASTC_10x5_RGBA_SRGB,
    ASTC_10x6_RGBA,
    ASTC_10x6_RGBA_SRGB,
    ASTC_10x8_RGBA,
    ASTC_10x8_RGBA_SRGB,
    ASTC_10x10_RGBA,
    ASTC_10x10_RGBA_SRGB,
    ASTC_12x10_RGBA,
    ASTC_12x10_RGBA_SRGB,
    ASTC_12x12_RGBA,
    ASTC_12x12_RGBA_SRGB,

    Z_UN16,
    Z_UN24,
    Z_F32,
    Z_UN24_S_UI8,
    Z_F32_S_UI8,

    YUV_NV12,
    YUV_420p,
  };

  enum class LoadOp : uint8_t
  {
    Invalid = 0,
    DontCare,
    Load,
    Clear,
    None,
  };

  enum class StoreOp : uint8_t
  {
    DontCare = 0,
    Store,
    MsaaResolve,
    None,
  };

  enum class ShaderStage : uint8_t
  {
    Vert,
    Tesc,
    Tese,
    Geom,
    Frag,
    Comp,
    Task,
    Mesh,
    //ray tracing
    RayGen,
    AnyHit,
    ClosestHit,
    Miss,
    Intersection,
    Callable,
  };

  ShaderStage ShaderStageFromFileName(const char* fileName);

  union ClearColorValue
  {
    float float32[4];
    int32_t int32[4];
    uint32_t uint32[4];
  };

  struct VertexInput final
  {
    static constexpr int VERTEX_ATTRIBUTES_MAX = 16;
    static constexpr int VERTEX_BUFFER_MAX = 16;

    struct VertexAttribute final
    {
      uint32_t location = 0; // a buffer which contains this attribute stream
      uint32_t binding = 0;
      VertexFormat format = VertexFormat::Invalid; // per-element format
      uintptr_t offset = 0;                        // an offset where the first element of this attribute stream starts
    } attributes[VERTEX_ATTRIBUTES_MAX];

    struct VertexInputBinding final
    {
      uint32_t stride = 0;
    } inputBindings[VERTEX_BUFFER_MAX];

    [[nodiscard]] uint32_t getNumAttributes() const
    {
      uint32_t n = 0;
      while (n < VERTEX_ATTRIBUTES_MAX && attributes[n].format != VertexFormat::Invalid) { n++; }
      return n;
    }

    [[nodiscard]] uint32_t getNumInputBindings() const
    {
      uint32_t n = 0;
      while (n < VERTEX_BUFFER_MAX && inputBindings[n].stride) { n++; }
      return n;
    }

    [[nodiscard]] uint32_t getVertexSize() const;

    bool operator==(const VertexInput& other) const { return memcmp(this, &other, sizeof(VertexInput)) == 0; }
  };

  struct ColorAttachment
  {
    Format format = Format::Invalid;
    bool blendEnabled = false;
    BlendOp rgbBlendOp = BlendOp::Add;
    BlendOp alphaBlendOp = BlendOp::Add;
    BlendFactor srcRGBBlendFactor = BlendFactor::One;
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstRGBBlendFactor = BlendFactor::Zero;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
  };

  struct ShaderModuleDesc
  {
    ShaderStage stage = ShaderStage::Frag;
    const char* data = nullptr;
    size_t dataSize = 0; // if `dataSize` is non-zero, interpret `data` as binary shader data
    const char* debugName = "";

    ShaderModuleDesc(const char* source, ShaderStage stage, const char* debugName) : stage(stage), data(source), debugName(debugName) {}

    ShaderModuleDesc(const void* data, size_t dataLength, ShaderStage stage, const char* debugName) : stage(stage), data(static_cast<const char*>(data)), dataSize(dataLength), debugName(debugName) { ASSERT(dataSize); }
  };

  struct SpecializationConstantEntry
  {
    uint32_t constantId = 0;
    uint32_t offset = 0; // offset within ShaderSpecializationConstantDesc::data
    size_t size = 0;
  };

  struct SpecializationConstantDesc
  {
    static constexpr int SPECIALIZATION_CONSTANTS_MAX = 16;
    SpecializationConstantEntry entries[SPECIALIZATION_CONSTANTS_MAX] = {};
    const void* data = nullptr;
    size_t dataSize = 0;

    [[nodiscard]] uint32_t getNumSpecializationConstants() const
    {
      uint32_t n = 0;
      while (n < SPECIALIZATION_CONSTANTS_MAX && entries[n].size) { n++; }
      return n;
    }
  };

  struct RenderPipelineDesc final
  {
    Topology topology = Topology::Triangle;

    VertexInput vertexInput;

    ShaderModuleHandle smVert;
    ShaderModuleHandle smTesc;
    ShaderModuleHandle smTese;
    ShaderModuleHandle smGeom;
    ShaderModuleHandle smTask;
    ShaderModuleHandle smMesh;
    ShaderModuleHandle smFrag;

    SpecializationConstantDesc specInfo = {};

    const char* entryPointVert = "main";
    const char* entryPointTesc = "main";
    const char* entryPointTese = "main";
    const char* entryPointGeom = "main";
    const char* entryPointTask = "main";
    const char* entryPointMesh = "main";
    const char* entryPointFrag = "main";

    ColorAttachment color[MAX_COLOR_ATTACHMENTS] = {};
    Format depthFormat = Format::Invalid;
    Format stencilFormat = Format::Invalid;

    CullMode cullMode = CullMode::None;
    WindingMode frontFaceWinding = WindingMode::CCW;
    PolygonMode polygonMode = PolygonMode::Fill;

    StencilState backFaceStencil = {};
    StencilState frontFaceStencil = {};

    uint32_t samplesCount = 1u;
    uint32_t patchControlPoints = 0;
    float minSampleShading = 0.0f;

    const char* debugName = "";

    [[nodiscard]] uint32_t getNumColorAttachments() const
    {
      uint32_t n = 0;
      while (n < MAX_COLOR_ATTACHMENTS && color[n].format != Format::Invalid) { n++; }
      return n;
    }
  };

  struct ComputePipelineDesc final
  {
    ShaderModuleHandle smComp;
    SpecializationConstantDesc specInfo = {};
    const char* entryPoint = "main";
    const char* debugName = "";
  };

  struct RayTracingPipelineDesc final
  {
    ShaderModuleHandle smRayGen[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    ShaderModuleHandle smAnyHit[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    ShaderModuleHandle smClosestHit[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    ShaderModuleHandle smMiss[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    ShaderModuleHandle smIntersection[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    ShaderModuleHandle smCallable[MAX_RAY_TRACING_SHADER_GROUP_SIZE] = {};
    SpecializationConstantDesc specInfo = {};
    const char* entryPoint = "main";
    const char* debugName = "";

#define GET_SHADER_GROUP_SIZE(name, module) \
  [[nodiscard]] uint32_t getShaderGroupSize##name() const { \
    uint32_t n = 0; \
    while (n < MAX_RAY_TRACING_SHADER_GROUP_SIZE && module[n]) n++; \
    return n; \
  }

    GET_SHADER_GROUP_SIZE(RayGen, smRayGen)

    GET_SHADER_GROUP_SIZE(AnyHit, smAnyHit)

    GET_SHADER_GROUP_SIZE(ClosestHit, smClosestHit)

    GET_SHADER_GROUP_SIZE(RayMiss, smMiss)

    GET_SHADER_GROUP_SIZE(Intersection, smIntersection)

    GET_SHADER_GROUP_SIZE(Callable, smCallable)

#undef GET_SHADER_GROUP_SIZE
  };

  struct RenderPass final
  {
    struct AttachmentDesc final
    {
      LoadOp loadOp = LoadOp::Invalid;
      StoreOp storeOp = StoreOp::Store;
      uint8_t layer = 0;
      uint8_t level = 0;
      float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
      float clearDepth = 1.0f;
      uint32_t clearStencil = 0;
    };

    AttachmentDesc color[MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depth = {.loadOp = LoadOp::DontCare, .storeOp = StoreOp::DontCare};
    AttachmentDesc stencil = {.loadOp = LoadOp::Invalid, .storeOp = StoreOp::DontCare};

    // Multiview rendering support
    uint32_t layerCount = 1;  // Number of layers to render (1 = standard, 6 = cube map)
    uint32_t viewMask = 0;    // Bitmask of views to render (0 = no multiview, 0b111111 = 6 views)

    [[nodiscard]] uint32_t getNumColorAttachments() const
    {
      uint32_t n = 0;
      while (n < MAX_COLOR_ATTACHMENTS && color[n].loadOp != LoadOp::Invalid) { n++; }
      return n;
    }
  };

  struct Framebuffer final
  {
    struct AttachmentDesc
    {
      TextureHandle texture;
      TextureHandle resolveTexture;
    };

    AttachmentDesc color[MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depthStencil;

    const char* debugName = "";

    [[nodiscard]] uint32_t getNumColorAttachments() const
    {
      uint32_t n = 0;
      while (n < MAX_COLOR_ATTACHMENTS && color[n].texture) { n++; }
      return n;
    }
  };

  enum BufferUsageBits : uint8_t
  {
    BufferUsageBits_Index    = 1 << 0,
    BufferUsageBits_Vertex   = 1 << 1,
    BufferUsageBits_Uniform  = 1 << 2,
    BufferUsageBits_Storage  = 1 << 3,
    BufferUsageBits_Indirect = 1 << 4,
    // ray tracing
    BufferUsageBits_ShaderBindingTable            = 1 << 5,
    BufferUsageBits_AccelStructBuildInputReadOnly = 1 << 6,
    BufferUsageBits_AccelStructStorage            = 1 << 7
  };

  struct BufferDesc final
  {
    uint8_t usage = 0;
    StorageType storage = StorageType::HostVisible;
    size_t size = 0;
    const void* data = nullptr;
    const char* debugName = "";
  };

  struct Offset3D
  {
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
  };

  struct TextureLayers
  {
    uint32_t mipLevel = 0;
    uint32_t layer = 0;
    uint32_t numLayers = 1;
  };

  struct TextureRangeDesc
  {
    Offset3D offset = {};
    Dimensions dimensions = {.width = 1, .height = 1, .depth = 1};
    uint32_t layer = 0;
    uint32_t numLayers = 1;
    uint32_t mipLevel = 0;
    uint32_t numMipLevels = 1;
  };

  enum TextureUsageBits : uint8_t
  {
    TextureUsageBits_Sampled    = 1 << 0,
    TextureUsageBits_Storage    = 1 << 1,
    TextureUsageBits_Attachment = 1 << 2,
  };

  enum Swizzle : uint8_t
  {
    Swizzle_Default = 0,
    Swizzle_0,
    Swizzle_1,
    Swizzle_R,
    Swizzle_G,
    Swizzle_B,
    Swizzle_A,
  };

  struct ComponentMapping
  {
    Swizzle r = Swizzle_Default;
    Swizzle g = Swizzle_Default;
    Swizzle b = Swizzle_Default;
    Swizzle a = Swizzle_Default;

    [[nodiscard]] bool identity() const
    {
      return r == Swizzle_Default && g == Swizzle_Default && b == Swizzle_Default && a == Swizzle_Default;
    }
  };

  struct TextureDesc
  {
    TextureType type = TextureType::Tex2D;
    Format format = Format::Invalid;

    Dimensions dimensions = {.width = 1, .height = 1, .depth = 1};
    uint32_t numLayers = 1;
    uint32_t numSamples = 1;
    uint8_t usage = TextureUsageBits_Sampled;
    uint32_t numMipLevels = 1;
    StorageType storage = StorageType::Device;
    ComponentMapping swizzle = {};
    const void* data = nullptr;
    uint32_t dataNumMipLevels = 1; // how many mip-levels we want to upload
    bool generateMipmaps = false;  // generate mip-levels immediately, valid only with non-null data
    const char* debugName = "";
  };

  struct TextureViewDesc
  {
    TextureType type = TextureType::Tex2D;
    uint32_t layer = 0;
    uint32_t numLayers = 1;
    uint32_t mipLevel = 0;
    uint32_t numMipLevels = 1;
    ComponentMapping swizzle = {};
  };

  enum class AccelStructType : uint8_t
  {
    Invalid = 0,
    TLAS    = 1,
    BLAS    = 2,
  };

  enum class AccelStructGeomType : uint8_t
  {
    Triangles = 0,
    AABBs     = 1,
    Instances = 2,
  };

  enum AccelStructBuildFlagBits : uint8_t
  {
    AccelStructBuildFlagBits_AllowUpdate     = 1 << 0,
    AccelStructBuildFlagBits_AllowCompaction = 1 << 1,
    AccelStructBuildFlagBits_PreferFastTrace = 1 << 2,
    AccelStructBuildFlagBits_PreferFastBuild = 1 << 3,
    AccelStructBuildFlagBits_LowMemory       = 1 << 4,
  };

  enum AccelStructGeometryFlagBits : uint8_t
  {
    AccelStructGeometryFlagBits_Opaque            = 1 << 0,
    AccelStructGeometryFlagBits_NoDuplicateAnyHit = 1 << 1,
  };

  enum AccelStructInstanceFlagBits : uint8_t
  {
    AccelStructInstanceFlagBits_TriangleFacingCullDisable = 1 << 0,
    AccelStructInstanceFlagBits_TriangleFlipFacing        = 1 << 1,
    AccelStructInstanceFlagBits_ForceOpaque               = 1 << 2,
    AccelStructInstanceFlagBits_ForceNoOpaque             = 1 << 3,
  };

  struct AccelStructSizes
  {
    uint64_t accelerationStructureSize = 0;
    uint64_t updateScratchSize = 0;
    uint64_t buildScratchSize = 0;
  };

  struct AccelStructBuildRange
  {
    uint32_t primitiveCount = 0;
    uint32_t primitiveOffset = 0;
    uint32_t firstVertex = 0;
    uint32_t transformOffset = 0;
  };

  struct mat3x4
  {
    float matrix[3][4];
  };

  struct AccelStructInstance
  {
    mat3x4 transform;
    uint32_t instanceCustomIndex : 24 = 0;
    uint32_t mask : 8 = 0xff;
    uint32_t instanceShaderBindingTableRecordOffset : 24 = 0;
    uint32_t flags : 8 = AccelStructInstanceFlagBits_TriangleFacingCullDisable;
    uint64_t accelerationStructureReference = 0;
  };

  struct AccelStructDesc
  {
    AccelStructType type = AccelStructType::Invalid;
    AccelStructGeomType geometryType = AccelStructGeomType::Triangles;
    uint8_t geometryFlags = AccelStructGeometryFlagBits_Opaque;

    VertexFormat vertexFormat = VertexFormat::Invalid;
    BufferHandle vertexBuffer;
    uint32_t vertexStride = 0; // zero means the size of `vertexFormat`
    uint32_t numVertices = 0;
    IndexFormat indexFormat = IndexFormat::UI32;
    BufferHandle indexBuffer;
    BufferHandle transformBuffer;
    BufferHandle instancesBuffer;
    AccelStructBuildRange buildRange = {};
    uint8_t buildFlags = AccelStructBuildFlagBits_PreferFastTrace;
    const char* debugName = "";
  };

  struct Dependencies
  {
    static constexpr int MAX_SUBMIT_DEPENDENCIES = 4;
    TextureHandle textures[MAX_SUBMIT_DEPENDENCIES] = {};
    BufferHandle buffers[MAX_SUBMIT_DEPENDENCIES] = {};
  };

  class ICommandBuffer
  {
    public:
      virtual ~ICommandBuffer() = default;

      virtual void transitionToShaderReadOnly(TextureHandle surface) const = 0;

      virtual void cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA = 0xffffffff) const = 0;

      virtual void cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA = 0xffffffff) const = 0;

      virtual void cmdPopDebugGroupLabel() const = 0;

      virtual void cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) = 0;

      virtual void cmdBindComputePipeline(ComputePipelineHandle handle) = 0;

      virtual void cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps = {}) = 0;

      virtual void cmdBeginRendering(const RenderPass& renderPass, const Framebuffer& desc, const Dependencies& deps = {}) = 0;

      virtual void cmdEndRendering() = 0;

      virtual void cmdBindViewport(const Viewport& viewport) = 0;

      virtual void cmdBindScissorRect(const ScissorRect& rect) = 0;

      virtual void cmdBindRenderPipeline(RenderPipelineHandle handle) = 0;

      virtual void cmdBindDepthState(const DepthState& state) = 0;

      virtual void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset = 0) = 0;

      virtual void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat indexFormat, uint64_t indexBufferOffset = 0) = 0;

      virtual void cmdPushConstants(const void* data, size_t size, size_t offset = 0) = 0;

      template <typename Struct>
      void cmdPushConstants(const Struct& data, size_t offset = 0)
      {
        this->cmdPushConstants(&data, sizeof(Struct), offset);
      }

      virtual void cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data) = 0;

      virtual void cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data) = 0;

      template <typename Struct>
      void cmdUpdateBuffer(BufferHandle buffer, const Struct& data, size_t bufferOffset = 0)
      {
        this->cmdUpdateBuffer(buffer, bufferOffset, sizeof(Struct), &data);
      }

      virtual void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t baseInstance = 0) = 0;

      virtual void cmdDrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t baseInstance = 0) = 0;

      virtual void cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) = 0;

      virtual void cmdDrawIndexedIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) = 0;

      virtual void cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0) = 0;

      virtual void cmdDrawMeshTasks(const Dimensions& threadgroupCount) = 0;

      virtual void cmdDrawMeshTasksIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) = 0;

      virtual void cmdDrawMeshTasksIndirectCount(BufferHandle indirectBuffer, size_t indirectBufferOffset, BufferHandle countBuffer, size_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride = 0) = 0;

      virtual void cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth = 1, const Dependencies& deps = {}) = 0;

      virtual void cmdSetBlendColor(const float color[4]) = 0;

      virtual void cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp = 0.0f) = 0;

      virtual void cmdSetDepthBiasEnable(bool enable) = 0;

      virtual void cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) = 0;

      virtual void cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query) = 0;

      virtual void cmdClearColorImage(TextureHandle tex, const ClearColorValue& value, const TextureLayers& layers = {}) = 0;

      virtual void cmdCopyBuffer(BufferHandle src, BufferHandle dst, size_t size, size_t srcOffset = 0, size_t dstOffset = 0) = 0;

      virtual void cmdCopyImage(TextureHandle src, TextureHandle dst, const Dimensions& extent, const Offset3D& srcOffset = {}, const Offset3D& dstOffset = {}, const TextureLayers& srcLayers = {}, const TextureLayers& dstLayers = {}) = 0;

      virtual void cmdGenerateMipmap(TextureHandle handle) = 0;

      virtual void cmdUpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer) = 0;
  };

  struct SubmitHandle
  {
    uint32_t bufferIndex_ = 0;
    uint32_t submitId_ = 0;

    SubmitHandle() = default;

    explicit SubmitHandle(uint64_t handle) : bufferIndex_(uint32_t(handle & 0xffffffff)), submitId_(uint32_t(handle >> 32)) { ASSERT(submitId_); }

    [[nodiscard]] bool empty() const { return submitId_ == 0; }

    [[nodiscard]] uint64_t handle() const { return (uint64_t(submitId_) << 32) + bufferIndex_; }
  };

  static_assert(sizeof(SubmitHandle) == sizeof(uint64_t));

  class IContext
  {
    protected:
      IContext() = default;

    public:
      virtual ~IContext() = default;

      virtual ICommandBuffer& acquireCommandBuffer() = 0;

      virtual SubmitHandle submit(ICommandBuffer& commandBuffer, TextureHandle present = {}) = 0;

      virtual void wait(SubmitHandle handle) = 0; // waiting on an empty handle results in vkDeviceWaitIdle()

      [[nodiscard]] virtual Holder<BufferHandle> createBuffer(const BufferDesc& desc, const char* debugName = nullptr, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<SamplerHandle> createSampler(const SamplerStateDesc& desc, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<TextureHandle> createTexture(const TextureDesc& desc, const char* debugName = nullptr, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<TextureHandle> createTextureView(TextureHandle texture, const TextureViewDesc& desc, const char* debugName = nullptr, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<ComputePipelineHandle> createComputePipeline(const ComputePipelineDesc& desc, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<RenderPipelineHandle> createRenderPipeline(const RenderPipelineDesc& desc, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<RayTracingPipelineHandle> createRayTracingPipeline(const RayTracingPipelineDesc& desc, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<ShaderModuleHandle> createShaderModule(const ShaderModuleDesc& desc, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<QueryPoolHandle> createQueryPool(uint32_t numQueries, const char* debugName, Result* outResult = nullptr) = 0;

      [[nodiscard]] virtual Holder<AccelStructHandle> createAccelerationStructure(const AccelStructDesc& desc, Result* outResult = nullptr) = 0;

      virtual void destroy(ComputePipelineHandle handle) = 0;

      virtual void destroy(RenderPipelineHandle handle) = 0;

      virtual void destroy(RayTracingPipelineHandle) = 0;

      virtual void destroy(ShaderModuleHandle handle) = 0;

      virtual void destroy(SamplerHandle handle) = 0;

      virtual void destroy(BufferHandle handle) = 0;

      virtual void destroy(TextureHandle handle) = 0;

      virtual void destroy(QueryPoolHandle handle) = 0;

      virtual void destroy(AccelStructHandle handle) = 0;

      virtual void destroy(Framebuffer& fb) = 0;

#pragma region Acceleration structure functions
      [[nodiscard]] virtual AccelStructSizes getAccelStructSizes(const AccelStructDesc& desc, Result* outResult = nullptr) const = 0;
#pragma endregion

#pragma region Buffer functions
      virtual Result upload(BufferHandle handle, const void* data, size_t size, size_t offset = 0) = 0;

      virtual Result download(BufferHandle handle, void* data, size_t size, size_t offset) = 0;

      [[nodiscard]] virtual uint8_t* getMappedPtr(BufferHandle handle) const = 0;

      virtual void flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const = 0;

      [[nodiscard]] virtual uint32_t getMaxStorageBufferRange() const = 0;
#pragma endregion

#pragma region Texture functions
      // `data` contains mip-levels and layers as in https://registry.khronos.org/KTX/specs/1.0/ktxspec.v1.html
      virtual Result upload(TextureHandle handle, const TextureRangeDesc& range, const void* data) = 0;

      virtual Result download(TextureHandle handle, const TextureRangeDesc& range, void* outData) = 0;

      [[nodiscard]] virtual Dimensions getDimensions(TextureHandle handle) const = 0;

      [[nodiscard]] virtual float getAspectRatio(TextureHandle handle) const = 0;

      [[nodiscard]] virtual Format getFormat(TextureHandle handle) const = 0;
#pragma endregion

      virtual TextureHandle getCurrentSwapchainTexture() = 0;

      [[nodiscard]] virtual Format getSwapchainFormat() const = 0;

      [[nodiscard]] virtual ColorSpace getSwapchainColorSpace() const = 0;

      [[nodiscard]] virtual uint32_t getSwapchainCurrentImageIndex() const = 0;

      [[nodiscard]] virtual uint32_t getNumSwapchainImages() const = 0;

      virtual void recreateSwapchain(int newWidth, int newHeight) = 0;

#if defined(__ANDROID__)
      [[nodiscard]] virtual SurfaceTransform getSwapchainPreTransform() const = 0;
#endif

      // Surface management (for Android lifecycle)
      virtual void createSurface(void* nativeWindow, void* display) = 0;

      virtual void destroySurface() = 0;

      virtual bool hasSurface() const = 0;

      virtual bool hasSwapchain() const = 0;

      // MSAA level is supported if ((samples & bitmask) != 0), where samples must be power of two.
      [[nodiscard]] virtual uint32_t getFramebufferMSAABitMask() const = 0;

#pragma region Performance queries
      [[nodiscard]] virtual double getTimestampPeriodToMs() const = 0;

      virtual bool getQueryPoolResults(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* outData, size_t stride) const = 0;
#pragma endregion
  };

  using ShaderModuleErrorCallback = void (*)(IContext*, ShaderModuleHandle, int line, int col, const char* debugName);

  constexpr uint32_t kMaxCustomExtensions = 32;

  enum VulkanVersion
  {
    VulkanVersion_1_3,
    VulkanVersion_1_4,
  };

  struct ContextConfig
  {
    VulkanVersion vulkanVersion = VulkanVersion_1_3;
    bool terminateOnValidationError = false; // invoke std::terminate() on any validation error
    bool enableValidation = true;
    ColorSpace swapchainRequestedColorSpace = ColorSpace::SRGB_LINEAR;
    bool disableVSync = false;
    // owned by the application - should be alive until createVulkanContextWithSwapchain() returns
    const void* pipelineCacheData = nullptr;
    size_t pipelineCacheDataSize = 0;
    ShaderModuleErrorCallback shaderModuleErrorCallback = nullptr;
    const char* extensionsInstance[kMaxCustomExtensions] = {}; // add extra instance extensions on top of required ones
    const char* extensionsDevice[kMaxCustomExtensions] = {};   // add extra device extensions on top of required ones
    void* extensionsDeviceFeatures = nullptr;                  // inserted into VkPhysicalDeviceVulkan11Features::pNext

    // LVK knows about these extensions and can manage them automatically upon request
    bool enableHeadlessSurface = false; // VK_EXT_headless_surface
  };

  [[nodiscard]] bool isDepthOrStencilFormat(Format format);

  [[nodiscard]] uint32_t getNumImagePlanes(Format format);

  [[nodiscard]] uint32_t getTextureBytesPerLayer(uint32_t width, uint32_t height, Format format, uint32_t level);

  [[nodiscard]] uint32_t getTextureBytesPerPlane(uint32_t width, uint32_t height, Format format, uint32_t plane);

  [[nodiscard]] uint32_t getVertexFormatSize(VertexFormat format);

  // Convert Vulkan VkFormat to internal Format enum (for KTX loading)
  [[nodiscard]] Format vkFormatToFormat(int vkFormat);

  void logShaderSource(const char* text);

  constexpr uint32_t calcNumMipLevels(uint32_t width, uint32_t height)
  {
    uint32_t levels = 1;

    while ((width | height) >> levels)
      levels++;

    return levels;
  }

} // namespace vk
