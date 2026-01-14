// Implementation file for header-only libraries (volk, VMA)
// This file must be compiled into hina-vk to provide symbols
// Platform defines are set by CMake, don't redefine them
// volk implementation (C linkage)
extern "C" {
#define VOLK_IMPLEMENTATION
#include "volk.h"
}

// VulkanMemoryAllocator implementation (C++)
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

// ---------------------------------------------------------------------------
// Tracy GPU Profiling (when TRACY_ENABLE is defined)
// ---------------------------------------------------------------------------
// These C-linkage functions wrap Tracy's C++ VkCtxScope API so hina_vk.c can call them.
// The VkCtxScope is an RAII object that writes GPU timestamps at construction/destruction.
// We use placement new to store VkCtxScope in hina_cmd::tracy_gpu_scope storage.
// ---------------------------------------------------------------------------
#ifdef TRACY_ENABLE
#include <tracy/TracyVulkan.hpp>
#include <new>  // For placement new

extern "C" {

// Create TracyVkCtx during device initialization
// Returns opaque pointer (TracyVkCtx) to store in hina_device::tracy_vk_ctx
void* hina_tracy_vk_init(VkPhysicalDevice phys, VkDevice dev, VkQueue queue, VkCommandBuffer cmd)
{
  return TracyVkContext(phys, dev, queue, cmd);
}

// Destroy TracyVkCtx during device shutdown
void hina_tracy_vk_shutdown(void* ctx)
{
  TracyVkDestroy(static_cast<TracyVkCtx>(ctx));
}

// Collect GPU timestamps - call once per frame before present
// cmdbuf can be VK_NULL_HANDLE to use host-side query reset (requires VK 1.2+)
void hina_tracy_vk_collect(void* ctx, VkCommandBuffer cmdbuf)
{
  TracyVkCollect(static_cast<TracyVkCtx>(ctx), cmdbuf);
}

// Begin a GPU zone (render pass or compute region)
// Uses placement new to construct VkCtxScope in the provided storage
// name must be a string literal (compile-time constant)
void hina_tracy_vk_zone_begin(void* ctx, void* scope_storage, VkCommandBuffer cmdbuf,
                               uint32_t line, const char* source, const char* function, const char* name)
{
    auto* tracy_ctx = static_cast<TracyVkCtx>(ctx);
    // Use the runtime-string constructor (TracyVkZoneTransient style)
    new (scope_storage) tracy::VkCtxScope(
        tracy_ctx,
        line,
        source, strlen(source),
        function, strlen(function),
        name, strlen(name),
        cmdbuf,
        true  // active
    );
}

// End a GPU zone - destructs the VkCtxScope to write the end timestamp
void hina_tracy_vk_zone_end(void* scope_storage)
{
    auto* scope = static_cast<tracy::VkCtxScope*>(scope_storage);
    scope->~VkCtxScope();
}

// Set the GPU context name (optional, shows in Tracy UI)
void hina_tracy_vk_context_name(void* ctx, const char* name)
{
    if (name) {
        auto* tracy_ctx = static_cast<TracyVkCtx>(ctx);
        TracyVkContextName(tracy_ctx, name, static_cast<uint16_t>(strlen(name)));
    }
}

}  // extern "C"

#endif  // TRACY_ENABLE
