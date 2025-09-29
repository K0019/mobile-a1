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
    // Fixed GPU buffers
    vk::Holder<vk::BufferHandle> m_vertexBuffer;
    vk::Holder<vk::BufferHandle> m_indexBuffer;
    vk::Holder<vk::BufferHandle> m_materialBuffer;

    // Simple texture ownership - no maps, just vector storage
    std::vector<vk::Holder<vk::TextureHandle>> m_ownedTextures;

  public:
    GPUBuffers(vk::IContext& context, size_t vertexBufferSize, size_t indexBufferSize, size_t materialBufferSize);

    ~GPUBuffers();

    bool uploadtoBuffer(vk::BufferHandle buffer, uint32_t byteOffset, const void* data, size_t dataSize);

    bool uploadMesh(uint32_t vertexOffset, uint32_t indexOffset, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    bool uploadMaterial(uint32_t byteOffset, const MaterialData& data);

    uint32_t uploadTexture(const vk::TextureDesc& desc, const std::vector<uint8_t>& data);

    void deleteTexture(uint32_t bindlessIndex);

    // Buffer accessors
    vk::BufferHandle GetVertexBuffer() const;

    vk::BufferHandle GetIndexBuffer() const;

    vk::BufferHandle GetMaterialBuffer() const;

    vk::BufferDesc GetVertexBufferDesc() const;

    vk::BufferDesc GetIndexBufferDesc() const;

    vk::BufferDesc GetMaterialBufferDesc() const;

    size_t GetOwnedTextureCount() const;

  private:
    void CreateFixedBuffers();
};
