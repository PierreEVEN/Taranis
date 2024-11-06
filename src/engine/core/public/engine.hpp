#pragma once
#include <memory>
#include <unordered_map>

#include "config.hpp"

#include <chrono>

namespace Engine
{
class AssetRegistry;
class Config;

namespace Gfx
{
struct WindowConfig;
class Window;
class Device;
class Instance;
} // namespace Engine::Gfx


class Engine
{
  public:
    Engine(Config config);
    Engine(Engine&&) = delete;
    Engine(Engine&)  = delete;
    ~Engine();

    std::weak_ptr<Gfx::Window> new_window(const Gfx::WindowConfig& config);

    void run();

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
    std::chrono::steady_clock::time_point last_time;

    std::unordered_map<size_t, std::shared_ptr<Gfx::Window>> windows;

    std::shared_ptr<Gfx::Instance> gfx_instance;
    std::shared_ptr<Gfx::Device>   gfx_device;
    std::unique_ptr<AssetRegistry> global_asset_registry;

    Config app_config;
};
} // namespace Engine
