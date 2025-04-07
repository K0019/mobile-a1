/******************************************************************************/
/*!
\file   Serializer.ipp
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
  This is the source file implementing templates within the Serializer system. It
  contains the base class PropertiesSerializeable, and Entity serialization helper classes
  SerializeableComponent and SerializeableEntity.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Serializer.h"
#include <type_traits>

template<typename T>
void Serializer::Serialize(const std::string& key, const std::vector<T>& data)
{
    StartArray(key);

    for (const T& elem : data)
        Serialize("", elem);

    EndArray();
}


template<typename KeyType, typename ValueType>
void Serializer::Serialize(const std::string& key, const std::map<KeyType, ValueType>& data)
{
    StartArray(key);
    for (const auto& elem : data)
    {
        StartObject();
        Serialize("key", elem.first);
        Serialize("value", elem.second);
        EndObject();
    }
    EndArray();
}

template<typename KeyType, typename ValueType, typename Predicate>
    requires std::is_same_v<Predicate, std::nullptr_t> || std::predicate<Predicate, const std::pair<KeyType, ValueType>*, const std::pair<KeyType, ValueType>*>
void Serializer::Serialize(const std::string& key, const std::unordered_map<KeyType, ValueType>& data, Predicate pred)
{
    StartArray(key);

    // Execute lambda to produce constexpr code that calls ToSortedVectorOfRefs with different params depending on whether pred was provided
    const auto sortedElements{ [&data, &pred]() {
        if constexpr (!std::is_same_v<Predicate, std::nullptr_t>)
            return util::ToSortedVectorOfRefs(data, pred);
        else
            return util::ToSortedVectorOfRefs(data);
        }()
    };

    // Serialize the elements
    for (const auto& elem : sortedElements)
    {
        StartObject();
        Serialize("key", elem.first);
        Serialize("value", elem.second);
        EndObject();
    }

    EndArray();
}

template<typename T>
bool Deserializer::DeserializeVar(const std::string& key, T* out)
{
    if (!PushAccess(key))
        return false;

    const rj::Value& reader{ GetCurrValue() };

    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, char>)
        *out = static_cast<T>(reader.GetInt());
    else if constexpr (std::is_same_v<T, unsigned int>)
        *out = static_cast<T>(reader.GetUint());
    else if constexpr (std::is_same_v<T, size_t>)
        *out = reader.GetUint64();
    else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>)
        *out = static_cast<T>(reader.GetDouble());
    else if constexpr (std::is_same_v<T, bool>)
        *out = reader.GetBool();
    else if constexpr (std::is_same_v<T, Vector2>)
    {
        out->x = reader["x"].GetFloat();
        out->y = reader["y"].GetFloat();
    }
    else if constexpr (std::is_same_v<T, Vector3>)
    {
        out->x = reader["x"].GetFloat();
        out->y = reader["y"].GetFloat();
        out->z = reader["z"].GetFloat();
    }
    else if constexpr (std::is_same_v<T, Vector4>)
    {
        out->x = reader["x"].GetFloat();
        out->y = reader["y"].GetFloat();
        out->z = reader["z"].GetFloat();
        out->w = reader["w"].GetFloat();
    }
    else if constexpr (std::is_same_v<T, std::string>)
        *out = reader.GetString();
    else if constexpr (std::is_same_v<T, property::data> || std::derived_from<T, ISerializeable>)
        Deserialize("", out);
    else
        //static_assert(false, "Unimplemented type");//Some devices may need to change this to assert to compile the project.
        assert(false); // Unimplemented type
    PopAccess();
    return true;
}

template <typename T>
bool Deserializer::DeserializeVar(const std::string& key, std::vector<T>* out)
{
    if (!PushAccess(key))
        return false;

    const rj::Value& arr{ GetCurrValue() };
    size_t numElem{ arr.Size() };
    out->resize(numElem);

    for (size_t i{}; i < numElem; ++i)
    {
        if (!PushArrayElementAccess(i))
        {
            PopAccess();
            return false;
        }
        DeserializeVar("", &out->at(i));
        PopAccess();
    }

    PopAccess();
    return true;
}

template<typename KeyType, typename ValueType>
inline bool Deserializer::DeserializeVar(const std::string& key, std::map<KeyType, ValueType>* out)
{
    if (!PushAccess(key))
        return false;

    const rj::Value& arr{ GetCurrValue() };
    size_t numElem{ arr.Size() };

    for (size_t i{}; i < numElem; ++i)
    {
        if (!PushArrayElementAccess(i))
            return false;
        std::pair<KeyType, ValueType> p;
        DeserializeVar<std::decay_t<decltype(p.first)>>("key", &p.first);
        if (std::is_same_v<ValueType, property::data>)
            p.second = (*out)[p.first];//Required to get the type if it is a property::data
        DeserializeVar<std::decay_t<decltype(p.second)>>("value", &p.second);

        (*out)[p.first] = std::move(p.second);
        PopAccess();
    }

    PopAccess();
    return true;
}

template<typename KeyType, typename ValueType>
inline bool Deserializer::DeserializeVar(const std::string& key, std::unordered_map<KeyType, ValueType>* out)
{
    return DeserializeVar(key, out, [](Deserializer& reader, auto mapPtr) -> void {
        std::pair<KeyType, ValueType> p{};
        reader.DeserializeVar<std::decay_t<decltype(p.first)>>("key", &p.first);
        if (std::is_same_v<ValueType, property::data>)
            p.second = (*mapPtr)[p.first];//Required to get the type if it is a property::data
        reader.DeserializeVar<std::decay_t<decltype(p.second)>>("value", &p.second);

        (*mapPtr)[p.first] = std::move(p.second);
    });

    //if (!PushAccess(key))
    //    return false;

    //const rj::Value& arr{ GetCurrValue() };
    //size_t numElem{ arr.Size() };

    //for (size_t i{}; i < numElem; ++i)
    //{
    //    if (!PushArrayElementAccess(i))
    //        return false;
    //    std::pair<KeyType, ValueType> p;
    //    DeserializeVar<std::decay_t<decltype(p.first)>>("key", &p.first);
    //    if (std::is_same_v<ValueType, property::data>)
    //        p.second = (*out)[p.first];//Required to get the type if it is a property::data
    //    DeserializeVar<std::decay_t<decltype(p.second)>>("value", &p.second);

    //    (*out)[p.first] = std::move(p.second);
    //    PopAccess();
    //}

    //PopAccess();
    //return true;
}

template <typename MapType, typename KeyType, typename ValueType, typename Operation>
    requires util::IsMap_v<MapType>&& std::regular_invocable<Operation, Deserializer&, MapType*>
bool Deserializer::DeserializeVar(const std::string& key, MapType* out, Operation operationFunc)
{
    if (!PushAccess(key))
        return false;

    const rj::Value& arr{ GetCurrValue() };
    size_t numElem{ arr.Size() };

    for (size_t i{}; i < numElem; ++i)
    {
        if (!PushArrayElementAccess(i))
            return false;
        operationFunc(*this, out);
        PopAccess();
    }

    PopAccess();
    return true;
}
