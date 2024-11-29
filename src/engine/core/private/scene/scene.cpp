#include <ranges>

#include "scene/scene.hpp"

#include "engine.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "object_allocator.hpp"
#include "profiler.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/components/scene_component.hpp"

struct Lights
{
    uint32_t directional_lights = 0;
    uint32_t point_lights       = 0;
    uint32_t spot_lights        = 0;
};

struct SceneBufferData
{
    glm::mat4 view_mat;
    glm::mat4 perspective_mat;
    glm::mat4 perspective_view_mat;
    glm::mat4 in_view_mat;
    glm::mat4 inv_perspective_mat;
    glm::mat4 inv_perspective_view_mat;
    Lights    lights;
};

namespace Eng
{
Scene::Scene()
{
    merge_queue_mtx = std::make_unique<std::mutex>();
    allocator       = std::make_unique<ContiguousObjectAllocator>();
}

void Scene::tick(double delta_second)
{
    PROFILER_SCOPE(SceneTick);
    {
        PROFILER_SCOPE(MergeScenes);
        std::lock_guard lk(*merge_queue_mtx);
        for (auto& scene : scenes_to_merge)
        {
            scene.for_each<SceneComponent>(
                [&](SceneComponent& object)
                {
                    object.scene = this;
                });
            assert(scene.allocator);
            allocator->merge_with(*scene.allocator);
            root_nodes.reserve(root_nodes.size() + scene.root_nodes.size());
            for (const auto& component : scene.root_nodes)
                root_nodes.push_back(component);
            scene.root_nodes.clear();
        }
        scenes_to_merge.clear();
    }

    std::vector<std::vector<TObjectPtr<SceneComponent>>::iterator> deleted_nodes;
    for (auto node = root_nodes.begin(); node != root_nodes.end(); ++node)
    {
        if (!*node)
            deleted_nodes.push_back(node);
    }
    for (auto& deleted_node : std::ranges::reverse_view(deleted_nodes))
        root_nodes.erase(deleted_node);

    for_each<SceneComponent>(
        [delta_second](SceneComponent& object)
        {
            object.tick(delta_second);
        });
}

void Scene::pre_draw(const Gfx::RenderPassInstance& render_pass)
{
    PROFILER_SCOPE(ScenePreDraw);
    if (!active_camera)
        return;

    if (!scene_buffer)
        scene_buffer = Gfx::Buffer::create("Scene_buffer", Engine::get().get_device(), Gfx::Buffer::CreateInfos{.usage = Gfx::EBufferUsage::GPU_MEMORY, .type = Gfx::EBufferType::IMMEDIATE}, sizeof(SceneBufferData), 1);

    const glm::mat4& new_pv = active_camera->perspective_view_matrix(render_pass.resolution());
    last_pv                        = new_pv;
    glm::mat4 inv_view             = inverse(active_camera->view_matrix());
    glm::mat4 inv_perspective      = inverse(active_camera->perspective_matrix(render_pass.resolution()));
    glm::mat4 inv_perspective_view = inv_view * inv_perspective;

    scene_buffer->set_data(0, Gfx::BufferData{SceneBufferData{.view_mat                 = transpose(active_camera->view_matrix()),
                                                              .perspective_mat          = transpose(active_camera->perspective_matrix(render_pass.resolution())),
                                                              .perspective_view_mat     = transpose(last_pv),
                                                              .in_view_mat              = transpose(inv_view),
                                                              .inv_perspective_mat      = transpose(inv_perspective),
                                                              .inv_perspective_view_mat = transpose(inv_perspective_view),
                                                              .lights                   = {}}});
}

void Scene::pre_submit(const Gfx::RenderPassInstance&)
{
    scene_buffer->wait_data_upload();
}

void Scene::draw(const Gfx::RenderPassInstance&, Gfx::CommandBuffer& cmd_buffer, size_t idx, size_t num_threads)
{
    PROFILER_SCOPE(SceneDraw);
    for_each_part<MeshComponent>(
        [&cmd_buffer](MeshComponent& object)
        {
            PROFILER_SCOPE_NAMED(DrawMesh, "Draw mesh " + std::string(object.get_name()));
            object.draw(cmd_buffer);
        },
        idx, num_threads);
}
} // namespace Eng