#pragma once

#include "dev/dev_class.gen.hpp"

class TestCl1
{
};

REFL_DECLARE_TYPENAME(TestCl1)

namespace Test
{
class TestCl2
{
    REFLECT_BODY()

    RPROPERTY()
    float test;

    RPROPERTY()
    ::TestCl1 test2;
};
} // namespace Test