#include "test_class.hpp"


struct DefaultTypeRecorder
{
    DefaultTypeRecorder()
    {
        Reflection::Type::register_type<float>();
        Reflection::Type::register_type<bool>();
        Reflection::Type::register_type<std::vector<float>>();
    }
};
static DefaultTypeRecorder default_type_recorder;
