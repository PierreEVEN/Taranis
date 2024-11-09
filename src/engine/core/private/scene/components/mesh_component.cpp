#include "scene/components/mesh_component.hpp"

#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/components/camera_component.hpp"

namespace Eng
{

void MeshComponent::draw(const Gfx::CommandBuffer& command_buffer) const
{
    if (mesh)
    {
        struct Pc
        {
            glm::mat4 camera = glm::mat4(1);
        };
        Pc pc;
        pc.camera = transpose(temp_cam->perspective_view_matrix());

        for (const auto& section : mesh->get_sections())
        {
            if (section.material)
            {
                command_buffer.bind_pipeline(*section.material->get_base_resource());
                command_buffer.bind_descriptors(*section.material->get_descriptor_resource(), *section.material->get_base_resource());
                command_buffer.push_constant(Gfx::EShaderStage::Vertex, *section.material->get_base_resource(), Gfx::BufferData(pc));
                command_buffer.draw_mesh(*section.mesh);
            }
        }
    }
}
}