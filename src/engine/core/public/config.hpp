#pragma once
#include "gfx/gfx.hpp"

namespace Eng
{
class Config
{
  public:
    Gfx::GfxConfig gfx;

    // 0 for max
    uint32_t       worker_threads = 0;
};

} // namespace Eng
