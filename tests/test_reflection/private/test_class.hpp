#pragma once
#include "test_class.gen.hpp"

REFL_DECLARE_TYPENAME(float)
REFL_DECLARE_TYPENAME(bool)
REFL_DECLARE_TYPENAME(std::vector<float>)

class MyTestClass
{
    REFLECT_BODY()

public:
    RPROPERTY()
    float test_float;

    RPROPERTY()
    std::vector<float> test_vector_float;

private:
    RPROPERTY()
    bool test_bool;
};


namespace test
{
inline std::vector<float> test_var;

inline void test_fn()
{

}
}