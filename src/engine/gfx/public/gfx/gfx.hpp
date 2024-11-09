#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace Eng::Gfx
{

class GfxConfig
{
  public:
    std::string app_name                 = "Engine";
    bool        enable_validation_layers = true;
    bool        allow_integrated_gpus    = false;
    uint8_t     swapchain_image_count    = 2;
};
} // namespace Eng::Gfx