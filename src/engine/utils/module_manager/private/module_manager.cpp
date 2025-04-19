#include "module_manager.hpp"

#if _WIN32
#include <Windows.h>
#endif

#include <iostream>

namespace std::filesystem
{
class path;
}

namespace Module
{

using LoadModuleFn = Module*(__stdcall*)();

ankerl::unordered_dense::map<std::string, Module::ModuleInfo> Module::modules;

Module* Module::load_from_path(const std::filesystem::path& path, const std::string& name)
{
    if (find_module_by_name(name))
    {
        std::cerr << "Module " << name << " is already loaded\n";
        return nullptr;
    }

    if (!exists(path))
    {
        std::cerr << "Cannot load module from path " << path << " : File does not exists\n";
        return nullptr;
    }

    HINSTANCE module_dll = LoadLibraryW(TEXT(path.c_str()));
    if (module_dll == nullptr)
    {
        std::cerr << "ERROR: Unable to load DLL\n";
        return nullptr;
    }

    std::string  init_module_fn_name = std::format("__INIT_MODULE_{}", name);
    LoadModuleFn load_module_fn      = reinterpret_cast<LoadModuleFn>(GetProcAddress(module_dll, init_module_fn_name.c_str()));
    if (load_module_fn == nullptr)
    {
        std::cerr << "ERROR: Failed to find load module function '" << init_module_fn_name << "()'" << std::endl;
        return nullptr;
    }

    Module* loaded_module = load_module_fn();

    modules.emplace(name, ModuleInfo{.module = loaded_module});
    return loaded_module;
}

void Module::init_static_modules()
{
    for (const auto& module : static_modules)
        modules.emplace(module.first, ModuleInfo{.module = module.second});
    static_modules.clear();
}
}