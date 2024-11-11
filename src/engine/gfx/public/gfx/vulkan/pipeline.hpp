#pragma once
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "gfx/shaders/shader_compiler.hpp"

namespace Eng::Gfx
{
class ShaderModule;
class Device;
class VkRendererPass;
enum class ColorFormat;

enum class ECulling
{
    None,
    Front,
    Back,
    Both
};

enum class EFrontFace
{
    Clockwise,
    CounterClockwise,
};

enum class ETopology
{
    Points,
    Lines,
    Triangles,
};

enum class EPolygonMode
{
    Point,
    Line,
    Fill,
};

enum class EAlphaMode
{
    Opaque,
    Translucent,
    Additive
};

class Pipeline
{
public:
    struct CreateInfos
    {
        ECulling                                                culling      = ECulling::Front;
        EFrontFace                                              front_face   = EFrontFace::CounterClockwise;
        ETopology                                               topology     = ETopology::Triangles;
        EPolygonMode                                            polygon_mode = EPolygonMode::Fill;
        EAlphaMode                                              alpha_mode   = EAlphaMode::Opaque;
        bool                                                    depth_test   = true;
        float                                                   line_width   = 1.0f;
        std::optional<std::vector<StageInputOutputDescription>> stage_input_override;
    };

    static std::shared_ptr<Pipeline> create(const std::string& name, std::weak_ptr<Device> device, const std::weak_ptr<VkRendererPass>& render_pass, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage,
                                            CreateInfos        create_infos)
    {
        return std::shared_ptr<Pipeline>(new Pipeline(name, std::move(device), render_pass, shader_stage, std::move(create_infos)));
    }

    Pipeline(Pipeline&)  = delete;
    Pipeline(Pipeline&&) = delete;
    ~Pipeline();

    const VkPipelineLayout& get_layout() const
    {
        return layout;
    }

    const VkDescriptorSetLayout& get_descriptor_layout() const
    {
        return descriptor_set_layout;
    }

    const VkPipeline& raw() const
    {
        return ptr;
    }

    const CreateInfos& infos() const
    {
        return create_infos;
    }

    const std::vector<BindingDescription>& get_bindings() const
    {
        return descriptor_bindings;
    }

private:
    Pipeline(const std::string& name, std::weak_ptr<Device> device, const std::weak_ptr<VkRendererPass>& render_pass, const std::vector<std::shared_ptr<ShaderModule>>& shader_stage, CreateInfos create_infos);
    std::vector<BindingDescription> descriptor_bindings;
    CreateInfos                     create_infos;
    VkPipelineLayout                layout                = VK_NULL_HANDLE;
    VkDescriptorSetLayout           descriptor_set_layout = VK_NULL_HANDLE;
    VkPipeline                      ptr                   = VK_NULL_HANDLE;
    std::weak_ptr<Device>           device;
};
} // namespace Eng::Gfx