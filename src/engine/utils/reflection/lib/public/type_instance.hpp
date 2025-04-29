#pragma once

#include "type_id.hpp"

#include <string>

namespace Reflection
{
class TypeInstance;
class Type;

class TypeInstance
{
    friend struct std::hash<TypeInstance>;

    static constexpr uint8_t FLAG_IS_CONST = 1 << 0;
    static constexpr uint8_t FLAG_IS_REF   = 1 << 1;

public:
    template <typename T> static TypeInstance create()
    {
        return TypeInstance(TypeId::create<T>(), sizeof(T));
    }

    TypeInstance& set_const()
    {
        flags |= FLAG_IS_CONST;
        return *this;
    }

    TypeInstance& set_ref()
    {
        flags |= FLAG_IS_REF;
        return *this;
    }

    TypeInstance& set_ptr_indirections(uint8_t indirections)
    {
        flags = (indirections << 4) + (flags & 0xFF);
        return *this;
    }

    uint32_t stride() const
    {
        return size;
    }

    bool is_const() const
    {
        return flags & FLAG_IS_CONST;
    }

    bool is_ref() const
    {
        return flags & FLAG_IS_REF;
    }

    uint8_t get_ptr_indirections() const
    {
        return flags >> 4;
    }

    const TypeId& id() const
    {
        return base_id;
    }

    std::string display() const;

    bool operator==(const TypeInstance& o) const
    {
        return base_id == o.base_id && flags == o.flags && size == o.size;
    }

    bool operator!=(const TypeInstance& o) const
    {
        return !operator==(o);
    }

private:
    TypeInstance(TypeId id, uint32_t in_size) : size(in_size), base_id(id)
    {
    }

    // 4 last bytes are the number of ptr indirections
    uint8_t  flags = 0;
    uint32_t size  = 0;
    TypeId   base_id;
};
} // namespace Reflection

template <> struct std::hash<Reflection::TypeInstance>
{
    size_t operator()(const Reflection::TypeInstance& val) const noexcept
    {
        size_t hash = 0;
        hash_combine(hash, val.flags);
        hash_combine(hash, val.size);
        hash_combine(hash, val.base_id);
        return hash;
    }
};