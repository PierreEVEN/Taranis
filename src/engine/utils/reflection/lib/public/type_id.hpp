#pragma once
#include "small_string.hpp"

#include <iostream>
#include <string>
#include <typeindex>
#include <logger.hpp>

namespace Reflection
{
template <typename> struct StaticTypeInfos
{
    constexpr static bool value    = false;
    constexpr static bool        is_class = false;
    constexpr static bool        is_enum = false;
    constexpr static const char* name     = nullptr;
};

class TypeId final
{
    friend struct std::hash<TypeId>;

public:
    template <typename T> static TypeId create()
    {
        ASSERT(StaticTypeInfos<T>::value, "{} is not a reflected type !", typeid(T).name());
        return TypeId(StaticTypeInfos<T>::name, StaticTypeInfos<T>::name ? std::hash<std::string>()(StaticTypeInfos<T>::name) : 0);
    }

    TypeId() : type_name(nullptr)
    {
    }

    TypeId(const TypeId&) = default;
    TypeId(TypeId&&)      = default;
    ~TypeId()             = default;

    TypeId& operator=(const TypeId&) = default;
    TypeId& operator=(TypeId&&)      = default;

    bool operator==(const TypeId& other) const
    {
        return hash_code == other.hash_code && type_name == other.type_name;
    }

    operator bool() const
    {
        return type_name != nullptr;
    }

    const char* name() const
    {
        return type_name;
    }

private:
    const char*     type_name;
    size_t          hash_code;

    TypeId(const char* name, const size_t hash) : type_name(name), hash_code(hash)
    {
    }
};
}

template <> struct std::hash<Reflection::TypeId>
{
    size_t operator()(const Reflection::TypeId& val) const noexcept
    {
        return val.hash_code;
    }
};