/******************************************************************************/
/*!
\file   Serializer.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (50%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
  This is the implementation file for the Serializer system. It contains definitions
  for all functions related to the serialization system.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Serializer.h"
#include "EntityUID.h"

enum class PROPERTY_TYPE
{
    VAR,
    ARR_START,
    ARR_VAR
};

// Get whether a property name indicates a normal variable, the start of an array or an array item.
PROPERTY_TYPE GetPropertyType(const std::string_view& propertyName)
{
    if (propertyName.back() != ']')
        return PROPERTY_TYPE::VAR;

    if (*(propertyName.end() - 2) == '[')
        return PROPERTY_TYPE::ARR_START;
    else
        return PROPERTY_TYPE::ARR_VAR;
}

// Extracts only the name of the property, removing the class names, etc.
std::string PropertiesStrToMinimalStr(const std::string_view& propertiesStr)
{
    size_t slashIndex{ propertiesStr.rfind('/') };
    return std::string{ propertiesStr.substr(slashIndex + 1) };
}

// Extracts the number of slashes within the name of a property, providing information about the scope level within the properties object
size_t GetScopeLevel(const std::string_view& propertiesStr)
{
    return std::count(propertiesStr.begin(), propertiesStr.end(), '/');
}

#pragma region ISerializeable

void ISerializeable::Serialize(Serializer& writer) const
{
    writer.Serialize(static_cast<const PropertiesSerializeable&>(*this));
}

void ISerializeable::Deserialize(Deserializer& reader)
{
    reader.Deserialize(static_cast<PropertiesSerializeable*>(this));
}

#pragma endregion // ISerializeable

#pragma region Serializer

Serializer::Serializer(const std::string& filepath)
    : ofs{ filepath }
    , sb{}
    , writer{ sb }
    , layerStack{}
    , numEntities{}
{
    if (IsOpen())
        StartObject();
    else
        CONSOLE_LOG(LEVEL_ERROR) << "Serializer failed to open file " << filepath;
}

Serializer::~Serializer()
{
    if (IsOpen())
        SaveAndClose();
}

bool Serializer::IsOpen() const
{
    return ofs.is_open();
}

bool Serializer::SaveAndClose()
{
    if (!IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Serializer failed to save as the output file stream is closed!";
        return false;
    }

    //Flush all entities currently in the buffer
    FlushEntities();

    // Ensure we've properly ended all objects
    while (!layerStack.empty())
        switch (layerStack.top())
        {
        case LAYER::OBJ: EndObject(); break;
        case LAYER::ARRAY: EndArray(); break;
        default: CONSOLE_LOG(LEVEL_ERROR) << "Unimplemented object type";
        }

    // Output whatever rj has written to file
    ofs << sb.GetString();
    ofs.close();
    return true;
}

void Serializer::StartObject(const std::string& key)
{
    if (!key.empty())
        writer.Key(key);
    writer.StartObject();
    layerStack.push(LAYER::OBJ);
}

void Serializer::EndObject()
{
    if (!IsTopLayerEqualTo(LAYER::OBJ))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Serializer failed to end an object - Invalid operation";
        return;
    }

    writer.EndObject();
    layerStack.pop();
}
void Serializer::StartArray(const std::string& key)
{
    if (!key.empty())
        writer.Key(key);
    writer.StartArray();
    layerStack.push(LAYER::ARRAY);
}

void Serializer::EndArray()
{
    if (!IsTopLayerEqualTo(LAYER::ARRAY))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Serializer failed to end an array - Invalid operation";
        return;
    }

    writer.EndArray();
    layerStack.pop();
}

void Serializer::FlushEntities()
{
    std::map<ecs::EntityHash, int> entityIndexMap{};
    std::vector<const RegisteredComponentData*> entityRegisteredCompData{}; // For ordering components

    for (auto& [_, entities] : entityChildLevelMap)
    {
        // Sort entities by UID to ensure consistency
        std::sort(entities.begin(), entities.end(), [](ecs::ConstEntityHandle a, ecs::ConstEntityHandle b) -> bool {
            return a->GetComp<EntityUIDComponent>()->GetUID() < b->GetComp<EntityUIDComponent>()->GetUID();
        });

        // Serialize the entities
        for (const auto& entity : entities)
        {
            int entityindex = numEntities++;
            entityIndexMap.emplace(entity->GetHash(), entityindex);

            StartObject("entity" + std::to_string(entityindex));

            const Transform& transform{ entity->GetTransform() };
            Serialize("position", transform.GetWorldPosition());
            Serialize("zPosition", transform.GetZPos());
            Serialize("rotation", transform.GetWorldRotation());
            Serialize("scale", transform.GetWorldScale());

            // Serialize parent if child (but only if we've also serialized the parent)
            std::string parentStr{};
            if (const Transform* parent{ transform.GetParent() })
            {
                auto indexIter{ entityIndexMap.find(parent->GetEntity()->GetHash()) };
                if (indexIter != entityIndexMap.end())
                    parentStr = "entity" + std::to_string(indexIter->second);
            }
            Serialize("parent", parentStr);

            StartObject("components");

            // Sort components by hash
            for (auto compIter{ entity->Comps_Begin() }, endIter{ entity->Comps_End() }; compIter != endIter; ++compIter)
                if (auto registeredData{ RegisteredComponents::GetData(compIter.GetCompHash()) })
                    entityRegisteredCompData.push_back(registeredData);
            std::sort(entityRegisteredCompData.begin(), entityRegisteredCompData.end(), [](auto a, auto b) -> bool {
                return a->GetMetaData()->hash < b->GetMetaData()->hash;
            });

            // Serialize the components
            for (auto registeredCompData : entityRegisteredCompData)
            {
                auto compHash{ registeredCompData->GetMetaData()->hash };
                StartObject(std::to_string(compHash));
                
                Serialize("_active", ecs::GetCompActive(entity->GetComp(compHash)));
                registeredCompData->SerializeFuncPtr(entity->GetComp(compHash), *this);

                EndObject();
            }
            entityRegisteredCompData.clear();

            EndObject(); // components

            EndObject(); // entity
        }
    }
}

bool Serializer::IsTopLayerEqualTo(LAYER layer)
{
    if (layerStack.empty())
        return false;
    return layerStack.top() == layer;
}

void Serializer::Serialize(ecs::ConstEntityHandle entity)
{
    int childlevel = 0;
    const Transform* t = entity->GetTransform().GetParent();
    while (t)
    {
        ++childlevel;
        t = t->GetParent();
    }
    entityChildLevelMap[childlevel].push_back(entity);
}

void Serializer::Serialize(const PropertiesSerializeable& obj)
{
    std::stack<int> arraySizes{}; // For tracking the size of arrays that we receive from properties

    // Extract each of the serializeable object's entries and write them to rj
    property::DisplayEnum(const_cast<PropertiesSerializeable&>(obj), [this, &arraySizes](void* objPtr, std::string_view propertyName, property::data&& data, const property::table&, std::size_t, property::flags::type flags) {
        // Helper function for calling the appropriate function to serialize a property variable
        auto SerializePropertyVar = [this](const std::string& key, bool isScope, void* objPtr, const property::data& data) -> void {
            if (isScope)
            {
                // This is a variable that implements PropertiesSerializeable, meaning it could have additional variables within it.
                // Let's recursively call our serializer function so we have more options than what the properties library provides us by default
                PropertiesSerializeable* basePtr{ reinterpret_cast<PropertiesSerializeable*>(objPtr) };
                assert(basePtr);
                // TODO: ISerializeableWithoutJsonObj isn't supported here due to being unable to easily detect whether we've already put a key into the current object and are just waiting to write a value only.
                // This might kinda mean that ISerializeableWithoutJsonObj shouldn't actually exist in the first place...
                if (ISerializeable* serializeable{ dynamic_cast<ISerializeable*>(basePtr) })
                    Serialize(key, *serializeable);
                else
                    Serialize(key, *basePtr);
            }
            else
                // This is a simple dumb properties variable with no further inner variables
                Serialize(key, data);
        };
        
        // If this variable is within an inner scope, ignore it since we should've explicitly serialized it already.
        // Minus arraySizes.size() to account for array scope level within the propertyName.
        if (GetScopeLevel(propertyName) > 1)
            return;

        std::string key{ PropertiesStrToMinimalStr(propertyName) };

        switch (GetPropertyType(propertyName))
        {
        case PROPERTY_TYPE::VAR:
            // A normal variable.
            SerializePropertyVar(key, flags.m_isScope, objPtr, data);
            break;
        case PROPERTY_TYPE::ARR_START:
        {
            // The start of an array.
            key.erase(key.size() - 2); // Remove []
            StartArray(key);

            // If the array is empty, end the array now.
            int numElem{ std::get<int>(data) };
            if (!numElem)
                EndArray();
            else
                // Otherwise we'll need to track the number of elements that we will receive from this array.
                arraySizes.push(numElem);

            break;
        }
        case PROPERTY_TYPE::ARR_VAR:
            // An element of the current array.
            SerializePropertyVar("", flags.m_isScope, objPtr, data);

            // End the array if this is the last variable of the array
            if (!--arraySizes.top())
            {
                arraySizes.pop();
                EndArray();
            }
            break;
        }
    });
}

void Serializer::Serialize(const std::string& key, const PropertiesSerializeable& obj)
{
    StartObject(key);
    Serialize(obj);
    EndObject();
}

void Serializer::Serialize(const ISerializeable& obj)
{
    obj.Serialize(*this);
}

void Serializer::Serialize(const std::string& key, const ISerializeable& obj)
{
    StartObject(key);
    obj.Serialize(*this);
    EndObject();
}

void Serializer::Serialize(const std::string& key, const ISerializeableWithoutJsonObj& obj)
{
    if (!key.empty())
        writer.Key(key);
    obj.Serialize(*this);
}

void Serializer::Serialize(const std::string& key, const property::data& data)
{
    if (!key.empty())
        writer.Key(key);

    // Write to rj depending on type of data stored within data
    // https://en.cppreference.com/w/cpp/utility/variant/visit
    std::visit([&writer = writer, this](auto&& arg) -> void {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, char>)
            writer.Int(arg);
        else if constexpr (std::is_same_v<T, unsigned int>)
            writer.Uint(arg);
        else if constexpr (std::is_same_v<T, size_t>)
            writer.Uint64(arg);
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
            writer.Double(arg);
        else if constexpr (std::is_same_v<T, bool>)
            writer.Bool(arg);
        else if constexpr (std::is_same_v<T, Vector2>)
        {
            StartObject();
            Serialize("x", arg.x);
            Serialize("y", arg.y);
            EndObject();
        }
        else if constexpr (std::is_same_v<T, Vector3>)
        {
            StartObject();
            Serialize("x", arg.x);
            Serialize("y", arg.y);
            Serialize("z", arg.z);
            EndObject();
        }
        else if constexpr (std::is_same_v<T, Vector4>)
        {
            StartObject();
            Serialize("x", arg.x);
            Serialize("y", arg.y);
            Serialize("z", arg.z);
            Serialize("w", arg.w);
            EndObject();
        }
        else if constexpr (std::is_same_v<T, std::string>)
            writer.String(arg);
        else
            assert(false);// Unimplemented property data variant type
        }, data);
}

#pragma endregion // Serializer

#pragma region Deserializer

Deserializer::Deserializer(const std::string& filepath)
    : isValid{ false }
    , document{}
    , valueStack{}
    , currentEntityIndex{}
{
    std::ifstream ifs{ filepath };
    if (!ifs.is_open())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer failed to open file " << filepath;
        return;
    }

    document.Parse(std::string{ std::istreambuf_iterator{ifs}, std::istreambuf_iterator<char>{} });
    isValid = !document.HasParseError();
    if (!isValid)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File Error when deserializing " << filepath;
        return;
    }
}

bool Deserializer::IsValid() const
{
    return isValid;
}

bool Deserializer::PushAccess(const std::string& key)
{
    const rj::Value& reader{ GetCurrValue() };

    // Duplicate top if key is empty to emulate not accessing into a child object
    // while not breaking logic flow of push/pop.
    if (key.empty())
    {
        valueStack.push(&reader);
        return true;
    }

    auto iter{ reader.FindMember(key) };
    if (iter != reader.MemberEnd())
    {
        valueStack.push(&iter->value);
        return true;
    }
    else
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer failed to access object/array with key " << key << '!';
        return false;
    }
}

bool Deserializer::PopAccess()
{
    if (valueStack.empty())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer has no object/array accesses to pop!";
        return false;
    }

    valueStack.pop();
    return true;
}

bool Deserializer::PushArrayElementAccess(size_t index)
{
    const rj::Value& reader{ GetCurrValue() };
    if (!reader.IsArray())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer attempted to access an array element when current object is not an array!";
        return false;
    }
    else if (index >= reader.Size())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer attempted to access an out-of-bounds array element!";
        return false;
    }

    valueStack.push(&reader[static_cast<rj::SizeType>(index)]);
    return true;
}

bool Deserializer::Deserialize(ecs::EntityHandle entity)
{
    if (!HasEntity())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Deserializer has no entity available for reading!";
        return false;
    }
    int index = currentEntityIndex++;
    if (!PushAccess("entity" + std::to_string(index)))
        return false;

    Transform& transform{ entity->GetTransform() };
    Vector2 vec2{};
    float f{};
    DeserializeVar("position", &vec2), transform.SetWorldPosition(vec2);
    DeserializeVar("zPosition", &f), transform.SetZPos(f);
    DeserializeVar("rotation", &f), transform.SetWorldRotation(f);
    DeserializeVar("scale", &vec2), transform.SetWorldScale(vec2);

    entityIndexMap.emplace(index, entity->GetHash());

    std::string parentName{};
    DeserializeVar("parent", &parentName);
    if (!parentName.empty())
    {
        int entityIndex{ std::stoi(parentName.substr(parentName.find_first_of("1234567890"))) };
        ecs::EntityHash parentEntityHash{ entityIndexMap.at(entityIndex) }; // This should not fail if we've serialized properly
        transform.SetParent(ecs::GetEntity(parentEntityHash)->GetTransform());
    }


    if (!PushAccess("components"))
    {
        PopAccess(); // entity
        return false;
    }

    const rj::Value& compsValue{ GetCurrValue() };
    for (auto iter{ compsValue.MemberBegin() }, end{ compsValue.MemberEnd() }; iter != end; ++iter)
    {
        std::string compHashStr{ iter->name.GetString() };
        ecs::CompHash compHash{ std::stoull(compHashStr) };
        const RegisteredComponentData* registeredData{ RegisteredComponents::GetData(compHash) };
        if (!registeredData)
        {
            CONSOLE_LOG(LEVEL_WARNING) << "Component with hash " << compHash << " does not exist! Skipping deserialization of component.";
            continue;
        }

        void* compHandle{ registeredData->ConstructDefaultAndAttachNowTo(entity) };
        if (!compHandle)
            // The component is already attached to the entity.
            compHandle = entity->GetComp(compHash);

        PushAccess(compHashStr); // Guaranteed to succeed because it's a member

        registeredData->DeserializeFuncPtr(compHandle, *this);
        bool isActive{ true };
        DeserializeVar("_active", &isActive);
        ecs::SetCompActive(entity->GetComp(compHash), isActive);
        
        PopAccess();
    }

    PopAccess(); // components

    PopAccess(); // entity

    return true;
}

bool Deserializer::Deserialize(PropertiesSerializeable* obj)
{
    bool succeeded{ true };
    std::stack<std::pair<size_t, size_t>> arraySizes{}; // For tracking the size of arrays that we receive from rj, (currIndex, numElem)

    property::DisplayEnum(*obj, [this, &obj, &succeeded, &arraySizes](void* objPtr, std::string_view propertyName, property::data&& data, const property::table&, std::size_t, property::flags::type flags) {
        // Helper function for calling the appropriate function to deserialize a property variable
        auto DeserializePropertyVar = [this, &succeeded](const std::string& key, PropertiesSerializeable* obj, const std::string_view& propertyName, bool isScope, void* objPtr, property::data* data) -> void {
            if (isScope)
            {
                // This is a variable that implements PropertiesSerializeable, meaning it could have additional variables within it.
                // Let's recursively call our serializer function so we have more options than what the properties library provides us by default
                PropertiesSerializeable* basePtr{ reinterpret_cast<PropertiesSerializeable*>(objPtr) };
                assert(basePtr);
                // TODO: ISerializeableWithoutJsonObj isn't supported here due to being unable to easily detect whether we've already put a key into the current object and are just waiting to write a value only.
                // This might kinda mean that ISerializeableWithoutJsonObj shouldn't actually exist in the first place...
                if (ISerializeable * serializeable{ dynamic_cast<ISerializeable*>(basePtr) })
                    (Deserialize(key, serializeable) ? true : succeeded = false);
                else
                    (Deserialize(key, basePtr) ? true : succeeded = false);
            }
            else
                // This is a simple dumb properties variable with no further inner variables
                (Deserialize(key, data) ? property::set(*obj, propertyName.data(), *data) : succeeded = false);
            };
        
        // If this variable is within an inner scope, ignore it since we should've explicitly deserialized it already.
        if (GetScopeLevel(propertyName) > 1)
            return;

        std::string key{ PropertiesStrToMinimalStr(propertyName) };

        switch (GetPropertyType(propertyName))
        {
        case PROPERTY_TYPE::VAR:
            // This is a normal variable.
            DeserializePropertyVar(key, obj, propertyName, flags.m_isScope, objPtr, &data);
            break;
        case PROPERTY_TYPE::ARR_START:
        {
            // The start of an array.
            key.erase(key.size() - 2); // Remove []
            if (!PushAccess(key))
            {
                succeeded = false;
                return;
            }

            // The number of array elements is the number of entries within current rj value
            size_t numElem{ static_cast<size_t>(GetCurrValue().Size()) };
            // Set the current array to the expected size
            property::set(*obj, propertyName.data(), static_cast<int>(numElem));

            // If the array is empty, end the array access now.
            if (!numElem)
                PopAccess(); // Array
            else
                // Otherwise we'll need to track the number of elements that we will receive from this array.
                arraySizes.push({ 0, numElem });

            break;
        }
        case PROPERTY_TYPE::ARR_VAR:
            if (arraySizes.empty())
                // This array has already failed deserialization...
                return;

            // Read the current element
            if (!PushArrayElementAccess(arraySizes.top().first++))
            {
                succeeded = false;
                return;
            }
            DeserializePropertyVar("", obj, propertyName, flags.m_isScope, objPtr, &data);
            PopAccess(); // Array element

            // Stop accessing the array if this is the last variable of the array
            if (arraySizes.top().first >= arraySizes.top().second)
            {
                arraySizes.pop();
                PopAccess(); // Array
            }
            break;
        }
    });

    return succeeded;
}

bool Deserializer::Deserialize(const std::string& key, PropertiesSerializeable* obj)
{
    if (!PushAccess(key))
        return false;
    Deserialize(obj);
    PopAccess();
    return true;
}

bool Deserializer::Deserialize(ISerializeable* obj)
{
    obj->Deserialize(*this);
    return true;
}

bool Deserializer::Deserialize(const std::string& key, ISerializeable* obj)
{
    if (!PushAccess(key))
        return false;
    obj->Deserialize(*this);
    PopAccess();
    return true;
}

bool Deserializer::Deserialize(const std::string& key, property::data* data)
{
    bool succeeded{ true };

    // Read from rj depending on type of data stored within data
    // https://en.cppreference.com/w/cpp/utility/variant/visit
    std::visit([this, &key, &succeeded](auto&& arg) -> void {
        succeeded = DeserializeVar<std::decay_t<decltype(arg)>>(key, &arg);
    }, *data);

    return succeeded;
}
bool Deserializer::GetArraySize(const std::string& key, size_t* size)
{
    bool succeeded{ true };

    //Default size to 0 in case it fails
    *size = 0;
    if (GetCurrValue().HasMember(key))
    {
        auto array = GetCurrValue()[key].GetArray();
        *size = array.Size();
    }
    else
        succeeded = false;

    return succeeded;
}

bool Deserializer::HasEntity() const
{
    return GetCurrValue().HasMember("entity" + std::to_string(currentEntityIndex));
}

const rj::Value& Deserializer::GetCurrValue() const
{
    return (valueStack.empty() ? document : *valueStack.top());
}

#pragma endregion // Deserializer
