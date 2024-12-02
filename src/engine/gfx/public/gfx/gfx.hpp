#pragma once

#include <string>

namespace Eng::Gfx
{

class GfxConfig
{
  public:
    std::string app_name                 = "Engine";
    bool        enable_validation_layers = false;
    bool        allow_integrated_gpus    = false;
    bool        v_sync                   = true;
    uint8_t     swapchain_image_count    = 2;
};
} // namespace Eng::Gfx