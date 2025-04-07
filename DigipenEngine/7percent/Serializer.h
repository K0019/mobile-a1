/******************************************************************************/
/*!
\file   Serializer.h
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
  This is an interface file for the Serializer system. It contains the base class PropertiesSerializeable,
  and Entity serialization helper classes SerializeableComponent and SerializeableEntity.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Properties.h"

#undef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/writer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/pointer.h"

#include "Singleton.h"
#include "Utilities.h"
#include "PercentMath.h"

#include <fstream>
#include <stack>
#include <map>
#include <unordered_map>

#ifdef max
#undef max
#endif

namespace rj = rapidjson;

#define PRINTER(Variable) (#Variable)


// Forward declarations
class Serializer;
class Deserializer;

namespace CSharpScripts {
    class ScriptInstance;
}


// Indicates that a class is registered to properties and is serializeable.
using PropertiesSerializeable = property::base;

/*****************************************************************//*!
\class ISerializeable
\brief
    Interface to enable serialization on a class.
*//******************************************************************/
class ISerializeable : public PropertiesSerializeable
{
public:
    /*****************************************************************//*!
    \brief
        Serializes this object to file.
    \param writer
        The serializer object to write data to.
    *//******************************************************************/
    virtual void Serialize(Serializer& writer) const;

    /*****************************************************************//*!
    \brief
        Deserializes this object from file.
    \param reader
        The reader object to read data from.
    *//******************************************************************/
    virtual void Deserialize(Deserializer& reader);
};

/*****************************************************************//*!
\class ISerializeableWithoutJsonObj
\brief
    Interface to enable serialization on a class and tells the serializer
    to not create a json object encapsulating this class's serialization.
*//******************************************************************/
class ISerializeableWithoutJsonObj : public ISerializeable
{
};

/*****************************************************************//*!
\class Serializer
\brief
    Simplified interface for serializing data in json format.
*//******************************************************************/
class Serializer
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    \param filepath
        The filepath to serialize to. This should also include the file extension.
        Will override any existing file.
    *//******************************************************************/
    Serializer(const std::string& filepath);

    /*****************************************************************//*!
    \brief
        Destructor. Saves inserted data to file.
    *//******************************************************************/
    ~Serializer();

    /*****************************************************************//*!
    \brief
        Check whether a file for writing has been creating successfully.
    \return
        True if this serializer is in a valid state to write to. False otherwise.
    *//******************************************************************/
    bool IsOpen() const;

    /*****************************************************************//*!
    \brief
        Saves all inserted data to file and closes the writing stream.
    \return
        True if the file was successfully saved. False otherwise.
    *//******************************************************************/
    bool SaveAndClose();

    /*****************************************************************//*!
    \brief
        Starts a json object with the provided key.
    \param key
        The key of the json object.
    *//******************************************************************/
    void StartObject(const std::string& key = "");

    /*****************************************************************//*!
    \brief
        Ends a json object.
    *//******************************************************************/
    void EndObject();

    /*****************************************************************//*!
    \brief
        Starts a json array with the provided key.
    \param key
        The key of the json array.
    *//******************************************************************/
    void StartArray(const std::string& key = "");

    /*****************************************************************//*!
    \brief
        Ends a json array.
    *//******************************************************************/
    void EndArray();


    /*****************************************************************//*!
    \brief
        Serializes an entity and its components. Note that these entities
        will only be inserted into the json file upon saving.
    \param entity
        The entity to be serialized.
    *//******************************************************************/
    void Serialize(ecs::ConstEntityHandle entity);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from PropertiesSerializeable, aka is registered
        to properties library. No json object is created in this version.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    void Serialize(const PropertiesSerializeable& obj);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from PropertiesSerializeable, aka is registered
        to properties library. A json object with the provided key will be created in this version.
    \param key
        The key of the object.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    void Serialize(const std::string& key, const PropertiesSerializeable& obj);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from ISerializeable.
        No json object is created in this version.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    void Serialize(const ISerializeable& obj);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from ISerializeable.
        A json object with the provided key will be created in this version.
    \param key
        The key of the object.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    void Serialize(const std::string& key, const ISerializeable& obj);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from ISerializeableWithoutJsonObject.
    \param key
        The key of the object.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    void Serialize(const std::string& key, const ISerializeableWithoutJsonObj& obj);

    /*****************************************************************//*!
    \brief
        Serializes a variable, whose type is one of the registered types in
        the properties library.
    \param key
        The key to associate this variable with.
    \param data
        The variable.
    *//******************************************************************/
    void Serialize(const std::string& key, const property::data& data);

    /*****************************************************************//*!
    \brief
        Serializes a vector container.
    \param key
        The key to associate this container with.
    \param data
        The vector container.
    *//******************************************************************/
    template <typename T>
    void Serialize(const std::string& key, const std::vector<T>& data);

    /*****************************************************************//*!
    \brief
        Serializes a map container.
    \param key
        The key to associate this container with.
    \param data
        The map container.
    *//******************************************************************/
    template <typename KeyType, typename ValueType>
    void Serialize(const std::string& key, const std::map<KeyType, ValueType>& data);

    /*****************************************************************//*!
    \brief
        Serializes an unordered_map container.
    \param key
        The key to associate this container with.
    \param data
        The unordered_map container.
    \param pred
        The function used to sort the container to provide consistency between
        serializations. If unspecified, compares key values by default.
    *//******************************************************************/
    template <typename KeyType, typename ValueType, typename Predicate = std::nullptr_t>
        requires std::is_same_v<Predicate, std::nullptr_t> || std::predicate<Predicate, const std::pair<KeyType, ValueType>*, const std::pair<KeyType, ValueType>*>
    void Serialize(const std::string& key, const std::unordered_map<KeyType, ValueType>& data, Predicate pred = nullptr);

