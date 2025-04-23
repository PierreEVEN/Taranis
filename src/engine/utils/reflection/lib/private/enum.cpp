#include "enum.hpp"

#include <iostream>

namespace Reflection
{
ankerl::unordered_dense::map<TypeId, Enum*>* Enum::registered_enums = nullptr;

void Enum::register_field(const std::string& field)
{
    enum_fields.emplace(field);
}

Enum* Enum::register_enum_internal(const TypeId& enum_id, uint32_t in_size)
{
    Enum* object = new Enum(enum_id, in_size);
    if (!get_registered_enums_internal().emplace(enum_id, object).second)
    {
        std::cerr << "Failed to register enum'" << enum_id.name() << "\n";
        exit(-1);
    }
    return object;
}

ankerl::unordered_dense::map<TypeId, Enum*>& Enum::get_registered_enums_internal()
{
    if (!registered_enums)
        registered_enums = new ankerl::unordered_dense::map<TypeId, Enum*>();
    return *registered_enums;
}
}