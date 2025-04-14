#include "test_class.hpp"


struct DefaultTypeRecorder
{
    DefaultTypeRecorder()
    {
        Reflection::Type::register_type<float>();
        Reflection::Type::register_type<char>();
        Reflection::Type::register_type<bool>();
        Reflection::Type::register_type<std::string>();
        Reflection::Type::register_type_template<std::vector<bool>, bool, std::string>();
        Reflection::Type::register_type_template<std::vector<bool>, float>();
        Reflection::Type::register_type_template<std::vector<bool>, char>();
    }
};
static DefaultTypeRecorder default_type_recorder;
