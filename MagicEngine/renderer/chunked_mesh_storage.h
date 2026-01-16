/**
 * @file chunked_mesh_storage.h
 * @brief Chunked GPU memory allocation for mesh data.
 *
 * Provides:
 * - Fixed-size chunks of GPU memory (vertex + index)
 * - Sub-allocation of meshes within chunks using Sebastian Aaltonen's offset allocator
 * - Dynamic growth by adding new chunks as needed
 * - Chunk-level integration with RenderGraph for synchronization
 *
 * This avoids the BDA model (not mobile-friendly) while still allowing
 * the RenderGraph to track resource dependencies at chunk granularity.
 */

#pragma once

#include "gfx_interface.h"     // For gfx:: types including BufferDesc
#include "mesh_types.h"        // For FullVertex, MeshBounds
#include "interface.h"         // For vk types (historical dependency)
#include <offset_allocator/offsetAllocator.hpp>
#include <vector>
#include <cstdint>
#include <memory>

// Forward declarations
class RenderGraph;
class HinaContext;

namespace gfx {

// ============================================================================
// Configuration
// ============================================================================

// Default chunk size: 16MB for vertices, 8MB for indices
constexpr size_t DEFAULT_VERTEX_CHUNK_SIZE = 16 * 1024 * 1024;
constexpr size_t DEFAULT_INDEX_CHUNK_SIZE = 8 * 1024 * 1024;

// Minimum allocation alignment (for buffer offsets)
constexpr size_t MESH_ALLOCATION_ALIGNMENT = 16;

// Maximum allocations per chunk (for offset allocator)
constexpr uint32_t MAX_ALLOCATIONS_PER_CHUNK = 16 * 1024;

// ============================================================================
// Mesh Chunk
// ============================================================================

/**
 * @brief A chunk of GPU memory for mesh data.
 *
 * Contains separate vertex and index buffers with sub-allocators.
 * Uses Sebastian Aaltonen's offset allocator for O(1) alloc/free.
 */
struct MeshChunk {
    // GPU buffers
    Buffer vertexBuffer;      // Position stream
    Buffer attributeBuffer;   // Attribute stream (normals, UVs, tangents)
    Buffer indexBuffer;

    // Sub-allocators (offset allocator for O(1) performance)
    std::unique_ptr<OffsetAllocator::Allocator> vertexAllocator;
    std::unique_ptr<OffsetAllocator::Allocator> attributeAllocator;
    std::unique_ptr<OffsetAllocator::Allocator> indexAllocator;

    // Chunk info
    uint32_t chunkIndex = 0;
    size_t vertexCapacity = 0;
    size_t attributeCapacity = 0;
    size_t indexCapacity = 0;

    // Reference count (how many meshes are using this chunk)
    uint32_t meshCount = 0;

    bool isValid() const {
        return hina_buffer_is_valid(vertexBuffer) &&
               hina_buffer_is_valid(indexBuffer);
    }
};

// ============================================================================
// Mesh Allocation Info
// ============================================================================

/**
 * @brief Describes where a mesh's data lives within chunks.
 *
 * Stores offset allocator metadata for proper deallocation.
 */
struct MeshAllocation {
    // Which chunk contains this mesh
    uint32_t chunkIndex = UINT32_MAX;

    // Allocator handles (needed for free)
    OffsetAllocator::Allocation vertexAlloc;
    OffsetAllocator::Allocation attributeAlloc;
    OffsetAllocator::Allocation indexAlloc;

    // Sizes (for stats and debugging)
    uint32_t vertexSize = 0;
    uint32_t attributeSize = 0;
    uint32_t indexSize = 0;

    // Mesh data
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    MeshBounds bounds;

    bool isValid() const { return chunkIndex != UINT32_MAX; }

    // Convenience accessors for offsets
    uint32_t vertexOffset() const { return vertexAlloc.offset; }
    uint32_t attributeOffset() const { return attributeAlloc.offset; }
    uint32_t indexOffset() const { return indexAlloc.offset; }
};

// ============================================================================
// Chunked Mesh Handle
// ============================================================================

/**
 * @brief Handle to a mesh in the chunked storage.
 *
 * Uses generational index for use-after-free detection.
 */
struct ChunkedMeshHandle {
    uint32_t index = UINT32_MAX;
    uint16_t generation = 0;

    bool isValid() const { return index != UINT32_MAX; }

    bool operator==(const ChunkedMeshHandle& other) const {
        return index == other.index && generation == other.generation;
    }
    bool operator!=(const ChunkedMeshHandle& other) const {
        return !(*this == other);
    }
};

// ============================================================================
// Chunked Mesh Storage
// ============================================================================

/**
 * @brief Manages chunked GPU storage for mesh data.
 *
 * Design:
 * - Allocates chunks on demand (no fixed limit)
 * - Sub-allocates meshes within chunks
 * - Tracks which chunks contain which meshes
 * - Provides chunk handles for RenderGraph integration
 */
class ChunkedMeshStorage {
public:
    ChunkedMeshStorage();
    ~ChunkedMeshStorage();

    /**
     * @brief Initialize with custom chunk sizes.
     */
    void initialize(size_t vertexChunkSize = DEFAULT_VERTEX_CHUNK_SIZE,
                    size_t indexChunkSize = DEFAULT_INDEX_CHUNK_SIZE);

