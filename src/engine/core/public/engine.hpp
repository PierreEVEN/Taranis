#pragma once
#include <memory>
#include <ankerl/unordered_dense.h>

#include "config.hpp"
#include "jobsys/job_sys.hpp"
#include "logger.hpp"

#include <chrono>

namespace Eng
{
class AssetRegistry;
class Config;
class Engine;

namespace Gfx
{
struct WindowConfig;
class Window;
class Device;
class Instance;
} // namespace Gfx

class Application
{
  public:
    virtual ~Application() = default;

    virtual void init(Engine& engine, const std::weak_ptr<Gfx::Window>& default_window) = 0;
    virtual void tick_game(Engine& engine, double delta_second)                         = 0;
};

class Engine
{
  public:
    Engine(Config config);
    Engine(Engine&&) = delete;
    Engine(Engine&)  = delete;
    ~Engine();

    template <typename T, typename... Args> void run(const Gfx::WindowConfig& default_window_config, Args&&... args)
    {
        app = std::make_unique<T>(std::forward<Args>(args)...);
        app->init(*this, new_window(default_window_config));
        run_internal();
    }

    std::weak_ptr<Gfx::Window> new_window(const Gfx::WindowConfig& config);

    std::weak_ptr<Gfx::Instance> get_instance() const
    {
        return gfx_instance;
    }

    std::weak_ptr<Gfx::Device> get_device() const
    {
        return gfx_device;
    }

    static Engine& get();

    double delta_second;

    AssetRegistry& asset_registry() const;

    double get_seconds() const;

    JobSystem& jobs()
    {
        if (!job_system)
            LOG_FATAL("Job system is not valid in the current context");
        return *job_system;
    }

    std::vector<std::shared_ptr<Gfx::Window>> get_windows() const
    {
        std::vector<std::shared_ptr<Gfx::Window>> win;
        for (const auto& w : windows)
            win.emplace_back(w.second);
        return win;
    }

  private:
    void run_internal();

    std::chrono::steady_clock::time_point last_time;
    std::chrono::steady_clock::time_point start_time;

    ankerl::unordered_dense::map<size_t, std::shared_ptr<Gfx::Window>> windows;

    std::shared_ptr<Gfx::Instance> gfx_instance;
    std::shared_ptr<Gfx::Device>   gfx_device;
    std::unique_ptr<AssetRegistry> global_asset_registry;

    std::unique_ptr<Application> app;

    Config app_config;

    std::unique_ptr<JobSystem> job_system;
};
} // namespace Eng