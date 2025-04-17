#pragma once
#include "property.hpp"
#include "stream.hpp"
#include "type.hpp"
#include "type_instance.hpp"

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

    template <typename Serializer, typename... Args> static void register_serializer(const TypeInstance& type_instance, Args&&... args)
    {
        get_serializers_internal().insert_or_assign(type_instance, new Serializer(std::forward<Args>(args)...));
    }

    static Serializer* get(const TypeInstance& type)
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
    static ankerl::unordered_dense::map<TypeInstance, Serializer*>& get_serializers_internal();
    static ankerl::unordered_dense::map<TypeInstance, Serializer*>* serializers;
};


class Archive final
{
public:
    template <typename T, typename... Args> static Archive create(Args&&... args)
    {
        Archive archive;
        archive.stream = std::make_unique<T>(std::forward<Args>(args)...);
        return archive;
    }

    Archive(Archive&)  = delete;
    Archive(Archive&&) = default;

    ~Archive()
    {
        stream->flush();
    }

    bool is_reading() const
    {
        return stream->get_mode() == Io::Stream::Mode::Input;
    }

    void archive_raw(uint8_t* data, size_t size) const
    {
        size_t serialized = stream->stream_bytes(data, size);
        if (size != serialized)
        {
            std::cerr << "Failed to serialize property : expected " << size << ", got " << serialized
                << " bytes\n";
        }
    }

    template <typename T> Archive& operator<=>(T& alloc)
    {
        static_assert(StaticTypeInfos<T>::value, "This type is not a reflected type");

        Serializer* serializer = Serializer::get(Type::make_type_instance<T>());
        if (!serializer)
        {
            std::cerr << "No serializer for raw type " << StaticTypeInfos<T>::name << "\n";
            return *this;
        }

        serializer->serialize(*this, &alloc);

        return *this;
    }

    Archive& operator<=>(Field member)
    {
        auto& type = member.property.get_type_instance();

        Serializer* serializer = Serializer::get(type);
        if (!serializer)
        {
            std::cerr << "No serializer for " << type.display() << "\n";
            return *this;
        }

        if (type.get_ptr_indirections() > 0)
        {
            std::cerr << "Cannot serialize pointer types " << type.display() << "\n";
            return *this;
        }

        if (type.is_ref())
        {
            std::cerr << "Cannot serialize reference types " << type.display() << "\n";
            return *this;
        }

        serializer->serialize(*this, member.property.ptr(member.object));

        return *this;
    }

    void flush() const
    {
        stream->flush();
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