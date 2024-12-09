#pragma once

#include <cassert>
#include "test_refl_class.gen.hpp"

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
