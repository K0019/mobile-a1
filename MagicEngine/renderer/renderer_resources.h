#pragma once
#include "gpu_data.h"
#include "gfx_interface.h"

class GPUBuffers
{
  private:
    // Buffer descriptors using gfx types
    gfx::BufferDesc m_vertexBufferDesc{};
    gfx::BufferDesc m_indexBufferDesc{};
    gfx::BufferDesc m_materialBufferDesc{};
    gfx::BufferDesc m_meshDecompressionBufferDesc{};
    gfx::BufferDesc m_skinningBufferDesc{};
    gfx::BufferDesc m_morphDeltaBufferDesc{};
    gfx::BufferDesc m_morphVertexBaseBufferDesc{};
    gfx::BufferDesc m_morphVertexCountBufferDesc{};

    // Fixed GPU buffers using gfx Holders
    gfx::Holder<gfx::Buffer> m_vertexBuffer;
    gfx::Holder<gfx::Buffer> m_indexBuffer;
    gfx::Holder<gfx::Buffer> m_materialBuffer;
    gfx::Holder<gfx::Buffer> m_meshDecompressionBuffer;
    gfx::Holder<gfx::Buffer> m_skinningBuffer;
    gfx::Holder<gfx::Buffer> m_morphDeltaBuffer;
    gfx::Holder<gfx::Buffer> m_morphVertexBaseBuffer;
    gfx::Holder<gfx::Buffer> m_morphVertexCountBuffer;

    // Simple texture ownership - no maps, just vector storage
    std::vector<gfx::Holder<gfx::Texture>> m_ownedTextures;

  public:
    GPUBuffers(size_t vertexBufferSize,
               size_t indexBufferSize,
               size_t materialBufferSize,
               size_t meshDecompressionBufferSize,
               size_t skinningBufferSize,
               size_t morphDeltaBufferSize,
               size_t morphVertexBaseBufferSize,
               size_t morphVertexCountBufferSize);

    ~GPUBuffers();

    bool uploadtoBuffer(gfx::Buffer buffer, uint32_t byteOffset, const void* data, size_t dataSize);

    bool uploadMesh(uint32_t vertexOffset, uint32_t indexOffset, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    bool uploadMaterial(uint32_t byteOffset, const MaterialData& data);

    bool uploadMeshDecompression(uint32_t byteOffset, const MeshDecompressionData& data);

    uint32_t uploadTexture(const gfx::TextureDesc& desc, const std::vector<uint8_t>& data);

    void deleteTexture(uint32_t textureId);

    // Buffer accessors
    gfx::Buffer GetVertexBuffer() const;

    gfx::Buffer GetIndexBuffer() const;

    gfx::Buffer GetMaterialBuffer() const;

    gfx::Buffer GetMeshDecompressionBuffer() const;

    gfx::Buffer GetSkinningBuffer() const;

    gfx::Buffer GetMorphDeltaBuffer() const;

    gfx::Buffer GetMorphVertexBaseBuffer() const;

    gfx::Buffer GetMorphVertexCountBuffer() const;

    gfx::BufferDesc GetVertexBufferDesc() const;

    gfx::BufferDesc GetIndexBufferDesc() const;

    gfx::BufferDesc GetMaterialBufferDesc() const;

    gfx::BufferDesc GetMeshDecompressionBufferDesc() const;

    gfx::BufferDesc GetSkinningBufferDesc() const;

    gfx::BufferDesc GetMorphDeltaBufferDesc() const;

    gfx::BufferDesc GetMorphVertexBaseBufferDesc() const;

    gfx::BufferDesc GetMorphVertexCountBufferDesc() const;

    size_t GetOwnedTextureCount() const;

  private:
    void CreateFixedBuffers();
};
