#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"

#include "assets\sampler_asset.gen.hpp"

namespace Eng
{
namespace Gfx
{
class Sampler;
}

class SamplerAsset : public AssetBase
{
    REFLECT_BODY();
    friend AssetRegistry;

  public:
    const std::shared_ptr<Gfx::Sampler>& get_resource()
    {
        return sampler;
    }

  private:
    SamplerAsset();

    std::shared_ptr<Gfx::Sampler> sampler;
};

} // namespace Eng
