#include "serialization.hpp"

namespace Reflection
{
ankerl::unordered_dense::map<TypeInstance, Serializer*>* Serializer::serializers;

ankerl::unordered_dense::map<TypeInstance, Serializer*>& Serializer::get_serializers_internal()
{
    if (!serializers)
        serializers = new ankerl::unordered_dense::map<TypeInstance, Serializer*>();
    return *serializers;
}
}