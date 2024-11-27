#include "scene/components/mesh_component.hpp"

#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "profiler.hpp"
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
        PROFILER_SCOPE_NAMED(DrawMesh, "Draw mesh component " + std::string(get_name()) + " : " + std::to_string(mesh->get_sections().size()) + " sections");
        for (const auto& section : mesh->get_sections())
        {
            if (section.material)
            {
                section.material->set_scene_data(command_buffer.render_pass(), get_scene().get_scene_buffer());

                auto pipeline = section.material->get_base_resource(command_buffer.render_pass());
                if (!pipeline)
                    return;
                command_buffer.bind_pipeline(pipeline);
                command_buffer.push_constant(Gfx::EShaderStage::Vertex, *pipeline, Gfx::BufferData(Pc{.model = get_transform()}));

                auto resources = section.material->get_descriptor_resource(command_buffer.render_pass());
                assert(resources);
                command_buffer.bind_descriptors(*resources, *pipeline);
                command_buffer.draw_mesh(*section.mesh);
            }
        }
    }
}
} // namespace Eng