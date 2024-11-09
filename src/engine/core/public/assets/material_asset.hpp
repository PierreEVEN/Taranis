#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "assets\material_asset.gen.hpp"

namespace Eng::Gfx
{
class DescriptorSet;
class Pipeline;
}

namespace Eng
{
class MaterialAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialAsset(const std::shared_ptr<Gfx::Pipeline>& pipeline);

    const std::shared_ptr<Gfx::Pipeline>& get_resource() const
    {
        return pipeline;
    }

private:
    std::shared_ptr<Gfx::Pipeline> pipeline;
};
} // namespace Eng