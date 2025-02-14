#pragma once
#include "tools/debug_draw.hpp"

#include <numbers>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using SphereCoord = glm::dvec2;


/**
 * Approximately evenly distributed points on a sphere : https://www.johndcook.com/blog/2023/08/12/fibonacci-lattice/
 * @tparam Point_T Custom point data
 */
//template <typename Point_T>
class PlanetSampleMesh
{
public:
    struct PointInfo
    {
        glm::dvec3          coord;
        std::vector<size_t> triangles;
        // Point_T             data;
    };

    PlanetSampleMesh(size_t n_points)
    {
        for (size_t i = 0; i < n_points; ++i)
        {

            vertices.emplace_back(PointInfo{.coord = sphere_to_rect_coords({acos(1 - 2.0 * static_cast<double>(i) / static_cast<double>(n_points)),
                                                                            std::fmod((2 * std::numbers::pi * static_cast<double>(i) / std::numbers::phi), 2.0 * std::numbers::pi)}),
                                            .triangles = {}});

            Eng::DebugDraw::get().add_segment({0, 0, 0}, vertices.back().coord * 6500.0, {0, 0, 1}, 100);
        }

        triangulate();
    }

    std::vector<uint32_t> get_triangle_indices() const
    {
        std::vector<uint32_t> tris;
        for (const auto& triangle : triangles)
        {
            tris.emplace_back(static_cast<uint32_t>(triangle.a));
            tris.emplace_back(static_cast<uint32_t>(triangle.b));
            tris.emplace_back(static_cast<uint32_t>(triangle.c));
        }
        return tris;
    }

    const std::vector<PointInfo>& get_vertices() const
    {
        return vertices;
    }

    static SphereCoord rect_to_sphere_coords(const glm::dvec3& rect)
    {
        return {acos(rect.z / std::sqrt(rect.x * rect.x + rect.y * rect.y + rect.z * rect.z)), atan2(rect.y, rect.x)};
    }

    static glm::dvec3 sphere_to_rect_coords(const SphereCoord& rect)
    {
        return {sin(rect.x) * cos(rect.y), sin(rect.x) * sin(rect.y), cos(rect.x)};
    }

private:
    static double signed_volume(const glm::dvec3& a, const glm::dvec3& b, const glm::dvec3& c, const glm::dvec3& d)
    {
        return 1 / 6.0 * dot(cross(b - a, c - a), d - a);
    }

    static bool point_in_triangle(const glm::dvec3& P, glm::dvec3 A, glm::dvec3 B, glm::dvec3 C)
    {
        const glm::dvec3 AB = B - A;
        const glm::dvec3 AC = C - A;
        const glm::dvec3 N     = cross(normalize(P), AC);
        const double     a     = dot(AB, N);
        if (a > -std::numeric_limits<double>::epsilon() && a < std::numeric_limits<double>::epsilon())
            return false;

        double     f = 1.0 / a;
        glm::dvec3 s = -A;
        double     u = f * dot(s, N);
        if (u < 0.0 || u > 1.0)
            return false;
        glm::dvec3 q = cross(s, AB);
        double     v = f * dot(normalize(P), q);
        if (v < 0.0 || u + v > 1.0)
            return false;

        double t = f * dot(AC, q);
        if (t > std::numeric_limits<double>::epsilon())
            return true;
        return false;
    }

    bool test_delaunay(size_t t, size_t v1, size_t v2) const
    {
        const Triangle& tri = triangles[t];

        auto AB = tri.B - tri.A;
        auto AC = tri.C - tri.A;

        glm::dvec3 G = cross(AB, AC) / (length(AB) * length(AC));

        for (const auto& t1 : vertices[v1].triangles)
            for (const auto& t2 : vertices[v2].triangles)
                if (t1 == t2 && t1 != t)
                {
                    const Triangle& tri_opp = triangles[t1];
                    const auto&     N       = (tri_opp.a == v1 || tri_opp.a == v2) ? (tri_opp.b == v1 || tri_opp.b == v2) ? tri_opp.C : tri_opp.B : tri_opp.A;
                    if (length(G - tri.A) > length(G - N))
                    {
                        // @TODO : flip edge
                        Eng::DebugDraw::get().add_segment({}, G * 6000.0, {1, 0, 0}, 100);
                        return true;
                    }
                    return false;
                }

        LOG_FATAL("Unhandled case");
    }

