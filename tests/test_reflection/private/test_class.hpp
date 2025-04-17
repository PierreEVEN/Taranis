#pragma once
#include "test_class.gen.hpp"

#include "native_types.hpp"

class TestChild : public std::vector<float>
{
    REFLECT_BODY()

public:
    float untracked_value = 0.05f;

    //RPROPERTY()
    //std::vector<std::string> string_vector = {"VAL_A", "VAL_B", "VAL_C"};
};

class MyTestClass
{
    REFLECT_BODY()


public:
    RPROPERTY()
    float test_float = 8;

    RPROPERTY(Transient)
    float test_float_transient = 22.5f;

    RPROPERTY()
    std::vector<float> test_vector_float = {1, 2, 3, 3.5f, 6};

    //RPROPERTY()
    //std::vector<std::vector<double>> test_vector_vector_float = {{25.8}, {987.8}, {74.8}};

    RPROPERTY()
    std::string test_string = "caca";

private:
    //RPROPERTY()
    //MyTestClass* test_class_ptr;

    RPROPERTY()
    TestChild child_value;

    RPROPERTY()
    bool test_bool = true;
};