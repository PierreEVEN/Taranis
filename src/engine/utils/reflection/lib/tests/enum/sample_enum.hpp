#pragma once
#include "enum/sample_enum.gen.hpp"
#include "sample_enum.gen.hpp"

RENUM()
enum TestEnum : uint32_t
{
    Test,
    Test2,
};


RENUM(EnumFlags)
enum class AssetFlags : uint32_t
{
    Test4 = 1,
    Test3 = 1 << 1,
    Test2 = 1 << 2,
};

RENUM()
enum class TestEnum3 : size_t
{

};
