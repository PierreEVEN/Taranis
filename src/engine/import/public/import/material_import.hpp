#pragma once
#include "object_ptr.hpp"
#include "gfx/vulkan/pipeline.hpp"

namespace Eng::Gfx
{
class RenderPassInstance;
}

namespace Eng
{
class MaterialAsset;

class MaterialImport
{
public:
    static TObjectRef<MaterialAsset> from_path(const std::filesystem::path& path, const Gfx::Pipeline::CreateInfos& create_infos, const std::vector<Gfx::EShaderStage>& stages,
                                               const std::weak_ptr<Gfx::VkRendererPass>& renderer_pass);
};


}