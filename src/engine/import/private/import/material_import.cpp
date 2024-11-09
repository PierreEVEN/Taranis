#include "import/material_import.hpp"

#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "assets/material_asset.hpp"

namespace Eng
{
TObjectRef<MaterialAsset> MaterialImport::from_path(const std::filesystem::path& path, const Gfx::Pipeline::CreateInfos& create_infos, const std::vector<Gfx::EShaderStage>& stages,
                                                    const std::weak_ptr<Gfx::VkRendererPass>& renderer_pass)
{
    std::vector<std::shared_ptr<Gfx::ShaderModule>> modules;

    // Open the file using ifstream
    std::ifstream file(path);
    if (!file.is_open())
    {
        LOG_ERROR("File {} not found", path.string());
        return {};
    }
    std::string raw_code;
    std::string line;
    while (getline(file, line))
        raw_code += line + "\n";

    Gfx::ShaderCompiler compiler;
    for (const auto& stage : stages)
    {
        std::string ep;
        switch (stage)
        {
        case Gfx::EShaderStage::Vertex:
            ep = "vs_main";
            break;
        case Gfx::EShaderStage::Fragment:
            ep = "fs_main";
            break;
        default:
            LOG_FATAL("Unhandled shader stage")
        }

        const auto module_code = compiler.compile_raw(raw_code, ep, stage, path.string());
        modules.push_back(Gfx::ShaderModule::create(Engine::get().get_device(), module_code.get()));
    }
    auto pipeline = Gfx::Pipeline::create(path.filename().string(), Engine::get().get_device(), renderer_pass, modules, create_infos);
    return Engine::get().asset_registry().create<MaterialAsset>(path.filename().string(), pipeline);
}
} // namespace Eng