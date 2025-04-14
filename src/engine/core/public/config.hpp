#pragma once
#include "macros.hpp"
#include "gfx/gfx.hpp"

#include <filesystem>
#include "config.gen.hpp"

namespace Eng
{
class Config
{
    REFLECT_BODY()
  public:


    Gfx::GfxConfig gfx;

    // 0 for max
    uint32_t worker_threads = 0;

    bool auto_update_materials = false;
private:
    std::filesystem::path config_path;
};

} // namespace Eng
