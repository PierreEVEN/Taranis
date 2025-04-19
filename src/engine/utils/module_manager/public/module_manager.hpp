#pragma once
#include <filesystem>
#include <iostream>
#include <ankerl/unordered_dense.h>

#define IMPLEMENT_MODULE(ModuleClass, ModuleName)             \
extern "C" __declspec(dllexport) Module::Module* __INIT_MODULE_##ModuleName() \
{                                                             \
    return new ModuleClass();                                 \
} \
struct __ModuleRegisterStatic \
{ \
    __ModuleRegisterStatic()\
    { \
        Module::Module::register_module_static(#ModuleName, __INIT_MODULE_##ModuleName()); \
    }\
} __ModuleRegisterStatic_Instance;


struct __ModuleRegisterStatic;

namespace Module
{
class Module
{
public:
    class ModuleInfo
    {
    public:
        bool    b_is_loaded = false;
        Module* module      = nullptr;
    };

    static Module* load_from_path(const std::filesystem::path& path, const std::string& name);

    static Module* find_module_by_name(const std::string& name)
    {
        auto it = modules.find(name);
        return it != modules.end() ? it->second.module : nullptr;
    }

    static void init_static_modules();

    virtual void load_module() = 0;
    virtual void shutdown_module() = 0;

private:
    friend struct __ModuleRegisterStatic;

    static void register_module_static(const std::string& module_name, Module* module)
    {
        auto it = static_modules.find(module_name);
        if (it != static_modules.end())
        {
            std::cerr << "Module " << module_name << " is already statically registered";
            return;
        }

        static_modules.emplace(module_name, module);
    }

    inline static ankerl::unordered_dense::map<std::string, Module*> static_modules;
    static ankerl::unordered_dense::map<std::string, ModuleInfo> modules;
};
}