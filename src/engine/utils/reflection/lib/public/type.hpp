#pragma once
#include "type_id.hpp"
#include "type_instance.hpp"

#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Type;

class Type
{
public:
    static TypeId make_type_id(const char* type_name)
    {
        return TypeId::create(type_name);
    }

    Type(const char* in_type_name, uint32_t in_type_size) : type_size(in_type_size), type_id(make_type_id(in_type_name))
    {
    }

    template <typename Typename> static Type* register_type()
    {
        static_assert(StaticTypeInfos<Typename>::value, "Failed to register type : not a reflected type.");
        Type* new_type = new Type(StaticTypeInfos<Typename>::name, sizeof(Typename));
        register_type_internal(new_type);
        return new_type;
    }

    template <typename Base, typename Alias> static void register_type_alias()
    {
        static_assert(StaticTypeInfos<Base>::value, "Failed to register type : not a reflected type.");
        static_assert(StaticTypeInfos<Alias>::value, "Failed to register type : not a reflected type.");
        get_types_aliases_internal().emplace(make_type_id<Alias>(), get_type(make_type_id<Base>()));
    }

    template <typename Base, typename First, typename Second, typename... Next> static void register_type_alias()
    {
        register_type_alias<Base, First>();
        register_type_alias<Base, Second, Next...>();
    }

    template <typename Typename, typename... Args> static Type* register_type_template()
    {
        static_assert(StaticTypeInfos<Typename>::value, "Failed to register type : not a reflected type.");
        auto it = get_types_internal().find(make_type_id(StaticTypeInfos<Typename>::name));
        if (it == get_types_internal().end())
        {
            Type* new_type          = new Type(StaticTypeInfos<Typename>::name, 0);
            new_type->template_type = true;
            register_type_internal(new_type);
            return new_type;
        }
        return nullptr;
    }

public:
    template <typename T> static TypeId make_type_id()
    {
        static_assert(StaticTypeInfos<T>::value, "Cannot get type id : this type is not a reflected type.");
        return make_type_id(StaticTypeInfos<T>::name);
    }

    template <typename T> static TypeInstance make_type_instance()
    {
        static_assert(StaticTypeInfos<T>::value, "Cannot get type id : this type is not a reflected type.");
        return TypeInstance(get_type(make_type_id<T>()), sizeof(T));
    }

    template <typename T> static Type* get_type()
    {
        return get_type(make_type_id<T>());
    }

    static Type* get_type(const TypeId& type_id)
    {
        auto it = get_types_internal().find(type_id);
        if (it != get_types_internal().end())
            return it->second;

        auto alias = get_types_aliases_internal().find(type_id);
        if (alias != get_types_aliases_internal().end())
            return alias->second;

        return nullptr;
    }

    const char* name() const
    {
        return type_id.name();
    }

    bool is_template_type() const
    {
        return template_type;
    }

    TypeId id() const
    {
        return type_id;
    }

    uint32_t stride() const
    {
        return type_size;
    }

    static const ankerl::unordered_dense::map<TypeId, Type*>& get_types()
    {
        return get_types_internal();
    }

protected:
    static void register_type_internal(Type* in_type);

private:
    struct InstanceData
    {
        class Serializer* serializers;
    };

    bool template_type = false;

    uint32_t type_size = 0;
    TypeId   type_id;

    static ankerl::unordered_dense::map<TypeId, Type*>& get_types_internal();
    static ankerl::unordered_dense::map<TypeId, Type*>* types;
    static ankerl::unordered_dense::map<TypeId, Type*>& get_types_aliases_internal();
    static ankerl::unordered_dense::map<TypeId, Type*>* types_aliases;
};

template <typename T> void TypeSpecialization::push()
{
    auto type = Type::get_type(Type::make_type_id<T>());
    if (!type)
        std::cerr << "ask for type : " << StaticTypeInfos<T>::name << "\n";
    assert(type && "TODO : handle delayed registered type for template specializations");
    arguments.emplace_back(type);
}
} // namespace Reflection