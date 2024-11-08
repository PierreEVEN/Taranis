#include "scene/components/mesh_component.hpp"

#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "scene/components/camera_component.hpp"

namespace Engine
{

void MeshComponent::draw(const Gfx::CommandBuffer& command_buffer) const
{
    if (mesh && material)
    {
        struct Pc
        {
            glm::mat4 camera = glm::mat4(1);
        };
        Pc pc;
        pc.camera = temp_cam->perspective_view_matrix();

        command_buffer.bind_pipeline(*material->get_base_resource());
        command_buffer.bind_descriptors(*material->get_descriptor_resource(), *material->get_base_resource());
        command_buffer.push_constant(Gfx::EShaderStage::Vertex, *material->get_base_resource(), Gfx::BufferData(pc));
        command_buffer.draw_mesh(*mesh->get_resource());
    }
}
}