private:
    /*****************************************************************//*!
    \brief
        Writes all buffered entities to the json file.
    *//******************************************************************/
    void FlushEntities();

    /*****************************************************************//*!
    \enum LAYER
    \brief
        Identifies the type of json object pushed.
    *//******************************************************************/
    enum class LAYER
    {
        OBJ,
        ARRAY
    };

    /*****************************************************************//*!
    \brief
        Checks whether the top of the current layer stack is equal to
        a layer type.
    \param layer
        The expected layer type.
    \return
        True if the top layer in the stack matches the specified layer. False
        otherwise.
    *//******************************************************************/
    bool IsTopLayerEqualTo(LAYER layer);

    //! The file stream.
    std::ofstream ofs;
    //! The string buffer of the json writer.
    rj::StringBuffer sb;
    //! The json writer object.
    rj::PrettyWriter<rj::StringBuffer> writer;

    //! A buffer of entities inserted into this serializer, so we can resolve parenting references.
    std::map<int, std::vector<ecs::ConstEntityHandle>> entityChildLevelMap;

    //! The stack of layers to keep track of objects and arrays pushed
    std::stack<LAYER> layerStack;
    //! The number of entities written to this json
    int numEntities;
};

/*****************************************************************//*!
\class Deserializer
\brief
    Simplified interface for deserializing data in json format.
*//******************************************************************/
class Deserializer
{
public:
    /*****************************************************************//*!
    \brief
        Constructor.
    \param filepath
        The filepath to deserialize from. This should also include the file extension.
    *//******************************************************************/
    Deserializer(const std::string& filepath);

    /*****************************************************************//*!
    \brief
        Checks whether this deserializer is in a valid state to read from.
    \return
        True if this deserializer is in a valid state to read from. False otherwise.
    *//******************************************************************/
    bool IsValid() const;

    /*****************************************************************//*!
    \brief
        Enters a json object/array with the associated key.
    \param key
        The key of the json object/array.
    \return
        True if an object/array with the key exists. False otherwise.
    *//******************************************************************/
    bool PushAccess(const std::string& key);

    /*****************************************************************//*!
    \brief
        Exits the last json object/array on the stack.
    \return
        True if the exit operation was successful. False otherwise.
    *//******************************************************************/
    bool PopAccess();

    /*****************************************************************//*!
    \brief
        Enters a json array element at the specified index.
    \return
        True if the operation was successful. False otherwise.
    *//******************************************************************/
    bool PushArrayElementAccess(size_t index);

    /*****************************************************************//*!
    \brief
        Deserializes an entity.
    \param entity
        The entity to deserialize the data to.
    *//******************************************************************/
    bool Deserialize(ecs::EntityHandle entity);

