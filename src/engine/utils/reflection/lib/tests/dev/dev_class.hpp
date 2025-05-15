#pragma once

#include "dev/dev_class.gen.hpp"

REFL_DECLARE_TYPENAME(float)

class TestCl1
{
    REFLECT_BODY()
};

template <typename T> class TestCl1Templ
{
    REFLECT_BODY()
};

namespace Test
{
class TestCl2
{
    REFLECT_BODY()
  public:
    RPROPERTY()
    float test;

    RPROPERTY()
    ::TestCl1Templ<double> test_temp;

    RPROPERTY()
    ::std::vector<float> test_vec;

    RPROPERTY()
    ::TestCl1 test2;
};
} // namespace Test