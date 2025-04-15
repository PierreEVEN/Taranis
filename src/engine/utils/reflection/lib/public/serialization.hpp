#pragma once
#include "property.hpp"
#include "stream.hpp"
#include "type.hpp"
#include <filesystem>

#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Property;

class Field
{
    friend class Archive;

public:
    Field(void* in_object, const Property& in_property) : object(in_object), property(in_property)
    {
    }

private:
    void*           object;
    const Property& property;
};

class Serializer
{
public:
    virtual ~Serializer() = default;

    template <typename BaseClass, typename Serializer> static void register_serializer()
    {
        get_serializers_internal().emplace(Type::make_type_id<BaseClass>(), new Serializer{});
    }

    static Serializer* get(const TypeId& type)
    {
        auto it = get_serializers_internal().find(type);
        if (it != get_serializers_internal().end())
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


class Archive final
{
  public:

    template<typename T, typename...Args> static Archive create(Args&&... args)
    {
        Archive archive;
        archive.stream       = std::make_unique<T>(std::forward<Args>(args)...);
        return archive;
    }

    void archive_raw(uint8_t* data, size_t size) const
    {
        assert(size == stream->stream_bytes(data, size));
    }

    template <typename T> Archive& operator<=>(T& alloc)
    {
        static_assert(StaticTypeInfos<T>::value, "This type is not a reflected type");

        Serializer* serializer = Serializer::get(Type::make_type_id<T>());
        if (!serializer)
        {
            std::cerr << "No serializer for " << Reflection::StaticTypeInfos<T>::name << "\n";
            return *this;
        }

        serializer->serialize(*this, &alloc);

        return *this;
    }

    Archive& operator<=>(Field member)
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

  private:
    Archive() = default;

    std::unique_ptr<Io::Stream> stream;
};

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
        T* object = static_cast<T*>(alloc);

        for (const auto& property : object->get_class()->get_properties())
            archive <=> Field{alloc, property.second};
    }
};
}