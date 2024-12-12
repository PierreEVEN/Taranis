#include "assets/material_instance_asset.hpp"

#include "assets/material_asset.hpp"
#include "assets/sampler_asset.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "object_ptr.hpp"

namespace Eng
{

MaterialInstanceAsset::MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material) : base(base_material)
{
}

std::shared_ptr<Gfx::Pipeline> MaterialInstanceAsset::get_base_resource(const Gfx::RenderPassRef& render_pass_id)
{
    auto perm = permutation.lock();
    if (perm)
        return perm->get_resource(render_pass_id);

    permutation = base->get_permutation(permutation_description);
    if (!permutation.lock())
    {
        permutation_description = base->get_default_permutation();
        permutation             = base->get_permutation(permutation_description);
    }
    perm = permutation.lock();
    if (!perm)
        return nullptr;
    return perm->get_resource(render_pass_id);
}

std::shared_ptr<Gfx::DescriptorSet> MaterialInstanceAsset::get_descriptor_resource(const Gfx::RenderPassRef& render_pass_id)
{
    {
        std::shared_lock lk(descriptor_lock);
        if (auto found = descriptors.find(render_pass_id); found != descriptors.end())
            return found->second;
    }
    std::unique_lock lk(descriptor_lock);
    if (auto base_material = get_base_resource(render_pass_id))
    {
        auto new_descriptor = descriptors.emplace(render_pass_id, Gfx::DescriptorSet::create(std::string(get_name()) + "_descriptors_" + render_pass_id.to_string(), Engine::get().get_device(), base_material->get_layout())).first->
                                          second;

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
    std::unique_lock lk(descriptor_lock);
    samplers.insert_or_assign(binding, sampler);
    for (const auto& desc : descriptors)
        desc.second->bind_sampler(binding, sampler->get_resource());
}

void MaterialInstanceAsset::set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture)
{
    std::unique_lock lk(descriptor_lock);
    textures.insert_or_assign(binding, texture);
    for (const auto& desc : descriptors)
        desc.second->bind_image(binding, texture->get_view());
}

void MaterialInstanceAsset::set_buffer(const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer)
{
    std::unique_lock lk(descriptor_lock);
    buffers.insert_or_assign(binding, buffer);
    for (const auto& desc : descriptors)
        desc.second->bind_buffer(binding, buffer.lock());
}

void MaterialInstanceAsset::set_buffer(const Gfx::RenderPassRef& render_pass_id, const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer)
{
    std::unique_lock lk(descriptor_lock);

    if (auto existing = buffers.find(binding); existing != buffers.end())
    {
        if (existing->second.lock() == buffer.lock())
            return;
        existing->second = buffer;
    }
    else
        buffers.emplace(binding, buffer);
    if (auto found = descriptors.find(render_pass_id); found != descriptors.end())
        found->second->bind_buffer(binding, buffer.lock());
}

void MaterialInstanceAsset::set_scene_data(const Gfx::RenderPassRef& render_pass_id, const std::weak_ptr<Gfx::Buffer>& buffer_data)
{
    set_buffer(render_pass_id, "scene_data_buffer", buffer_data);
}

void MaterialInstanceAsset::prepare_for_passes(const Gfx::RenderPassGenericId& render_pass_id)
{
    for (const auto& pass : Engine::get().get_device().lock()->get_all_pass_of_type(render_pass_id))
        get_base_resource(pass);
}
} // namespace Eng