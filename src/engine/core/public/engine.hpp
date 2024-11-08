#pragma once
#include <memory>
#include <unordered_map>

#include "config.hpp"

#include <chrono>

namespace Engine
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
    virtual void init(Engine& engine, const std::weak_ptr<Gfx::Window>& default_window) = 0;
    virtual void tick_game(Engine& engine, double delta_second) = 0;
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

  private:
    void run_internal();

    std::chrono::steady_clock::time_point last_time;

    std::unordered_map<size_t, std::shared_ptr<Gfx::Window>> windows;

    std::shared_ptr<Gfx::Instance> gfx_instance;
    std::shared_ptr<Gfx::Device>   gfx_device;
    std::unique_ptr<AssetRegistry> global_asset_registry;

    std::unique_ptr<Application> app;

    Config app_config;
};
} // namespace Engine
