#include "ThumbnailRenderer.h"
#include <hina_vk.h>
#include <cmath>
#include <cstring>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace AssetCompiler
{
    // Simple unlit shader for material preview using HSL syntax
    static const char* kPreviewShaderHSL = R"(
#hina
group Uniforms = 0;

bindings(Uniforms, start=0) {
    uniform(std140) PreviewUBO {
        mat4 viewProj;
        vec4 baseColor;
    } ubo;
}

struct VertexIn {
    vec3 a_position;
    vec3 a_normal;
};

struct Varyings {
    vec3 normal;
};

struct FragOut {
    vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;
    gl_Position = ubo.viewProj * vec4(in.a_position, 1.0);
    out.normal = in.a_normal;
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // Simple directional lighting
    vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
    vec3 normal = normalize(in.normal);
    float NdotL = max(dot(normal, lightDir), 0.0);

    // Ambient + diffuse
    vec3 ambient = ubo.baseColor.rgb * 0.3;
    vec3 diffuse = ubo.baseColor.rgb * NdotL * 0.7;

    out.color = vec4(ambient + diffuse, 1.0);
    return out;
}
#hina_end
)";

    // UBO structure matching shader
    struct PreviewUBO {
        float viewProj[16];
        float baseColor[4];
    };

    ThumbnailRenderer::~ThumbnailRenderer()
    {
        if (m_initialized) {
            Shutdown();
        }
    }

    bool ThumbnailRenderer::Initialize()
    {
        if (m_initialized) {
            return true;
        }

        // Initialize hina_vk in headless mode
        hina_desc desc = hina_desc_default();
        desc.native_window = nullptr;  // Headless mode
        desc.native_display = nullptr;

        if (!hina_init(&desc)) {
            std::cerr << "[ThumbnailRenderer] Failed to initialize hina_vk in headless mode\n";
            return false;
        }

        // Create render target
        if (!CreateRenderTarget()) {
            std::cerr << "[ThumbnailRenderer] Failed to create render target\n";
            hina_shutdown();
            return false;
        }

        // Create shader and pipeline
        if (!CreateShaderAndPipeline()) {
            std::cerr << "[ThumbnailRenderer] Failed to create shader/pipeline\n";
            DestroyResources();
            hina_shutdown();
            return false;
        }

        // Create sphere mesh
        if (!CreateSphereMesh()) {
            std::cerr << "[ThumbnailRenderer] Failed to create sphere mesh\n";
            DestroyResources();
            hina_shutdown();
            return false;
        }

        m_initialized = true;
        std::cout << "[ThumbnailRenderer] Initialized successfully (headless mode)\n";
        return true;
    }

    void ThumbnailRenderer::Shutdown()
    {
        if (!m_initialized) {
            return;
        }

        DestroyResources();
        hina_shutdown();
        m_initialized = false;
        std::cout << "[ThumbnailRenderer] Shutdown\n";
    }

    bool ThumbnailRenderer::CreateRenderTarget()
    {
        // Create color render target (RGBA8)
        hina_texture_desc colorDesc = hina_texture_desc_default();
        colorDesc.width = THUMBNAIL_SIZE;
        colorDesc.height = THUMBNAIL_SIZE;
        colorDesc.depth = 1;
        colorDesc.mip_levels = 1;
        colorDesc.layers = 1;
        colorDesc.format = HINA_FORMAT_R8G8B8A8_UNORM;
        colorDesc.type = HINA_TEX_TYPE_2D;
        colorDesc.samples = HINA_SAMPLE_COUNT_1_BIT;
        colorDesc.usage = (hina_texture_usage_flags)(HINA_TEXTURE_RENDER_TARGET_BIT | HINA_TEXTURE_TRANSFER_SRC_BIT);
        colorDesc.label = "thumbnail_color";

        m_colorTarget = hina_make_texture(&colorDesc);
        if (!hina_texture_is_valid(m_colorTarget)) {
            std::cerr << "[ThumbnailRenderer] Failed to create color target\n";
            return false;
        }

        m_colorTargetView = hina_texture_get_default_view(m_colorTarget);

        // Create depth render target
        hina_texture_desc depthDesc = hina_texture_desc_default();
        depthDesc.width = THUMBNAIL_SIZE;
        depthDesc.height = THUMBNAIL_SIZE;
        depthDesc.depth = 1;
        depthDesc.mip_levels = 1;
        depthDesc.layers = 1;
        depthDesc.format = HINA_FORMAT_D32_SFLOAT;
        depthDesc.type = HINA_TEX_TYPE_2D;
        depthDesc.samples = HINA_SAMPLE_COUNT_1_BIT;
        depthDesc.usage = HINA_TEXTURE_RENDER_TARGET_BIT;
        depthDesc.label = "thumbnail_depth";

        m_depthTarget = hina_make_texture(&depthDesc);
        if (!hina_texture_is_valid(m_depthTarget)) {
            std::cerr << "[ThumbnailRenderer] Failed to create depth target\n";
            return false;
        }

        m_depthTargetView = hina_texture_get_default_view(m_depthTarget);

        return true;
    }

    bool ThumbnailRenderer::CreateShaderAndPipeline()
    {
        // Compile HSL shader
        char* error = nullptr;
        hina_hsl_module* module = hslc_compile_hsl_source(kPreviewShaderHSL, "thumbnail_preview", &error);
        if (!module) {
            std::cerr << "[ThumbnailRenderer] Shader compilation failed: " << (error ? error : "unknown") << "\n";
            if (error) hslc_free_log(error);
            return false;
        }

        // Create bind group layout for UBO
        hina_bind_group_layout_entry layoutEntry = {};
        layoutEntry.binding = 0;
        layoutEntry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
        layoutEntry.count = 1;
        layoutEntry.stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
        layoutEntry.flags = HINA_BINDING_FLAG_NONE;

        hina_bind_group_layout_desc layoutDesc = {};
        layoutDesc.entries = &layoutEntry;
        layoutDesc.entry_count = 1;
        layoutDesc.label = "thumbnail_layout";

        m_bindGroupLayout = hina_create_bind_group_layout(&layoutDesc);
        if (!hina_bind_group_layout_is_valid(m_bindGroupLayout)) {
            std::cerr << "[ThumbnailRenderer] Failed to create bind group layout\n";
            hslc_hsl_module_free(module);
            return false;
        }

        // Vertex layout: position (vec3) + normal (vec3)
        hina_vertex_layout vertexLayout = {};
        vertexLayout.buffer_count = 1;
        vertexLayout.buffer_strides[0] = sizeof(float) * 6;  // pos + normal
        vertexLayout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
        vertexLayout.attr_count = 2;
        vertexLayout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };  // position
        vertexLayout.attrs[1] = { HINA_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3, 1, 0 };  // normal

        // Pipeline descriptor
        hina_hsl_pipeline_desc pipDesc = hina_hsl_pipeline_desc_default();
        pipDesc.layout = vertexLayout;
        pipDesc.cull_mode = HINA_CULL_MODE_BACK;
        pipDesc.depth.depth_test = true;
        pipDesc.depth.depth_write = true;
        pipDesc.depth.depth_compare = HINA_COMPARE_OP_LESS;
        pipDesc.depth_format = HINA_FORMAT_D32_SFLOAT;
        pipDesc.color_formats[0] = HINA_FORMAT_R8G8B8A8_UNORM;
        pipDesc.samples = HINA_SAMPLE_COUNT_1_BIT;
        pipDesc.bind_group_layouts[0] = m_bindGroupLayout;

        m_pipeline = hina_make_pipeline_from_module(module, &pipDesc, nullptr);
        hslc_hsl_module_free(module);

        if (!hina_pipeline_is_valid(m_pipeline)) {
            std::cerr << "[ThumbnailRenderer] Failed to create pipeline\n";
            return false;
        }

        // Create uniform buffer
        hina_buffer_desc uboDesc = {};
        uboDesc.size = sizeof(PreviewUBO);
        uboDesc.memory = HINA_BUFFER_CPU;
        uboDesc.usage = HINA_BUFFER_UNIFORM;
        uboDesc.label = "thumbnail_ubo";

        m_uniformBuffer = hina_make_buffer(&uboDesc);
        if (!hina_buffer_is_valid(m_uniformBuffer)) {
            std::cerr << "[ThumbnailRenderer] Failed to create uniform buffer\n";
            return false;
        }

        m_uniformMapped = hina_mapped_buffer_ptr(m_uniformBuffer);

        return true;
    }

    bool ThumbnailRenderer::CreateSphereMesh()
    {
        // Generate UV sphere
        const int stacks = 16;
        const int slices = 16;
        const float radius = 0.8f;

        std::vector<float> vertices;  // pos (3) + normal (3)
        std::vector<uint32_t> indices;

        // Generate vertices
        for (int i = 0; i <= stacks; ++i) {
            float phi = static_cast<float>(M_PI) * i / stacks;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * static_cast<float>(M_PI) * j / slices;
                float sinTheta = std::sin(theta);
                float cosTheta = std::cos(theta);

                float x = sinPhi * cosTheta;
                float y = cosPhi;
                float z = sinPhi * sinTheta;

                // Position
                vertices.push_back(x * radius);
                vertices.push_back(y * radius);
                vertices.push_back(z * radius);

                // Normal (same as position for unit sphere, normalized)
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
            }
        }

        // Generate indices
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                uint32_t first = i * (slices + 1) + j;
                uint32_t second = first + slices + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        m_sphereIndexCount = static_cast<uint32_t>(indices.size());

        // Create vertex buffer
        hina_buffer_desc vbDesc = {};
        vbDesc.size = vertices.size() * sizeof(float);
        vbDesc.memory = HINA_BUFFER_GPU;
        vbDesc.usage = HINA_BUFFER_VERTEX;
        vbDesc.initial_data = vertices.data();
        vbDesc.label = "sphere_vb";

        m_sphereVertexBuffer = hina_make_buffer(&vbDesc);
        if (!hina_buffer_is_valid(m_sphereVertexBuffer)) {
            std::cerr << "[ThumbnailRenderer] Failed to create sphere vertex buffer\n";
            return false;
        }

        // Create index buffer
        hina_buffer_desc ibDesc = {};
        ibDesc.size = indices.size() * sizeof(uint32_t);
        ibDesc.memory = HINA_BUFFER_GPU;
        ibDesc.usage = HINA_BUFFER_INDEX;
        ibDesc.initial_data = indices.data();
        ibDesc.label = "sphere_ib";

        m_sphereIndexBuffer = hina_make_buffer(&ibDesc);
        if (!hina_buffer_is_valid(m_sphereIndexBuffer)) {
            std::cerr << "[ThumbnailRenderer] Failed to create sphere index buffer\n";
            return false;
        }

        // Flush uploads and wait
        hina_ticket ticket = hina_flush_uploads();
        if (ticket != 0) {
            hina_wait_ticket(ticket);
        }

        return true;
    }

    void ThumbnailRenderer::DestroyResources()
    {
        if (hina_buffer_is_valid(m_uniformBuffer)) {
            hina_destroy_buffer(m_uniformBuffer);
            m_uniformBuffer = {};
        }
        if (hina_buffer_is_valid(m_sphereVertexBuffer)) {
            hina_destroy_buffer(m_sphereVertexBuffer);
            m_sphereVertexBuffer = {};
        }
        if (hina_buffer_is_valid(m_sphereIndexBuffer)) {
            hina_destroy_buffer(m_sphereIndexBuffer);
            m_sphereIndexBuffer = {};
        }
        if (hina_pipeline_is_valid(m_pipeline)) {
            hina_destroy_pipeline(m_pipeline);
            m_pipeline = {};
        }
        if (hina_bind_group_layout_is_valid(m_bindGroupLayout)) {
            hina_destroy_bind_group_layout(m_bindGroupLayout);
            m_bindGroupLayout = {};
        }
        if (hina_texture_view_is_valid(m_colorTargetView)) {
            hina_destroy_texture_view(m_colorTargetView);
            m_colorTargetView = {};
        }
        if (hina_texture_is_valid(m_colorTarget)) {
            hina_destroy_texture(m_colorTarget);
            m_colorTarget = {};
        }
        if (hina_texture_view_is_valid(m_depthTargetView)) {
            hina_destroy_texture_view(m_depthTargetView);
            m_depthTargetView = {};
        }
        if (hina_texture_is_valid(m_depthTarget)) {
            hina_destroy_texture(m_depthTarget);
            m_depthTarget = {};
        }
    }

    // Simple orthographic projection matrix
    static void MakeOrthoMatrix(float* out, float left, float right, float bottom, float top, float near_, float far_)
    {
        std::memset(out, 0, sizeof(float) * 16);
        out[0] = 2.0f / (right - left);
        out[5] = 2.0f / (top - bottom);
        out[10] = -2.0f / (far_ - near_);
        out[12] = -(right + left) / (right - left);
        out[13] = -(top + bottom) / (top - bottom);
        out[14] = -(far_ + near_) / (far_ - near_);
        out[15] = 1.0f;
    }

    bool ThumbnailRenderer::RenderMaterialPreview(const float baseColor[4], std::vector<uint8_t>& outPixels)
    {
        if (!m_initialized) {
            std::cerr << "[ThumbnailRenderer] Not initialized\n";
            return false;
        }

        // Update uniform buffer
        PreviewUBO* ubo = static_cast<PreviewUBO*>(m_uniformMapped);
        MakeOrthoMatrix(ubo->viewProj, -1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);
        ubo->baseColor[0] = baseColor[0];
        ubo->baseColor[1] = baseColor[1];
        ubo->baseColor[2] = baseColor[2];
        ubo->baseColor[3] = baseColor[3];

        // Create bind group
        hina_bind_group_entry entry = {};
        entry.binding = 0;
        entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
        entry.buffer.buffer = m_uniformBuffer;
        entry.buffer.offset = 0;
        entry.buffer.size = sizeof(PreviewUBO);

        hina_bind_group_desc bgDesc = {};
        bgDesc.layout = m_bindGroupLayout;
        bgDesc.entries = &entry;
        bgDesc.entry_count = 1;
        bgDesc.label = "preview_bind_group";

        hina_bind_group bindGroup = hina_create_bind_group(&bgDesc);
        if (!hina_bind_group_is_valid(bindGroup)) {
            std::cerr << "[ThumbnailRenderer] Failed to create bind group\n";
            return false;
        }

        // Begin command recording
        hina_cmd* cmd = hina_cmd_begin();

        // Setup render pass
        hina_pass_action passAction = {};
        passAction.colors[0].image = m_colorTargetView;
        passAction.colors[0].load_op = HINA_LOAD_OP_CLEAR;
        passAction.colors[0].store_op = HINA_STORE_OP_STORE;
        passAction.colors[0].clear_color[0] = 0.2f;  // Dark gray background
        passAction.colors[0].clear_color[1] = 0.2f;
        passAction.colors[0].clear_color[2] = 0.2f;
        passAction.colors[0].clear_color[3] = 1.0f;
        passAction.depth.image = m_depthTargetView;
        passAction.depth.load_op = HINA_LOAD_OP_CLEAR;
        passAction.depth.store_op = HINA_STORE_OP_DONT_CARE;
        passAction.depth.depth_clear = 1.0f;
        passAction.width = THUMBNAIL_SIZE;
        passAction.height = THUMBNAIL_SIZE;

        hina_cmd_begin_pass(cmd, &passAction);

        // Set viewport and scissor
        hina_viewport viewport = { 0.0f, 0.0f, (float)THUMBNAIL_SIZE, (float)THUMBNAIL_SIZE, 0.0f, 1.0f };
        hina_scissor scissor = { 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        hina_cmd_set_viewport(cmd, &viewport);
        hina_cmd_set_scissor(cmd, &scissor);

        // Bind pipeline
        hina_cmd_bind_pipeline(cmd, m_pipeline);

        // Bind resources
        hina_cmd_bind_group(cmd, 0, bindGroup);

        // Bind vertex/index buffers
        hina_vertex_input vertexInput = {};
        vertexInput.vertex_buffers[0] = m_sphereVertexBuffer;
        vertexInput.vertex_offsets[0] = 0;
        vertexInput.index_buffer = m_sphereIndexBuffer;
        vertexInput.index_type = HINA_INDEX_UINT32;

        hina_cmd_apply_vertex_input(cmd, &vertexInput);

        // Draw sphere
        hina_cmd_draw_indexed(cmd, m_sphereIndexCount, 1, 0, 0, 0);

        hina_cmd_end_pass(cmd);

        // Submit and wait
        hina_ticket ticket = hina_submit_immediate(cmd);
        hina_wait_ticket(ticket);

        // Download pixels
        size_t pixelSize = hina_texture_mip_size(m_colorTarget, 0);
        outPixels.resize(pixelSize);
        hina_download_texture(m_colorTarget, 0, 0, outPixels.data(), pixelSize);

        // Cleanup bind group
        hina_destroy_bind_group(bindGroup);

        return true;
    }

    bool ThumbnailRenderer::RenderMeshPreview(const std::vector<float>& positions,
                                               const std::vector<uint32_t>& indices,
                                               std::vector<uint8_t>& outPixels)
    {
        if (!m_initialized) {
            std::cerr << "[ThumbnailRenderer] Not initialized\n";
            return false;
        }

        if (positions.empty() || indices.empty()) {
            std::cerr << "[ThumbnailRenderer] Empty mesh data\n";
            return false;
        }

        // Calculate mesh bounds for auto-framing
        float minX = positions[0], maxX = positions[0];
        float minY = positions[1], maxY = positions[1];
        float minZ = positions[2], maxZ = positions[2];

        for (size_t i = 0; i < positions.size(); i += 3) {
            minX = std::min(minX, positions[i]);
            maxX = std::max(maxX, positions[i]);
            minY = std::min(minY, positions[i + 1]);
            maxY = std::max(maxY, positions[i + 1]);
            minZ = std::min(minZ, positions[i + 2]);
            maxZ = std::max(maxZ, positions[i + 2]);
        }

        float centerX = (minX + maxX) * 0.5f;
        float centerY = (minY + maxY) * 0.5f;
        float centerZ = (minZ + maxZ) * 0.5f;
        float extentX = (maxX - minX) * 0.5f;
        float extentY = (maxY - minY) * 0.5f;
        float extentZ = (maxZ - minZ) * 0.5f;
        float maxExtent = std::max({ extentX, extentY, extentZ });

        // Generate vertices with normals (simple flat normal per vertex)
        std::vector<float> vertices;
        for (size_t i = 0; i < positions.size(); i += 3) {
            // Position (centered and scaled)
            vertices.push_back((positions[i] - centerX) / maxExtent * 0.7f);
            vertices.push_back((positions[i + 1] - centerY) / maxExtent * 0.7f);
            vertices.push_back((positions[i + 2] - centerZ) / maxExtent * 0.7f);
            // Normal (pointing outward from center)
            float nx = positions[i] - centerX;
            float ny = positions[i + 1] - centerY;
            float nz = positions[i + 2] - centerZ;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            if (len > 0.0001f) {
                vertices.push_back(nx / len);
                vertices.push_back(ny / len);
                vertices.push_back(nz / len);
            } else {
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
        }

        // Create temporary vertex buffer
        hina_buffer_desc vbDesc = {};
        vbDesc.size = vertices.size() * sizeof(float);
        vbDesc.memory = HINA_BUFFER_GPU;
        vbDesc.usage = HINA_BUFFER_VERTEX;
        vbDesc.initial_data = vertices.data();
        vbDesc.label = "mesh_preview_vb";

        hina_buffer vb = hina_make_buffer(&vbDesc);
        if (!hina_buffer_is_valid(vb)) {
            std::cerr << "[ThumbnailRenderer] Failed to create mesh vertex buffer\n";
            return false;
        }

        // Create temporary index buffer
        hina_buffer_desc ibDesc = {};
        ibDesc.size = indices.size() * sizeof(uint32_t);
        ibDesc.memory = HINA_BUFFER_GPU;
        ibDesc.usage = HINA_BUFFER_INDEX;
        ibDesc.initial_data = indices.data();
        ibDesc.label = "mesh_preview_ib";

        hina_buffer ib = hina_make_buffer(&ibDesc);
        if (!hina_buffer_is_valid(ib)) {
            hina_destroy_buffer(vb);
            std::cerr << "[ThumbnailRenderer] Failed to create mesh index buffer\n";
            return false;
        }

        // Flush uploads
        hina_ticket uploadTicket = hina_flush_uploads();
        if (uploadTicket != 0) {
            hina_wait_ticket(uploadTicket);
        }

        // Update uniform buffer (gray color for mesh preview)
        PreviewUBO* ubo = static_cast<PreviewUBO*>(m_uniformMapped);
        MakeOrthoMatrix(ubo->viewProj, -1.0f, 1.0f, -1.0f, 1.0f, -10.0f, 10.0f);
        ubo->baseColor[0] = 0.7f;
        ubo->baseColor[1] = 0.7f;
        ubo->baseColor[2] = 0.8f;
        ubo->baseColor[3] = 1.0f;

        // Create bind group
        hina_bind_group_entry entry = {};
        entry.binding = 0;
        entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
        entry.buffer.buffer = m_uniformBuffer;
        entry.buffer.offset = 0;
        entry.buffer.size = sizeof(PreviewUBO);

        hina_bind_group_desc bgDesc = {};
        bgDesc.layout = m_bindGroupLayout;
        bgDesc.entries = &entry;
        bgDesc.entry_count = 1;
        bgDesc.label = "mesh_preview_bind_group";

        hina_bind_group bindGroup = hina_create_bind_group(&bgDesc);
        if (!hina_bind_group_is_valid(bindGroup)) {
            hina_destroy_buffer(vb);
            hina_destroy_buffer(ib);
            std::cerr << "[ThumbnailRenderer] Failed to create bind group\n";
            return false;
        }

        // Begin command recording
        hina_cmd* cmd = hina_cmd_begin();

        // Setup render pass
        hina_pass_action passAction = {};
        passAction.colors[0].image = m_colorTargetView;
        passAction.colors[0].load_op = HINA_LOAD_OP_CLEAR;
        passAction.colors[0].store_op = HINA_STORE_OP_STORE;
        passAction.colors[0].clear_color[0] = 0.15f;
        passAction.colors[0].clear_color[1] = 0.15f;
        passAction.colors[0].clear_color[2] = 0.15f;
        passAction.colors[0].clear_color[3] = 1.0f;
        passAction.depth.image = m_depthTargetView;
        passAction.depth.load_op = HINA_LOAD_OP_CLEAR;
        passAction.depth.store_op = HINA_STORE_OP_DONT_CARE;
        passAction.depth.depth_clear = 1.0f;
        passAction.width = THUMBNAIL_SIZE;
        passAction.height = THUMBNAIL_SIZE;

        hina_cmd_begin_pass(cmd, &passAction);

        hina_viewport viewport = { 0.0f, 0.0f, (float)THUMBNAIL_SIZE, (float)THUMBNAIL_SIZE, 0.0f, 1.0f };
        hina_scissor scissor = { 0, 0, THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        hina_cmd_set_viewport(cmd, &viewport);
        hina_cmd_set_scissor(cmd, &scissor);

        hina_cmd_bind_pipeline(cmd, m_pipeline);
        hina_cmd_bind_group(cmd, 0, bindGroup);

        hina_vertex_input vertexInput = {};
        vertexInput.vertex_buffers[0] = vb;
        vertexInput.vertex_offsets[0] = 0;
        vertexInput.index_buffer = ib;
        vertexInput.index_type = HINA_INDEX_UINT32;

        hina_cmd_apply_vertex_input(cmd, &vertexInput);
        hina_cmd_draw_indexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

        hina_cmd_end_pass(cmd);

        // Submit and wait
        hina_ticket ticket = hina_submit_immediate(cmd);
        hina_wait_ticket(ticket);

        // Download pixels
        size_t pixelSize = hina_texture_mip_size(m_colorTarget, 0);
        outPixels.resize(pixelSize);
        hina_download_texture(m_colorTarget, 0, 0, outPixels.data(), pixelSize);

        // Cleanup
        hina_destroy_bind_group(bindGroup);
        hina_destroy_buffer(vb);
        hina_destroy_buffer(ib);

        return true;
    }
}
