#include "logger.hpp"
#include "module_manager.hpp"

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    if (Module::Module* module = Module::Module::load_from_path("./build/windows/x64/debug/example_module.dll", "ExampleModule"))
        module->load_module();
    else
        LOG_ERROR("Failed to load module");
}