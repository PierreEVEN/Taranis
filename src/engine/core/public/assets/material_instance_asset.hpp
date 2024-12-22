#pragma once
#include "asset_base.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "object_ptr.hpp"

#include "assets/material_instance_asset.gen.hpp"
#include "gfx/renderer/definition/render_pass_id.hpp"

namespace Eng
{

class SamplerAsset;
class TextureAsset;
struct MaterialPermutation;
class MaterialAsset;

namespace Gfx
{
class Pipeline;
class DescriptorSet;
class RenderPassRef;
class Buffer;
class BufferData;
} // namespace Gfx

class MaterialInstanceAsset : public AssetBase
{
    REFLECT_BODY()

public:
    MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material);

    std::shared_ptr<Gfx::Pipeline>      get_base_resource(const Gfx::RenderPassRef& render_pass_id);
    std::shared_ptr<Gfx::DescriptorSet> get_descriptor_resource(const Gfx::RenderPassRef& render_pass_id);

    void set_sampler(const std::string& binding, const TObjectRef<SamplerAsset>& sampler);
    void set_texture(const std::string& binding, const TObjectRef<TextureAsset>& texture);
    void set_buffer(const Gfx::RenderPassRef& render_pass_id, const std::string& binding, const std::weak_ptr<Gfx::Buffer>& buffer);
    void set_scene_data(const Gfx::RenderPassRef& render_pass_id, const std::weak_ptr<Gfx::Buffer>& buffer);

    // Compile shader for given pass if available (avoid lag spike later)
    void prepare_for_passes(const Gfx::RenderPassGenericId& render_pass_id);

    glm::vec3 asset_color() const override
    {
        return {0.2, 0.7, 0.3};
    }

private:
    TObjectRef<MaterialAsset>                                                             base;
    std::shared_mutex                                                                     descriptor_lock;
    ankerl::unordered_dense::map<Gfx::RenderPassRef, std::shared_ptr<Gfx::DescriptorSet>> descriptors;

    Gfx::PermutationDescription        permutation_description;
    std::weak_ptr<MaterialPermutation> permutation;

    struct PassData
    {
        ankerl::unordered_dense::map<std::string, TObjectRef<SamplerAsset>>   samplers;
        ankerl::unordered_dense::map<std::string, TObjectRef<TextureAsset>>   textures;
        ankerl::unordered_dense::map<std::string, std::weak_ptr<Gfx::Buffer>> buffers;
    };
    ankerl::unordered_dense::map<Gfx::RenderPassRef, PassData> pass_data;

    ankerl::unordered_dense::map<std::string, TObjectRef<SamplerAsset>>   samplers;
    ankerl::unordered_dense::map<std::string, TObjectRef<TextureAsset>>   textures;
    ankerl::unordered_dense::map<std::string, std::weak_ptr<Gfx::Buffer>> buffers;
};
} // namespace Eng