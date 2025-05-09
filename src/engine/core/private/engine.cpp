#include "engine.hpp"

#include <iostream>

#include "assets/asset_registry.hpp"
#include "config.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/instance.hpp"
#include "gfx/vulkan/physical_device.hpp"
#include "gfx/vulkan/queue_family.hpp"
#include "gfx/vulkan/surface.hpp"
#include "gfx/window.hpp"
#include "profiler.hpp"
#include "assets/material_asset.hpp"
#include "assets/package.hpp"
#include "tools/debug_draw.hpp"

#if _WIN32
#include <Windows.h>
#endif

namespace Eng
{
Engine* engine_singleton = nullptr;

Engine::Engine(Config config) : app_config(std::move(config)), job_system(std::make_unique<JobSystem>(config.worker_threads ? config.worker_threads : std::thread::hardware_concurrency()))
{
    LOG_INFO("Using {} parallel workers", config.worker_threads ? config.worker_threads : std::thread::hardware_concurrency());
#if _WIN32
    timeBeginPeriod(1);
#endif
    PROFILER_SCOPE(EngineInitialization);
    if (engine_singleton)
        LOG_FATAL("Cannot start multiple engine instances at the same time")
    engine_singleton = this;
    last_time        = std::chrono::steady_clock::now();
    start_time       = std::chrono::steady_clock::now();

    gfx_instance = Gfx::Instance::create(config.gfx);
}

Engine::~Engine()
{
    DebugDraw::get().destroy();
    job_system = nullptr;
    windows.clear();
    app = nullptr;

    for (const auto& package_name : Package::get_all_packages())
        if (auto* package = Package::get(package_name))
            package->force_unload();

    if (gfx_device)
        gfx_device->destroy_resources();
    gfx_device       = nullptr;
    gfx_instance     = nullptr;
    engine_singleton = nullptr;
}

std::weak_ptr<Gfx::Window> Engine::new_window(const Gfx::WindowConfig& config)
{
    PROFILER_SCOPE(Engine_CreateWindow);
    const auto window = Gfx::Window::create(gfx_instance, config);

    if (!gfx_device)
    {
        if (auto physical_device = Gfx::PhysicalDevice::pick_best_physical_device(gfx_instance, app_config.gfx, window->get_surface()))
        {
            LOG_INFO("selected physical device {}", physical_device.get().get_device_name());
            gfx_device = Gfx::Device::create(app_config.gfx, gfx_instance, physical_device.get(), *window->get_surface());
        }
        else
            LOG_FATAL("{}", physical_device.error())
    }
    window->get_surface()->set_device(gfx_device);
    windows.emplace(window->get_id(), window);
    return window;
}

void Engine::run_internal()
{
    while (!windows.empty())
    {
        auto new_time = std::chrono::steady_clock::now();
        delta_second  = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(new_time - last_time).count()) / 1000000000.0;
        last_time     = new_time;

        if (app_config.auto_update_materials)
        {
            PROFILER_SCOPE(CheckForMaterialUpdates);
            for (const auto& package_name : Package::get_all_packages())
            {
                if (auto* _ = Package::get(package_name))
                {
                    LOG_ERROR("TODO : CHECK FOR UPDATE FOR ALL REGISTRY MATERIALS");
                    /*asset_registry().for_each<MaterialAsset>(
                        [](MaterialAsset& material)
                        {
                            material.check_for_updates();
                        });*/
                }
            }
        }

        app->tick_game(*this, delta_second);
        std::vector<size_t> windows_to_remove;
        for (const auto& [id, window] : windows)
        {
            if (window->render())
                windows_to_remove.emplace_back(id);
        }

        for (const auto& window : windows_to_remove)
            windows.erase(window);
        for (const auto& window : windows)
            window.second->reset_events();
        gfx_device->next_frame();
        DebugDraw::get().flush();
        Profiler::get().next_frame();
    }
}

Engine& Engine::get()
{
    if (!engine_singleton)
        LOG_FATAL("Engine is not initialized")
    return *engine_singleton;
}

double Engine::get_seconds() const
{
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count()) / 1000000.0;
}
} // namespace Eng