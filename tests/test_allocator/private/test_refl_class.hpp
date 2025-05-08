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

class TestReflectClassAlloc2
{
    REFLECT_BODY()

public:
    ~TestReflectClassAlloc2()
    {
        assert(identifier >= 0);
        identifier = -1;
    }

    int identifier = 0;
};
