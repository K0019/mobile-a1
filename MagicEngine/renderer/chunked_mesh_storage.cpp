/**
 * @file chunked_mesh_storage.cpp
 * @brief Implementation of chunked GPU memory allocation for mesh data.
 *
 * Uses Sebastian Aaltonen's offset allocator for O(1) alloc/free.
 */

#include "chunked_mesh_storage.h"
#include "hina_context.h"
#include "render_graph.h"
#include "logging/log.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>

namespace gfx {

// ============================================================================
// ChunkedMeshStorage Implementation
// ============================================================================

ChunkedMeshStorage::ChunkedMeshStorage() = default;

ChunkedMeshStorage::~ChunkedMeshStorage() {
    shutdown();
}

void ChunkedMeshStorage::initialize(size_t vertexChunkSize, size_t indexChunkSize) {
    if (m_initialized) return;

    m_vertexChunkSize = vertexChunkSize;
    m_indexChunkSize = indexChunkSize;
    m_initialized = true;

    LOG_INFO("[ChunkedMeshStorage] Initialized with vertex chunk size: {} MB, index chunk size: {} MB",
             vertexChunkSize / (1024 * 1024), indexChunkSize / (1024 * 1024));
}

void ChunkedMeshStorage::shutdown() {
    if (!m_initialized) return;

    // Free all chunks
    for (auto& chunk : m_chunks) {
        if (chunk) {
            if (hina_buffer_is_valid(chunk->vertexBuffer)) {
                hina_destroy_buffer(chunk->vertexBuffer);
            }
            if (hina_buffer_is_valid(chunk->attributeBuffer)) {
                hina_destroy_buffer(chunk->attributeBuffer);
            }
            if (hina_buffer_is_valid(chunk->indexBuffer)) {
                hina_destroy_buffer(chunk->indexBuffer);
            }
        }
    }
    m_chunks.clear();
    m_entries.clear();
    m_freeList.clear();
    m_meshCount = 0;
    m_initialized = false;

    LOG_INFO("[ChunkedMeshStorage] Shutdown complete");
}

ChunkedMeshHandle ChunkedMeshStorage::upload(
    const FullVertex* vertices, uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount)
{
    if (!m_initialized) {
        initialize();
    }

    // Pack vertices into position/attribute streams
    std::vector<VertexPosition> positions;
    std::vector<VertexAttributes> attributes;
    MeshBounds bounds;
    packVertices(vertices, vertexCount, positions, attributes, bounds);

    return uploadPacked(positions.data(), attributes.data(), vertexCount,
                        indices, indexCount, bounds);
}

ChunkedMeshHandle ChunkedMeshStorage::uploadPacked(
    const VertexPosition* positions,
    const VertexAttributes* attributes,
    uint32_t vertexCount,
    const uint32_t* indices, uint32_t indexCount,
    const MeshBounds& bounds)
{
    if (!m_initialized) {
        initialize();
    }

    if (!positions || !indices || vertexCount == 0 || indexCount == 0) {
        LOG_ERROR("[ChunkedMeshStorage] Invalid mesh data");
        return ChunkedMeshHandle{};
    }

    // Calculate sizes (aligned for offset allocator)
    uint32_t vertexSize = static_cast<uint32_t>(vertexCount * sizeof(VertexPosition));
    uint32_t attributeSize = attributes ? static_cast<uint32_t>(vertexCount * sizeof(VertexAttributes)) : 0;
    uint32_t indexSize = static_cast<uint32_t>(indexCount * sizeof(uint32_t));

    // Align sizes up to MESH_ALLOCATION_ALIGNMENT
    vertexSize = (vertexSize + MESH_ALLOCATION_ALIGNMENT - 1) & ~(MESH_ALLOCATION_ALIGNMENT - 1);
    if (attributeSize > 0) {
        attributeSize = (attributeSize + MESH_ALLOCATION_ALIGNMENT - 1) & ~(MESH_ALLOCATION_ALIGNMENT - 1);
    }
    indexSize = (indexSize + MESH_ALLOCATION_ALIGNMENT - 1) & ~(MESH_ALLOCATION_ALIGNMENT - 1);

    // Find or create a chunk with enough space
    uint32_t chunkIndex = findOrCreateChunk(vertexSize, attributeSize, indexSize);
    if (chunkIndex == UINT32_MAX) {
        LOG_ERROR("[ChunkedMeshStorage] Failed to allocate chunk for mesh");
        return ChunkedMeshHandle{};
    }

    auto& chunk = m_chunks[chunkIndex];

    // Allocate within the chunk using offset allocator
    OffsetAllocator::Allocation vertexAlloc = chunk->vertexAllocator->allocate(vertexSize);
    OffsetAllocator::Allocation attributeAlloc;
    if (attributes) {
        attributeAlloc = chunk->attributeAllocator->allocate(attributeSize);
    }
    OffsetAllocator::Allocation indexAlloc = chunk->indexAllocator->allocate(indexSize);

    if (!vertexAlloc.isValid() || !indexAlloc.isValid() ||
        (attributes && !attributeAlloc.isValid())) {
        LOG_ERROR("[ChunkedMeshStorage] Sub-allocation failed (should not happen)");
        // Rollback
        if (vertexAlloc.isValid()) chunk->vertexAllocator->free(vertexAlloc);
        if (attributeAlloc.isValid()) chunk->attributeAllocator->free(attributeAlloc);
        return ChunkedMeshHandle{};
    }

    // Upload data to GPU buffers (persistently mapped - no unmap needed)
    void* vertexMapped = hina_map_buffer(chunk->vertexBuffer);
    if (vertexMapped) {
        std::memcpy(static_cast<uint8_t*>(vertexMapped) + vertexAlloc.offset,
                    positions, vertexCount * sizeof(VertexPosition));
        // No unmap - HOST_VISIBLE buffers are persistently mapped
    }

    if (attributes && hina_buffer_is_valid(chunk->attributeBuffer)) {
        void* attrMapped = hina_map_buffer(chunk->attributeBuffer);
        if (attrMapped) {
            std::memcpy(static_cast<uint8_t*>(attrMapped) + attributeAlloc.offset,
                        attributes, vertexCount * sizeof(VertexAttributes));
            // No unmap - HOST_VISIBLE buffers are persistently mapped
        }
    }

    void* indexMapped = hina_map_buffer(chunk->indexBuffer);
    if (indexMapped) {
        std::memcpy(static_cast<uint8_t*>(indexMapped) + indexAlloc.offset,
                    indices, indexCount * sizeof(uint32_t));
        // No unmap - HOST_VISIBLE buffers are persistently mapped
    }

    // Allocate mesh entry
    ChunkedMeshHandle handle = allocateEntry();
    if (!handle.isValid()) {
        // Rollback allocations
        chunk->vertexAllocator->free(vertexAlloc);
        if (attributes) chunk->attributeAllocator->free(attributeAlloc);
        chunk->indexAllocator->free(indexAlloc);
        return ChunkedMeshHandle{};
    }

    // Fill in allocation info
    auto& entry = m_entries[handle.index];
    entry.allocation.chunkIndex = chunkIndex;
    entry.allocation.vertexAlloc = vertexAlloc;
    entry.allocation.attributeAlloc = attributeAlloc;
    entry.allocation.indexAlloc = indexAlloc;
    entry.allocation.vertexSize = vertexSize;
    entry.allocation.attributeSize = attributeSize;
    entry.allocation.indexSize = indexSize;
    entry.allocation.vertexCount = vertexCount;
    entry.allocation.indexCount = indexCount;
    entry.allocation.bounds = bounds;

    chunk->meshCount++;
    m_meshCount++;

    return handle;
}

void ChunkedMeshStorage::destroy(ChunkedMeshHandle handle) {
    if (!isValid(handle)) return;

    auto& entry = m_entries[handle.index];
    const auto& alloc = entry.allocation;

    if (alloc.chunkIndex < m_chunks.size()) {
        auto& chunk = m_chunks[alloc.chunkIndex];
        if (chunk) {
            // Free sub-allocations using offset allocator
            chunk->vertexAllocator->free(alloc.vertexAlloc);
            if (alloc.attributeAlloc.isValid()) {
                chunk->attributeAllocator->free(alloc.attributeAlloc);
            }
            chunk->indexAllocator->free(alloc.indexAlloc);
            chunk->meshCount--;
        }
    }

    freeEntry(handle.index);
    m_meshCount--;
}

const MeshAllocation* ChunkedMeshStorage::getAllocation(ChunkedMeshHandle handle) const {
    if (!isValid(handle)) return nullptr;
    return &m_entries[handle.index].allocation;
}

bool ChunkedMeshStorage::isValid(ChunkedMeshHandle handle) const {
    if (handle.index >= m_entries.size()) return false;
    const auto& entry = m_entries[handle.index];
    return entry.inUse && entry.generation == handle.generation;
}

const MeshChunk* ChunkedMeshStorage::getChunk(ChunkedMeshHandle handle) const {
    const MeshAllocation* alloc = getAllocation(handle);
    if (!alloc) return nullptr;
    return getChunkByIndex(alloc->chunkIndex);
}

const MeshChunk* ChunkedMeshStorage::getChunkByIndex(uint32_t chunkIndex) const {
    if (chunkIndex >= m_chunks.size()) return nullptr;
    return m_chunks[chunkIndex].get();
}

std::vector<uint32_t> ChunkedMeshStorage::getActiveChunks(
    const ChunkedMeshHandle* handles, size_t count) const
{
    std::vector<uint32_t> activeChunks;
    activeChunks.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        const MeshAllocation* alloc = getAllocation(handles[i]);
        if (alloc && alloc->isValid()) {
            // Add if not already present
            if (std::find(activeChunks.begin(), activeChunks.end(), alloc->chunkIndex) == activeChunks.end()) {
                activeChunks.push_back(alloc->chunkIndex);
            }
        }
    }

