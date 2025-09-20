/* PLANNED TO REFACTOR - Remove access via name and only allow access by hash so
    we force setting of resources via editor. If access by name is really required
    for some reason, provide that interface elsewhere which returns a hash that can
    be used here (some class responsible for tracking asset identifiers to hash?) */


/******************************************************************************/
/*!
\file   ResourceManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Resource manager class that hosts several functions to load Textures and Shaders. Each loaded texture
and/or shader is also stored for future reference by string handles. All functions and resources are static and no
public constructor is defined.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "scene.h"

#include "Animation.h"

#include "AssetBrowser.h"
#include "Sprite.h"
#include "GameSettings.h"

enum class RESOURCE_TYPE
{
    MESH
};

struct ResourceBase : public ISerializeable
{
    size_t hash;
    std::string name;

    virtual bool Load() = 0;
    virtual void Unload() = 0;

    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;
    property_vtable()
};
property_begin(ResourceBase)
{
    property_var(hash),
    property_var(name)
}
property_vend_h(ResourceBase)

struct ResourceMesh : public ResourceBase
{
    std::vector<SceneObject> objects;

    virtual bool Load() final;
    virtual void Unload() final;
};

template <std::derived_from<ResourceBase> ResourceType>
class ResourceContainerBase : public ISerializeableWithoutJsonObj
{
public:
    const ResourceType* GetResource(size_t hash);
    ResourceType* CreateResource(const std::string& name, const std::string& filepath);

public:
    void Serialize(Serializer& writer) const override;
    void Deserialize(Deserializer& reader) override;

private:
    std::unordered_map<size_t, ResourceType> resources;
};
template <std::derived_from<ResourceBase> ResourceType>
class UserResourceGetter
{
public:
    UserResourceGetter(ResourceContainerBase<ResourceType>* container);

    const ResourceType* GetResource(size_t hash) const;

private:
    ResourceContainerBase<ResourceType>* container;

};

class ResourceContainerMeshes : public ResourceContainerBase<ResourceMesh>
{
public:

private:

public:
};

class ResourceFilepaths
{
public:
    bool AddFilepath(size_t hash, const std::filesystem::path& filepath);

    const std::filesystem::path* GetAssetsRelativeFilepath(size_t hash) const;
    std::optional<std::filesystem::path> GetExeRelativeFilepath(size_t hash) const;

private:
    std::unordered_map<size_t, std::filesystem::path> filepaths;

};

class ResourceManager : public ISerializeable
{
public:
    static const ResourceContainerMeshes& Meshes();
    
    static bool Import(RESOURCE_TYPE type, const std::string& name, const std::filesystem::path& filepath);

    void SaveToFile() const;
    void LoadFromFile();

private:
    ResourceContainerMeshes meshes;

public:
    property_vtable()
};
property_begin(ResourceManager)
{
    property_var(meshes)
}
property_vend_h(ResourceManager)


// A static singleton ResourceManagerOld class that hosts several
// functions to load Textures and Shaders. Each loaded texture
// and/or shader is also stored for future reference by string
// handles. All functions and resources are static and no 
// public constructor is defined.
class ResourceManagerOld {
public:
    struct SpriteSlot;

public:
    // Resource management functions
    static bool ResourceExists(size_t nameHash);
    static const std::string& GetResourceName(size_t nameHash);
    /*static size_t LoadTexture(const std::string& file, const std::string& name);
    static size_t LoadTexture(const unsigned char* data, int width, int height, const std::string& name);
    /*static const Texture& GetTexture(const std::string& name);
    static const Texture& GetTexture(size_t nameHash);#1#
    static bool TextureExists(const std::string& name);
    
    // Font management
    static size_t LoadFont(const std::string& fontFile);
    /*static const FontAtlas& GetFont(const std::string& name);
    static const FontAtlas& GetFont(size_t nameHash);#1#
    static bool FontExists(const std::string& name);*/
    
    // Sound management
    static const AudioAsset& LoadSound(const std::string& name, AudioAsset& sound);
    static const AudioAsset& GetSound(const std::string& name);
    static const AudioAsset& GetSound(size_t nameHash);
    static void DeleteSound(const std::string& name);
    static bool SoundExists(const std::string& name);
    
    // Animation management
    static size_t CreateAnimationFromSprites(const std::string& name, const std::vector<FrameData>& frameData);
    static FrameData CreateFrameData(size_t spriteID, float duration = 0.5f);
    static Animation& GetAnimation(const std::string& name);
    static Animation& GetAnimation(size_t nameHash);
    static void DeleteAnimation(size_t nameHash);
    static bool AnimationExists(const std::string& name);

    static size_t AddSprite(const Sprite& sprite);
    static size_t GetSpriteID(const std::string& name);
    static const Sprite& GetSprite(size_t spriteID);
    static void RenameSprite(size_t spriteID, const std::string& newName);
    static void DeleteSprite(size_t spriteID);
    static bool SpriteExists(size_t spriteID);
    static size_t GetSpriteCount();
    // Resource cleanup
    static void Clear();
    
    static auto& GetAnimations() { return Animations; }

    static bool SaveAssetsToFile(const std::string& filename);
    static bool LoadAssetsFromFile(const std::string& filename);
    
    static constexpr uint32_t INVALID_TEXTURE_ID = std::numeric_limits<uint32_t>::max();
    static constexpr size_t INVALID_SPRITE_ID = std::numeric_limits<size_t>::max();

private:
    static std::unordered_map<size_t, Animation> Animations;
    static std::unordered_map<size_t, AudioAsset> Sounds;
    static std::unordered_map<size_t, std::string> ResourceNames;
    static Sprite CreateInvalidSprite();

    static std::unordered_map<size_t, SpriteSlot> Sprites;
    static size_t NextSpriteID;

    ResourceManagerOld() = default;

public:
    struct SpriteSlot : public ISerializeable
    {
        SpriteSlot() = default;
        SpriteSlot(const Sprite& inSprite, bool inActive, const std::string& inOriginalPath, bool inHasValidTexture);

        Sprite sprite;
        bool active = true;
        std::string originalPath;
        bool hasValidTexture = true;

        void Serialize(Serializer& writer) const override;
        void Deserialize(Deserializer& reader) override;

        property_vtable()
    };
};

property_begin(ResourceManagerOld::SpriteSlot)
{
}
property_vend_h(ResourceManagerOld::SpriteSlot)

#include "ResourceManager.ipp"
