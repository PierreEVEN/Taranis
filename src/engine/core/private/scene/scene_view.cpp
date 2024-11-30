#include "scene/scene_view.hpp"

#include "engine.hpp"
#include "profiler.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "scene/components/mesh_component.hpp"

#include <glm/ext/matrix_float4x4.hpp>

namespace Eng
{

struct SceneBufferData
{
    glm::mat4 perspective_view_mat;
    glm::mat4 view_mat;
    glm::mat4 perspective_mat;
    glm::mat4 inv_perspective_view_mat;
    glm::mat4 inv_view_mat;
    glm::mat4 inv_perspective_mat;
};

void SceneView::pre_draw(const Gfx::RenderPassInstance& render_pass)
{
    PROFILER_SCOPE(ScenePreDraw);

    if (!view_buffer)
        view_buffer = Gfx::Buffer::create("Scene_buffer", Engine::get().get_device(), Gfx::Buffer::CreateInfos{.usage = Gfx::EBufferUsage::GPU_MEMORY, .type = Gfx::EBufferType::IMMEDIATE}, sizeof(SceneBufferData), 1);

    update_buffer(render_pass.resolution());

    glm::mat4 inv_view             = inverse(view);
    glm::mat4 inv_perspective      = inverse(perspective_view);
    glm::mat4 inv_perspective_view = inv_view * inv_perspective;

    view_buffer->set_data(0, Gfx::BufferData{SceneBufferData{.perspective_view_mat = transpose(perspective_view),
                                                             .view_mat = transpose(view),
                                                             .perspective_mat = transpose(perspective_view),
                                                             .inv_perspective_view_mat = transpose(inv_perspective_view),
                                                             .inv_view_mat = transpose(inv_view),
                                                             .inv_perspective_mat = transpose(inv_perspective)}});
}

void SceneView::pre_submit() const
{
    view_buffer->wait_data_upload();
}

void SceneView::draw(const Scene& scene, const Gfx::RenderPassInstance&, Gfx::CommandBuffer& command_buffer, size_t idx, size_t num_threads)
{
    PROFILER_SCOPE(SceneDraw);
    scene.for_each_part<MeshComponent>(
        [&command_buffer, this](MeshComponent& object)
        {
            PROFILER_SCOPE_NAMED(DrawMesh, "Draw mesh " + std::string(object.get_name()));
            object.draw(command_buffer, *this);
        },
        idx, num_threads);
}

void SceneView::set_position(const glm::vec3& in_position)
{
    if (in_position == position)
        return;
    position = in_position;
    outdated = true;
}

void SceneView::set_rotation(const glm::quat& in_rotation)
{
    if (in_rotation == rotation)
        return;
    rotation = in_rotation;
    outdated = true;
}

void SceneView::update_buffer(const glm::uvec2& in_resolution)
{
    if (!outdated && resolution == in_resolution)
        return;
    outdated = false;

    view       = translate(mat4_cast(inverse(rotation)), -position);
    resolution = in_resolution;
    if (resolution.x == 0 || resolution.y == 0)
        return;
    float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
    float h     = 1.f / std::tan(glm::radians(fov) * 0.5f);
    float w     = h / aspect;
    perspective = {{0, 0, 0, 1}, {-w, 0, 0, 0}, {0, h, 0, 0}, {0, 0, z_near, 0}};
    perspective_view = perspective * view;
}

} // namespace Eng