    return activeChunks;
}

std::vector<uint32_t> ChunkedMeshStorage::getAllActiveChunks() const {
    std::vector<uint32_t> activeChunks;
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_chunks.size()); ++i) {
        if (m_chunks[i] && m_chunks[i]->meshCount > 0) {
            activeChunks.push_back(i);
        }
    }
    return activeChunks;
}

ChunkedMeshStorage::Stats ChunkedMeshStorage::getStats() const {
    Stats stats;
    stats.chunkCount = static_cast<uint32_t>(m_chunks.size());
    stats.meshCount = m_meshCount;

    for (const auto& chunk : m_chunks) {
        if (chunk) {
            auto vertexReport = chunk->vertexAllocator->storageReport();
            auto attrReport = chunk->attributeAllocator->storageReport();
            auto indexReport = chunk->indexAllocator->storageReport();

            stats.totalVertexMemory += chunk->vertexCapacity + chunk->attributeCapacity;
            stats.usedVertexMemory += (chunk->vertexCapacity - vertexReport.totalFreeSpace) +
                                      (chunk->attributeCapacity - attrReport.totalFreeSpace);
            stats.totalIndexMemory += chunk->indexCapacity;
            stats.usedIndexMemory += chunk->indexCapacity - indexReport.totalFreeSpace;
        }
    }

    return stats;
}

