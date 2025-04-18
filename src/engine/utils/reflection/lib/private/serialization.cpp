#include "serialization.hpp"

namespace Reflection
{
ankerl::unordered_dense::map<TypeId, Serializer*>* Serializer::serializers;

ankerl::unordered_dense::map<TypeId, Serializer*>& Serializer::get_serializers_internal()
{
    if (!serializers)
        serializers = new ankerl::unordered_dense::map<TypeId, Serializer*>();
    return *serializers;
}
}