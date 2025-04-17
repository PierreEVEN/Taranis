#pragma once

#include "type_id.hpp"

#include <cassert>
#include <optional>
#include <vector>

namespace Reflection
{
class TypeInstance;
class Type;

class TypeSpecializationDescription
{
  public:
    TypeSpecializationDescription() = default;

    TypeSpecializationDescription(std::initializer_list<TypeInstance> in_types) : types(in_types)
    {
    }

    void push(const TypeInstance& type)
    {
        types.push_back(type);
    }

    bool operator==(const TypeSpecializationDescription& other) const;

    const std::vector<TypeInstance>& get_types() const
    {
        return types;
    }

  private:
    std::vector<TypeInstance> types;
};

class TypeSpecialization
{
  public:
    template <typename T> void push();

    TypeSpecialization() = default;

    TypeSpecialization(size_t in_size) : size(in_size)
    {
    }

    size_t stride() const
    {
        return size;
    }

    const std::vector<TypeInstance>& get_args() const
    {
        return arguments;
    }

  private:
    size_t                    size = 0;
    std::vector<TypeInstance> arguments;
};

class TypeInstance
{
    friend struct std::hash<TypeInstance>;

    static constexpr uint8_t FLAG_IS_CONST = 1 << 0;
    static constexpr uint8_t FLAG_IS_REF   = 1 << 1;

  public:
    TypeInstance(const Type* in_base, uint32_t in_size) : size(in_size), base_type(in_base)
    {
        assert(base_type);
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
        return flags == o.flags && size == o.size && base_type == o.base_type && template_specialization == o.template_specialization;
    }

    bool operator!=(const TypeInstance& o) const
    {
        return !operator==(o);
    }

  private:
    // 4 last bytes are the number of ptr indirections
    uint8_t                                      flags     = 0;
    uint32_t                                     size      = 0;
    const Type*                                  base_type = nullptr;
    std::optional<TypeSpecializationDescription> template_specialization;
};
} // namespace Reflection

template <> struct std::hash<Reflection::TypeSpecializationDescription>
{
    size_t operator()(const Reflection::TypeSpecializationDescription& c) const noexcept
    {
        size_t result = 0;
        for (const auto& type : c.get_types())
            hash_combine(result, type);
        return result;
    }
};

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