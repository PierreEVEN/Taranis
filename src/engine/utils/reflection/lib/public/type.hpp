#pragma once
#include <memory>
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>

namespace Reflection
{
template <typename RClass> struct StaticTypeInfos
{
    constexpr static bool value = false;
};

using TypeId = size_t;

class Type
{
public:
    Type(std::string in_type_name, size_t in_type_size) : type_name(std::move(in_type_name)), type_size(in_type_size), type_id(std::hash<std::string>{}(type_name))
    {
    }

    template <typename Typename> static Type* register_type()
    {
        static_assert(StaticTypeInfos<Typename>::value, "Failed to register type : not a reflected type.");
        Type* new_type = new Type(StaticTypeInfos<Typename>::name, sizeof(Typename), make_type_id<Typename>());
        register_type_internal(new_type);
        return new_type;
    }

    template <typename T> static TypeId make_type_id()
    {
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
        return nullptr;
    }

    const char* name() const
    {
        return type_name.c_str();
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
    static ankerl::unordered_dense::map<TypeId, Type*>& get_types_internal();
    std::string                                         type_name;
    size_t                                              type_size = 0;
    TypeId                                              type_id   = 0;

    static ankerl::unordered_dense::map<TypeId, Type*>* types;
};
} // namespace Reflection