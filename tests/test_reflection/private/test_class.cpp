#include "test_class.hpp"

struct NativeTypeRecorder
{
    NativeTypeRecorder()
    {
        Reflection::Type::register_type<uint8_t>();
        Reflection::Type::register_type_alias<uint8_t, unsigned char>();
        Reflection::Type::register_type<uint16_t>();
        Reflection::Type::register_type<uint32_t>();
        Reflection::Type::register_type_alias<uint32_t, unsigned int>();
        Reflection::Type::register_type<uint64_t>();

        Reflection::Type::register_type<int8_t>();
        Reflection::Type::register_type_alias<int8_t, char>();
        Reflection::Type::register_type<int16_t>();
        Reflection::Type::register_type<int32_t>();
        Reflection::Type::register_type_alias<int32_t, int>();
        Reflection::Type::register_type<int64_t>();

        Reflection::Type::register_type<bool>();
        Reflection::Type::register_type<float>();
        Reflection::Type::register_type<double>();
        Reflection::Type::register_type<std::string>();
        std::cout << "TODO : move this in native_type.cpp \n";
    }
};
static NativeTypeRecorder native_type_recorder;
