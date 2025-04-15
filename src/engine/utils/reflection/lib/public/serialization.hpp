#pragma once
#include "property.hpp"
#include "type.hpp"
#include <filesystem>

#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Property;

class ObjectMember
{
    friend class Archive;

public:
    ObjectMember(void* in_object, const Property& in_property) : object(in_object), property(in_property)
    {
    }

private:
    void*           object;
    const Property& property;
};

class DataStream
{
    
};

class FileDataStream : public DataStream
{
    FileDataStream(const std::filesystem::path& source_file)
    {
    }
};

class MemoryDataStream : public DataStream
{
    MemoryDataStream(uint8_t* source, size_t source_size)
    {
    }
};

class Archive
{
public:
    Archive from_file(const std::filesystem::path& path)
    {
        Archive archive;
        archive.load_archive = true;
        return archive;
    }

    Archive from_bytes(const uint8_t* bytes, size_t length)
    {
        Archive archive;
        archive.load_archive = true;
        return archive;
    }

    virtual void archive_raw(const uint8_t* string, size_t size);

    template <typename T> Archive& operator<=>(T& alloc);
    inline Archive&                operator<=>(ObjectMember property);

private:
    bool load_archive = false;
};

class Serializer
{
public:
    virtual ~Serializer() = default;

    template <typename BaseClass, typename Serializer> static void register_serializer()
    {
        serializers->emplace(Type::make_type_id<BaseClass>(), new Serializer{});
    }

    static Serializer* get(const TypeId& type)
    {
        auto it = serializers->find(type);
        if (it != serializers->end())
            return it->second;
        return nullptr;
    }

    virtual void serialize(Archive& archive, void* alloc) = 0;

    virtual void serialize_network(Archive& archive, void* alloc)
    {
        return serialize(archive, alloc);
    }

private:
    static ankerl::unordered_dense::map<TypeId, Serializer*>& get_serializers_internal();
    static ankerl::unordered_dense::map<TypeId, Serializer*>* serializers;
};


template <typename T> Archive& Archive::operator<=>(T& alloc)
{
    static_assert(StaticTypeInfos<T>::value, "This type is not a reflected type");

    Serializer* serializer = Serializer::get(Type::make_type_id<T>());
    if (!serializer)
    {
        std::cerr << "No serializer for " << StaticTypeInfos<T>::name << "\n";
        return *this;
    }

    serializer->serialize(*this, &alloc);

    return *this;
}

Archive& Archive::operator<=>(ObjectMember member)
{
    Serializer* serializer = Serializer::get(member.property.get_type_instance().base()->id());
    if (!serializer)
    {
        std::cerr << "No serializer for " << member.property.get_type_instance().base()->name() << "\n";
        return *this;
    }

    serializer->serialize(*this, member.property.ptr(member.object));

    return *this;
}

template <typename T> class RawSerializer : public Serializer
{
public:
    void serialize(Archive& archive, void* alloc) override
    {
        archive.archive_raw(static_cast<uint8_t*>(alloc), sizeof(T));
    }
};

template <typename T> class ClassSerializer : public Serializer
{
public:
    void serialize(Archive& archive, void* alloc) override
    {
        for (const auto& property : static_cast<T*>(alloc)->get_class()->get_properties())
            archive <=> ObjectMember{alloc, property.second};
    }
};

}