uint32_t ChunkedMeshStorage::findOrCreateChunk(size_t vertexSize, size_t attributeSize, size_t indexSize) {
    // Try to find an existing chunk with enough space
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_chunks.size()); ++i) {
        auto& chunk = m_chunks[i];
        if (!chunk) continue;

        auto vertexReport = chunk->vertexAllocator->storageReport();
        auto attrReport = chunk->attributeAllocator->storageReport();
        auto indexReport = chunk->indexAllocator->storageReport();

        bool hasVertexSpace = vertexReport.largestFreeRegion >= vertexSize;
        bool hasAttrSpace = attributeSize == 0 || attrReport.largestFreeRegion >= attributeSize;
        bool hasIndexSpace = indexReport.largestFreeRegion >= indexSize;

        if (hasVertexSpace && hasAttrSpace && hasIndexSpace) {
            return i;
        }
    }

    // No suitable chunk found, create a new one
    return createChunk();
}

uint32_t ChunkedMeshStorage::createChunk() {
    auto chunk = std::make_unique<MeshChunk>();

    // Create vertex buffer (position stream)
    hina_buffer_desc vertexDesc = {};
    vertexDesc.size = m_vertexChunkSize;
    vertexDesc.flags = static_cast<hina_buffer_flags>(
        HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_TRANSFER_DST_BIT |
        HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    chunk->vertexBuffer = hina_make_buffer(&vertexDesc);

    // Create attribute buffer (same size as vertex for now)
    hina_buffer_desc attrDesc = {};
    attrDesc.size = m_vertexChunkSize;
    attrDesc.flags = static_cast<hina_buffer_flags>(
        HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_TRANSFER_DST_BIT |
        HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    chunk->attributeBuffer = hina_make_buffer(&attrDesc);

    // Create index buffer
    hina_buffer_desc indexDesc = {};
    indexDesc.size = m_indexChunkSize;
    indexDesc.flags = static_cast<hina_buffer_flags>(
        HINA_BUFFER_INDEX_BIT | HINA_BUFFER_TRANSFER_DST_BIT |
        HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    chunk->indexBuffer = hina_make_buffer(&indexDesc);

    if (!hina_buffer_is_valid(chunk->vertexBuffer) ||
        !hina_buffer_is_valid(chunk->attributeBuffer) ||
        !hina_buffer_is_valid(chunk->indexBuffer)) {
        LOG_ERROR("[ChunkedMeshStorage] Failed to create chunk buffers");
        if (hina_buffer_is_valid(chunk->vertexBuffer)) hina_destroy_buffer(chunk->vertexBuffer);
        if (hina_buffer_is_valid(chunk->attributeBuffer)) hina_destroy_buffer(chunk->attributeBuffer);
        if (hina_buffer_is_valid(chunk->indexBuffer)) hina_destroy_buffer(chunk->indexBuffer);
        return UINT32_MAX;
    }

    // Initialize offset allocators (O(1) alloc/free)
    chunk->vertexCapacity = m_vertexChunkSize;
    chunk->attributeCapacity = m_vertexChunkSize;
    chunk->indexCapacity = m_indexChunkSize;
    chunk->vertexAllocator = std::make_unique<OffsetAllocator::Allocator>(
        static_cast<uint32_t>(m_vertexChunkSize), MAX_ALLOCATIONS_PER_CHUNK);
    chunk->attributeAllocator = std::make_unique<OffsetAllocator::Allocator>(
        static_cast<uint32_t>(m_vertexChunkSize), MAX_ALLOCATIONS_PER_CHUNK);
    chunk->indexAllocator = std::make_unique<OffsetAllocator::Allocator>(
        static_cast<uint32_t>(m_indexChunkSize), MAX_ALLOCATIONS_PER_CHUNK);

    // Find a slot for the chunk
    uint32_t chunkIndex = static_cast<uint32_t>(m_chunks.size());
    chunk->chunkIndex = chunkIndex;
    m_chunks.push_back(std::move(chunk));

    LOG_INFO("[ChunkedMeshStorage] Created chunk {} (vertex: {} MB, index: {} MB)",
             chunkIndex, m_vertexChunkSize / (1024 * 1024), m_indexChunkSize / (1024 * 1024));

    return chunkIndex;
}

ChunkedMeshHandle ChunkedMeshStorage::allocateEntry() {
    uint32_t index;
    if (!m_freeList.empty()) {
        index = m_freeList.back();
        m_freeList.pop_back();
    } else {
        index = static_cast<uint32_t>(m_entries.size());
        m_entries.emplace_back();
    }

    auto& entry = m_entries[index];
    entry.inUse = true;
    // Generation already incremented on free, or is 0 for new entry

    return ChunkedMeshHandle{index, entry.generation};
}

void ChunkedMeshStorage::freeEntry(uint32_t index) {
    if (index >= m_entries.size()) return;

    auto& entry = m_entries[index];
    entry.inUse = false;
    entry.generation++;  // Increment generation to invalidate old handles
    entry.allocation = MeshAllocation{};
    m_freeList.push_back(index);
}

void ChunkedMeshStorage::packVertices(
    const FullVertex* vertices, uint32_t count,
    std::vector<VertexPosition>& positions,
    std::vector<VertexAttributes>& attributes,
    MeshBounds& outBounds)
{
    positions.resize(count);
    attributes.resize(count);

    glm::vec3 minPos(std::numeric_limits<float>::max());
    glm::vec3 maxPos(std::numeric_limits<float>::lowest());

    for (uint32_t i = 0; i < count; ++i) {
        const auto& v = vertices[i];

        // Pack position
        positions[i] = VertexPosition(v.position);

        // Pack attributes using the pack function
        // tangent.w contains the bitangent sign
        float bitangentSign = v.tangent.w;
        glm::vec3 tangent = glm::vec3(v.tangent);
        attributes[i].pack(v.normal, tangent, bitangentSign, v.getUV(), 0);

        // Update bounds
        minPos = glm::min(minPos, v.position);
        maxPos = glm::max(maxPos, v.position);
    }

    outBounds.aabbMin = minPos;
    outBounds.aabbMax = maxPos;
    outBounds.center = (minPos + maxPos) * 0.5f;
    outBounds.radius = glm::length(maxPos - outBounds.center);
}

// ============================================================================
// RenderGraph Integration
// ============================================================================

ChunkedMeshStorage::ChunkBufferInfo ChunkedMeshStorage::getChunkBufferInfo(
    uint32_t chunkIndex) const
{
    ChunkBufferInfo info;

    if (chunkIndex >= m_chunks.size() || !m_chunks[chunkIndex]) {
        return info;
    }

    const auto& chunk = m_chunks[chunkIndex];
    if (!chunk->isValid()) {
        return info;
    }

    // Create gfx::BufferDesc for each buffer
    info.vertexDesc = gfx::BufferDesc{
        .size = chunk->vertexCapacity,
        .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Vertex | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
    };

    info.attributeDesc = gfx::BufferDesc{
        .size = chunk->attributeCapacity,
        .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Vertex | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
    };

    info.indexDesc = gfx::BufferDesc{
        .size = chunk->indexCapacity,
        .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Index | gfx::BufferUsage::HostVisible | gfx::BufferUsage::HostCoherent)
    };

    // Return raw gfx::Buffer handles directly
    info.vertexBuffer = chunk->vertexBuffer;
    info.attributeBuffer = chunk->attributeBuffer;
    info.indexBuffer = chunk->indexBuffer;

    info.valid = gfx::isValid(info.vertexBuffer) &&
                 gfx::isValid(info.attributeBuffer) &&
                 gfx::isValid(info.indexBuffer);

    return info;
}

void ChunkedMeshStorage::importActiveChunksToRenderGraph(
    RenderGraph& graph) const
{
    auto activeChunks = getAllActiveChunks();

    for (uint32_t chunkIdx : activeChunks) {
        auto info = getChunkBufferInfo(chunkIdx);
        if (!info.valid) continue;

        // Generate resource names for this chunk
        std::ostringstream vertexName, attrName, indexName;
        vertexName << "MeshChunk" << chunkIdx << "_Vertex";
        attrName << "MeshChunk" << chunkIdx << "_Attr";
        indexName << "MeshChunk" << chunkIdx << "_Index";

        // Import as external buffers
        graph.ImportExternalBuffer(vertexName.str().c_str(), info.vertexBuffer, info.vertexDesc);
        graph.ImportExternalBuffer(attrName.str().c_str(), info.attributeBuffer, info.attributeDesc);
        graph.ImportExternalBuffer(indexName.str().c_str(), info.indexBuffer, info.indexDesc);
    }

    LOG_DEBUG("[ChunkedMeshStorage] Imported {} active chunks to RenderGraph", activeChunks.size());
}

} // namespace gfx
