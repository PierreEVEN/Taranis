#include "macros.hpp"

#include <iostream>
#include <vector>

#include "dev_class.hpp"


int main()
{
    //std::cout << Reflection::TypeId::create<Test::TestClass>().name() << "\n";
    std::cout << Reflection::TypeId::create<Test::TestCl2>().name() << "\n";


    for (const auto& prop : Test::TestCl2::static_class()->get_properties())
    {
        std::cout << "prop : " << prop.second.get_type_instance().display() << " : " << prop.first << "\n";
    }
}