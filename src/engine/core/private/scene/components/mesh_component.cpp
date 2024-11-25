#include "scene/components/mesh_component.hpp"

#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/components/camera_component.hpp"

struct Pc
{
    glm::mat4 model;
};

namespace Eng
{

void MeshComponent::draw(Gfx::CommandBuffer& command_buffer)
{
    if (mesh)
    {
        for (const auto& section : mesh->get_sections())
        {
            if (section.material)
            {
                section.material->set_scene_data(get_scene().get_scene_buffer());
                command_buffer.bind_pipeline(section.material->get_base_resource(command_buffer.render_pass()));
                command_buffer.bind_descriptors(*section.material->get_descriptor_resource(command_buffer.render_pass()), *section.material->get_base_resource(command_buffer.render_pass()));
                command_buffer.push_constant(Gfx::EShaderStage::Vertex, *section.material->get_base_resource(command_buffer.render_pass()), Gfx::BufferData(Pc{.model = get_transform()}));
                command_buffer.draw_mesh(*section.mesh);
            }
        }
    }
}
}