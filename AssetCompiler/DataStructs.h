#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <random>
#include <vector>
#include <variant>
#include <filesystem>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

struct aiScene;
struct aiMaterial;

namespace compiler
{
    struct LoadingConfig
    {
        // Performance settings
        float maxFrameTimeMs = 2.0f;
        bool enableParallelLoading = true;
        size_t jobSystemThreads = 0; // 0 = auto-detect

        // Mesh settings
        bool optimizeMeshes = true;
        bool autoDetectTangentNeed = true;
        bool generateTangents = false;
        bool flipUVs = true;
        uint32_t maxVerticesPerMesh = 1'000'000;

        // Texture settings
        bool compressTextures = true;
        uint32_t maxTextureResolution = 4096;
        bool generateMipmaps = true;

        // Material settings
        bool deduplicateMaterials = false;
        float deduplicationTolerance = 0.001f;

        // Scene settings
        bool extractLights = true;
        bool extractCameras = true;
        bool calculateBounds = true;

        // Memory limits
        uint32_t maxMeshes = 1000;
        uint32_t maxMaterials = 500;
        uint32_t maxTextures = 500;

        // Factory methods for common configs
        static LoadingConfig createFast()
        {
            LoadingConfig config;
            config.optimizeMeshes = false;
            config.compressTextures = false;
            config.deduplicateMaterials = false;
            config.extractLights = false;
            config.extractCameras = false;
            config.maxFrameTimeMs = 5.0f;
            return config;
        }

        static LoadingConfig createBalanced()
        {
            return LoadingConfig{}; // Use defaults
        }

        static LoadingConfig createQuality()
        {
            LoadingConfig config;
            config.generateTangents = true;
            config.maxFrameTimeMs = 1.0f;
            config.maxTextureResolution = 8192;
            return config;
        }
    };

    struct Vertex
    {
        vec3 position;
        float uv_x;
        vec3 normal;
        float uv_y;
        vec4 tangent; // xyz = tangent direction, w = bitangent handedness
        Vertex() = default;

        Vertex(const vec3& pos, const vec3& n, const vec2& uv) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(0.0f, 0.0f, 0.0f, 1.0f) {}

        Vertex(const vec3& pos, const vec3& n, const vec2& uv, const vec4& tan) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(tan) {}

        // Helper to get UV as vec2
        vec2 getUV() const { return { uv_x, uv_y }; }

        void setUV(const vec2& uv)
        {
            uv_x = uv.x;
            uv_y = uv.y;
        }

        void setUV(float u, float v)
        {
            uv_x = u;
            uv_y = v;
        }
    };

    struct CompilerNode
    {
        std::string name;
        mat4 localTransform;
        int32_t meshIndex = -1;
        int32_t parentIndex = -1;
    };

    /*
    enum class LightType : uint8_t
    {
        Directional = 0,
        Point = 1,
        Spot = 2,
        Area = 3
    };

    struct SceneLight
    {
        LightType type = LightType::Point;
        vec3 position = vec3(0.0f);
        vec3 direction = vec3(0.0f, -1.0f, 0.0f);
        vec3 color = vec3(1.0f);
        vec3 attenuation = vec3(1.0f, 0.09f, 0.032f); // constant, linear, quadratic
        float intensity = 1.0f;
        float innerConeAngle = 0.0f;   // For spot lights (radians)
        float outerConeAngle = 0.785f; // 45 degrees in radians
        vec2 areaSize = vec2(1.0f);    // For area lights
        std::string name;
    };

    struct SceneCamera
    {
        vec3 position = vec3(0.0f);
        vec3 target = vec3(0.0f, 0.0f, -1.0f);
        vec3 up = vec3(0.0f, 1.0f, 0.0f);
        float fov = 60.0f; // Field of view in degrees
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float aspectRatio = 16.0f / 9.0f;
        std::string name;
    };

    enum class SceneObjectType : uint8_t
    {
        Mesh = 0,
        Light = 1,
        Camera = 2,
        Empty = 3
    };

    struct SceneObject
    {
        SceneObjectType type = SceneObjectType::Empty;
        mat4 transform = mat4(1.0f);

        //MeshHandle mesh;
        //MaterialHandle material;
        // 
        // Light/Camera indices (into scene arrays)
        //uint32_t lightIndex = UINT32_MAX;
        //uint32_t cameraIndex = UINT32_MAX;

        uint32_t flags = 0; // visibility, shadow casting, etc.
        std::string name;

        uint32_t meshIndex = UINT32_MAX;
        uint32_t materialIndex = UINT32_MAX;

        //bool isRenderable() const { return type == SceneObjectType::Mesh && mesh.isValid() && material.isValid(); }
    };
    */

