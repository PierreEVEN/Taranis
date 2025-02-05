#pragma once
#include <array>
#include <corecrt_math.h>
#include <numbers>
#include <vector>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

inline static glm::dvec3 cube_to_sphere(const glm::dvec3& cube)
{
    auto x2 = cube.x * cube.x;
    auto y2 = cube.y * cube.y;
    auto z2 = cube.z * cube.z;
    return {cube.x * std::sqrt(1 - (y2 + z2) / 2 + y2 * z2 / 3), cube.y * std::sqrt(1 - (x2 + z2) / 2 + x2 * z2 / 3), cube.z * std::sqrt(1 - (y2 + x2) / 2 + y2 * x2 / 3)};
}

enum Face
{
    Front = 0,
    Back = 1,
    Right = 2,
    Left = 3,
    Top = 4,
    Bottom = 5
};

struct PlanetMapPosition
{
    glm::dvec2 uv;
    Face       face;
};

inline static PlanetMapPosition sphere_to_cube(const glm::dvec3& sphere)
{
    double x = sphere.x;
    double y = sphere.y;
    double z = sphere.z;

    double fx = glm::abs(x);
    double fy = glm::abs(y);
    double fz = glm::abs(z);

    constexpr double inv_sqrt_2 = 1.0 / std::numbers::sqrt2;

    // +y or -y
    if (fy >= fx && fy >= fz)
    {
        double a2         = x * x * 2.0;
        double b2         = z * z * 2.0;
        double inner      = -a2 + b2 - 3;
        double inner_sqrt = -std::sqrt((inner * inner) - 12.0 * a2);

        if (x < 0)
            x = -std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;
        else if (x > 0)
            x = std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;

        if (z < 0)
            z = -std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;
        else if (z > 0)
            z = std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;

        return {
            .uv = {x, z},
            .face = y > 0 ? Right : Left,
        };
    }

    // +x or -x
    if (fx >= fy && fx >= fz)
    {
        double a2         = y * y * 2.0;
        double b2         = z * z * 2.0;
        double inner      = -a2 + b2 - 3;
        double inner_sqrt = -std::sqrt((inner * inner) - 12.0 * a2);

        if (y < 0)
            y = -std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;
        else if (y > 0)
            y = std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;

        if (z < 0)
            z = -std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;
        else if (z > 0)
            z = std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;

        return {
            .uv = {y, z},
            .face = x > 0 ? Right : Left,
        };
    }

    // +z or -z
    {
        double a2         = x * x * 2.0;
        double b2         = y * y * 2.0;
        double inner      = -a2 + b2 - 3;
        double inner_sqrt = -std::sqrt((inner * inner) - 12.0 * a2);

        if (x < 0)
            x = -std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;
        else if (x > 0)
            x = std::sqrt(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;

        if (y < 0)
            y = -std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;
        else if (y > 0)
            y = std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;

        return {
            .uv = {x, y},
            .face = z > 0 ? Right : Left,
        };
    }
}

template <typename Pixel_T>
class PlanetMap
{
public:
    uint32_t resolution() const
    {
        return res;
    }

    const Pixel_T& operator[](std::pair<Face, const glm::uvec2&> coords) const
    {
        return data_cube[static_cast<uint8_t>(coords.first)][coords.second.x + coords.second.y * res];
    }

    const Pixel_T& operator[](std::pair<Face, size_t> coords) const
    {
        return data_cube[static_cast<uint8_t>(coords.first)][coords.second];
    }

    Pixel_T& operator[](std::pair<Face, const glm::uvec2&> coords)
    {
        return data_cube[static_cast<uint8_t>(coords.first)][coords.second.x + coords.second.y * res];
    }

    Pixel_T& operator[](std::pair<Face, size_t> coords)
    {
        return data_cube[static_cast<uint8_t>(coords.first)][coords.second];
    }

    PlanetMap() = default;

    PlanetMap(uint32_t in_res)
    {
        res = in_res;
        for (int i = 0; i < 6; ++i)
            data_cube[i].resize(static_cast<size_t>(res) * static_cast<size_t>(res));
    }

    glm::dvec3 get_cube_position(Face face, size_t pixel_index) const
    {
        return get_cube_position(face, {pixel_index / res, pixel_index % res});
    }

    glm::dvec3 get_cube_position(Face face, const glm::uvec2& coords) const
    {
        glm::dvec2 linear_coords = {static_cast<double>(coords.x) / res * 2 - 1, static_cast<double>(coords.y) / res * 2 - 1};
        switch (face)
        {
        case Front:
            return {1.0, linear_coords.y, linear_coords.y};
        case Back:
            return {-1.0, linear_coords.y, linear_coords.y};
        case Right:
            return {linear_coords.y, 1.0, linear_coords.y};
        case Left:
            return {linear_coords.y, -1.0, linear_coords.y};
        case Top:
            return {linear_coords.y, linear_coords.y, 1.0};
        case Bottom:
            return {linear_coords.y, linear_coords.y, -1.0};
        }
        return {};
    }

    struct SampleValues
    {
        size_t p1, p2, p3, p4;
        float  v1, v2, v3, v4;
    };

    SampleValues sample(const PlanetMapPosition& linear_coords)
    {
        glm::dvec2 scaled = (linear_coords.uv / 2.0 + 0.5) * static_cast<double>(res - 1);

        glm::uvec2 p1 = {
            std::floor(scaled.x),
            std::floor(scaled.y),
        };
        glm::uvec2 p2 = {
            std::ceil(scaled.x),
            std::floor(scaled.y),
        };
        glm::uvec2 p3 = {
            std::ceil(scaled.x),
            std::ceil(scaled.y),
        };
        glm::uvec2 p4 = {
            std::floor(scaled.x),
            std::ceil(scaled.y),
        };

        return {
            .p1 = p1.x + p1.y * res,
            .p2 = p2.x + p2.y * res,
            .p3 = p3.x + p3.y * res,
            .p4 = p4.x + p4.y * res,
            .v1 = 0.25f,
            .v2 = 0.25f,
            .v3 = 0.25f,
            .v4 = 0.25f
        };
    }

private:
    uint32_t                            res = 0;
    std::array<std::vector<Pixel_T>, 6> data_cube;
};