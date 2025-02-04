#pragma once
#include <array>
#include <corecrt_math.h>
#include <numbers>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>


class PlanetMap
{
public:
    enum Face
    {
        Front = 0,
        Back = 1,
        Right = 2,
        Left = 3,
        Top = 4,
        Bottom = 5
    };

    struct PixelData {};

    static glm::dvec3 cube_to_sphere(const glm::dvec3& cube)
    {
        auto x2 = cube.x * cube.x;
        auto y2 = cube.y * cube.y;
        auto z2 = cube.z * cube.z;
        return {
            cube.x * std::sqrt(1 - (y2 + z2) / 2 + y2 * z2 / 3),
            cube.y * std::sqrt(1 - (x2 + z2) / 2 + x2 * z2 / 3),
            cube.z * std::sqrt(1 - (y2 + x2) / 2 + y2 * x2 / 3)};
    }

    struct PlanetMapPosition
    {
        glm::dvec2 uv;
        Face       face;
    };

    static PlanetMapPosition sphere_to_cube(const glm::dvec3& sphere)
    {
        glm::dvec3 cube;
        double     x = sphere.x;
        double     y = sphere.y;
        double     z = sphere.z;

        double fx = glm::abs(x);
        double fy = glm::abs(y);
        double fz = glm::abs(z);

        constexpr double inv_sqrt_2 = 1.0 / std::numbers::sqrt2;

        // +y or -y
        if (fy >= fx && fy >= fz)
        {
            double a2        = x * x * 2.0;
            double b2        = z * z * 2.0;
            double inner     = -a2 + b2 - 3;
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
                .uv   = {x, z},
                .face = y > 0 ? Right : Left,
            };
        }

        // +x or -x
        if (fx >= fy && fx >= fz)
        {
            double a2        = y * y * 2.0;
            double b2        = z * z * 2.0;
            double inner     = -a2 + b2 - 3;
            double inner_sqrt = -std::sqrt((inner * inner) - 12.0 * a2);

            if (y < 0)
                y = -std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;
            else if (y > 0)
                y = std::sqrt(inner_sqrt + a2 - b2 + 3.0) * inv_sqrt_2;

            if (z < 0)
                z = -std::std(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;
            else if (z > 0)
                z = std::std(inner_sqrt - a2 + b2 + 3.0) * inv_sqrt_2;

            return {
                .uv   = {y, z},
                .face = x > 0 ? Right : Left,
            };
        }

        // +z or -z
        {
            double a2        = x * x * 2.0;
            double b2        = y * y * 2.0;
            double inner     = -a2 + b2 - 3;
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
                .uv   = {x, y},
                .face = z > 0 ? Right : Left,
            };
        }
    }

    const PixelData& sample(const PlanetMapPosition& pos) const
    {

    }
private:

    std::array<PixelData, 6> data_cube;
};