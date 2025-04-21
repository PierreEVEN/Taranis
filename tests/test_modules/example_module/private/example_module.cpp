
#include "example_module.hpp"


IMPLEMENT_MODULE(ExampleModule, ExampleModule);

void ExampleModule::load_module()
{
    std::cout << "EXAMPLE MODULE LOADED !\n";

    for (const auto& cl : Reflection::Class::get_classes())
    {
        std::cout << "classe A : " << cl.first.name() << "\n";
    }

}

void ExampleModule::shutdown_module()
{
}