    /**
     * @brief Shutdown and free all GPU resources.
     */
    void shutdown();

    // ========================================================================
    // Mesh Upload
    // ========================================================================

    /**
     * @brief Upload a mesh with full vertex data.
     * @return Handle to the mesh, or invalid handle on failure
     */
    ChunkedMeshHandle upload(
        const FullVertex* vertices, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount);

    /**
     * @brief Upload a mesh with pre-packed vertex data.
     */
    ChunkedMeshHandle uploadPacked(
        const VertexPosition* positions,
        const VertexAttributes* attributes,
        uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount,
        const MeshBounds& bounds);

    /**
     * @brief Destroy a mesh and free its allocation.
     */
    void destroy(ChunkedMeshHandle handle);

    // ========================================================================
    // Mesh Access
    // ========================================================================

    /**
     * @brief Get mesh allocation info.
     * @return Pointer to allocation, or nullptr if invalid
     */
    const MeshAllocation* getAllocation(ChunkedMeshHandle handle) const;

    /**
     * @brief Check if a handle is valid.
     */
    bool isValid(ChunkedMeshHandle handle) const;

    /**
     * @brief Get the chunk containing a mesh.
     */
    const MeshChunk* getChunk(ChunkedMeshHandle handle) const;

    /**
     * @brief Get chunk by index.
     */
    const MeshChunk* getChunkByIndex(uint32_t chunkIndex) const;

    // ========================================================================
    // RenderGraph Integration
    // ========================================================================

    /**
     * @brief Get indices of chunks that contain the given meshes.
     *
     * Call this before rendering to know which chunks to import.
     */
    std::vector<uint32_t> getActiveChunks(
        const ChunkedMeshHandle* handles, size_t count) const;

    /**
     * @brief Get all non-empty chunks.
     */
    std::vector<uint32_t> getAllActiveChunks() const;

    /**
     * @brief Get the number of chunks.
     */
    uint32_t getChunkCount() const { return static_cast<uint32_t>(m_chunks.size()); }

    /**
     * @brief Get total mesh count.
     */
    uint32_t getMeshCount() const { return m_meshCount; }

    // ========================================================================
    // RenderGraph Integration - vk::BufferHandle wrappers
    // ========================================================================

    /**
     * @brief Info needed to import a chunk's buffers into the RenderGraph.
     */
    struct ChunkBufferInfo {
        gfx::Buffer vertexBuffer;
        gfx::Buffer attributeBuffer;
        gfx::Buffer indexBuffer;
        gfx::BufferDesc vertexDesc;
        gfx::BufferDesc attributeDesc;
        gfx::BufferDesc indexDesc;
        bool valid = false;
    };

    /**
     * @brief Get buffer handles and descriptions for a chunk.
     *
     * Returns the raw gfx::Buffer handles and descriptors for RenderGraph import.
     *
     * @param chunkIndex Index of the chunk
     * @return ChunkBufferInfo with handles and descriptions
     */
    ChunkBufferInfo getChunkBufferInfo(uint32_t chunkIndex) const;

    /**
     * @brief Import all active chunks into a RenderGraph.
     *
     * Convenience method that imports all non-empty chunks as external buffers.
     * Uses naming convention "MeshChunk{N}_Vertex", "MeshChunk{N}_Attr", "MeshChunk{N}_Index".
     *
     * @param graph The RenderGraph to import into
     */
    void importActiveChunksToRenderGraph(RenderGraph& graph) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    struct Stats {
        uint32_t chunkCount = 0;
        uint32_t meshCount = 0;
        size_t totalVertexMemory = 0;
        size_t usedVertexMemory = 0;
        size_t totalIndexMemory = 0;
        size_t usedIndexMemory = 0;
    };

    Stats getStats() const;

private:
    // Mesh entry with allocation and generation
    struct MeshEntry {
        MeshAllocation allocation;
        uint16_t generation = 0;
        bool inUse = false;
    };

    // Find or create a chunk with enough space
    uint32_t findOrCreateChunk(size_t vertexSize, size_t attributeSize, size_t indexSize);

    // Create a new chunk
    uint32_t createChunk();

    // Allocate a mesh entry
    ChunkedMeshHandle allocateEntry();

    // Free a mesh entry
    void freeEntry(uint32_t index);

    // Pack vertices into position/attribute streams
    void packVertices(
        const FullVertex* vertices, uint32_t count,
        std::vector<VertexPosition>& positions,
        std::vector<VertexAttributes>& attributes,
        MeshBounds& outBounds);

    // Pack vertices with winding-order-based normal consistency fix
    void packVerticesWithNormalFix(
        const FullVertex* vertices, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount,
        std::vector<VertexPosition>& positions,
        std::vector<VertexAttributes>& attributes,
        MeshBounds& outBounds);

    // Chunks
    std::vector<std::unique_ptr<MeshChunk>> m_chunks;

    // Mesh entries
    std::vector<MeshEntry> m_entries;
    std::vector<uint32_t> m_freeList;
    uint32_t m_meshCount = 0;

    // Configuration
    size_t m_vertexChunkSize = DEFAULT_VERTEX_CHUNK_SIZE;
    size_t m_indexChunkSize = DEFAULT_INDEX_CHUNK_SIZE;

    bool m_initialized = false;
};

} // namespace gfx
