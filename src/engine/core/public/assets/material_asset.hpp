#pragma once
#include "asset_base.hpp"
#include "assets\material_asset.gen.hpp"
#include "gfx/vulkan/pipeline.hpp"

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
    MaterialAsset();

    void set_shader_code(const std::string& code);

    const std::shared_ptr<Gfx::Pipeline>& get_resource(const Gfx::ShaderPermutation& permutation) const
    {
        if (auto found = pipelines.find(permutation); found != pipelines.end())
            return found->second;
        return {};
    }

private:

    std::unordered_map<Gfx::ShaderPermutation, std::shared_ptr<Gfx::Pipeline>> pipelines;
    std::string shader_code;
};
} // namespace Eng