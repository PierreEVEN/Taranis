#include "type.hpp"

#include <iostream>

namespace Reflection
{
ankerl::unordered_dense::map<TypeId, Type*>* Type::types;
ankerl::unordered_dense::map<TypeId, Type*>* Type::types_aliases;

ankerl::unordered_dense::map<TypeId, Type*>& Type::get_types_internal()
{
    if (!types)
        types = new ankerl::unordered_dense::map<TypeId, Type*>();
    return *types;
}

ankerl::unordered_dense::map<TypeId, Type*>& Type::get_types_aliases_internal()
{
    if (!types_aliases)
        types_aliases = new ankerl::unordered_dense::map<TypeId, Type*>();
    return *types_aliases;
}

void Type::register_type_internal(Type* in_type)
{
    if (!get_types_internal().emplace(in_type->id(), in_type).second)
    {
        std::cerr << "Failed to register type " << in_type->name() << "\n";
        exit(-1);
    }
}

} // namespace Reflection