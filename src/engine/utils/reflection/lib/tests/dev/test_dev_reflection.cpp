#include "macros.hpp"

#include <iostream>
#include <vector>

#include "dev_class.hpp"


int main()
{
    //std::cout << Reflection::TypeId::create<Test::TestClass>().name() << "\n";
    std::cout << Reflection::TypeId::create<Test::TestCl2>().name() << "\n";
}