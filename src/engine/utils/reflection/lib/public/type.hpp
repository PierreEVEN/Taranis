#pragma once
#include <assert.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Type;

template <typename RClass> struct StaticTypeInfos
{
    constexpr static bool value    = false;
    constexpr static bool is_class = false;
};

using TypeId = size_t;

class TypeSpecializationDescription
{
public:
    TypeSpecializationDescription() = default;

    TypeSpecializationDescription(std::initializer_list<TypeId> in_types) : types(in_types)
    {
    }

    void push(TypeId type)
    {
        types.emplace_back(type);
    }

    bool operator==(const TypeSpecializationDescription& other) const
    {
        auto ita = other.types.begin();
        auto itb = types.begin();
        for (; ita != other.types.end() && itb != types.end(); ++ita, ++itb)
            if (*ita != *itb)
                return false;
        return ita == other.types.end() && itb == types.end();
    }

    const std::vector<TypeId>& get_types() const
    {
        return types;
    }

private:
    std::vector<TypeId> types;
};
} // namespace Reflection

namespace std
{
template <class T> void hash_combine(::size_t& s, const T& v)
{
    hash<T> h;
    s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

template <> struct hash<Reflection::TypeSpecializationDescription>
{
    size_t operator()(const Reflection::TypeSpecializationDescription& c) const noexcept
    {
        size_t result = 0;
        for (const auto& type : c.get_types())
            hash_combine(result, type);
        return result;
    }
};
} // namespace std

namespace Reflection
{

class TypeSpecialization
{
public:
    template <typename T> void push();

    TypeSpecialization() = default;

    //@TODO : HANDLE TYPE SPECIALIZATION (we should be able to handle Type<Type<>> recursively
    TypeSpecialization(size_t in_size)
        : size(in_size)
    {
    }

    size_t stride() const
    {
        return size;
    }

    const std::vector<Type*>& get_args() const
    {
        return arguments;
    }

private:
    size_t             size = 0;
    std::vector<Type*> arguments;
};

class Type
{
public:
    Type(std::string in_type_name, size_t in_type_size) : type_name(std::move(in_type_name)), type_size(in_type_size), type_id(std::hash<std::string>{}(type_name))
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

    static TypeId make_type_id(const char* type_name)
    {
        return std::hash<std::string>{}(type_name);
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
        if (it != get_types_aliases_internal().end())
            return it->second;

        return nullptr;
    }

    const char* name() const
    {
        return type_name.c_str();
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

    size_t stride() const
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
    bool                                                                            template_type = false;
    ankerl::unordered_dense::map<TypeSpecializationDescription, TypeSpecialization> template_specializations;

    std::string type_name;
    size_t      type_size = 0;
    TypeId      type_id   = 0;

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