#pragma once
#include "gpu_data.h"
#include "interface.h"
class GPUBuffers
{
  private:
    vk::IContext& m_context;

    vk::BufferDesc m_vertexBufferDesc{.usage = vk::BufferUsageBits_Vertex | vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_indexBufferDesc{.usage = vk::BufferUsageBits_Index | vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_materialBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_meshDecompressionBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_skinningBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_morphDeltaBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_morphVertexBaseBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    vk::BufferDesc m_morphVertexCountBufferDesc{.usage = vk::BufferUsageBits_Storage, .storage = vk::StorageType::Device, .size = 0,};
    // Fixed GPU buffers
    vk::Holder<vk::BufferHandle> m_vertexBuffer;
    vk::Holder<vk::BufferHandle> m_indexBuffer;
    vk::Holder<vk::BufferHandle> m_materialBuffer;
    vk::Holder<vk::BufferHandle> m_meshDecompressionBuffer;
    vk::Holder<vk::BufferHandle> m_skinningBuffer;
    vk::Holder<vk::BufferHandle> m_morphDeltaBuffer;
    vk::Holder<vk::BufferHandle> m_morphVertexBaseBuffer;
    vk::Holder<vk::BufferHandle> m_morphVertexCountBuffer;

    // Simple texture ownership - no maps, just vector storage
    std::vector<vk::Holder<vk::TextureHandle>> m_ownedTextures;

  public:
    GPUBuffers(vk::IContext& context,
               size_t vertexBufferSize,
               size_t indexBufferSize,
               size_t materialBufferSize,
               size_t meshDecompressionBufferSize,
               size_t skinningBufferSize,
               size_t morphDeltaBufferSize,
               size_t morphVertexBaseBufferSize,
               size_t morphVertexCountBufferSize);

    ~GPUBuffers();

    bool uploadtoBuffer(vk::BufferHandle buffer, uint32_t byteOffset, const void* data, size_t dataSize);

    bool uploadMesh(uint32_t vertexOffset, uint32_t indexOffset, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    bool uploadMaterial(uint32_t byteOffset, const MaterialData& data);

    bool uploadMeshDecompression(uint32_t byteOffset, const MeshDecompressionData& data);

    uint32_t uploadTexture(const vk::TextureDesc& desc, const std::vector<uint8_t>& data);

    void deleteTexture(uint32_t bindlessIndex);

    // Buffer accessors
    vk::BufferHandle GetVertexBuffer() const;

    vk::BufferHandle GetIndexBuffer() const;

    vk::BufferHandle GetMaterialBuffer() const;

    vk::BufferHandle GetMeshDecompressionBuffer() const;

    vk::BufferHandle GetSkinningBuffer() const;

    vk::BufferHandle GetMorphDeltaBuffer() const;

    vk::BufferHandle GetMorphVertexBaseBuffer() const;

    vk::BufferHandle GetMorphVertexCountBuffer() const;

    vk::BufferDesc GetVertexBufferDesc() const;

    vk::BufferDesc GetIndexBufferDesc() const;

    vk::BufferDesc GetMaterialBufferDesc() const;

    vk::BufferDesc GetMeshDecompressionBufferDesc() const;

    vk::BufferDesc GetSkinningBufferDesc() const;

    vk::BufferDesc GetMorphDeltaBufferDesc() const;

    vk::BufferDesc GetMorphVertexBaseBufferDesc() const;

    vk::BufferDesc GetMorphVertexCountBufferDesc() const;

    size_t GetOwnedTextureCount() const;

  private:
    void CreateFixedBuffers();
};
