
/******************************************************************************/
/*!
\file   Materials.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
  This is an interface file for the material system.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
class MaterialSystem;

namespace MaterialFlags {
    static constexpr uint32_t None = 0;

    // Light interaction properties
    static constexpr uint32_t ReceivesLight = 1 << 0;   // Material is affected by scene lighting
    static constexpr uint32_t OccludesLight = 1 << 1;   // Creates shadows in lighting pass
    static constexpr uint32_t SolidColor = 1 << 5;   // Uses a solid color instead of texture
    static constexpr uint32_t Repeating = 1 << 6;   // Texture repeats instead of being to scale
}

struct MaterialParameters : public ISerializeable {
    Vector4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f }; // RGB + Alpha for transparency
	Vector2 tiling{ 1.0f, 1.0f }; // UV tiling for texture
    property_vtable()
};

property_begin(MaterialParameters)
{
    property_var(baseColor),
    property_var(tiling)
}
property_vend_h(MaterialParameters)

// Serializable material instance
class MaterialInstance : public ISerializeable {
    std::string m_baseMaterialName;
    uint32_t m_overrideFlags{ 0 };
    bool m_hasOverrides{ false };
    bool m_hasParameterOverrides{ false };
    MaterialParameters m_overrideParameters;

    public:
    MaterialInstance();

    explicit MaterialInstance(std::string baseMaterialName);

    const std::string& getBaseMaterialName() const;

    void setBaseMaterialName(const std::string& name);

    // Flag operations now use uint32_t
    void setOverrideFlag(uint32_t flag, bool enabled);

    bool hasOverrideFlag(uint32_t flag) const;

    uint32_t getOverrideFlags() const;
    // Parameter override management
    void setParameterOverrides(const MaterialParameters& params);

    void clearParameterOverrides();

    bool hasParameterOverrides() const;

    const MaterialParameters& getOverrideParameters() const;

    // Property registration for serialization
    property_vtable()
};

// Property registration for MaterialInstance
property_begin(MaterialInstance)
{
    property_var(m_baseMaterialName),
        property_var(m_overrideFlags),
        property_var(m_hasOverrides),
        property_var(m_hasParameterOverrides),
        property_var(m_overrideParameters)
}
property_vend_h(MaterialInstance)

// Material system singleton
class MaterialSystem {
    friend class ST<MaterialSystem>;

    struct MaterialData {
        MaterialParameters parameters;
        uint32_t flags{ 0 };  // Using uint32_t instead of enum class
        std::string name;
    };

    std::map<std::string, MaterialData> m_materials;

    void createDefaultMaterials();

    bool loadMaterials();

    bool saveMaterials();

    public:
    void initialize();

    ~MaterialSystem();

    void createMaterial(const std::string& name, uint32_t flags = 0);

    uint32_t getEffectiveFlags(const MaterialInstance& instance) const;

    const MaterialParameters& getEffectiveParameters(const MaterialInstance& instance) const;

    const std::map<std::string, MaterialData>& getMaterials() const;

    bool materialExists(const std::string& name) const;

    void updateMaterialParameters(const std::string& name, const MaterialParameters& params);
};