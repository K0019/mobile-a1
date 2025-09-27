/******************************************************************************/
/*!
\file   ResourceManager.cpp
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

#include "ResourceManager.h"
#include "ResourceImporter.h"
#include "ResourceSerialization.h"

#include "AudioManager.h"

const std::string* ResourceNames::GetName(size_t hash) const
{
    auto nameIter{ names.find(hash) };
    return (nameIter != names.end() ? &nameIter->second : nullptr);
}

void ResourceNames::SetName(size_t hash, const std::string& name)
{
    names[hash] = name;
}

void ResourceNames::RemoveName(size_t hash)
{
    names.erase(hash);
}

void ResourceManager::Init()
{
    Messaging::Subscribe("NeedResourceLoaded", OnResourceRequestedLoad);
}
void ResourceManager::Shutdown()
{
    Messaging::Unsubscribe("NeedResourceLoaded", OnResourceRequestedLoad);
}

void ResourceManager::OnResourceRequestedLoad(size_t resourceHash)
{
    if (const auto* fileEntry{ ST<ResourceManager>::Get()->filepathsManager.GetFileEntry(resourceHash) })
        ResourceImporter::Import(ST<Filepaths>::Get()->assets / fileEntry->path);
}

UserResourceGetter<ResourceMesh> ResourceManager::Meshes()
{
    return UserResourceGetter<ResourceMesh>{ &ST<ResourceManager>::Get()->meshes };
}
UserResourceGetter<ResourceMaterial> ResourceManager::Materials()
{
    return UserResourceGetter<ResourceMaterial>{ &ST<ResourceManager>::Get()->materials };
}
UserResourceGetter<ResourceTexture> ResourceManager::Textures()
{
    return UserResourceGetter<ResourceTexture>{ &ST<ResourceManager>::Get()->textures };
}

void ResourceManager::SaveToFile() const
{
    Serializer writer{ ST<Filepaths>::Get()->assetsJson };
    if (!writer.IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to save resources: " << ST<Filepaths>::Get()->assetsJson;
        return;
    }

    ResourceSerialization::Serialize(writer, filepathsManager, namesManager);
}
void ResourceManager::LoadFromFile()
{
    Deserializer reader{ ST<Filepaths>::Get()->assetsJson };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open resources: " << ST<Filepaths>::Get()->assetsJson;
        return;
    }

    ResourceSerialization::Deserialize(reader, &filepathsManager, &namesManager, [](size_t resourceTypeHash, size_t resourceHash) -> void {
        ST<ResourceManager>::Get()->INTERNAL_CreateEmptyResource(resourceTypeHash, resourceHash);
    });
}

const ResourceContainerMeshes& ResourceManager::Editor_GetMeshes()
{
    return meshes;
}
const ResourceContainerMaterials& ResourceManager::Editor_GetMaterials()
{
    return materials;
}
const ResourceContainerTextures& ResourceManager::Editor_GetTextures()
{
    return textures;
}

const std::string& ResourceManager::Editor_GetName(size_t hash)
{
    const std::string* name{ namesManager.GetName(hash) };
    assert(name);
    return *name;
}

ResourceFilepaths& ResourceManager::INTERNAL_GetFilepathsManager()
{
    return filepathsManager;
}
ResourceNames& ResourceManager::INTERNAL_GetNamesManager()
{
    return namesManager;
}
ResourceContainerMeshes& ResourceManager::INTERNAL_GetMeshes()
{
    return meshes;
}
ResourceContainerMaterials& ResourceManager::INTERNAL_GetMaterials()
{
    return materials;
}
ResourceContainerTextures& ResourceManager::INTERNAL_GetTextures()
{
    return textures;
}

void ResourceManager::INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash)
{
    // Sorry for this if spam kinda running out of time, this function existing's also kinda ugly anyway
    if (resourceTypeHash == typeid(ResourceMesh).hash_code())
        meshes.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == typeid(ResourceMaterial).hash_code())
        materials.INTERNAL_CreateResource(resourceHash);
}


namespace fs = std::filesystem;

#ifdef max
#undef max
#endif
// Instantiate static variables
std::unordered_map<size_t, Animation>     ResourceManagerOld::Animations;
std::unordered_map<size_t, AudioAsset>  ResourceManagerOld::Sounds;
std::unordered_map<size_t, std::string>   ResourceManagerOld::ResourceNames;
std::unordered_map<size_t, ResourceManagerOld::SpriteSlot> ResourceManagerOld::Sprites;
size_t ResourceManagerOld::NextSpriteID = 0;

bool ResourceManagerOld::ResourceExists(size_t nameHash)
{
    return ResourceNames.contains(nameHash);
}

const std::string& ResourceManagerOld::GetResourceName(size_t nameHash)
{
    return ResourceNames[nameHash];
}

/*size_t ResourceManagerOld::LoadTexture(const std::string& file, const std::string& name)
{
    size_t nameHash = util::GenHash(name);
    ResourceNames[nameHash] = name;
    VulkanManager::Get().VkTextureManager().loadTextureFromFile(file, name);
    return nameHash;
}

size_t ResourceManagerOld::LoadTexture(const unsigned char* data, int width, int height, const std::string& name)
{
    size_t nameHash = util::GenHash(name);
    ResourceNames[nameHash] = name;
    VulkanManager::Get().VkTextureManager().LoadTextureFromMemory(data, width, height, name);
    return nameHash;
}

const Texture& ResourceManagerOld::GetTexture(const std::string& name)
{
    return VulkanManager::Get().VkTextureManager().getTexture(name);
}

const Texture& ResourceManagerOld::GetTexture(size_t nameHash)
{
    return VulkanManager::Get().VkTextureManager().getTexture(ResourceNames[nameHash]);
}

bool ResourceManagerOld::TextureExists(const std::string& name)
{
    return VulkanManager::Get().VkTextureManager().TextureExists(name);
}*/

