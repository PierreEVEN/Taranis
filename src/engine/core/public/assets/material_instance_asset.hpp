#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "assets\material_instance_asset.gen.hpp"

namespace Engine
{
class MaterialAsset;

namespace Gfx
{
class Pipeline;
class DescriptorSet;
}

class MaterialInstanceAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material);

    const std::shared_ptr<Gfx::Pipeline>& get_base_resource() const;
    const std::shared_ptr<Gfx::DescriptorSet>& get_descriptor_resource() const;

private:
    TObjectRef<MaterialAsset>           base;
    std::shared_ptr<Gfx::DescriptorSet> descriptors;
};
} // namespace Engine