#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "assets\material_instance_asset.gen.hpp"

namespace Eng
{
class SamplerAsset;
class TextureAsset;
}

namespace Eng
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

    void set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler) const;
    void set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture) const;

private:
    TObjectRef<MaterialAsset>           base;
    std::shared_ptr<Gfx::DescriptorSet> descriptors;
};
} // namespace Eng