    /*
    //MATERIALS
    struct FilePathSource
    {
        std::filesystem::path path;

        bool operator<(const FilePathSource& other) const { return path < other.path; }

        bool operator==(const FilePathSource& other) const { return path == other.path; }
    };

    struct EmbeddedMemorySource
    {
        const aiScene* scene = nullptr; // Pointer to the scene containing the texture
        std::string identifier;         // e.g., "*0", "*1"
        std::string scenePath;          // The path of the parent scene file, for unique caching

        bool operator<(const EmbeddedMemorySource& other) const
        {
            if (scenePath != other.scenePath)
                return scenePath < other.scenePath;
            return identifier < other.identifier;
        }

        bool operator==(const EmbeddedMemorySource& other) const
        {
            if (!scene || !other.scene)
                return false;
            return scene == other.scene && identifier == other.identifier && scenePath == other.scenePath;
        }
    };
    using TextureDataSource = std::variant<std::monostate, FilePathSource, EmbeddedMemorySource>;

    enum class AlphaMode : uint32_t
    {
        Opaque = 0,
        Mask = 1,
        Blend = 2,
    };

    enum MaterialFlags : uint32_t
    {
        sMaterialFlags_CastShadow = 0x1,
        sMaterialFlags_ReceiveShadow = 0x2,
        sMaterialFlags_Transparent = 0x4,
        sMaterialFlags_DoubleSided = 0x8,
        sMaterialFlags_Unlit = 0x10,
        sMaterialFlags_AlphaTested = 0x20,
        // Uses alpha cutoff
    };

    enum MaterialType : uint32_t
    {
        MaterialType_Invalid = 0,
        MaterialType_MetallicRoughness = 0x1,
        MaterialType_SpecularGlossiness = 0x2,
        // For legacy FBX/OBJ support
        MaterialType_Unlit = 0x80,
    };

    struct MaterialTexture
    {
        TextureDataSource source;
        uint32_t uvSet = 0;
        vec2 uvScale = vec2(1.0f);
        vec2 uvOffset = vec2(0.0f);

        bool hasTexture() const
        {
            return !std::holds_alternative<std::monostate>(source);
        }
    };

    struct Material
    {
        std::string name;

        // Core PBR properties
        vec4 baseColorFactor = vec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        vec3 emissiveFactor = vec3(0.0f); // Changed from vec4 to vec3
        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
        float alphaCutoff = 0.5f;

        // Texture properties with transforms
        MaterialTexture baseColorTexture;
        MaterialTexture metallicRoughnessTexture;
        MaterialTexture normalTexture;
        MaterialTexture emissiveTexture;
        MaterialTexture occlusionTexture;

        // Material properties
        AlphaMode alphaMode = AlphaMode::Opaque;
        uint32_t materialTypeFlags = MaterialType_MetallicRoughness;
        uint32_t flags = MaterialFlags::sMaterialFlags_CastShadow | MaterialFlags::sMaterialFlags_ReceiveShadow;
        bool doubleSided = false;

        bool isTransparent() const
        {
            return alphaMode == AlphaMode::Blend || baseColorFactor.a < 1.0f;
        }

        bool isAlphaTested() const
        {
            return alphaMode == AlphaMode::Mask;
        }
    };
    */


}


#if 0

inline vec2 clampLength(const vec2& v, float maxLength)
{
    const float l = length(v);

    return l > maxLength ? normalize(v) * maxLength : v;
}

template <typename T>
T clamp(T v, T a, T b)
{
    if (v < a)
        return a;
    if (v > b)
        return b;
    return v;
}

namespace internal
{
    static std::mt19937 globalRandomEngine;

    struct RandomEngineSeeder
    {
        RandomEngineSeeder()
        {
            std::random_device rd;
            globalRandomEngine.seed(rd());
            // globalRandomEngine.seed(12345);
        }
    };

    static RandomEngineSeeder seeder;
}