/*size_t ResourceManagerOld::LoadFont(const std::string& fontFile)
{
    auto name = VulkanManager::Get().VkTextureManager().loadFontAtlasFromFile(fontFile);
    size_t nameHash = util::GenHash(name);
    ResourceNames[nameHash] = name;
    return nameHash;
}

const FontAtlas& ResourceManagerOld::GetFont(const std::string& name)
{
    return VulkanManager::Get().VkTextureManager().getFontAtlas(name);
}

const FontAtlas& ResourceManagerOld::GetFont(size_t nameHash)
{
    return VulkanManager::Get().VkTextureManager().getFontAtlas(ResourceNames[nameHash]);
}

bool ResourceManagerOld::FontExists(const std::string& name)
{
    return VulkanManager::Get().VkTextureManager().FontAtlasExists(name);
}*/

size_t ResourceManagerOld::CreateAnimationFromSprites(const std::string& name,
                                                   const std::vector<FrameData>& frameData) {
    if(frameData.empty()) {
        CONSOLE_LOG(LEVEL_ERROR) << "Cannot create animation with no frames";
        return INVALID_SPRITE_ID;
    }

    // Validate all sprite IDs
    for(const auto& frame : frameData) {
        if(frame.spriteID == INVALID_SPRITE_ID || !SpriteExists(frame.spriteID)) {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid sprite ID in animation: " << frame.spriteID;
            return INVALID_SPRITE_ID;
        }
    }

    size_t nameHash = util::GenHash(name);
    ResourceNames[nameHash] = name;

    Animation animation(frameData.size());

    // Use first valid sprite for dimensions
    bool foundValidSprite = false;
    for(const auto& frame : frameData) {
        const Sprite& sprite = GetSprite(frame.spriteID);
        if(sprite.textureID != INVALID_TEXTURE_ID) {
            animation.Width = sprite.width;
            animation.Height = sprite.height;
            foundValidSprite = true;
            break;
        }
    }

    if(!foundValidSprite) {
        CONSOLE_LOG(LEVEL_WARNING) << "Creating animation with no valid sprites";
        animation.Width = 0;
        animation.Height = 0;
    }

    animation.frames = frameData;
    animation.totalFrames = frameData.size();
    Animations[nameHash] = std::move(animation);
    return nameHash;
}
FrameData ResourceManagerOld::CreateFrameData(size_t spriteID, float duration) {
    FrameData frame;
    frame.spriteID = spriteID;
    frame.duration = duration;
    return frame;
}


