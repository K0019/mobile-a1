#include "memory.h"

// Define USE_MIMALLOC to enable mimalloc, otherwise use standard allocation
#ifdef USE_MIMALLOC
  #include <mimalloc-override.h>
  #include <mimalloc-new-delete.h>
  #include <mimalloc-stats.h>
  #include "mimalloc.h"
#endif

#include "logging/log.h"
#include <cstdlib>
#include <new>

// Optional: Define mimalloc debug level if MEMORY_DEBUG_ENABLED is set
#if defined(USE_MIMALLOC) && MEMORY_DEBUG_ENABLED && !defined(MI_DEBUG)
#define MI_DEBUG 2
#endif

// --- Allocation / Deallocation ---

void Memory::Initialise()
{
#ifdef USE_MIMALLOC
  mi_version();
#endif
}

void* Memory::Allocate(size_t size, size_t alignment)
{
  if (size == 0)
    return nullptr;
    
  if ((alignment & (alignment - 1)) != 0 || alignment == 0)
  {
#if MEMORY_DEBUG_ENABLED
    LOG_ERROR("Alignment must be a power of 2.");
#endif
    return nullptr;
  }

#ifdef USE_MIMALLOC
  return mi_malloc_aligned(size, alignment);
#else
  // Use C++17 std::aligned_alloc or platform-specific alternatives
  #if defined(_WIN32)
    return _aligned_malloc(size, alignment);
  #elif defined(__GLIBC__) && (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 16))
    return std::aligned_alloc(alignment, size);
  #else
    // Fallback using posix_memalign
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) == 0)
      return ptr;
    return nullptr;
  #endif
#endif
}

void* Memory::Allocate(size_t size)
{
  if (size == 0)
    return nullptr;

#ifdef USE_MIMALLOC
  return mi_malloc(size);
#else
  return std::malloc(size);
#endif
}

void Memory::Deallocate(void* ptr, [[maybe_unused]] size_t alignment)
{
  if (ptr == nullptr)
    return;

#ifdef USE_MIMALLOC
  mi_free_aligned(ptr, alignment);
#else
  #if defined(_WIN32)
    _aligned_free(ptr);
  #else
    std::free(ptr); // POSIX aligned_alloc and posix_memalign use regular free
  #endif
#endif
}

void Memory::Deallocate(void* ptr)
{
  if (ptr == nullptr)
    return;

#ifdef USE_MIMALLOC
  mi_free(ptr);
#else
  std::free(ptr);
#endif
}

// --- Debug Statistics (Conditional) ---
#ifndef NDEBUG

#ifdef USE_MIMALLOC
namespace
{
  bool get_mimalloc_stats(mi_stats_t* stats)
  {
    if (!stats)
      return false;
    mi_stats_get(sizeof(mi_stats_t), stats);
    return true;
  }

  void mi_string_output_fun(const char* msg, void* arg)
  {
    if (msg == nullptr || arg == nullptr)
      return;
    auto output_string = static_cast<std::string*>(arg);
    *output_string += msg;
  }
}
#endif

size_t Memory::GetThreadAllocatedBytes()
{
#ifdef USE_MIMALLOC
  mi_stats_t stats = {0};
  if (get_mimalloc_stats(&stats))
  {
    return static_cast<size_t>(stats.malloc_requested.current);
  }
#endif
  return 0; // No stats available with standard allocator
}

size_t Memory::GetThreadPeakAllocatedBytes()
{
#ifdef USE_MIMALLOC
  mi_stats_t stats = {0};
  if (get_mimalloc_stats(&stats))
  {
    return static_cast<size_t>(stats.malloc_requested.peak);
  }
#endif
  return 0;
}

size_t Memory::GetThreadAllocationCount()
{
#ifdef USE_MIMALLOC
  mi_stats_t stats = {0};
  if (get_mimalloc_stats(&stats))
  {
    return static_cast<size_t>(stats.malloc_normal_count.total + stats.malloc_huge_count.total);
  }
#endif
  return 0;
}

void Memory::LogStats()
{
#ifdef USE_MIMALLOC
  std::string combined_output;
  combined_output.reserve(4096);
  combined_output += "--- Mimalloc Stats ---\n";
  
  mi_stats_print_out(&mi_string_output_fun, &combined_output);
  
  if (!combined_output.empty() && combined_output.back() != '\n') 
  { 
    combined_output += '\n'; 
  }
  combined_output += "----------------------";
  
  LOG_INFO("{}", combined_output);
#else
  LOG_INFO("Memory stats not available (mimalloc disabled)");
#endif
}

#endif // MEMORY_DEBUG_ENABLED