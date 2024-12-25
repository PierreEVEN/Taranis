#include "assets/material_instance_asset.hpp"

#include "assets/material_asset.hpp"
#include "assets/sampler_asset.hpp"
#include "assets/texture_asset.hpp"
#include "engine.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "object_ptr.hpp"
#include "profiler.hpp"

namespace Eng
{

MaterialInstanceAsset::MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material) : base(base_material)
{
    permutation_description = base->get_default_permutation();
}

std::shared_ptr<Gfx::Pipeline> MaterialInstanceAsset::get_base_resource(const Gfx::RenderPassRef& render_pass_id)
{
    auto perm = permutation.lock();
    if (perm)
        return perm->get_resource(render_pass_id);

    PROFILER_SCOPE(GetBaseResourceFindPermutation);
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
    PROFILER_SCOPE(GetUpdateDescriptorResources);
    std::unique_lock lk(descriptor_lock);
    if (auto base_material = get_base_resource(render_pass_id))
    {
        auto new_descriptor =
            descriptors.emplace(render_pass_id, Gfx::DescriptorSet::create(std::string(get_name()) + "_descriptors_" + render_pass_id.to_string(), Engine::get().get_device(), base_material->get_layout())).first->second;

        for (const auto& sampler : samplers)
            new_descriptor->bind_sampler(sampler.first, sampler.second->get_resource());

        for (const auto& texture : textures)
            new_descriptor->bind_image(texture.first, texture.second->get_view());

        for (const auto& buffer : buffers)
            new_descriptor->bind_buffer(buffer.first, buffer.second.lock());

        if (auto found = pass_data.find(render_pass_id); found != pass_data.end())
        {
            for (const auto& sampler : found->second.samplers)
                new_descriptor->bind_sampler(sampler.first, sampler.second->get_resource());

            for (const auto& texture : found->second.textures)
                new_descriptor->bind_image(texture.first, texture.second->get_view());

            for (const auto& buffer : found->second.buffers)
                new_descriptor->bind_buffer(buffer.first, buffer.second.lock());
        }

        return new_descriptor;
    }
    return {};
}

void MaterialInstanceAsset::set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler)
{
    PROFILER_SCOPE(SetSampler1);
    std::unique_lock lk(descriptor_lock);
    samplers.insert_or_assign(binding, sampler);
    for (const auto& desc : descriptors)
        desc.second->bind_sampler(binding, sampler->get_resource());
}

void MaterialInstanceAsset::set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture)
{
    PROFILER_SCOPE(SetTexture1);
    std::unique_lock lk(descriptor_lock);
    textures.insert_or_assign(binding, texture);
    for (const auto& desc : descriptors)
        desc.second->bind_image(binding, texture->get_view());
}

void MaterialInstanceAsset::set_buffer(const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer)
{
    PROFILER_SCOPE(SetBuffer1);
    std::unique_lock lk(descriptor_lock);
    buffers.insert_or_assign(binding, buffer);
    for (const auto& desc : descriptors)
        desc.second->bind_buffer(binding, buffer.lock());
}

void MaterialInstanceAsset::set_sampler(const Gfx::RenderPassRef& render_pass_id, const std::string& binding, const TObjectRef<SamplerAsset>& sampler)
{
    PROFILER_SCOPE(SetSampler);
    std::unique_lock lk(descriptor_lock);
    if (auto found = descriptors.find(render_pass_id); found != descriptors.end())
        found->second->bind_sampler(binding, sampler->get_resource());
    else
    {
        if (auto existing = samplers.find(binding); existing != samplers.end())
        {
            if (existing->second == sampler)
                return;
            existing->second = sampler;
        }
        else
            pass_data.emplace(render_pass_id, PassData{}).first->second.samplers.emplace(binding, sampler);
    }
}

void MaterialInstanceAsset::set_texture(const Gfx::RenderPassRef& render_pass_id, const std::string& binding, const TObjectRef<TextureAsset>& texture)
{
    PROFILER_SCOPE(SetTexture);
    std::unique_lock lk(descriptor_lock);
    if (auto found = descriptors.find(render_pass_id); found != descriptors.end())
        found->second->bind_image(binding, texture->get_view());
    else
    {
        if (auto existing = textures.find(binding); existing != textures.end())
        {
            if (existing->second == texture)
                return;
            existing->second = texture;
        }
        else
            pass_data.emplace(render_pass_id, PassData{}).first->second.textures.emplace(binding, texture);
    }
}

void MaterialInstanceAsset::set_buffer(const Gfx::RenderPassRef& render_pass_id, const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer)
{
    PROFILER_SCOPE_NAMED(SetBufferWithLock, "Set buffer " + buffer.lock()->get_name());
    std::unique_lock lk(descriptor_lock);
    if (auto found = descriptors.find(render_pass_id); found != descriptors.end())
        found->second->bind_buffer(binding, buffer.lock());
    else
    {
        if (auto existing = buffers.find(binding); existing != buffers.end())
        {
            if (existing->second.lock() == buffer.lock())
                return;
            existing->second = buffer;
        }
        else
            pass_data.emplace(render_pass_id, PassData{}).first->second.buffers.emplace(binding, buffer);
    }
}

void MaterialInstanceAsset::set_scene_data(const Gfx::RenderPassRef& render_pass_id, const std::weak_ptr<Gfx::Buffer>& buffer_data)
{
    std::lock_guard lk(test);
    if (auto found = last_scene_buffer.find(render_pass_id); found != last_scene_buffer.end() && buffer_data.lock() == found->second.lock())
        return;
    last_scene_buffer.insert_or_assign(render_pass_id, buffer_data);
    set_buffer(render_pass_id, "scene_data_buffer", buffer_data);
}

void MaterialInstanceAsset::prepare_for_passes(const Gfx::RenderPassGenericId& render_pass_id)
{
    for (const auto& pass : Engine::get().get_device().lock()->get_all_pass_of_type(render_pass_id))
        get_base_resource(pass);
}

Gfx::PermutationDescription MaterialInstanceAsset::get_permutation()
{
    return permutation_description;
}

void MaterialInstanceAsset::set_permutation(const Gfx::PermutationDescription& perm)
{
    permutation_description = perm;
    permutation             = {};
}
} // namespace Eng