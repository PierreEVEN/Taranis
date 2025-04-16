#pragma once
#include "small_string.hpp"

#include <string>

namespace Reflection
{
template <typename RClass> struct StaticTypeInfos
{
    constexpr static bool value    = false;
    constexpr static bool is_class = false;
};

class TypeId final
{
    friend struct std::hash<TypeId>;

public:
    static TypeId create(const char* name)
    {
        return {name};
    }

    TypeId()              = default;
    TypeId(const TypeId&) = default;
    TypeId(TypeId&&)      = default;
    ~TypeId()             = default;

    TypeId& operator=(const TypeId&) = default;
    TypeId& operator=(TypeId&&)      = default;

    bool operator==(const TypeId& other) const
    {
        return hash == other.hash && raw_name == other.raw_name;
    }

    operator bool() const
    {
        return !raw_name.empty();
    }

    const char* name() const
    {
        return raw_name.c_str();
    }

private:
    size_t      hash = 0;
    SmallString raw_name;

    TypeId(const char* in_raw_name) : hash(std::hash<SmallString>()(in_raw_name)), raw_name(in_raw_name)
    {
    }
};
}

template <> struct std::hash<Reflection::TypeId>
{
    size_t operator()(const Reflection::TypeId& val) const noexcept
    {
        return val.hash;
    }
};