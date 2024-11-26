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
}

std::shared_ptr<Gfx::Pipeline> MaterialInstanceAsset::get_base_resource(const std::string& shader_pass) 
{
    auto perm = permutation.lock();
    if (perm)
        return perm->get_resource(shader_pass);

    permutation = base->get_permutation(permutation_description);
    if (!permutation.lock())
    {
        permutation_description = base->get_default_permutation();
        permutation             = base->get_permutation(permutation_description);
    }
    perm = permutation.lock();
    if (!perm)
        return nullptr;
    return perm->get_resource(shader_pass);
}

std::shared_ptr<Gfx::DescriptorSet> MaterialInstanceAsset::get_descriptor_resource(const std::string& shader_pass)
{
    if (auto found = descriptors.find(shader_pass); found != descriptors.end())
        return found->second;

    if (auto base_material = get_base_resource(shader_pass))
    {
        auto new_descriptor = descriptors.emplace(shader_pass, Gfx::DescriptorSet::create(std::string(get_name()) + "_descriptors", Engine::get().get_device(), base_material, b_static)).first->second;

        for (const auto& sampler : samplers)
            new_descriptor->bind_sampler(sampler.first, sampler.second->get_resource());

        for (const auto& texture : textures)
            new_descriptor->bind_image(texture.first, texture.second->get_view());

        for (const auto& buffer : buffers)
            new_descriptor->bind_buffer(buffer.first, buffer.second.lock());
        return new_descriptor;
    }
    return {};
}

void MaterialInstanceAsset::set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler)
{
    samplers.insert_or_assign(binding, sampler);
    for (const auto& desc : descriptors)
        desc.second->bind_sampler(binding, sampler->get_resource());
}

void MaterialInstanceAsset::set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture)
{
    textures.insert_or_assign(binding, texture);
    for (const auto& desc : descriptors)
        desc.second->bind_image(binding, texture->get_view());
}

void MaterialInstanceAsset::set_buffer(const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer)
{
    buffers.insert_or_assign(binding, buffer);
    for (const auto& desc : descriptors)
        desc.second->bind_buffer(binding, buffer.lock());
}

void MaterialInstanceAsset::set_scene_data(const std::weak_ptr<Gfx::Buffer>& buffer_data)
{
    auto in_data = buffer_data.lock();
    if (scene_buffer_data.lock() == in_data)
        return;

    set_buffer("scene_data_buffer", in_data);
    scene_buffer_data = buffer_data;
}

void MaterialInstanceAsset::prepare_for_passes(const std::string& render_pass)
{
    get_base_resource(render_pass);
}
} // namespace Eng