template <typename T>
T randomRange(T minInclusive, T maxExclusive)
{
    static_assert(std::is_arithmetic_v<T>, "randomRange requires arithmetic type");
    assert(minInclusive <= maxExclusive && "minInclusive must be less than or equal to maxExclusive");
    if constexpr (std::is_integral_v<T>)
    {
        if (minInclusive == maxExclusive) { return minInclusive; }
        std::uniform_int_distribution<T> dist(minInclusive, maxExclusive - 1);
        return dist(internal::globalRandomEngine);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> dist(minInclusive, maxExclusive);
        return dist(internal::globalRandomEngine);
    }
    return {}; // avoid compile time warning
}

inline float random01() { return randomRange(0.0f, 1.0f); }

inline float randomFloat(float min, float max) { return randomRange(min, max); }

inline vec3 randomVec(const vec3& min, const vec3& max)
{
    return { randomFloat(min.x, max.x), randomFloat(min.y, max.y), randomFloat(min.z, max.z) };
}

inline vec3 randVec() { return randomVec(vec3(-5.0f, -5.0f, -5.0f), vec3(5.0f, 5.0f, 5.0f)); }

inline void getFrustumPlanes(mat4 viewProj, vec4* planes)
{
    viewProj = transpose(viewProj);
    planes[0] = vec4(viewProj[3] + viewProj[0]); // left
    planes[1] = vec4(viewProj[3] - viewProj[0]); // right
    planes[2] = vec4(viewProj[3] + viewProj[1]); // bottom
    planes[3] = vec4(viewProj[3] - viewProj[1]); // top
    planes[4] = vec4(viewProj[3] + viewProj[2]); // near
    planes[5] = vec4(viewProj[3] - viewProj[2]); // far
}

inline void getFrustumCorners(mat4 viewProj, vec4* points)
{
    const vec4 corners[] = { vec4(-1, -1, -1, 1), vec4(1, -1, -1, 1), vec4(1, 1, -1, 1), vec4(-1, 1, -1, 1), vec4(-1, -1, 1, 1), vec4(1, -1, 1, 1), vec4(1, 1, 1, 1), vec4(-1, 1, 1, 1) };

    const mat4 invViewProj = inverse(viewProj);

    for (int i = 0; i != 8; i++)
    {
        const vec4 q = invViewProj * corners[i];
        points[i] = q / q.w;
    }
}

inline bool isBoxInFrustum(vec4* frustumPlanes, vec4* frustumCorners, const BoundingBox& box)
{
    using glm::dot;

    for (int i = 0; i < 6; i++)
    {
        int r = 0;
        r += dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.min_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.min_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.max_.x, box.min_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.min_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
        r += dot(frustumPlanes[i], vec4(box.max_.x, box.max_.y, box.max_.z, 1.0f)) < 0.0 ? 1 : 0;
        if (r == 8)
            return false;
    }

    // check frustum outside/inside box
    int r = 0;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].x > box.max_.x ? 1 : 0;
    if (r == 8)
        return false;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].x < box.min_.x ? 1 : 0;
    if (r == 8)
        return false;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].y > box.max_.y ? 1 : 0;
    if (r == 8)
        return false;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].y < box.min_.y ? 1 : 0;
    if (r == 8)
        return false;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].z > box.max_.z ? 1 : 0;
    if (r == 8)
        return false;
    r = 0;
    for (int i = 0; i < 8; i++)
        r += frustumCorners[i].z < box.min_.z ? 1 : 0;
    if (r == 8)
        return false;

    return true;
}

inline BoundingBox combineBoxes(const std::vector<BoundingBox>& boxes)
{
    std::vector<vec3> allPoints;
    allPoints.reserve(boxes.size() * 8);

    for (const auto& b : boxes)
    {
        allPoints.emplace_back(b.min_.x, b.min_.y, b.min_.z);
        allPoints.emplace_back(b.min_.x, b.min_.y, b.max_.z);
        allPoints.emplace_back(b.min_.x, b.max_.y, b.min_.z);
        allPoints.emplace_back(b.min_.x, b.max_.y, b.max_.z);

        allPoints.emplace_back(b.max_.x, b.min_.y, b.min_.z);
        allPoints.emplace_back(b.max_.x, b.min_.y, b.max_.z);
        allPoints.emplace_back(b.max_.x, b.max_.y, b.min_.z);
        allPoints.emplace_back(b.max_.x, b.max_.y, b.max_.z);
    }

    return { allPoints.data(), allPoints.size() };
}

#endif