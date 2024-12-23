#include "scene/scene_view.hpp"

#include "engine.hpp"
#include "profiler.hpp"
#include "gfx/renderer/instance/render_pass_instance_base.hpp"
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

void SceneView::pre_draw(const Gfx::RenderPassInstanceBase& render_pass)
{
    PROFILER_SCOPE(ScenePreDraw);

    update_matrices(render_pass.resolution(), render_pass.get_definition().reversed_logarithmic_depth);

    glm::mat4 inv_view             = inverse(view);
    glm::mat4 inv_perspective      = inverse(projection_view);
    glm::mat4 inv_perspective_view = inv_view * inv_perspective;

    if (!view_buffer)
        view_buffer = Gfx::Buffer::create("Scene_buffer", Engine::get().get_device(), Gfx::Buffer::CreateInfos{.usage = Gfx::EBufferUsage::GPU_MEMORY, .type = Gfx::EBufferType::IMMEDIATE}, sizeof(SceneBufferData), 1);
    view_buffer->set_data(0, Gfx::BufferData{SceneBufferData{.perspective_view_mat = projection_view,
                                                             .view_mat = view,
                                                             .perspective_mat = projection_view,
                                                             .inv_perspective_view_mat = inv_perspective_view,
                                                             .inv_view_mat = inv_view,
                                                             .inv_perspective_mat = inv_perspective}});
}

void SceneView::pre_submit() const
{
    view_buffer->wait_data_upload();
}

void SceneView::draw(const Scene& scene, const Gfx::RenderPassInstanceBase&, Gfx::CommandBuffer& command_buffer, size_t idx, size_t num_threads) const
{
    PROFILER_SCOPE(SceneDraw);
    scene.for_each_part<MeshComponent>(
        [&command_buffer, this](MeshComponent& object)
        {
            object.draw(command_buffer, *this);
        },
        idx, std::max(1llu, num_threads));
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

void SceneView::update_matrices(const glm::uvec2& in_resolution, bool reversed_z)
{
    if (!outdated && resolution == in_resolution)
        return;
    if (orthographic && reversed_z)
        LOG_ERROR("Reversed_z with orthographic perspectives is not supported");
    outdated = false;

    view       = translate(mat4_cast(inverse(rotation)), -position);
    resolution = in_resolution;
    if (resolution.x == 0 || resolution.y == 0)
        return;

    float aspect = static_cast<float>(resolution.x) / static_cast<float>(resolution.y);
    assert(std::abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));

    if (orthographic)
    {
        float right  = fov;
        float left   = -fov;
        float top    = fov / aspect;
        float bottom = -fov / aspect;

        float h = 2.f / (top - bottom);
        float w = 2.f / (right - left);

        projection = {
            {0, 0, 1.f / (z_far - z_near), 0},
            {-w, 0, 0, 0},
            {0, h, 0, 0},
            {-(right + left) / (right - left), -(top + bottom) / (top - bottom), -z_near / (z_far - z_near), 1},
        };
    }
    else
    {
        if (reversed_z)
        {
            float h    = 1.f / std::tan(glm::radians(fov) * 0.5f);
            float w    = h / aspect;
            projection = {
                {0, 0, 0, 1},
                {-w, 0, 0, 0},
                {0, h, 0, 0},
                {0, 0, z_near, 0}};
        }
        else
        {
            float const tan_half_fov = tan(glm::radians(fov) * 0.5f);
            float       h            = 1 / tan_half_fov;
            float       w            = 1 / (aspect * tan_half_fov);
            projection               = {
                {0, 0, z_far / (z_far - z_near), 1},
                {-w, 0, 0, 0},
                {0, h, 0, 0},
                {0, 0, -(z_far * z_near) / (z_far - z_near), 0}};
        }
    }

    projection_view = projection * view;

    frustum = Frustum(projection_view);
}

} // namespace Eng