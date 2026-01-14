#include "renderer_resources.h"

#include "resource/resource_types.h"

GPUBuffers::GPUBuffers(size_t vertexBufferSize,
                       size_t indexBufferSize,
                       size_t materialBufferSize,
                       size_t meshDecompressionBufferSize,
                       size_t skinningBufferSize,
                       size_t morphDeltaBufferSize,
                       size_t morphVertexBaseBufferSize,
                       size_t morphVertexCountBufferSize)
{
  m_ownedTextures.reserve(ResourceLimits::MAX_TEXTURES);

  // Set up buffer descriptors
  m_vertexBufferDesc = {
    .size = vertexBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Vertex | gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_indexBufferDesc = {
    .size = indexBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Index | gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_materialBufferDesc = {
    .size = materialBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_meshDecompressionBufferDesc = {
    .size = meshDecompressionBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_skinningBufferDesc = {
    .size = skinningBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_morphDeltaBufferDesc = {
    .size = morphDeltaBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_morphVertexBaseBufferDesc = {
    .size = morphVertexBaseBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };
  m_morphVertexCountBufferDesc = {
    .size = morphVertexCountBufferSize,
    .flags = static_cast<hina_buffer_flags>(gfx::BufferUsage::Storage | gfx::BufferUsage::DeviceLocal | gfx::BufferUsage::TransferDst)
  };

  CreateFixedBuffers();

  if(!gfx::isValid(m_vertexBuffer.get()) || !gfx::isValid(m_indexBuffer.get()) ||
     !gfx::isValid(m_materialBuffer.get()) || !gfx::isValid(m_meshDecompressionBuffer.get()) ||
     !gfx::isValid(m_skinningBuffer.get()) || !gfx::isValid(m_morphDeltaBuffer.get()) ||
     !gfx::isValid(m_morphVertexBaseBuffer.get()) || !gfx::isValid(m_morphVertexCountBuffer.get()))
  {
    throw std::runtime_error("Failed to create GPU buffers");
  }
}

GPUBuffers::~GPUBuffers() = default;

bool GPUBuffers::uploadtoBuffer(gfx::Buffer buffer, uint32_t byteOffset, const void* data, size_t dataSize)
{
  if (!data || !gfx::isValid(buffer))
  {
    LOG_ERROR("Invalid upload");
    return false;
  }
  LOG_INFO("Uploading {} bytes to buffer at offset {}", dataSize, byteOffset);
  hina_upload_buffer(buffer, data, dataSize, byteOffset);
  return true;
}

bool GPUBuffers::uploadMesh(uint32_t vertexOffset, uint32_t indexOffset, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
  bool success = true;

  if (!vertices.empty())
  {
    hina_upload_buffer(m_vertexBuffer.get(), vertices.data(), vertices.size() * sizeof(Vertex), vertexOffset);
  }

  if (!indices.empty() && success)
  {
    hina_upload_buffer(m_indexBuffer.get(), indices.data(), indices.size() * sizeof(uint32_t), indexOffset);
  }

  return success;
}

bool GPUBuffers::uploadMaterial(uint32_t byteOffset, const MaterialData& data)
{
  hina_upload_buffer(m_materialBuffer.get(), &data, sizeof(MaterialData), byteOffset);
  return true;
}

bool GPUBuffers::uploadMeshDecompression(uint32_t byteOffset, const MeshDecompressionData& data)
{
  hina_upload_buffer(m_meshDecompressionBuffer.get(), &data, sizeof(MeshDecompressionData), byteOffset);
  return true;
}

uint32_t GPUBuffers::uploadTexture(const gfx::TextureDesc& desc, const std::vector<uint8_t>& data)
{
  gfx::TextureDesc textureDesc = desc;
  textureDesc.initial_data = data.data();

  gfx::Texture texture = gfx::createTexture(textureDesc);

  if (!gfx::isValid(texture))
  {
    LOG_ERROR("Failed to create texture");
    return 0; // Invalid texture ID
  }

  uint32_t textureId = gfx_get_texture_id(texture);

  // Store texture for lifetime management
  m_ownedTextures.emplace_back(texture);

  LOG_DEBUG("Texture uploaded with ID {}", textureId);
  return textureId;
}

void GPUBuffers::deleteTexture(uint32_t textureId)
{
  // Find texture by ID and remove
  for (auto it = m_ownedTextures.begin(); it != m_ownedTextures.end(); ++it)
  {
    if (gfx_get_texture_id(it->get()) == textureId)
    {
      LOG_DEBUG("Destroying texture with ID {}", textureId);
      m_ownedTextures.erase(it);
      break;
    }
  }
}

gfx::Buffer GPUBuffers::GetVertexBuffer() const { return m_vertexBuffer.get(); }

gfx::Buffer GPUBuffers::GetIndexBuffer() const { return m_indexBuffer.get(); }

gfx::Buffer GPUBuffers::GetMaterialBuffer() const { return m_materialBuffer.get(); }

gfx::Buffer GPUBuffers::GetMeshDecompressionBuffer() const { return m_meshDecompressionBuffer.get(); }

gfx::Buffer GPUBuffers::GetSkinningBuffer() const { return m_skinningBuffer.get(); }

gfx::Buffer GPUBuffers::GetMorphDeltaBuffer() const { return m_morphDeltaBuffer.get(); }

gfx::Buffer GPUBuffers::GetMorphVertexBaseBuffer() const { return m_morphVertexBaseBuffer.get(); }

gfx::Buffer GPUBuffers::GetMorphVertexCountBuffer() const { return m_morphVertexCountBuffer.get(); }

gfx::BufferDesc GPUBuffers::GetVertexBufferDesc() const { return m_vertexBufferDesc; }

gfx::BufferDesc GPUBuffers::GetIndexBufferDesc() const { return m_indexBufferDesc; }

gfx::BufferDesc GPUBuffers::GetMaterialBufferDesc() const { return m_materialBufferDesc; }

gfx::BufferDesc GPUBuffers::GetMeshDecompressionBufferDesc() const { return m_meshDecompressionBufferDesc; }

gfx::BufferDesc GPUBuffers::GetSkinningBufferDesc() const { return m_skinningBufferDesc; }

gfx::BufferDesc GPUBuffers::GetMorphDeltaBufferDesc() const { return m_morphDeltaBufferDesc; }

gfx::BufferDesc GPUBuffers::GetMorphVertexBaseBufferDesc() const { return m_morphVertexBaseBufferDesc; }

gfx::BufferDesc GPUBuffers::GetMorphVertexCountBufferDesc() const { return m_morphVertexCountBufferDesc; }

size_t GPUBuffers::GetOwnedTextureCount() const { return m_ownedTextures.size(); }

void GPUBuffers::CreateFixedBuffers()
{
  m_vertexBuffer.reset(gfx::createBuffer(m_vertexBufferDesc));
  m_indexBuffer.reset(gfx::createBuffer(m_indexBufferDesc));
  m_materialBuffer.reset(gfx::createBuffer(m_materialBufferDesc));
  m_meshDecompressionBuffer.reset(gfx::createBuffer(m_meshDecompressionBufferDesc));
  m_skinningBuffer.reset(gfx::createBuffer(m_skinningBufferDesc));
  m_morphDeltaBuffer.reset(gfx::createBuffer(m_morphDeltaBufferDesc));
  m_morphVertexBaseBuffer.reset(gfx::createBuffer(m_morphVertexBaseBufferDesc));
  m_morphVertexCountBuffer.reset(gfx::createBuffer(m_morphVertexCountBufferDesc));
}
