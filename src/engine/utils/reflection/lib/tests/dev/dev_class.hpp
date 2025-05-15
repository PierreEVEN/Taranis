#pragma once

#include "dev/dev_class.gen.hpp"

REFL_DECLARE_TYPENAME(float)

class TestCl1
{
    REFLECT_BODY()
};

namespace Test
{
class TestCl2
{
    REFLECT_BODY()

    RPROPERTY()
    float test;

    
    RPROPERTY()
        ::std::vector<float> test_vec;

    RPROPERTY()
    ::TestCl1 test2;
};
} // namespace Test