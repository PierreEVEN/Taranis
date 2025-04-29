#pragma once
#include "property.hpp"
#include "stream.hpp"
#include "type.hpp"

#include <iostream>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Property;
class Archive;

class Field
{
    friend Archive;

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

    template <typename T, typename Serializer, typename... Args> static void register_serializer(Args&&... args)
    {
        get_serializers_internal().insert_or_assign(TypeId::create<T>(), new Serializer(std::forward<Args>(args)...));
    }

    template <typename Serializer, typename... Args> static void register_serializer(const TypeId& type_instance, Args&&... args)
    {
        get_serializers_internal().insert_or_assign(type_instance, new Serializer(std::forward<Args>(args)...));
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
        Serializer* serializer = Serializer::get(TypeId::create<T>());
        if (!serializer)
        {
            std::cerr << "No serializer for raw type " << TypeId::create<T>().name() << "\n";
            return *this;
        }

        serializer->serialize(*this, &alloc);

        return *this;
    }

    Archive& operator<=>(Field member)
    {
        auto& type = member.property.get_type_instance();

        Serializer* serializer = Serializer::get(type.id());
        if (!serializer)
        {
            std::cerr << "No serializer for " << type.display() << "\n";
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

class StringSerializer : public Serializer
{
  public:
    void serialize(Archive& archive, void* alloc) override
    {
        std::string& data   = *static_cast<std::string*>(alloc);
        size_t       length = data.size();
        archive <=> length;
        if (length == 0)
            return;
        if (archive.is_reading())
            data.resize(length);
        archive.archive_raw(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.c_str())), length);
    }
};

template <typename T> class VectorSerializer : public Serializer
{
  public:
    void serialize(Archive& archive, void* alloc) override
    {
        Serializer* serializer = get(TypeId::create<T>());
        if (!serializer)
        {
            std::cerr << "There is no serializer for type " << TypeId::create<T>().name() << "\n";
            return;
        }

        std::vector<T>& data   = *static_cast<std::vector<T>*>(alloc);
        size_t          length = data.size();
        archive <=> length;
        if (length == 0)
            return;
        if (archive.is_reading())
        {
            data.clear();
            data.reserve(length);
            for (size_t i = 0; i < length; ++i)
            {
                T item;
                serializer->serialize(archive, &item);
                data.emplace_back(std::move(item));
            }
        }
        else
            for (size_t i = 0; i < length; ++i)
                serializer->serialize(archive, &data[i]);
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