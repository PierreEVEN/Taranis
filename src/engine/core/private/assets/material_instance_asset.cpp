#include "assets/material_instance_asset.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/material_asset.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"

namespace Engine
{

MaterialInstanceAsset::MaterialInstanceAsset(const TObjectRef<MaterialAsset>& base_material) : base(base_material)
{
    if (base_material)
    {
        descriptors = Gfx::DescriptorSet::create(std::string(get_name()) + "_descriptors", Engine::get().get_device(), base_material->get_resource());
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
} // namespace Engine