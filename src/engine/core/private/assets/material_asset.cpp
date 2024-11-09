#include "assets/material_asset.hpp"

#include "engine.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"

namespace Eng
{

MaterialAsset::MaterialAsset(const std::shared_ptr<Gfx::Pipeline>& in_pipeline) : pipeline(in_pipeline)
{
}
}