Animation& ResourceManagerOld::GetAnimation(const std::string& name)
{
    return Animations.at(util::GenHash(name));
}

Animation& ResourceManagerOld::GetAnimation(size_t nameHash)
{
    return Animations.at(nameHash);
}

void ResourceManagerOld::DeleteAnimation(size_t nameHash)
{
    if (Animations.contains(nameHash)) {
        Animations.erase(nameHash);
        ResourceNames.erase(nameHash);
    }
}

bool ResourceManagerOld::AnimationExists(const std::string& name)
{
    return Animations.contains(util::GenHash(name));
}

size_t ResourceManagerOld::AddSprite(const Sprite& sprite) {
    // Validate texture ID
    if(sprite.textureID == INVALID_TEXTURE_ID && !std::filesystem::exists(sprite.textureName)) {
        CONSOLE_LOG(LEVEL_WARNING) << "Adding sprite with invalid texture: " << sprite.textureName;
    }

    size_t id = NextSpriteID++;
    Sprites[id] = SpriteSlot{
        sprite,
        true,
        sprite.textureName,
        sprite.textureID != INVALID_TEXTURE_ID
    };
    return id;
}


size_t ResourceManagerOld::GetSpriteID(const std::string& name) {
    for(const auto& [id, slot] : Sprites) {
        if(slot.active && slot.sprite.name == name) {
            return id;
        }
    }
    return INVALID_SPRITE_ID;
}

const Sprite& ResourceManagerOld::GetSprite(size_t spriteID) {
    static const Sprite invalidSprite = CreateInvalidSprite();

    auto it = Sprites.find(spriteID);
    if(it == Sprites.end() || !it->second.active) {
        //CONSOLE_LOG(LEVEL_WARNING) << "Attempting to access invalid sprite ID: " << spriteID;
        return invalidSprite;
    }

    // If texture is invalid, try to reload it
    if(!it->second.hasValidTexture) {
        try {
            if(std::filesystem::exists(it->second.originalPath)) {
                /*if(!TextureExists(it->second.originalPath)) {
                    LoadTexture(it->second.originalPath, it->second.originalPath);
                }*/
                //const Texture& tex = GetTexture(it->second.originalPath);
                //it->second.sprite.textureID = tex.index;
                //it->second.hasValidTexture = true;
            }
            else {
                it->second.sprite.textureID = INVALID_TEXTURE_ID;
                return invalidSprite;
            }
        }
        catch(...) {
            it->second.sprite.textureID = INVALID_TEXTURE_ID;
            return invalidSprite;
        }
    }

    return it->second.sprite;
}

void ResourceManagerOld::RenameSprite(size_t spriteID, const std::string& newName)
{
    auto it = Sprites.find(spriteID);
    if(it != Sprites.end() && it->second.active) {
        it->second.sprite.name = newName;
    }
}


void ResourceManagerOld::DeleteSprite(size_t spriteID)
{
    auto it = Sprites.find(spriteID);
    if(it != Sprites.end()) {
        it->second.active = false;
    }
}

bool ResourceManagerOld::SpriteExists(size_t spriteID)
{
    auto it = Sprites.find(spriteID);
    return it != Sprites.end() && it->second.active;
}

size_t ResourceManagerOld::GetSpriteCount()
{
    return NextSpriteID;
}

const AudioAsset& ResourceManagerOld::LoadSound(const std::string& name, AudioAsset& sound)
{
    ResourceNames[util::GenHash(name)] = name;
    return Sounds[util::GenHash(name)] = sound;
}

const AudioAsset& ResourceManagerOld::GetSound(const std::string& name)
{
    return Sounds[util::GenHash(name)];
}

const AudioAsset& ResourceManagerOld::GetSound(size_t nameHash)
{
    return Sounds[nameHash];
}

void ResourceManagerOld::DeleteSound(const std::string& name)
{
    Sounds.erase(util::GenHash(name));
}

