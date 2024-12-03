#pragma once

#include "test_refl_class.gen.hpp"
#include <cassert>

class TestReflectClass
{
    REFLECT_BODY()

public:
    ~TestReflectClass()
    {
        assert(identifier >= 0);
        identifier = -1;
    }

    int identifier = 0;
};
