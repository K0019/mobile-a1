#include "renderer_resources.h"

#include "resource/resource_types.h"

GPUBuffers::GPUBuffers(vk::IContext& context,
                       size_t vertexBufferSize,
                       size_t indexBufferSize,
                       size_t materialBufferSize,
                       size_t meshDecompressionBufferSize,
                       size_t skinningBufferSize,
                       size_t morphDeltaBufferSize,
                       size_t morphVertexBaseBufferSize,
                       size_t morphVertexCountBufferSize)
  : m_context(context)
{
  m_ownedTextures.reserve(ResourceLimits::MAX_TEXTURES);

  m_indexBufferDesc.size = indexBufferSize;
  m_vertexBufferDesc.size = vertexBufferSize;
  m_materialBufferDesc.size = materialBufferSize;
  m_meshDecompressionBufferDesc.size = meshDecompressionBufferSize;
  m_skinningBufferDesc.size = skinningBufferSize;
  m_morphDeltaBufferDesc.size = morphDeltaBufferSize;
  m_morphVertexBaseBufferDesc.size = morphVertexBaseBufferSize;
  m_morphVertexCountBufferDesc.size = morphVertexCountBufferSize;
  CreateFixedBuffers();
  if(!m_vertexBuffer.valid()  || !m_indexBuffer.valid() || !m_materialBuffer.valid() || !m_meshDecompressionBuffer.valid() ||
     !m_skinningBuffer.valid() || !m_morphDeltaBuffer.valid() || !m_morphVertexBaseBuffer.valid() || !m_morphVertexCountBuffer.valid())
  {
    throw std::runtime_error("Failed to create GPU buffers");
  }
}

GPUBuffers::~GPUBuffers() = default;

bool GPUBuffers::uploadtoBuffer(vk::BufferHandle buffer, uint32_t byteOffset, const void* data, size_t dataSize)
{
  if (!data || !buffer.valid())
  {
    LOG_ERROR("Invalid upload");
    return false;
  }
  LOG_INFO("Uploading {} bytes to buffer at offset {}", dataSize, byteOffset);
  return m_context.upload(buffer, data, dataSize, byteOffset).isOk();
}

bool GPUBuffers::uploadMesh(uint32_t vertexOffset, uint32_t indexOffset, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
  bool success = true;

  if (!vertices.empty())
  {
    vk::Result result = m_context.upload(m_vertexBuffer, vertices.data(), vertices.size() * sizeof(Vertex), vertexOffset);
    if (!result.isOk())
    {
      LOG_ERROR("Failed to upload vertex data: {}", result.message);
      success = false;
    }
  }

  if (!indices.empty() && success)
  {
    vk::Result result = m_context.upload(m_indexBuffer, indices.data(), indices.size() * sizeof(uint32_t), indexOffset);
    if (!result.isOk())
    {
      LOG_ERROR("Failed to upload index data: {}", result.message);
      success = false;
    }
  }

  return success;
}

bool GPUBuffers::uploadMaterial(uint32_t byteOffset, const MaterialData& data)
{
  vk::Result result = m_context.upload(m_materialBuffer, &data, sizeof(MaterialData), byteOffset);

  if (!result.isOk())
  {
    LOG_ERROR("Failed to upload material data: {}", result.message);
    return false;
  }

  return true;
}

bool GPUBuffers::uploadMeshDecompression(uint32_t byteOffset, const MeshDecompressionData& data)
{
  vk::Result result = m_context.upload(m_meshDecompressionBuffer, &data, sizeof(MeshDecompressionData), byteOffset);

  if (!result.isOk())
  {
    LOG_ERROR("Failed to upload mesh decompression data: {}", result.message);
    return false;
  }

  return true;
}