bool ResourceManagerOld::SoundExists(const std::string& name)
{
    return Sounds.contains(util::GenHash(name));
}

void ResourceManagerOld::Clear()
{
    // delete all shaders	

    for(auto& [_, soundAsset] : Sounds)
    {
        soundAsset.sound->release();
    }
    Sounds.clear();

    Animations.clear();

    ResourceNames.clear();
}


bool ResourceManagerOld::SaveAssetsToFile(const std::string& filename)
{
    Serializer file{ filename };
    if (!file.IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open file for writing: " << filename;
        return false;
    }

    // Sort sprite slots by ID for consistency
    auto activeSprites{ util::ToSortedVectorOfRefs(Sprites, util::internal::DefaultBinaryPairPred<size_t, SpriteSlot>,[](const auto& a) -> bool { return a.second.active; }) };

    // Serialize sprites
    file.StartArray("sprites");
    for (const auto& [id, slot] : activeSprites)
    {
        file.StartObject();
        file.Serialize("id", id);
        file.Serialize(slot);
        file.EndObject();
    }
    file.EndArray();

    // Sort animations by name for consistency
    auto sortedAnimations{ util::ToSortedVectorOfRefs(Animations) };

    // Serialize animations
    file.StartArray("animations");
    for (const auto& [nameHash, anim] : sortedAnimations)
    {
        file.StartObject();
        file.Serialize("name", ResourceNames[nameHash]);
        file.Serialize(anim);
        file.EndObject();
    }
    file.EndArray();

    auto sortedSounds{ util::ToSortedVectorOfRefs(Sounds) };

	file.StartArray("sounds");
    for (const auto& [nameHash, sound] : sortedSounds)
    {
        file.StartObject();
        file.Serialize("name", ResourceNames[nameHash]);
        file.EndObject();
    }
    file.EndArray();

    return true;
}


bool ResourceManagerOld::LoadAssetsFromFile(const std::string& filename) {
    Sprites.clear();
    NextSpriteID = 0;
    Animations.clear();
    ResourceNames.clear();

    Deserializer file{ filename };
    if (!file.IsValid())
    {
        CONSOLE_LOG(LEVEL_INFO) << "No existing assets file found at: " << filename;
        return false;
    }

    // Deserialize sprites
    size_t elemsCount{};
    file.GetArraySize("sprites", &elemsCount);
    if (file.PushAccess("sprites"))
    {
        for (size_t index{}; index < elemsCount; ++index)
        {
            file.PushArrayElementAccess(index);
            size_t id{};
            file.DeserializeVar("id", &id);
            NextSpriteID = std::max(NextSpriteID, id + 1);

            SpriteSlot& slot{ Sprites[id] };
            file.Deserialize(&slot);

            // Try to load texture
            slot.sprite.textureID = INVALID_TEXTURE_ID; // start invalid
            if (std::filesystem::exists(slot.originalPath))
            {
                try
                {
                    /*if (!TextureExists(slot.originalPath))
                        LoadTexture(slot.originalPath, slot.originalPath);*/
                    //slot.sprite.textureID = GetTexture(slot.originalPath).index;
                    slot.hasValidTexture = true;
                }
                catch (const std::exception& e)
                {
                    CONSOLE_LOG(LEVEL_WARNING) << "Failed to load texture: " << slot.originalPath << " Error: " << e.what();
                }
            }
            else
                CONSOLE_LOG(LEVEL_WARNING) << "Texture file not found: " << slot.originalPath;

            file.PopAccess();
        }
        file.PopAccess();
    }

    // Deserialize animations
    file.GetArraySize("animations", &elemsCount);
    if (file.PushAccess("animations"))
    {
        for (size_t index{}; index < elemsCount; ++index)
        {
            file.PushArrayElementAccess(index);

            std::string name;
            file.DeserializeVar("name", &name);
            uint64_t nameHash{ util::GenHash(name) };

            ResourceNames[nameHash] = name;
            Animation& anim{ Animations[nameHash] };
            file.Deserialize(&anim);

            // Verify all sprite references exist and collect valid sprites
            bool hasValidSprites = false;
            for (const auto& frame : anim.frames)
            {
                if (frame.spriteID == INVALID_SPRITE_ID)
                {
                    CONSOLE_LOG(LEVEL_WARNING) << "Animation frame references invalid sprite ID";
                    continue;
                }
                auto it = Sprites.find(frame.spriteID);
                if (it == Sprites.end())
                {
                    CONSOLE_LOG(LEVEL_WARNING) << "Animation frame references non-existent sprite: " << frame.spriteID;
                    continue;
                }
                if (it->second.sprite.textureID != INVALID_TEXTURE_ID)
                    hasValidSprites = true;
            }
            if (!hasValidSprites)
            {
                CONSOLE_LOG(LEVEL_WARNING) << "Skipping animation '" << name << "' - no valid sprites found";
                ResourceNames.erase(nameHash);
                Animations.erase(nameHash);
            }

            file.PopAccess();
        }

        file.PopAccess();
    }

	// Deserialize sounds
	file.GetArraySize("sounds", &elemsCount);
    if (file.PushAccess("sounds"))
    {
        for (size_t index{}; index < elemsCount; ++index)
        {
            file.PushArrayElementAccess(index);
            std::string name;
            file.DeserializeVar("name", &name);
            uint64_t nameHash{ util::GenHash(name) };
            ResourceNames[nameHash] = name;
			ST<AudioManager>::Get()->CreateSound(name); 
            file.PopAccess();
        }
    }

    return true;
}

