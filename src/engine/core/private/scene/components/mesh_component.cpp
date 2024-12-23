#include "scene/components/mesh_component.hpp"

#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "profiler.hpp"
#include "gfx/mesh.hpp"
#include "scene/scene_view.hpp"
#include "scene/components/camera_component.hpp"

struct Pc
{
    glm::mat4 model;
};

namespace Eng
{
void MeshComponent::draw(Gfx::CommandBuffer& command_buffer, const SceneView& view)
{
    if (mesh)
    {
        if (!view.frustum_test(get_world_transform() * mesh->get_bounds()))
            return;

        PROFILER_SCOPE_NAMED(DrawMesh, "Draw mesh component " + std::string(get_name()) + " : " + std::to_string(mesh->get_sections().size()) + " sections");
        for (const auto& section : mesh->get_sections())
        {
            if (!view.frustum_test(get_world_transform() * section.bounds))
                continue;
            if (section.material)
            {
                //PROFILER_SCOPE_NAMED(DrawMeshSection, "Draw mesh section" + section.mesh->get_name());
                section.material->set_scene_data(command_buffer.render_pass(), view.get_view_buffer());

                //PROFILER_SCOPE(DrawMeshSectionA);
                auto pipeline = section.material->get_base_resource(command_buffer.render_pass());
                if (!pipeline)
                    return;
                //PROFILER_SCOPE(DrawMeshSectionB);
                command_buffer.bind_pipeline(pipeline);
                command_buffer.push_constant(Gfx::EShaderStage::Vertex, *pipeline, Gfx::BufferData(Pc{.model = get_world_transform()}));

                //PROFILER_SCOPE(DrawMeshSectionC);
                auto resources = section.material->get_descriptor_resource(command_buffer.render_pass());
                //PROFILER_SCOPE(DrawMeshSectionC1);
                assert(resources);
                command_buffer.bind_descriptors(*resources, *pipeline);
                //PROFILER_SCOPE(DrawMeshSectionD);
                command_buffer.draw_mesh(*section.mesh);
            }
        }
    }
}
} // namespace Eng