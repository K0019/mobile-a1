/******************************************************************************/
/*!
\file   Materials.cpp
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

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Materials.h"
#include <GameSettings.h>

MaterialInstance::MaterialInstance() = default;

MaterialInstance::MaterialInstance(std::string baseMaterialName): m_baseMaterialName(std::move(baseMaterialName)) {
}

const std::string& MaterialInstance::getBaseMaterialName() const {
    return m_baseMaterialName;
}

void MaterialInstance::setBaseMaterialName(const std::string& name) {
    m_baseMaterialName = name;
}

void MaterialInstance::setOverrideFlag(uint32_t flag, bool enabled) {
    if(enabled) {
        m_overrideFlags |= flag;
    }
    else {
        m_overrideFlags &= ~flag;
    }
}

bool MaterialInstance::hasOverrideFlag(uint32_t flag) const {
    return (m_overrideFlags & flag) != 0;
}

uint32_t MaterialInstance::getOverrideFlags() const {
    return m_overrideFlags;
}

void MaterialInstance::setParameterOverrides(const MaterialParameters& params) {
    m_overrideParameters = params;
    m_hasParameterOverrides = true;
}

void MaterialInstance::clearParameterOverrides() {
    m_hasParameterOverrides = false;
}

bool MaterialInstance::hasParameterOverrides() const {
    return m_hasParameterOverrides;
}

const MaterialParameters& MaterialInstance::getOverrideParameters() const {
    return m_overrideParameters;
}

void MaterialSystem::createDefaultMaterials() {
    // Standard material for most sprites
    // Uses deferred rendering path and participates in lighting
    createMaterial(
            "default",
            MaterialFlags::ReceivesLight
        );
        
        // Set conservative default parameters
        auto& defaultMaterial = m_materials["default"];
        defaultMaterial.parameters.baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

    {
        // Solid occluder material (e.g., walls, solid objects)
        MaterialData& occluderMaterial = m_materials["occluder"];
        occluderMaterial.flags = MaterialFlags::OccludesLight;
        occluderMaterial.name = "occluder";
        occluderMaterial.parameters.baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    {
        // Non-lighting participant material (UI elements, backgrounds)
        MaterialData& unlitMaterial = m_materials["unlit"];
        unlitMaterial.flags = 0;  // No lighting calculations
        unlitMaterial.name = "unlit";
        unlitMaterial.parameters.baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

bool MaterialSystem::loadMaterials() {

    auto path = (ST<Filepaths>::Get()->materialsSave + "/materials.json");
    try {
        Deserializer reader(path.c_str());
        if(!reader.IsValid()) {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to open materials file for reading";
            return false;
        }

        // The JSON structure is an array named "materials"
        size_t materialCount;
        if(!reader.GetArraySize("materials", &materialCount)) {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to read materials array size";
            return false;
        }

        if(!reader.PushAccess("materials")) {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to access materials array";
            return false;
        }

        m_materials.clear();

        // Process each material in the array
        for(size_t i = 0; i < materialCount; ++i) {
            if(!reader.PushArrayElementAccess(i)) {
                CONSOLE_LOG(LEVEL_ERROR) << "Failed to access material element " << i;
                continue;
            }

            // Deserialize material data
            std::string materialName;
            uint32_t flags;

            if(!reader.DeserializeVar("name", &materialName)) {
                CONSOLE_LOG(LEVEL_ERROR) << "Failed to read material name for element " << i;
                reader.PopAccess();
                continue;
            }

            if(!reader.DeserializeVar("flags", &flags)) {
                CONSOLE_LOG(LEVEL_ERROR) << "Failed to read material flags for " << materialName;
                reader.PopAccess();
                continue;
            }

            // Create material entry
            MaterialData& material = m_materials[materialName];
            material.name = materialName;
            material.flags = flags;

            // Handle parameters object
            if(reader.PushAccess("parameters")) {
                if(!reader.Deserialize(&material.parameters)) {
                    CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize parameters for " << materialName;
                }
                reader.PopAccess();
            }
            else {
                CONSOLE_LOG(LEVEL_ERROR) << "Failed to access parameters for " << materialName;
            }

            reader.PopAccess(); // Pop material object
        }

        reader.PopAccess(); // Pop materials array

        // Validation: Ensure default material exists
        if(!m_materials.contains("default")) {
            CONSOLE_LOG(LEVEL_WARNING) << "Default material not found in loaded materials, creating...";
            createMaterial(
            "default",
            MaterialFlags::ReceivesLight    // Affected by scene lighting// Participates in shadow casting
        );
        
        // Set conservative default parameters
        auto& defaultMaterial = m_materials["default"];
        defaultMaterial.parameters.baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

        }

        return true;

    }
    catch(const std::exception& e) {
        CONSOLE_LOG(LEVEL_ERROR) << "Exception while loading materials: " << e.what();
        return false;
    }
}

bool MaterialSystem::saveMaterials() {

    auto path = (ST<Filepaths>::Get()->materialsSave + "/materials.json");

    try {
        Serializer writer(path.c_str());
        if(!writer.IsOpen()) {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to open materials file for writing";
            return false;
        }

        // Start with the materials array directly
        writer.StartArray("materials"); // This creates {"materials": []}

        // Serialize each material
        for(auto& [name, material] : m_materials) {
            writer.StartObject(); // Start material object - no name needed here

            // Serialize material properties
            writer.Serialize("name", material.name);
            writer.Serialize("flags", material.flags);

            // Parameters object
            writer.StartObject("parameters");
            material.parameters.Serialize(writer);
            writer.EndObject();

            writer.EndObject(); // End material object
        }

        writer.EndArray(); // End materials array

        return writer.SaveAndClose();
    }
    catch(const std::exception& e) {
        CONSOLE_LOG(LEVEL_ERROR) << "Exception while saving materials: " << e.what();
        return false;
    }
}

void MaterialSystem::initialize() {
    // Create default materials using uint32_t flags
    if(!loadMaterials()) {
        // If loading fails, create default materials and save them
        CONSOLE_LOG(LEVEL_INFO) << "No existing materials file found. Creating default materials...";
        createDefaultMaterials();
        saveMaterials();
    }
}

MaterialSystem::~MaterialSystem() {
    saveMaterials();
}

void MaterialSystem::createMaterial(const std::string& name, uint32_t flags) {
    MaterialData& material = m_materials[name];
    material.flags = flags;
    material.name = name;
}

uint32_t MaterialSystem::getEffectiveFlags(const MaterialInstance& instance) const {
    auto it = m_materials.find(instance.getBaseMaterialName());
    if(it == m_materials.end()) {
        it = m_materials.find("default");
    }

    return it->second.flags | instance.getOverrideFlags();
}

const MaterialParameters& MaterialSystem::getEffectiveParameters(const MaterialInstance& instance) const {
    auto it = m_materials.find(instance.getBaseMaterialName());
    if(it == m_materials.end()) {
        it = m_materials.find("default");
    }

    if(instance.hasParameterOverrides()) {
        return instance.getOverrideParameters();
    }
    return it->second.parameters;
}

const std::map<std::string, MaterialSystem::MaterialData>& MaterialSystem::getMaterials() const {
    return m_materials;
}

bool MaterialSystem::materialExists(const std::string& name) const {
    return m_materials.contains(name);
}

void MaterialSystem::updateMaterialParameters(const std::string& name, const MaterialParameters& params) {
    auto it = m_materials.find(name);
    if(it != m_materials.end()) {
        it->second.parameters = params;
    }
}
