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
    template <typename Typename> static Type* register_type()
    {
        static_assert(StaticTypeInfos<Typename>::value, "Failed to register type : not a reflected type.");
        Type* new_type = new Type(TypeId::create<Typename>(), sizeof(Typename));
        register_type_internal(new_type);
        return new_type;
    }

    template <typename Base, typename Alias> static void register_type_alias()
    {
        static_assert(StaticTypeInfos<Base>::value, "Failed to register type : not a reflected type.");
        static_assert(StaticTypeInfos<Alias>::value, "Failed to register type : not a reflected type.");
        get_types_aliases_internal().emplace(TypeId::create<Alias>(), get_type(TypeId::create<Base>()));
    }

    template <typename Base, typename First, typename Second, typename... Next> static void register_type_alias()
    {
        register_type_alias<Base, First>();
        register_type_alias<Base, Second, Next...>();
    }

public:
    template <typename T> static TypeInstance make_type_instance()
    {
        return TypeInstance(get_type(TypeId::create<T>()), sizeof(T));
    }

    template <typename T> static Type* get_type()
    {
        return get_type(TypeId::create<T>());
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

    const TypeId& id() const
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

    Type(TypeId in_type_id, uint32_t in_type_size) : type_size(in_type_size), type_id(in_type_id)
    {
    }

private:
    uint32_t type_size = 0;
    TypeId   type_id;

    static ankerl::unordered_dense::map<TypeId, Type*>& get_types_internal();
    static ankerl::unordered_dense::map<TypeId, Type*>* types;
    static ankerl::unordered_dense::map<TypeId, Type*>& get_types_aliases_internal();
    static ankerl::unordered_dense::map<TypeId, Type*>* types_aliases;
};
} // namespace Reflection