    /*****************************************************************//*!
    \brief
        Serializes an object inheriting from PropertiesSerializeable, aka is registered
        to properties library. No json object is created in this version.
    \param obj
        The object to deserialize the data to.
    *//******************************************************************/
    bool Deserialize(PropertiesSerializeable* obj);

    /*****************************************************************//*!
    \brief
        Deserializes an object inheriting from PropertiesSerializeable, aka is registered
        to properties library. A json object with the provided key will be created in this version.
    \param key
        The key of the object.
    \param obj
        The object to deserialize the data to.
    *//******************************************************************/
    bool Deserialize(const std::string& key, PropertiesSerializeable* obj);

    /*****************************************************************//*!
    \brief
        Deserializes an object inheriting from ISerializeable.
        No json object is created in this version.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    bool Deserialize(ISerializeable* obj);

    /*****************************************************************//*!
    \brief
        Deserializes an object inheriting from ISerializeable.
        A json object with the provided key will be created in this version.
    \param key
        The key of the object.
    \param obj
        The data to be serialized.
    *//******************************************************************/
    bool Deserialize(const std::string& key, ISerializeable* obj);

    /*****************************************************************//*!
    \brief
        Deserializes a properties variable.
    \param key
        The key associated with the data.
    \param data
        The properties variable to deserialize the data to.
    *//******************************************************************/
    bool Deserialize(const std::string& key, property::data* data);

    /*****************************************************************//*!
    \brief
        Gets the number of elements in a json array.
    \param key
        The key associated with the array.
    \param size
        The variable to write the number of elements within the json array to.
    *//******************************************************************/
    bool GetArraySize(const std::string& key, size_t* size);

    /*****************************************************************//*!
    \brief
        Deserializes a variable.
    \param key
        The key associated with the variable.
    \param out
        The variable to deserialize the data to.
    *//******************************************************************/
    template <typename T>
    bool DeserializeVar(const std::string& key, T* out);

    /*****************************************************************//*!
    \brief
        Deserializes an array into a vector.
    \param key
        The key associated with the variable.
    \param out
        The vector to deserialize the data to.
    *//******************************************************************/
    template <typename T>
    bool DeserializeVar(const std::string& key, std::vector<T>* out);

    /*****************************************************************//*!
    \brief
        Deserializes a json object into a map.
    \param out
        The map to deserialize the data to.
    *//******************************************************************/
    template <typename KeyType, typename ValueType>
    bool DeserializeVar(const std::string& key, std::map<KeyType, ValueType>* out);

    /*****************************************************************//*!
    \brief
        Deserializes a json object into an unordered_map.
    \param out
        The unordered_map to deserialize the data to.
    *//******************************************************************/
    template <typename KeyType, typename ValueType>
    bool DeserializeVar(const std::string& key, std::unordered_map<KeyType, ValueType>* out);

    /*****************************************************************//*!
    \brief
        Deserializes a json object into a map or unordered_map, applying a custom
        deserialize operation per element found within the json object.
    \param key
        The key of the json object
    \param out
        The unordered_map to deserialize the data to.
    \param operationFunc
        The deserialize operation.
    *//******************************************************************/
    template <typename MapType, typename KeyType = typename MapType::key_type, typename ValueType = typename MapType::value_type, typename Operation = void(*)(Deserializer&, MapType*)>
        requires util::IsMap_v<MapType> && std::regular_invocable<Operation, Deserializer&, MapType*>
    bool DeserializeVar(const std::string& key, MapType* out, Operation operationFunc);

    /*****************************************************************//*!
    \brief
        Checks whether there are still entities available for reading.
    \return
        True if there is still an entity available for reading. False otherwise.
    *//******************************************************************/
    bool HasEntity() const;

private:
    /*****************************************************************//*!
    \brief
        Checks whether there are still entities available for reading.
    \return
        True if there is still an entity available for reading. False otherwise.
    *//******************************************************************/
    const rj::Value& GetCurrValue() const;

    //! Whether the deserializer is in a valid state to read data from.
    bool isValid;
    //! The json reader.
    rj::Document document;

    //! The current value that we're reading from
    std::stack<const rj::Value*> valueStack;
    //! The current entity index to read from
    int currentEntityIndex;

    //! Tracks entities' indexes for the purpose of parenting.
    std::map<int, ecs::EntityHash> entityIndexMap;
};

#include "Serializer.ipp"
