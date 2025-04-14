#pragma once
#include "test_class.gen.hpp"

#include "native_types.hpp"

class TestChild : public std::vector<float>
{
    REFLECT_BODY()
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
    std::vector<std::vector<float>> test_vector_vector_float;

    RPROPERTY()
    std::string test_string;

private:
    RPROPERTY()
    MyTestClass* test_class_ptr;

    RPROPERTY()
    bool test_bool;
};