#pragma once
#include "module_manager.hpp"
#include "example_module.gen.hpp"

class ExampleModule : public Module::Module
{
public:
    void load_module() override;
    void shutdown_module() override;
};


class ExampleClass
{
    REFLECT_BODY()
};
