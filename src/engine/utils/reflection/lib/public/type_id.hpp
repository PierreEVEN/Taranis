#pragma once
#include "small_string.hpp"

#include <typeindex>

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
    template <typename T> static TypeId create()
    {
        return TypeId(typeid(T));
    }

    TypeId() : type_id(typeid(void))
    {
    }

    TypeId(const TypeId&) = default;
    TypeId(TypeId&&)      = default;
    ~TypeId()             = default;

    TypeId& operator=(const TypeId&) = default;
    TypeId& operator=(TypeId&&)      = default;

    bool operator==(const TypeId& other) const
    {
        return type_id == other.type_id;
    }

    operator bool() const
    {
        return type_id != typeid(void);
    }

    const char* name() const
    {
        return type_id.name();
    }

private:
    std::type_index type_id;

    TypeId(std::type_index type_index) : type_id(type_index)
    {
    }
};
}

template <> struct std::hash<Reflection::TypeId>
{
    size_t operator()(const Reflection::TypeId& val) const noexcept
    {
        return val.type_id.hash_code();
    }
};