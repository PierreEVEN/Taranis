#pragma once
#include "type_id.hpp"

#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Enum
{
public:
    template <typename T> static Enum* register_enum()
    {
        return register_enum_internal(TypeId::create<T>(), sizeof(T));
    }

    void register_field(const std::string& field);

    static const ankerl::unordered_dense::map<TypeId, Enum*>& get_enums()
    {
        return get_registered_enums_internal();
    }

    const char* name() const
    {
        return type.name();
    }

    uint32_t stride() const
    {
        return size;
    }

    const ankerl::unordered_dense::set<std::string>& fields() const
    {
        return enum_fields;
    }

private:
    static Enum* register_enum_internal(const TypeId& enum_id, uint32_t in_size);

    Enum(const TypeId& in_type, uint32_t in_size) : type(in_type), size(in_size)
    {
    }

    static ankerl::unordered_dense::map<TypeId, Enum*>& get_registered_enums_internal();
    static ankerl::unordered_dense::map<TypeId, Enum*>* registered_enums;
    ankerl::unordered_dense::set<std::string>          enum_fields;

    TypeId   type;
    uint32_t size;
};
}