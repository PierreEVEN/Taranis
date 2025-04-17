#pragma once
#include "test_class.gen.hpp"

#include "native_types.hpp"

class TestChild : public std::vector<float>
{
    REFLECT_BODY()

public:
    static TestChild make_special()
    {
        TestChild cl;
        cl.untracked_value = 12.5687f;
        cl.string_vector   = {"a", "bb", "", "ROH", "Ceci est une très longue", "phrase"};
        return cl;
    }

    float untracked_value = 0.05f;

private:
    RPROPERTY()
    std::vector<std::string> string_vector = {"VAL_A", "VAL_B", "VAL_C"};
};

class MyTestClass
{
    REFLECT_BODY()

public:
    static MyTestClass make_special()
    {
        MyTestClass cl;
        cl.test_float               = 98;
        cl.test_float_transient     = 78.97f;
        cl.test_vector_float        = {65, 64, 63.5, 63.2f, 62, 61};
        cl.test_vector_vector_float = {{12, 13, 14}, {28.5, 28.6, 28.7}, {}, {4, 3, 2, 1, 0}};
        cl.test_string              = "Pas caca";
        cl.child_value              = TestChild::make_special();
        cl.test_bool                = true;
        return cl;
    }

    RPROPERTY()
    float test_float = 8;

    RPROPERTY(Transient)
    float test_float_transient = 22.5f;

    RPROPERTY()
    std::vector<float> test_vector_float = {1, 2, 3, 3.5f, 6};

    RPROPERTY()
    std::vector<std::vector<double>> test_vector_vector_float = {{25.8}, {987.8}, {74.8}};

    RPROPERTY()
    std::string test_string = "caca";

private:
    RPROPERTY()
    MyTestClass* test_class_ptr;

    RPROPERTY()
    TestChild child_value;

    RPROPERTY()
    bool test_bool = false;
};
