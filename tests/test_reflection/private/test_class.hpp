#pragma once
#include "test_class.gen.hpp"

#include "native_types.hpp"
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

    RPROPERTY(Transient)
    float test_float_transient;

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