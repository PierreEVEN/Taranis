#pragma once
#include "test_class.gen.hpp"

#include "native_types.hpp"
#include "property.hpp"
#include "serialization.hpp"

class TestChild : public std::vector<float>
{
    REFLECT_BODY()

    float untracked_value;

    RPROPERTY()
    std::vector<std::string> string_vector;

};

class MyTestClass
{
    REFLECT_BODY()

public:
    RPROPERTY()
    float test_float;

    RPROPERTY()
    std::vector<float> test_vector_float;

    RPROPERTY()
    std::vector<std::vector<double>> test_vector_vector_float;

    RPROPERTY()
    std::string test_string;

private:
    RPROPERTY()
    MyTestClass* test_class_ptr;

    RPROPERTY()
    TestChild child_value;

    RPROPERTY()
    bool test_bool;
};

inline void serialize()
{
    MyTestClass test_instance;

    Reflection::Archive out_archive;

    float test_prop = 5;
    out_archive <=> test_prop;

    for (const auto& prop : test_instance.get_class()->get_properties())
    {
        out_archive <=> prop.second;
    }
}
