/**
 * @file gfx_mesh_storage.cpp
 * @brief Implementation of mesh storage for hina-vk backend.
 *
 * Delegates to ChunkedMeshStorage for actual GPU memory management.
 */

#include "gfx_mesh_storage.h"
#include "logging/log.h"
#include <cstring>
#include <algorithm>

namespace gfx {

GfxMeshStorage::GfxMeshStorage() {
    // Reserve some initial capacity for entries
    m_entries.reserve(256);
    m_freeList.reserve(64);

    // Initialize chunked storage with default chunk sizes
    m_chunkedStorage.initialize();

    LOG_INFO("[GfxMeshStorage] Created with ChunkedMeshStorage backend");
}

GfxMeshStorage::~GfxMeshStorage() {
    clear();
    LOG_INFO("[GfxMeshStorage] Destroyed");
}

MeshHandle GfxMeshStorage::allocateEntry() {
    MeshHandle handle;

    if (!m_freeList.empty()) {
        // Reuse freed slot
        handle.index = m_freeList.back();
        m_freeList.pop_back();
        handle.generation = m_entries[handle.index].generation;
    } else {
        // Allocate new slot
        if (m_entries.size() >= 65535) {
            LOG_ERROR("[GfxMeshStorage] Maximum mesh count exceeded");
            return MeshHandle{};
        }
        handle.index = static_cast<uint16_t>(m_entries.size());
        handle.generation = 1;
        m_entries.push_back({});
        m_entries.back().generation = 1;
    }

    m_entries[handle.index].inUse = true;
    m_meshCount++;
    return handle;
}

void GfxMeshStorage::freeEntry(uint16_t index) {
    if (index >= m_entries.size()) return;

    MeshEntry& entry = m_entries[index];
    if (!entry.inUse) return;

    // Destroy the chunked allocation
    if (entry.chunkedHandle.isValid()) {
        m_chunkedStorage.destroy(entry.chunkedHandle);
    }

    entry.chunkedHandle = ChunkedMeshHandle{};
    entry.inUse = false;
    entry.generation++; // Increment generation to invalidate old handles

    m_freeList.push_back(index);
    m_meshCount--;
}

GpuMesh GfxMeshStorage::buildGpuMesh(const ChunkedMeshHandle& chunkedHandle) const {
    GpuMesh mesh;

    const MeshAllocation* alloc = m_chunkedStorage.getAllocation(chunkedHandle);
    if (!alloc || !alloc->isValid()) {
        return mesh;
    }

    const MeshChunk* chunk = m_chunkedStorage.getChunkByIndex(alloc->chunkIndex);
    if (!chunk || !chunk->isValid()) {
        return mesh;
    }

    // Fill in buffer references and offsets
    mesh.vertexBuffer = chunk->vertexBuffer;
    mesh.attributeBuffer = chunk->attributeBuffer;
    mesh.indexBuffer = chunk->indexBuffer;
    mesh.vertexOffset = alloc->vertexOffset();
    mesh.attributeOffset = alloc->attributeOffset();
    mesh.indexOffset = alloc->indexOffset();
    mesh.vertexCount = alloc->vertexCount;
    mesh.indexCount = alloc->indexCount;
    mesh.bounds = alloc->bounds;
    mesh.valid = true;

    return mesh;
}

MeshHandle GfxMeshStorage::upload(
    const FullVertex* vertices, uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount)
{
    if (!vertices || vertexCount == 0 || !indices || indexCount == 0) {
        LOG_ERROR("[GfxMeshStorage] Invalid upload parameters");
        return MeshHandle{};
    }

    // Allocate entry first
    MeshHandle handle = allocateEntry();
    if (!handle.isValid()) {
        return handle;
    }

    // Upload to chunked storage
    ChunkedMeshHandle chunkedHandle = m_chunkedStorage.upload(vertices, vertexCount, indices, indexCount);
    if (!chunkedHandle.isValid()) {
        LOG_ERROR("[GfxMeshStorage] ChunkedMeshStorage upload failed");
        freeEntry(handle.index);
        return MeshHandle{};
    }

    // Store the mapping
    m_entries[handle.index].chunkedHandle = chunkedHandle;

    const MeshAllocation* alloc = m_chunkedStorage.getAllocation(chunkedHandle);
    LOG_DEBUG("[GfxMeshStorage] Uploaded mesh: {} vertices, {} indices -> chunk {}",
              vertexCount, indexCount, alloc ? alloc->chunkIndex : UINT32_MAX);

    return handle;
}

MeshHandle GfxMeshStorage::uploadPacked(
    const VertexPosition* positions,
    const VertexAttributes* attributes,
    uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount,
    const MeshBounds& bounds)
{
    if (!positions || vertexCount == 0 || !indices || indexCount == 0) {
        LOG_ERROR("[GfxMeshStorage] Invalid packed upload parameters");
        return MeshHandle{};
    }

    // Allocate entry first
    MeshHandle handle = allocateEntry();
    if (!handle.isValid()) {
        return handle;
    }

    // Upload to chunked storage
    ChunkedMeshHandle chunkedHandle = m_chunkedStorage.uploadPacked(
        positions, attributes, vertexCount, indices, indexCount, bounds);
    if (!chunkedHandle.isValid()) {
        LOG_ERROR("[GfxMeshStorage] ChunkedMeshStorage uploadPacked failed");
        freeEntry(handle.index);
        return MeshHandle{};
    }

    // Store the mapping
    m_entries[handle.index].chunkedHandle = chunkedHandle;

    const MeshAllocation* alloc = m_chunkedStorage.getAllocation(chunkedHandle);
    LOG_DEBUG("[GfxMeshStorage] Uploaded packed mesh: {} vertices, {} indices -> chunk {}",
              vertexCount, indexCount, alloc ? alloc->chunkIndex : UINT32_MAX);

    return handle;
}

MeshHandle GfxMeshStorage::upload16(
    const FullVertex* vertices, uint32_t vertexCount,
    const uint16_t* indices, uint32_t indexCount)
{
    // Convert 16-bit indices to 32-bit
    std::vector<uint32_t> indices32(indexCount);
    for (uint32_t i = 0; i < indexCount; ++i) {
        indices32[i] = indices[i];
    }
    return upload(vertices, vertexCount, indices32.data(), indexCount);
}

void GfxMeshStorage::destroy(MeshHandle handle) {
    if (!isValid(handle)) return;

    // Invalidate cache if this is the cached mesh
    if (m_cachedHandle == handle) {
        m_cachedHandle = MeshHandle{};
        m_cachedMesh = GpuMesh{};
    }

    freeEntry(handle.index);
    LOG_DEBUG("[GfxMeshStorage] Destroyed mesh handle {}:{}", handle.index, handle.generation);
}

const GpuMesh* GfxMeshStorage::get(MeshHandle handle) const {
    if (!isValid(handle)) return nullptr;

    // Check cache first
    if (m_cachedHandle == handle && m_cachedMesh.valid) {
        return &m_cachedMesh;
    }

    // Build GpuMesh from chunked allocation
    const MeshEntry& entry = m_entries[handle.index];
    m_cachedMesh = buildGpuMesh(entry.chunkedHandle);
    m_cachedHandle = handle;

    return m_cachedMesh.valid ? &m_cachedMesh : nullptr;
}

bool GfxMeshStorage::isValid(MeshHandle handle) const {
    if (handle.index >= m_entries.size()) return false;
    const MeshEntry& entry = m_entries[handle.index];
    return entry.inUse && entry.generation == handle.generation;
}

void GfxMeshStorage::clear() {
    // Clear all entries
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].inUse) {
            if (m_entries[i].chunkedHandle.isValid()) {
                m_chunkedStorage.destroy(m_entries[i].chunkedHandle);
            }
        }
    }
    m_entries.clear();
    m_freeList.clear();
    m_meshCount = 0;

    // Clear cache
    m_cachedHandle = MeshHandle{};
    m_cachedMesh = GpuMesh{};

    LOG_INFO("[GfxMeshStorage] Cleared all meshes");
}

} // namespace gfx
