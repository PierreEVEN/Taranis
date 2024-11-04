#pragma once
#include <string>
#include <vector>

#include "gfx/types.hpp"
#include "gfx/vulkan/vk_check.hpp"

struct IDxcCompiler3;
struct IDxcIncludeHandler;
struct IDxcUtils;

namespace std::filesystem
{
class path;
}

namespace Engine
{
struct StageInputOutputDescription
{
    uint32_t    location;
    uint32_t    offset;
    ColorFormat format;
};

enum class EBindingType
{
    SAMPLER,
    COMBINED_IMAGE_SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE,
    UNIFORM_TEXEL_BUFFER,
    STORAGE_TEXEL_BUFFER,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    UNIFORM_BUFFER_DYNAMIC,
    STORAGE_BUFFER_DYNAMIC,
    INPUT_ATTACHMENT
};

enum class EShaderStage
{
    Vertex   = 0x00000001,
    Fragment = 0x00000010
};

struct BindingDescription
{
    BindingDescription(std::string in_name, uint32_t in_binding, EBindingType in_type) : name(std::move(in_name)), binding(in_binding), type(in_type)
    {
    }

    std::string  name;
    uint32_t     binding;
    EBindingType type;
};

struct ShaderProperties
{
    EShaderStage                             stage       = EShaderStage::Vertex;
    std::string                              entry_point = "main";
    std::vector<BindingDescription>          bindings;
    std::vector<StageInputOutputDescription> stage_inputs;
    std::vector<StageInputOutputDescription> stage_outputs;
    uint32_t                                 push_constant_size = 0;
    std::vector<uint8_t>                     spirv;
};

class ShaderCompiler
{
  public:
    ShaderCompiler();

    Result<ShaderProperties> compile_raw(const std::string& raw, const std::string& entry_point, EShaderStage stage, const std::filesystem::path& path, bool b_debug = false);
    Result<ShaderProperties> load_from_path(const std::filesystem::path& path, const std::string& entry_point, EShaderStage stage, bool b_debug = false);

  private:
    static std::optional<std::string> extract_spirv_properties(ShaderProperties& properties);

    IDxcIncludeHandler* include_handler;
    IDxcCompiler3*      compiler = nullptr;
    IDxcUtils*          utils    = nullptr;
};
} // namespace Engine
