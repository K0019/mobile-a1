// OIT.cpp - Order Independent Transparency
//
// TODO: Implement WBOIT (Weighted Blended OIT) per migration plan
//
// The previous per-pixel linked list implementation has been removed because it requires:
// - Buffer device addresses (Vulkan 1.2+)
// - Bindless texture arrays
// - 64-bit atomics
// None of these are available on mobile/hina-vk.
//
// WBOIT implementation needs:
// 1. Accumulation pass (transparent geometry):
//    - RT0 (RGBA16F): out_accum = vec4(color.rgb * color.a, color.a) * weight
//    - RT1 (R8): out_reveal = color.a
//    - Blend modes: RT0 = ONE/ONE, RT1 = ZERO/ONE_MINUS_SRC_ALPHA
//
// 2. Resolve pass (fullscreen quad over opaque):
//    - vec4 accum = texture(accumTex, uv);
//    - float reveal = texture(revealTex, uv).r;
//    - out_color = vec4(accum.rgb / max(accum.a, 1e-5), reveal);
//    - Blend mode: ONE_MINUS_SRC_ALPHA/SRC_ALPHA
//
// See: HINA_VK_MIGRATION_PLAN.md section "Transparency - Weighted Blended OIT"

#include "OIT.h"
#include "render_graph.h"
#include "hina_context.h"

void OITSystem::Initialize(HinaContext* context)
{
  (void)context;
  // TODO: Create WBOIT pipelines
  m_initialized = false;  // Not functional until WBOIT is implemented
}

void OITSystem::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  (void)passBuilder;
  // TODO: Implement WBOIT passes
  //
  // Will need:
  // 1. DeclareTransientResource for ACCUMULATION (RGBA16F) and REVEAL (R8) textures
  // 2. Transparent geometry pass that writes to both with appropriate blend modes
  // 3. Resolve pass that composites over opaque scene color
  //
  // For now, OIT is disabled. Transparent objects will render without proper sorting.
}
