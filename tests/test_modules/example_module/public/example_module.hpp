#pragma once
#include "module_manager.hpp"

class ExampleModule : public Module::Module
{
public:
    void load_module() override;
    void shutdown_module() override;
};