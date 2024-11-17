#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"

namespace Eng
{
MaterialAsset::MaterialAsset()
{
}

void MaterialAsset::set_shader_code(const std::string& code)
{
    shader_code = code;
    pipelines.clear();
}
}