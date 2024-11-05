#pragma once

#include "class.hpp"
#include "test_reflected_header.gen.hpp"


class MaClassDeTest
{
    REFLECT_BODY();
};


void test()
{
    MaClassDeTest my_class;

    const Reflection::Class* cl = my_class.get_class();
    const Reflection::Class* cl2 = MaClassDeTest::static_class();


    printf("%s", cl->name());

}