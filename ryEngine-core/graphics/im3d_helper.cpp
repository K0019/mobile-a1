#include "im3d_helper.h"
#include "renderer.h"
#include <glm/gtc/type_ptr.hpp>
#include <im3d.h>

namespace Im3dHelper
{
    static inline Im3d::Vec2 ToIm3D(const glm::vec2& v) { return { v.x, v.y }; }
    static inline Im3d::Vec3 ToIm3D(const glm::vec3& v) { return { v.x, v.y, v.z }; }

    void BeginFrame(const FrameData& /*frameData*/)
    {
        // We don't rely on AppData; just mark frame scope for user code emitting primitives
        Im3d::NewFrame();
    }

    void EndFrame(uint64_t featureHandle, Renderer& renderer)
    {
        Im3d::EndFrame();

        const Im3d::DrawList* lists = Im3d::GetDrawLists();
        const uint32_t count = Im3d::GetDrawListCount();

        auto* p = static_cast<Im3dRenderFeature::Parameters*>(
            renderer.GetFeatureParameterBlockPtr(featureHandle));
        if (!p) return;

        // First pass: counts
        uint32_t np = 0, nl = 0, nt = 0;
        for (uint32_t i = 0; i < count; ++i) {
            const auto& dl = lists[i];
            if (dl.m_primType == Im3d::DrawPrimitive_Points)         np += dl.m_vertexCount;
            else if (dl.m_primType == Im3d::DrawPrimitive_Lines)     nl += dl.m_vertexCount / 2; // pairs
            else if (dl.m_primType == Im3d::DrawPrimitive_Triangles) nt += dl.m_vertexCount / 3;
        }

        const size_t bytesP = size_t(np) * sizeof(PointGPU);
        const size_t bytesL = size_t(nl) * sizeof(LineGPU);
        const size_t bytesT = size_t(nt) * sizeof(TriGPU);

        p->offPoints = 0;                 p->numPoints = np;
        p->offLines = (uint32_t)bytesP;  p->numLines = nl;
        p->offTris = (uint32_t)(bytesP + bytesL); p->numTris = nt;

        const size_t total = bytesP + bytesL + bytesT;
        p->packed.resize(total);

        // Second pass: fill
        auto* outP = reinterpret_cast<PointGPU*>(p->packed.data() + p->offPoints);
        auto* outL = reinterpret_cast<LineGPU*>(p->packed.data() + p->offLines);
        auto* outT = reinterpret_cast<TriGPU*>(p->packed.data() + p->offTris);

        uint32_t ip = 0, il = 0, it = 0;

        for (uint32_t i = 0; i < count; ++i) {
            const auto& dl = lists[i];
            const auto* v = dl.m_vertexData;
            const uint32_t n = dl.m_vertexCount;

            switch (dl.m_primType) {
            case Im3d::DrawPrimitive_Points:
                for (uint32_t k = 0; k < n; ++k) {
                    const auto& a = v[k];
                    outP[ip++] = {
                        { a.m_positionSize.x, a.m_positionSize.y, a.m_positionSize.z }, a.m_positionSize.w,
                        a.m_color.v, 0,0,0
                    };
                }
                break;

            case Im3d::DrawPrimitive_Lines:
                for (uint32_t k = 0; k + 1 < n; k += 2) {
                    const auto& a = v[k + 0];
                    const auto& b = v[k + 1];
                    outL[il++] = {
                        { a.m_positionSize.x, a.m_positionSize.y, a.m_positionSize.z },
                        0.5f * (a.m_positionSize.w + b.m_positionSize.w), // stored but fixed-function lines ignore per-seg width
                        { b.m_positionSize.x, b.m_positionSize.y, b.m_positionSize.z },
                        a.m_color.v
                    };
                }
                break;

            case Im3d::DrawPrimitive_Triangles:
                for (uint32_t k = 0; k + 2 < n; k += 3) {
                    const auto& a = v[k + 0], b = v[k + 1], c = v[k + 2];
                    outT[it++] = {
                        { a.m_positionSize.x, a.m_positionSize.y, a.m_positionSize.z }, a.m_color.v,
                        { b.m_positionSize.x, b.m_positionSize.y, b.m_positionSize.z }, b.m_color.v,
                        { c.m_positionSize.x, c.m_positionSize.y, c.m_positionSize.z }, c.m_color.v
                    };
                }
                break;

            default: break;
            }
        }
    }

