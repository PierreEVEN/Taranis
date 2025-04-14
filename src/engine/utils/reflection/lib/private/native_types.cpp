#include "type.hpp"
#include "native_types.hpp"

namespace Reflection
{
NativeTypeRecorder::NativeTypeRecorder()
{
    Type::register_type<uint8_t>();
    Type::register_type_alias<uint8_t, unsigned char>();
    Type::register_type<uint16_t>();
    Type::register_type<uint32_t>();
    Type::register_type_alias<uint32_t, unsigned int>();
    Type::register_type<uint64_t>();

    Type::register_type<int8_t>();
    Type::register_type_alias<int8_t, char>();
    Type::register_type<int16_t>();
    Type::register_type<int32_t>();
    Type::register_type_alias<int32_t, int>();
    Type::register_type<int64_t>();

    Type::register_type<bool>();
    Type::register_type<float>();
    Type::register_type<double>();
    Type::register_type<std::string>();
}
} // namespace Reflection