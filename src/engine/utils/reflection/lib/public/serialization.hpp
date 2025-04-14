#pragma once
#include "property.hpp"
#include "type.hpp"

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
    void*     object;
    const Property& property;
};

class Archive
{
public:
    template <typename T> Archive& operator<=>(T& alloc);
    inline Archive&                operator<=>(ObjectMember property);

private:
    bool load_archive = false;
};

class Serializer
{
public:
    virtual                                                        ~Serializer() = default;

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

    virtual void serialize(Archive& archive, const void* alloc) = 0;
    virtual void deserialize(Archive& archive, const void* alloc) = 0;

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

    if (load_archive)
        serializer->deserialize(*this, &alloc);
    else
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

    if (load_archive)
        serializer->deserialize(*this, member.property.ptr(member.object));
    else
        serializer->serialize(*this, member.property.ptr(member.object));

    return *this;
}
}