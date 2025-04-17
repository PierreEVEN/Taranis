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
        auto  it = get_types_internal().find(make_type_id(StaticTypeInfos<Typename>::name));
        Type* new_type;
        if (it != get_types_internal().end())
            new_type = it->second;
        else
        {
            new_type                = new Type(StaticTypeInfos<Typename>::name, 0);
            new_type->template_type = true;
            register_type_internal(new_type);
        }
        TypeSpecializationDescription description;
        TypeSpecialization            specialization(sizeof(Typename));
        new_type->register_template_args<Args...>(description, specialization);
        new_type->template_specializations.emplace(description, specialization);
        return new_type;
    }

    template <typename T, typename... Args> void set_serializer(Args&&...)
    {

    }

private:
    template <typename FirstArg> void register_template_args(TypeSpecializationDescription& description, TypeSpecialization& specialization)
    {
        static_assert(StaticTypeInfos<FirstArg>::value, "Template typename of reflected types should also be reflected types.");
        description.push(make_type_id<FirstArg>());
        specialization.push<FirstArg>();
    }

    template <typename FirstArg, typename SecondArg, typename... Args> void register_template_args(TypeSpecializationDescription& description, TypeSpecialization& specialization)
    {
        register_template_args<FirstArg>(description, specialization);
        register_template_args<SecondArg, Args...>(description, specialization);
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

    static Type* get_type(TypeId type_id)
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

    const ankerl::unordered_dense::map<TypeSpecializationDescription, TypeSpecialization>& get_specializations() const
    {
        return template_specializations;
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

    bool                                                                            template_type = false;
    ankerl::unordered_dense::map<TypeSpecializationDescription, TypeSpecialization> template_specializations;

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