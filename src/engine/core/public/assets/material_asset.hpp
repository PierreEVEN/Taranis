#pragma once
#include "asset_base.hpp"
#include "assets\material_asset.gen.hpp"
#include "gfx/vulkan/pipeline.hpp"

namespace ShaderCompiler
{
class ShaderParser;
}

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

    const std::shared_ptr<Gfx::Pipeline>& get_resource(const std::string& render_pass);

private:
    struct CompiledPass
    {
        std::vector<uint32_t> spirv;
    };

    void compile_pass(const std::string& render_pass);

    std::unordered_map<std::string, std::shared_ptr<Gfx::Pipeline>> test;

    std::unique_ptr<ShaderCompiler::ShaderParser> parser;

    std::unordered_map<std::string, CompiledPass>                   compiled_passes;
    std::unordered_map<std::string, std::shared_ptr<Gfx::Pipeline>> pipelines;
    std::string                                                     shader_code;
    std::unordered_map<std::string, bool>                           options;
};
} // namespace Eng