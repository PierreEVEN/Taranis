#pragma once
#include "test_class.gen.hpp"

REFL_DECLARE_TYPENAME(float)

REFL_DECLARE_TYPENAME(bool)

REFL_DECLARE_TYPENAME(char)

REFL_DECLARE_TYPENAME(std::string)

REFL_DECLARE_TYPENAME_ARGS(std::vector)

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
    std::string test_string;

private:
    RPROPERTY()
    MyTestClass* test_class_ptr;

    RPROPERTY()
    bool test_bool;
};