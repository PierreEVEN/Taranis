#pragma once

#include "type_id.hpp"
#include "type_specialization.hpp"

#include <optional>
#include <vector>

namespace Reflection
{
class Type;

class TypeInstance
{
    friend struct std::hash<TypeInstance>;

    static constexpr uint8_t FLAG_IS_CONST = 1 << 0;
    static constexpr uint8_t FLAG_IS_REF   = 1 << 1;

public:
    TypeInstance(const Type* in_base, uint32_t in_size) : size(in_size), base_type(in_base)
    {
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
        flags = (indirections << 4) + flags & 0xFF;
        return *this;
    }

    TypeInstance& set_template_specialization(const TypeSpecializationDescription& specialization)
    {
        template_specialization = specialization;
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

    const Type* base() const
    {
        return base_type;
    }

    std::string display() const;

    bool operator==(const TypeInstance& o) const
    {
        return
            flags == o.flags &&
            size == o.size &&
            base_type == o.base_type &&
            template_specialization == o.template_specialization;
    }

private:
    // 4 last bytes are the number of ptr indirections
    uint8_t                                      flags     = 0;
    uint32_t                                     size      = 0;
    const Type*                                  base_type = nullptr;
    std::optional<TypeSpecializationDescription> template_specialization;
};
} // namespace std


template <> struct std::hash<Reflection::TypeInstance>
{
    size_t operator()(const Reflection::TypeInstance& val) const noexcept
    {
        size_t hash;
        hash_combine(hash, val.flags);
        hash_combine(hash, val.size);
        hash_combine(hash, val.base_type);
        hash_combine(hash, val.template_specialization);
        return hash;
    }
};