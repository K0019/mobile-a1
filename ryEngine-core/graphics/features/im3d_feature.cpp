// im3d_feature.cpp
#include "im3d_feature.h"

// Im3d shaders
static const char* kVertexShader = R"(
layout(location = 0) in vec4 in_position_size;  // xyz=pos, w=size
layout(location = 1) in vec4 in_color;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec2 viewport;
} pc;

layout(location = 0) out vec4 out_color;
layout(location = 1) out float out_size;

void main() {
    gl_Position = pc.viewProj * vec4(in_position_size.xyz, 1.0);
    out_color = in_color;
    out_size = in_position_size.w;
}
)";

static const char* kGeometryShader = R"(
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec4 in_color[];
layout(location = 1) in float in_size[];

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec2 viewport;
} pc;

layout(location = 0) out vec4 out_color;

void main() {
    vec2 p0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    
    vec2 dir = normalize(p1 - p0);
    vec2 perp = vec2(-dir.y, dir.x);
    
    float thickness = max(in_size[0], in_size[1]) / pc.viewport.y;
    vec2 offset = perp * thickness;
    
    // Emit quad
    gl_Position = vec4((p0 - offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    out_color = in_color[0];
    EmitVertex();
    
    gl_Position = vec4((p0 + offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    out_color = in_color[0];
    EmitVertex();
    
    gl_Position = vec4((p1 - offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    out_color = in_color[1];
    EmitVertex();
    
    gl_Position = vec4((p1 + offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    out_color = in_color[1];
    EmitVertex();
    
    EndPrimitive();
}
)";

static const char* kFragmentShader = R"(
layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = in_color;
}
)";

void Im3dFeature::SetupPasses(internal::RenderPassBuilder& builder) {
    vk::BufferDesc vertexDesc {
        .usage = vk::BufferUsageBits_Storage,
        .storage = vk::StorageType::HostVisible,
        .size = 256 * 1024  // 256KB for Im3d vertices
    };
    
    PassDeclarationInfo passInfo;
    passInfo.framebufferDebugName = "Im3dDebug";
    passInfo.colorAttachments[0] = {
        .textureName = RenderResources::SWAPCHAIN_IMAGE,
        .loadOp = vk::LoadOp::Load,
        .storeOp = vk::StoreOp::Store
    };
    passInfo.depthAttachment = {
        .textureName = RenderResources::SCENE_DEPTH,
        .loadOp = vk::LoadOp::Load,
        .storeOp = vk::StoreOp::Store
    };
    
    builder.CreatePass()
        .DeclareTransientResource("Im3dVertexBuffer", vertexDesc)
        .UseResource(RenderResources::SWAPCHAIN_IMAGE, AccessType::Write)
        .UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite)
        .UseResource("Im3dVertexBuffer", AccessType::ReadWrite)
        .SetPriority(internal::RenderPassBuilder::PassPriority::UI)
        .AddGraphicsPass("Im3dRender", passInfo, [this](internal::ExecutionContext& ctx) {
            RenderIm3dPass(ctx);
        });
}

void Im3dFeature::EnsurePipelineCreated(internal::ExecutionContext& context) {
    if (m_resourcesCreated) return;
    
    vk::IContext& vkContext = context.GetvkContext();
    
    m_vertShader = vkContext.createShaderModule({
        kVertexShader, 
        vk::ShaderStage::Vert, 
        "Im3d Vertex Shader"
    });
    
    m_geomShader = vkContext.createShaderModule({
        kGeometryShader,
        vk::ShaderStage::Geom,
        "Im3d Geometry Shader"
    });
    
    m_fragShader = vkContext.createShaderModule({
        kFragmentShader,
        vk::ShaderStage::Frag,
        "Im3d Fragment Shader"
    });
    
    m_resourcesCreated = true;
}

void Im3dFeature::RenderIm3dPass(internal::ExecutionContext& ctx) {
    uint32_t drawListCount = Im3d::GetDrawListCount();
    if (drawListCount == 0) return;
    
    EnsurePipelineCreated(ctx);
    
    vk::ICommandBuffer& cmd = ctx.GetvkCommandBuffer();
    vk::IContext& vkContext = ctx.GetvkContext();
    const FrameData& frameData = ctx.GetFrameData();
    
    // Get vertex buffer
    vk::BufferHandle vertexBuffer = ctx.GetBuffer("Im3dVertexBuffer");
    
    const Im3d::DrawList* drawLists = Im3d::GetDrawLists();
    
    for (uint32_t i = 0; i < drawListCount; ++i) {
        const Im3d::DrawList& dl = drawLists[i];
        
        if (dl.m_vertexCount == 0) continue;
        
        // Resize buffer if needed
        size_t requiredSize = dl.m_vertexCount * sizeof(Im3d::VertexData);
        if (requiredSize > ctx.GetBufferSize("Im3dVertexBuffer")) {
            ctx.ResizeBuffer("Im3dVertexBuffer", requiredSize);
            vertexBuffer = ctx.GetBuffer("Im3dVertexBuffer");
        }
        
        // Upload vertices
        void* mappedData = vkContext.getMappedPtr(vertexBuffer);
        std::memcpy(mappedData, dl.m_vertexData, requiredSize);
        vkContext.flushMappedMemory(vertexBuffer, 0, requiredSize);
        
        // Create pipeline on first use
        if (dl.m_primType == Im3d::DrawPrimitive_Lines && m_linesPipeline.empty()) {
            vk::TextureHandle swapchain = ctx.GetTexture(RenderResources::SWAPCHAIN_IMAGE);
            vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
            
            m_linesPipeline = vkContext.createRenderPipeline({
                .smVert = m_vertShader,
                .smGeom = m_geomShader,
                .smFrag = m_fragShader,
                .color = {{
                    .format = vkContext.getFormat(swapchain),
                    .blendEnabled = true,
                    .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha,
                    .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha
                }},
                .depthFormat = vkContext.getFormat(depth),
                .cullMode = vk::CullMode::None,
                .debugName = "Im3d Lines Pipeline"
            }, nullptr);
        }
        
        if (dl.m_primType == Im3d::DrawPrimitive_Triangles && m_trianglesPipeline.empty()) {
            vk::TextureHandle swapchain = ctx.GetTexture(RenderResources::SWAPCHAIN_IMAGE);
            vk::TextureHandle depth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
            
            m_trianglesPipeline = vkContext.createRenderPipeline({
                .smVert = m_vertShader,
                .smFrag = m_fragShader,
                .color = {{
                    .format = vkContext.getFormat(swapchain),
                    .blendEnabled = true,
                    .srcRGBBlendFactor = vk::BlendFactor::SrcAlpha,
                    .dstRGBBlendFactor = vk::BlendFactor::OneMinusSrcAlpha
                }},
                .depthFormat = vkContext.getFormat(depth),
                .cullMode = vk::CullMode::None,
                .debugName = "Im3d Triangles Pipeline"
            }, nullptr);
        }
        
        // Bind appropriate pipeline
        if (dl.m_primType == Im3d::DrawPrimitive_Lines) {
            cmd.cmdBindRenderPipeline(m_linesPipeline);
        } else if (dl.m_primType == Im3d::DrawPrimitive_Triangles) {
            cmd.cmdBindRenderPipeline(m_trianglesPipeline);
        } else {
            continue;  // Skip points for now
        }
        
        // Push constants
        struct PushConstants {
            mat4 viewProj;
            vec2 viewport;
        } pc;
        pc.viewProj = frameData.projMatrix * frameData.viewMatrix;
        pc.viewport = vec2(frameData.width, frameData.height);
        
        cmd.cmdPushConstants(pc);
        cmd.cmdDraw(dl.m_vertexCount, 1, 0, 0);
    }
}