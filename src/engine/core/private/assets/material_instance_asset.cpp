#include "assets/material_instance_asset.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/material_asset.hpp"
#include "assets/sampler_asset.hpp"
#include "assets/texture_asset.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"

namespace Eng
{

MaterialInstanceAsset::MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material, bool b_in_static) : base(base_material), b_static(b_in_static)
{
    if (base_material)
    {
        descriptors = Gfx::DescriptorSet::create(std::string(get_name()) + "_descriptors", Engine::get().get_device(), base_material->get_resource(), b_static);
    }
}

const std::shared_ptr<Gfx::Pipeline>& MaterialInstanceAsset::get_base_resource() const
{
    return base->get_resource();
}

const std::shared_ptr<Gfx::DescriptorSet>& MaterialInstanceAsset::get_descriptor_resource() const
{
    return descriptors;
}

void MaterialInstanceAsset::set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler) const
{
    descriptors->bind_sampler(binding, sampler->get_resource());
}

void MaterialInstanceAsset::set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture) const
{
    if (!texture)
        LOG_FATAL("Cannot set null texture");
    descriptors->bind_image(binding, texture->get_view());
}

void MaterialInstanceAsset::set_scene_data(const std::weak_ptr<Gfx::Buffer>& buffer_data)
{
    auto in_data = buffer_data.lock();
    if (scene_buffer_data.lock() == in_data)
        return;

    descriptors->bind_buffer("scene_data_buffer", in_data);
    scene_buffer_data = buffer_data;
}
} // namespace Eng