uint32_t GPUBuffers::uploadTexture(const vk::TextureDesc& desc, const std::vector<uint8_t>& data)
{
  vk::TextureDesc textureDesc = desc;
  textureDesc.data = data.data();

  vk::Holder<vk::TextureHandle> vkTexture = m_context.createTexture(textureDesc);

  if (!vkTexture.valid())
  {
    LOG_ERROR("Failed to create texture");
    return 0; // Invalid bindless index
  }

  uint32_t bindlessIndex = vkTexture.index();

  // Store texture for lifetime management
  m_ownedTextures.emplace_back(std::move(vkTexture));

  LOG_DEBUG("Texture uploaded with bindless index {}", bindlessIndex);
  return bindlessIndex;
}

void GPUBuffers::deleteTexture(uint32_t bindlessIndex)
{
  // Find texture by bindless index and remove
  for (auto it = m_ownedTextures.begin(); it != m_ownedTextures.end(); ++it)
  {
    if (it->index() == bindlessIndex)
    {
      LOG_DEBUG("Destroying texture with bindless index {}", bindlessIndex);
      m_ownedTextures.erase(it);
      break;
    }
  }
}

vk::BufferHandle GPUBuffers::GetVertexBuffer() const { return m_vertexBuffer; }

vk::BufferHandle GPUBuffers::GetIndexBuffer() const { return m_indexBuffer; }

vk::BufferHandle GPUBuffers::GetMaterialBuffer() const { return m_materialBuffer; }

vk::BufferHandle GPUBuffers::GetMeshDecompressionBuffer() const { return m_meshDecompressionBuffer; }

vk::BufferHandle GPUBuffers::GetSkinningBuffer() const { return m_skinningBuffer; }

vk::BufferHandle GPUBuffers::GetMorphDeltaBuffer() const { return m_morphDeltaBuffer; }

vk::BufferHandle GPUBuffers::GetMorphVertexBaseBuffer() const { return m_morphVertexBaseBuffer; }

vk::BufferHandle GPUBuffers::GetMorphVertexCountBuffer() const { return m_morphVertexCountBuffer; }

vk::BufferDesc GPUBuffers::GetVertexBufferDesc() const { return m_vertexBufferDesc; }


vk::BufferDesc GPUBuffers::GetIndexBufferDesc() const { return m_indexBufferDesc; }

vk::BufferDesc GPUBuffers::GetMaterialBufferDesc() const { return m_materialBufferDesc; }

vk::BufferDesc GPUBuffers::GetMeshDecompressionBufferDesc() const { return m_meshDecompressionBufferDesc; }

vk::BufferDesc GPUBuffers::GetSkinningBufferDesc() const { return m_skinningBufferDesc; }

vk::BufferDesc GPUBuffers::GetMorphDeltaBufferDesc() const { return m_morphDeltaBufferDesc; }

vk::BufferDesc GPUBuffers::GetMorphVertexBaseBufferDesc() const { return m_morphVertexBaseBufferDesc; }

vk::BufferDesc GPUBuffers::GetMorphVertexCountBufferDesc() const { return m_morphVertexCountBufferDesc; }

size_t GPUBuffers::GetOwnedTextureCount() const { return m_ownedTextures.size(); }

void GPUBuffers::CreateFixedBuffers()
{
  m_vertexBuffer = m_context.createBuffer(m_vertexBufferDesc, "VertexBuffer");

  m_indexBuffer = m_context.createBuffer(m_indexBufferDesc, "IndexBuffer");

  m_materialBuffer = m_context.createBuffer(m_materialBufferDesc, "MaterialBuffer");

  m_meshDecompressionBuffer = m_context.createBuffer(m_meshDecompressionBufferDesc, "MeshDecompressionBuffer");

  m_skinningBuffer = m_context.createBuffer(m_skinningBufferDesc, "SkinningBuffer");

  m_morphDeltaBuffer = m_context.createBuffer(m_morphDeltaBufferDesc, "MorphDeltaBuffer");

  m_morphVertexBaseBuffer = m_context.createBuffer(m_morphVertexBaseBufferDesc, "MorphVertexBaseBuffer");

  m_morphVertexCountBuffer = m_context.createBuffer(m_morphVertexCountBufferDesc, "MorphVertexCountBuffer");
}