    void triangulate()
    {
        // Initialize with octahedron triangles
        const size_t octahedron_start = vertices.size();
        vertices.push_back(PointInfo{.coord = {0, 0, 1}, .triangles = {0, 1, 2, 3}});
        vertices.push_back(PointInfo{.coord = {0, 1, 0}, .triangles = {0, 3, 4, 7}});
        vertices.push_back(PointInfo{.coord = {1, 0, 0}, .triangles = {0, 1, 4, 5}});
        vertices.push_back(PointInfo{.coord = {0, -1, 0}, .triangles = {1, 2, 5, 6}});
        vertices.push_back(PointInfo{.coord = {-1, 0, 0}, .triangles = {2, 3, 6, 7}});
        vertices.push_back(PointInfo{.coord = {0, 0, -1}, .triangles = {4, 5, 6, 7}});
        triangles = {
            make_triangle(octahedron_start + 0, octahedron_start + 2, octahedron_start + 1),
            make_triangle(octahedron_start + 0, octahedron_start + 3, octahedron_start + 2),
            make_triangle(octahedron_start + 0, octahedron_start + 4, octahedron_start + 3),
            make_triangle(octahedron_start + 0, octahedron_start + 1, octahedron_start + 4),
            make_triangle(octahedron_start + 1, octahedron_start + 2, octahedron_start + 5),
            make_triangle(octahedron_start + 2, octahedron_start + 3, octahedron_start + 5),
            make_triangle(octahedron_start + 3, octahedron_start + 4, octahedron_start + 5),
            make_triangle(octahedron_start + 4, octahedron_start + 1, octahedron_start + 5),
        };

        for (size_t p = 20; p < vertices.size(); ++p)
        {
            // Search the concerned triangle and split it
            for (size_t t = 0; t < triangles.size(); ++t)
            {
                const glm::dvec3 P = vertices[p].coord;

                // Break-insert
                if (point_in_triangle(P, triangles[t].A, triangles[t].B, triangles[t].C))
                {
                    Triangle old_triangle = triangles[t];

                    // Update / add new adjacent triangles

                    // Update A (Keep T1)
                    vertices[old_triangle.a].triangles.emplace_back(triangles.size() + 1); // Add T3
                    // Update B (Keep T1)
                    vertices[old_triangle.b].triangles.emplace_back(triangles.size()); // Add T2
                    // Update C
                    vertices[old_triangle.c].triangles.emplace_back(triangles.size());                // Add T2
                    *std::ranges::find(vertices[old_triangle.c].triangles, t) = triangles.size() + 1; // Unset T1 / Set T3

                    // Update / generate triangles
                    triangles[t] = Triangle{.A = old_triangle.B, .B = P, .C = old_triangle.A, .a = old_triangle.b, .b = p, .c = old_triangle.a};          // T1
                    triangles.emplace_back(Triangle{.A = old_triangle.C, .B = P, .C = old_triangle.B, .a = old_triangle.c, .b = p, .c = old_triangle.b}); // T2
                    triangles.emplace_back(Triangle{.A = old_triangle.A, .B = P, .C = old_triangle.C, .a = old_triangle.a, .b = p, .c = old_triangle.c}); // T3

                    if (test_delaunay(t, old_triangle.a, old_triangle.b))
                    {
                    }

                    if (test_delaunay(triangles.size() - 2, old_triangle.b, old_triangle.c))
                    {
                    }
                    if (test_delaunay(triangles.size() - 1, old_triangle.c, old_triangle.a))
                    {
                    }

                    break;
                }
            }

            break; // @todo : remove this
        }

        for (const auto& triangle : triangles)
        {
            Eng::DebugDraw::get().add_segment(triangle.A * 6000.0, triangle.B * 6000.0, {1, 1, 0}, 100);
            Eng::DebugDraw::get().add_segment(triangle.A * 6000.0, triangle.C * 6000.0, {1, 1, 0}, 100);
            Eng::DebugDraw::get().add_segment(triangle.C * 6000.0, triangle.B * 6000.0, {1, 1, 0}, 100);
        }
    }

    struct Triangle
    {
        glm::dvec3 A, B, C;
        size_t     a, b, c;
    };

    bool test_delaunay(const glm::dvec3& A, const glm::dvec3& B, const glm::dvec3& C, const glm::dvec3& D)
    {
        auto AB = B - A;
        auto AC = C - A;

        auto G = AB * AC / (length(AB) * length(AC));

    }

    Triangle make_triangle(size_t a, size_t b, size_t c) const
    {
        return Triangle{.A = vertices[a].coord, .B = vertices[b].coord, .C = vertices[c].coord, .a = a, .b = b, .c = c};
    }

    std::vector<PointInfo> vertices;
    std::vector<Triangle>  triangles;
};