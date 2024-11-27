#include "assets/sampler_asset.hpp"

#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/sampler.hpp"

namespace Eng
{

SamplerAsset::SamplerAsset()
{
    sampler = Gfx::Sampler::create(get_name(), Engine::get().get_device(), Gfx::Sampler::CreateInfos{});
}

} // namespace Eng