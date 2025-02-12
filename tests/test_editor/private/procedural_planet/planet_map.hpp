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

inline glm::dvec3 cubify(const glm::dvec3& s)
{
    const double inv_sqrt_2 = 1.0 / std::sqrt(2.0);

    double xx2 = s.x * s.x * 2.0;
    double yy2 = s.y * s.y * 2.0;

    glm::dvec2 v = glm::dvec2(xx2 - yy2, yy2 - xx2);

    double ii = v.y - 3.f;
    ii *= ii;

    double isqrt = -std::sqrt(ii - 12.0 * xx2) + 3.0;

    v = glm::dvec2(std::sqrt(v.x + isqrt), std::sqrt(v.y + isqrt));
    v *= inv_sqrt_2;

    return sign(s) * glm::dvec3(v, 1.0);
}

inline glm::dvec3 sphere_to_cube(const glm::dvec3 sphere)
{
    glm::dvec3 f = glm::abs(sphere);

    if (f.y >= f.x && f.y >= f.z)
    {
        const auto res = cubify(glm::dvec3(sphere.x, sphere.z, sphere.y));
        return {res.x, res.z, res.y};
    }
    if (f.x >= f.z)
    {
        const auto res = cubify(glm::dvec3(sphere.y, sphere.z, sphere.x));
        return {res.y, res.z, res.x};
    }
    return cubify(sphere);
}

enum Face
{
    Front  = 0,
    Back   = 1,
    Right  = 2,
    Left   = 3,
    Top    = 4,
    Bottom = 5
};

struct CubeFacePosition
{
    Face       face;
    glm::dvec2 uv;
};

inline CubeFacePosition sphere_to_cube_face(const glm::dvec3 sphere)
{
    glm::dvec3 f = glm::abs(sphere);

    if (f.y >= f.x && f.y >= f.z)
    {
        const auto res = cubify(glm::dvec3(sphere.x, sphere.z, sphere.y));
        return {.face = f.y > 0 ? Face::Right : Face::Left, .uv = {res.x, res.z}};
    }
    if (f.x >= f.z)
    {
        const auto res = cubify(glm::dvec3(sphere.y, sphere.z, sphere.x));
        return {.face = f.x > 0 ? Face::Front : Face::Back, .uv = {res.y, res.z}};
    }
    const auto res = cubify(sphere);
    return {.face = f.z > 0 ? Face::Top : Face::Bottom, .uv = {res.x, res.y}};
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

    SampleValues sample(const CubeFacePosition& linear_coords)
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