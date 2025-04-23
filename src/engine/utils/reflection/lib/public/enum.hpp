#pragma once
#include "type.hpp"
#include "type_id.hpp"

#include <ankerl/unordered_dense.h>

namespace Reflection
{
class Enum : public Type
{
public:
    template <typename T> static Enum* register_enum()
    {
        Enum* new_enum = register_enum_internal(TypeId::create<T>(), sizeof(T));
        register_type_internal(new_enum);
        return new_enum;
    }

    void register_field(const std::string& field);

    static const ankerl::unordered_dense::map<TypeId, Enum*>& get_enums()
    {
        return get_registered_enums_internal();
    }

    const ankerl::unordered_dense::set<std::string>& fields() const
    {
        return enum_fields;
    }

private:
    static Enum* register_enum_internal(const TypeId& enum_id, uint32_t in_size);

    Enum(const TypeId& in_type, uint32_t in_size) : Type(in_type, in_size)
    {
    }

    static ankerl::unordered_dense::map<TypeId, Enum*>& get_registered_enums_internal();
    static ankerl::unordered_dense::map<TypeId, Enum*>* registered_enums;
    ankerl::unordered_dense::set<std::string>           enum_fields;
};
}