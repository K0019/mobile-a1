#pragma once
#include <cstddef>
#include <map>

#include "logging/log.h"

#ifndef NDEBUG
#define MEMORY_DEBUG_ENABLED 1
#else
#define MEMORY_DEBUG_ENABLED 0
#endif

class Memory
{
  public:
    static void Initialise();

    static void* Allocate(size_t size, size_t alignment);

    static void* Allocate(size_t size);

    static void Deallocate(void* ptr, size_t alignment);

    static void Deallocate(void* ptr);

    // --- Debug Statistics (Conditional) ---
    // These functions only provide data in debug builds (MEMORY_DEBUG_ENABLED = 1)
#if MEMORY_DEBUG_ENABLED
    static size_t GetThreadAllocatedBytes();

    static size_t GetThreadPeakAllocatedBytes();

    static size_t GetThreadAllocationCount();

    static void LogStats();
#endif
    Memory() = delete;

    ~Memory() = delete;

    Memory(const Memory&) = delete;

    Memory& operator=(const Memory&) = delete;

    Memory(Memory&&) = delete;

    Memory& operator=(Memory&&) = delete;
};
#if MEMORY_DEBUG_ENABLED
void inline TestMemoryOverride()
{
  // --- Initial Stats ---
  // Print stats before any allocations are made by this program.
  // This establishes a baseline.
  Memory::LogStats();

  // --- Test 1: C-style malloc/free ---
  LOG_INFO("Testing malloc/free...");
  const size_t malloc_size = 1024; // Allocate 1 KiB
  void* c_ptr = malloc(malloc_size);
  // Optional: Touch the memory to ensure pages are mapped if needed by the OS/allocator.
  static_cast<char*>(c_ptr)[0] = 'a';
  static_cast<char*>(c_ptr)[malloc_size - 1] = 'z';

  // Print stats after malloc - expect increase in allocated blocks/bytes.
  Memory::LogStats();

  free(c_ptr); // Deallocate the memory

  // Print stats after free - expect decrease in current usage, increase in total frees.
  Memory::LogStats();

  // --- Test 2: C++ new/delete ---
  LOG_INFO("Testing new/delete ...");
  int* cpp_ptr = new int(42); // Allocate a single int using C++ operator new
  // Print stats after new - expect increase in allocated blocks/bytes.
  Memory::LogStats();

  delete cpp_ptr; // Deallocate using C++ operator delete

  // Print stats after delete - expect decrease in current usage.
  Memory::LogStats();

  // --- Test 3: C++ new[]/delete[] ---
  LOG_INFO("Testing new[]/delete[] ...");
  const size_t array_size = 512;                  // Allocate an array of 512 doubles
  double* cpp_array_ptr = new double[array_size]; // Allocate using C++ operator new[]
  // Optional: Touch memory.
  cpp_array_ptr[0] = 1.0;
  cpp_array_ptr[array_size - 1] = 2.0;

  // Print stats after new[] - expect increase in allocated blocks/bytes.
  Memory::LogStats();

  delete[] cpp_array_ptr; // Deallocate using C++ operator delete[]

  // Print stats after delete[] - expect decrease in current usage.
  Memory::LogStats();

  // --- Test 4: STL Containers ---
  // These containers use allocators, which typically call ::operator new internally.
  // If mimalloc overrides ::operator new, it should handle these too.
  LOG_INFO("Testing C STL...");
  {
    // Use a scope block so containers are destroyed before the final stats print

    // std::vector
    LOG_INFO("Testing std::vector ...");
    std::vector<int> my_vector;
    my_vector.reserve(1000); // Often allocates memory upfront
    for (int i = 0; i < 500; ++i)
    {
      my_vector.push_back(i); // May cause reallocations as vector grows
    }
    // Print stats while vector is alive - expect increases from reserve/push_back.
    Memory::LogStats();

    // std::string
    LOG_INFO("Testing std::string ...");
    std::string my_string;
    my_string.reserve(2048); // Often allocates memory upfront
    my_string = "This is a test string that will likely allocate memory on the heap.";
    my_string += " Appending more data which might cause reallocation...";
    // Print stats while string is alive - expect increases from reserve/assignments.
    Memory::LogStats();

    // std::map
    // Maps allocate memory for nodes (key, value, pointers)
    LOG_INFO("Testing std::map ...");
    std::map<int, std::string> my_map;
    for (int i = 0; i < 100; ++i)
    {
      // Allocations for map nodes and potentially internal strings for small string optimization bypass.
      my_map[i] = "Value " + std::to_string(i);
    }
    // Print stats while map is alive - expect increases from node/string allocations.
    Memory::LogStats();
  } // my_vector, my_string, my_map go out of scope here, their destructors run, freeing memory.

  LOG_INFO("Test Ended");
  // --- Final Stats ---
  // Print stats after STL containers have been destroyed.
  // Expect current usage to decrease compared to the 'After std::map usage' stage.
  Memory::LogStats();
}
#endif