Sprite ResourceManagerOld::CreateInvalidSprite()
{
    Sprite invalidSprite;
    invalidSprite.textureID = INVALID_TEXTURE_ID;
    invalidSprite.width = 0;
    invalidSprite.height = 0;
    invalidSprite.texCoords = Vec4(0, 0, 1, 1);
    invalidSprite.name = "INVALID_SPRITE";
    return invalidSprite;

}

ResourceManagerOld::SpriteSlot::SpriteSlot(const Sprite& inSprite, bool inActive, const std::string& inOriginalPath, bool inHasValidTexture)
    : sprite{ inSprite }
    , active{ inActive }
    , originalPath{ inOriginalPath }
    , hasValidTexture{ inHasValidTexture }
{
}

void ResourceManagerOld::SpriteSlot::Serialize(Serializer& writer) const
{
    writer.Serialize("name", sprite.name);
    writer.Serialize("texturePath", ST<Filepaths>::Get()->TrimWorkingDirectoryFrom(originalPath));
    writer.Serialize("width", sprite.width);
    writer.Serialize("height", sprite.height);

    // I(Kendrick) am dumb and this doesn't work because the format of default Vec4 serialization is different... and Ryan's way is better...
    //writer.Serialize("texCoords", sprite.texCoords);
    writer.StartArray("texCoords");
    writer.Serialize("", sprite.texCoords.x);
    writer.Serialize("", sprite.texCoords.y);
    writer.Serialize("", sprite.texCoords.z);
    writer.Serialize("", sprite.texCoords.w);
    writer.EndArray();
}

void ResourceManagerOld::SpriteSlot::Deserialize(Deserializer& reader)
{
    reader.DeserializeVar("name", &sprite.name);
    reader.DeserializeVar("texturePath", &originalPath), ST<Filepaths>::Get()->AddWorkingDirectoryTo(&originalPath);
    sprite.textureName = originalPath; // Keep textureName matching original for now
    reader.DeserializeVar("width", &sprite.width);
    reader.DeserializeVar("height", &sprite.height);

    //reader.DeserializeVar("texCoords", &sprite.texCoords);
    reader.PushAccess("texCoords");
    reader.PushArrayElementAccess(0), reader.DeserializeVar("", &sprite.texCoords.x), reader.PopAccess();
    reader.PushArrayElementAccess(1), reader.DeserializeVar("", &sprite.texCoords.y), reader.PopAccess();
    reader.PushArrayElementAccess(2), reader.DeserializeVar("", &sprite.texCoords.z), reader.PopAccess();
    reader.PushArrayElementAccess(3), reader.DeserializeVar("", &sprite.texCoords.w), reader.PopAccess();
    reader.PopAccess();
}
