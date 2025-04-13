#pragma once
#include "test_class.gen.hpp"

REFL_DECLARE_TYPENAME(float)

REFL_DECLARE_TYPENAME(bool)


class MyTestClass
{
    REFLECT_BODY()

public:
    RPROPERTY()
    float test;

private:
    RPROPERTY()
    bool value;
};