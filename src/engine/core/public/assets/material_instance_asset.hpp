#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "assets\material_instance_asset.gen.hpp"

namespace Eng::Gfx
{
class Buffer;
class BufferData;
}

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
    MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material, bool b_static = true);

    const std::shared_ptr<Gfx::Pipeline>&      get_base_resource() const;
    const std::shared_ptr<Gfx::DescriptorSet>& get_descriptor_resource() const;

    void set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler) const;
    void set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture) const;
    void set_scene_data(const std::weak_ptr<Gfx::Buffer>& buffer_data);

private:
    std::weak_ptr<Gfx::Buffer>          scene_buffer_data;
    TObjectRef<MaterialAsset>           base;
    bool                                b_static = true;
    std::shared_ptr<Gfx::DescriptorSet> descriptors;
};
} // namespace Eng