
#include "example_module.hpp"


IMPLEMENT_MODULE(ExampleModule, ExampleModule);

void ExampleModule::load_module()
{
    std::cout << "EXAMPLE MODULE LOADED !\n";
}

void ExampleModule::shutdown_module()
{
}