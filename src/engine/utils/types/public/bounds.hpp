#pragma once
#include <valarray>
#include <glm/vec3.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_geometric.hpp>

namespace Eng
{
class Bounds
{
public:
    Bounds() : min_val({FLT_MAX, FLT_MAX, FLT_MAX}), max_val({FLT_MIN, FLT_MIN, FLT_MIN})
    {
    }

    Bounds(const glm::vec3& in_min, const glm::vec3& in_max) : min_val(in_min), max_val(in_max)
    {
    }

    static Bounds from_extent(const glm::vec3& center, const glm::vec3& extent)
    {
        auto half_extent = extent / 2.f;
        return {center - half_extent, center + half_extent};
    }

    void add_point(const glm::vec3& pos)
    {
        if (pos.x > max_val.x)
            max_val.x = pos.x;
        if (pos.x < min_val.x)
            min_val.x = pos.x;

        if (pos.y > max_val.y)
            max_val.y = pos.y;
        if (pos.y < min_val.y)
            min_val.y = pos.y;

        if (pos.z > max_val.z)
            max_val.z = pos.z;
        if (pos.z < min_val.z)
            min_val.z = pos.z;
        outdated_radius = true;
    }

    const glm::vec3& min() const
    {
        return min_val;
    }

    const glm::vec3& max() const
    {
        return max_val;
    }

    glm::vec3 extent() const
    {
        return max_val - min_val;
    }

    glm::vec3 center() const
    {
        return (min_val + max_val) * 0.5f;
    }

    float radius()
    {
        if (outdated_radius)
        {
            outdated_radius = false;

            auto ext = extent();
            ext *= ext;
            radius_val = length(ext * 0.5f);
        }

        return radius_val;
    }

    Bounds& operator+=(const Bounds& other)
    {
        if (other.min_val.x < min_val.x)
            min_val.x = other.min_val.x;
        if (other.max_val.x > max_val.x)
            max_val.x = other.max_val.x;

        if (other.min_val.y < min_val.y)
            min_val.y = other.min_val.y;
        if (other.max_val.y > max_val.y)
            max_val.y = other.max_val.y;

        if (other.min_val.z < min_val.z)
            min_val.z = other.min_val.z;
        if (other.max_val.z > max_val.z)
            max_val.z = other.max_val.z;
        return *this;
    }

    operator bool() const
    {
        return min_val != glm::vec3{FLT_MAX, FLT_MAX, FLT_MAX} && max_val != glm::vec3{FLT_MIN, FLT_MIN, FLT_MIN};
    }

private:
    bool      outdated_radius = true;
    float     radius_val;
    glm::vec3 min_val;
    glm::vec3 max_val;
};

inline Bounds operator*(const glm::mat4& model_matrix, const Bounds& bounds)
{
    return Bounds{model_matrix * glm::vec4(bounds.min(), 1), model_matrix * glm::vec4(bounds.max(), 1)};
}

}