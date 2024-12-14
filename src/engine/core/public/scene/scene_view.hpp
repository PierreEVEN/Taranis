#pragma once
#include "bounds.hpp"

#include <memory>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>

namespace Eng
{
namespace Gfx
{
class Buffer;
class RenderPassInstanceBase;
class CommandBuffer;
}

class Scene;

// Quickly grabbed from https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644
class Frustum
{
    enum EPlanes
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Count,
        Combinations = Count * (Count - 1) / 2
    };

    template <EPlanes i, EPlanes j> struct ij2k
    {
        enum
        {
            k = i * (9 - i) / 2 + j - 1
        };
    };

    template <EPlanes a, EPlanes b, EPlanes c> glm::vec3 intersection(const glm::vec3* crosses) const
    {
        float     D   = dot(glm::vec3(m_planes[a]), crosses[ij2k<b, c>::k]);
        glm::vec3 res = glm::mat3(crosses[ij2k<b, c>::k], -crosses[ij2k<a, c>::k], crosses[ij2k<a, b>::k]) * glm::vec3(m_planes[a].w, m_planes[b].w, m_planes[c].w);
        return res * (-1.0f / D);
    }

    glm::vec4 m_planes[Count];
    glm::vec3 m_points[4];

public:
    Frustum() = default;

    Frustum(glm::mat4 view_proj)
    {
        view_proj        = transpose(view_proj);
        m_planes[Left]   = view_proj[3] + view_proj[0];
        m_planes[Right]  = view_proj[3] - view_proj[0];
        m_planes[Bottom] = view_proj[3] + view_proj[1];
        m_planes[Top]    = view_proj[3] - view_proj[1];
        m_planes[Near]   = view_proj[3] + view_proj[2];

        glm::vec3 crosses[Combinations] = {
            cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Right])), cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Bottom])), cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Top])),
            cross(glm::vec3(m_planes[Left]), glm::vec3(m_planes[Near])), cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Bottom])),
            cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Top])), cross(glm::vec3(m_planes[Right]), glm::vec3(m_planes[Near])),
            cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Top])), cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Near])),
            cross(glm::vec3(m_planes[Top]), glm::vec3(m_planes[Near]))};

        m_points[0] = intersection<Left, Bottom, Near>(crosses);
        m_points[1] = intersection<Left, Top, Near>(crosses);
        m_points[2] = intersection<Right, Bottom, Near>(crosses);
        m_points[3] = intersection<Right, Top, Near>(crosses);
    }

    bool test(const Bounds& bounds) const
    {
        // check box outside/inside of frustum
        for (auto m_plane : m_planes)
        {
            if (dot(m_plane, glm::vec4(bounds.min().x, bounds.min().y, bounds.min().z, 1.0f)) < 0.0 && dot(m_plane, glm::vec4(bounds.max().x, bounds.min().y, bounds.min().z, 1.0f)) < 0.0 &&
                dot(m_plane, glm::vec4(bounds.min().x, bounds.max().y, bounds.min().z, 1.0f)) < 0.0 && dot(m_plane, glm::vec4(bounds.max().x, bounds.max().y, bounds.min().z, 1.0f)) < 0.0 &&
                dot(m_plane, glm::vec4(bounds.min().x, bounds.min().y, bounds.max().z, 1.0f)) < 0.0 && dot(m_plane, glm::vec4(bounds.max().x, bounds.min().y, bounds.max().z, 1.0f)) < 0.0 &&
                dot(m_plane, glm::vec4(bounds.min().x, bounds.max().y, bounds.max().z, 1.0f)) < 0.0 && dot(m_plane, glm::vec4(bounds.max().x, bounds.max().y, bounds.max().z, 1.0f)) < 0.0)
            {
                return false;
            }
        }

        // check frustum outside/inside box
        int out = 0;
        for (auto& m_point : m_points)
            out += m_point.x > bounds.max().x ? 1 : 0;
        if (out == 8)
            return false;
        out = 0;
        for (auto& m_point : m_points)
            out += m_point.x < bounds.min().x ? 1 : 0;
        if (out == 8)
            return false;
        out = 0;
        for (auto& m_point : m_points)
            out += m_point.y > bounds.max().y ? 1 : 0;
        if (out == 8)
            return false;
        out = 0;
        for (auto& m_point : m_points)
            out += m_point.y < bounds.min().y ? 1 : 0;
        if (out == 8)
            return false;
        out = 0;
        for (auto& m_point : m_points)
            out += m_point.z > bounds.max().z ? 1 : 0;
        if (out == 8)
            return false;
        out = 0;
        for (auto& m_point : m_points)
            out += m_point.z < bounds.min().z ? 1 : 0;
        if (out == 8)
            return false;

        return true;
    }
};

class SceneView
{
public:
    static std::shared_ptr<SceneView> create()
    {
        return std::shared_ptr<SceneView>(new SceneView());
    }

    void pre_draw(const Gfx::RenderPassInstanceBase& render_pass);
    void pre_submit() const;
    void draw(const Scene& scene, const Gfx::RenderPassInstanceBase& render_pass, Gfx::CommandBuffer& command_buffer, size_t idx, size_t num_threads) const;

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

    bool frustum_test(const Bounds& bounds) const
    {
        return frustum.test(bounds);
    }

    void set_fov(float in_fov)
    {
        if (in_fov != fov)
            outdated = true;
        fov = in_fov;
    }

    void set_orthographic_width(float in_width)
    {
        if (in_width != fov)
            outdated = true;
        fov = in_width;
    }

    void set_perspective(bool b_is_perspective = true)
    {
        if (b_is_perspective == orthographic)
            outdated = true;
        orthographic = !b_is_perspective;
    }

    void set_z_far(float in_z_far)
    {
        if (in_z_far != z_far)
            outdated = true;
        z_far = in_z_far;
    }

    void set_z_near(float in_z_near)
    {
        if (in_z_near != z_near)
            outdated = true;
        z_near = in_z_near;
    }

    const glm::mat4& get_projection_view_matrix() const
    {
        return projection_view;
    }

    const glm::mat4& get_projection_matrix() const
    {
        return projection;
    }

    const glm::mat4& get_view_matrix() const
    {
        return view;
    }

  private:
    SceneView()
    {
    }

    void update_matrices(const glm::uvec2& in_resolution, bool reversed_z);

    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 position = {0, 0, 0};

    glm::mat4  view;
    glm::mat4  projection;
    glm::mat4  projection_view;
    bool       outdated = true;
    float      fov      = 90.0f;
    float      z_near   = 0.01f;
    float      z_far   = 1000.f;
    glm::uvec2 resolution;
    bool       orthographic = false;

    Frustum frustum;

    std::shared_ptr<Gfx::Buffer> view_buffer;
};


} // namespace Eng