    void DrawBoneHierarchy(const glm::mat4& objectTransform,
        const std::vector<glm::mat4>& boneMatrices,
        const std::vector<uint32_t>& parentIndices,
        const std::vector<glm::mat4>& inverseBindMatrices,
        float boneThickness,
        float jointSize)
    {
        const uint32_t jointCount = (uint32_t)boneMatrices.size();
        if (!jointCount || jointCount != parentIndices.size() || jointCount != inverseBindMatrices.size())
            return;

        std::vector<glm::vec3> globalJointPositions(jointCount);

        for (uint32_t i = 0; i < jointCount; ++i) {
            const glm::mat4 globalJoint = boneMatrices[i] * glm::inverse(inverseBindMatrices[i]);
            const glm::vec4 posWorld = objectTransform * (globalJoint * glm::vec4(0, 0, 0, 1));
            globalJointPositions[i] = glm::vec3(posWorld);
        }

        // Bones (lines)
        Im3d::PushDrawState();
        Im3d::SetSize(boneThickness);

        for (uint32_t j = 0; j < jointCount; ++j) {
            const uint32_t pIdx = parentIndices[j];
            if (pIdx != Resource::INVALID_JOINT_INDEX && pIdx < jointCount) {
                const glm::vec3 parentPos = globalJointPositions[pIdx];
                const glm::vec3 jointPos = globalJointPositions[j];
                const glm::vec3 dir = glm::normalize(jointPos - parentPos);
                const glm::vec3 rgb = glm::abs(dir);

                const Im3d::Color col(rgb.x, rgb.y, rgb.z, 1.0f);
                Im3d::DrawLine(ToIm3D(parentPos), ToIm3D(jointPos), boneThickness, col);
            }
        }
        Im3d::PopDrawState();

        // Joints (points)
        Im3d::PushDrawState();
        Im3d::SetSize(jointSize);
        const Im3d::Color jointCol = Im3d::Color_Yellow;

        for (uint32_t j = 0; j < jointCount; ++j) {
            Im3d::DrawPoint(ToIm3D(globalJointPositions[j]), jointSize, jointCol);
        }
        Im3d::PopDrawState();
    }

    void DrawSceneBones(const Resource::Scene& scene,
        const Resource::ResourceManager& resources,
        float boneThickness,
        float jointSize)
    {
        const size_t objectCount = scene.objects.size();

        for (size_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
        {
            const SceneObject& object = scene.objects[objectIndex];

            if (!object.mesh.isValid())
                continue;

            const SceneObject::AnimBinding* anim = object.anim.has_value() ? &object.anim.value() : nullptr;
            if (!anim || anim->jointCount == 0)
                continue;

            const auto* meshCold = resources.getMeshMetadata(object.mesh);
            if (!meshCold || meshCold->jointParentIndices.empty())
                continue;

            const uint32_t jointCount = anim->jointCount;
            if (anim->skinMatrices.size() < jointCount)
                continue;

            std::vector<glm::mat4> boneMatrices(jointCount);
            for (uint32_t i = 0; i < jointCount; ++i) {
                boneMatrices[i] = anim->skinMatrices[i];
            }

            DrawBoneHierarchy(
                object.transform,
                boneMatrices,
                meshCold->jointParentIndices,
                meshCold->jointInverseBindMatrices,
                boneThickness,
                jointSize
            );
        }
    }
} // namespace Im3dHelper
