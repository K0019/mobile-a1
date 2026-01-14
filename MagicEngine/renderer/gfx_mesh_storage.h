/**
 * @file gfx_mesh_storage.h
 * @brief Mesh storage system for hina-vk backend.
 *
 * Provides:
 * - GPU buffer management for vertex/index data
 * - Mesh handle allocation with generational indices
 * - Support for both packed (24-byte) and full (48-byte) vertex formats
 *
 * Unlike the BDA-based GPUBuffers, this uses traditional vertex buffers
 * that can be bound via hina_cmd_apply_vertex_input().
 */

#pragma once

#include "gfx_types.h"
#include "mesh_types.h"           // For FullVertex, MeshBounds
#include "chunked_mesh_storage.h" // For ChunkedMeshStorage, ChunkedMeshHandle
#include <vector>
#include <cstdint>
#include <memory>

namespace gfx {

// ============================================================================
// Mesh Data Structures (FullVertex and MeshBounds are in mesh_types.h)
// ============================================================================

/**
 * @brief GPU-side mesh info (returned after upload).
 *
 * With chunked storage, buffers may be shared between meshes.
 * Use the offset fields when binding buffers.
 */
struct GpuMesh {
    Buffer vertexBuffer;           // Position stream (may be shared chunk buffer)
    Buffer attributeBuffer;        // Attribute stream (may be shared chunk buffer)
    Buffer indexBuffer;            // Index buffer (may be shared chunk buffer)
    uint32_t vertexOffset = 0;     // Byte offset in vertex buffer
    uint32_t attributeOffset = 0;  // Byte offset in attribute buffer
    uint32_t indexOffset = 0;      // Byte offset in index buffer
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    MeshBounds bounds;
    bool valid = false;
};

/**
 * @brief Mesh allocation entry in the storage.
 *
 * Stores a ChunkedMeshHandle that refers to data in the ChunkedMeshStorage.
 */
struct MeshEntry {
    ChunkedMeshHandle chunkedHandle;  // Handle to chunked storage
    uint16_t generation = 0;
    bool inUse = false;
};

// ============================================================================
// GfxMeshStorage
// ============================================================================

/**
 * @brief Manages GPU mesh storage for the gfx renderer.
 *
 * Design decisions:
 * - Uses separate position/attribute streams (TBDR-friendly)
 * - Uses ChunkedMeshStorage for sub-allocation within fixed-size chunks
 * - Generational handles to catch use-after-free
 * - Chunk-level tracking for RenderGraph resource lifetime management
 */
class GfxMeshStorage {
public:
    GfxMeshStorage();
    ~GfxMeshStorage();

    /**
     * @brief Upload a mesh with full vertex data.
     * @param vertices Array of full vertices
     * @param vertexCount Number of vertices
     * @param indices Array of indices (32-bit)
     * @param indexCount Number of indices
     * @return Handle to the mesh, or invalid handle on failure
     */
    MeshHandle upload(
        const FullVertex* vertices, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount);

    /**
     * @brief Upload a mesh with pre-packed vertex data.
     * @param positions Position stream (12 bytes per vertex)
     * @param attributes Attribute stream (12 bytes per vertex)
     * @param vertexCount Number of vertices
     * @param indices Array of indices (32-bit)
     * @param indexCount Number of indices
     * @param bounds Pre-computed bounds
     * @return Handle to the mesh, or invalid handle on failure
     */
    MeshHandle uploadPacked(
        const VertexPosition* positions,
        const VertexAttributes* attributes,
        uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount,
        const MeshBounds& bounds);

    /**
     * @brief Upload a mesh with 16-bit indices.
     */
    MeshHandle upload16(
        const FullVertex* vertices, uint32_t vertexCount,
        const uint16_t* indices, uint32_t indexCount);

    /**
     * @brief Destroy a mesh and free its GPU resources.
     */
    void destroy(MeshHandle handle);

    /**
     * @brief Get mesh data by handle.
     * @return Pointer to mesh data, or nullptr if handle is invalid
     */
    const GpuMesh* get(MeshHandle handle) const;

    /**
     * @brief Check if a handle is valid.
     */
    bool isValid(MeshHandle handle) const;

    /**
     * @brief Get the total number of meshes currently stored.
     */
    uint32_t getMeshCount() const { return m_meshCount; }

    /**
     * @brief Destroy all meshes.
     */
    void clear();

    // ========================================================================
    // RenderGraph Integration
    // ========================================================================

    /**
     * @brief Get access to the underlying chunked storage for RenderGraph import.
     */
    ChunkedMeshStorage& getChunkedStorage() { return m_chunkedStorage; }
    const ChunkedMeshStorage& getChunkedStorage() const { return m_chunkedStorage; }

    /**
     * @brief Import all active chunks to RenderGraph.
     * Convenience wrapper around ChunkedMeshStorage::importActiveChunksToRenderGraph.
     */
    void importChunksToRenderGraph(RenderGraph& graph) const {
        m_chunkedStorage.importActiveChunksToRenderGraph(graph);
    }

private:
    /**
     * @brief Allocate a new mesh entry.
     */
    MeshHandle allocateEntry();

    /**
     * @brief Free a mesh entry.
     */
    void freeEntry(uint16_t index);

    /**
     * @brief Build GpuMesh from chunked allocation.
     */
    GpuMesh buildGpuMesh(const ChunkedMeshHandle& chunkedHandle) const;

    // Chunked storage backend
    mutable ChunkedMeshStorage m_chunkedStorage;

    // Handle mapping (MeshHandle -> ChunkedMeshHandle)
    std::vector<MeshEntry> m_entries;
    std::vector<uint16_t> m_freeList;
    uint32_t m_meshCount = 0;

    // Cached GpuMesh for get() calls (avoids rebuilding on every call)
    mutable GpuMesh m_cachedMesh;
    mutable MeshHandle m_cachedHandle;
};

} // namespace gfx
