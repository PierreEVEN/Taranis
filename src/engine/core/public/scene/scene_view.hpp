#pragma once
#include <memory>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace Eng
{
namespace Gfx
{
class Buffer;
class RenderPassInstance;
class CommandBuffer;
}

class Scene;

class SceneView
{
public:
    static std::shared_ptr<SceneView> create()
    {
        return std::shared_ptr<SceneView>(new SceneView());
    }

    void pre_draw(const Gfx::RenderPassInstance& render_pass);
    void pre_submit() const;
    void draw(const Scene& scene, const Gfx::RenderPassInstance& render_pass, Gfx::CommandBuffer& command_buffer, size_t idx, size_t num_threads);

    const glm::uvec2& get_resolution() const
    {
        return resolution;
    }

    void set_rotation(const glm::quat& in_rotation);
    void set_position(const glm::vec3& in_position);
    const std::shared_ptr<Gfx::Buffer>& get_view_buffer() const
    {
        return view_buffer;
    }
  private:
    SceneView()
    {
    }

    void update_buffer(const glm::uvec2& in_resolution);

    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 position = {0, 0, 0};

    glm::mat4 view;
    glm::mat4 perspective;
    glm::mat4  perspective_view;
    bool       outdated = true;
    float      fov      = 90.0f;
    float      z_near   = 0.01f;
    glm::uvec2 resolution;

    std::shared_ptr<Gfx::Buffer> view_buffer;
};

} // namespace Eng