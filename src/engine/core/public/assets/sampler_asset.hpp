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

    glm::vec3 asset_color() const override
    {
        return {1, 0.5, 0.7};
    }
  private:
    SamplerAsset();

    std::shared_ptr<Gfx::Sampler> sampler;
};

} // namespace Eng
