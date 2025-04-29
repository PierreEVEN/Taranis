#pragma once

#include <cassert>
#include "test_refl_class.gen.hpp"

class TestReflectClassAlloc
{
    REFLECT_BODY()

public:
    ~TestReflectClassAlloc()
    {
        assert(identifier >= 0);
        identifier = -1;
    }

    int identifier = 0;
};
