#pragma once
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
            vertices.emplace_back(PointInfo{
                .coord = sphere_to_rect_coords({acos(1 - 2.0 * static_cast<double>(i) / static_cast<double>(n_points)),
                                                std::fmod((2 * std::numbers::pi * static_cast<double>(i) / std::numbers::phi), 2.0 * std::numbers::pi)}),
                .triangles = {}});

        test_delaunay();
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
    static double sign(const SphereCoord& p1, const SphereCoord& p2, const SphereCoord& p3)
    {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }

    static bool point_in_triangle(const glm::dvec3& P, glm::dvec3 A, glm::dvec3 B, glm::dvec3 C)
    {
        LOG_WARNING("T");

        return dot(C - B, P - B) > 0 && dot(A - C, P - C) > 0 && dot(B - A, P - A) > 0;

        auto AC = C - A;

        auto AB = B - A;

        auto AP = P - A;

        // Compute dot products
        auto dot00 = dot(AC, AC);
        auto dot01 = dot(AC, AB);
        auto dot02 = dot(AC, AP);
        auto dot11 = dot(AB, AB);
        auto dot12 = dot(AB, AP);

        // Compute barycentric coordinates
        auto invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
        auto u        = (dot11 * dot02 - dot01 * dot12) * invDenom;
        auto v        = (dot00 * dot12 - dot01 * dot02) * invDenom;

        // Check if point is in triangle
        return (u >= 0) && (v >= 0) && (u + v < 1);
    }

    void test_delaunay()
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

        for (size_t p = 20; p < vertices.size() && p < 27; ++p)
        {
            // Search the concerned triangle and split it
            for (size_t t = 0; t < triangles.size(); ++t)
            {
                glm::dvec3 Pr = sphere_to_rect_coords(vertices[p].coord);
                glm::dvec3 Ar = sphere_to_rect_coords(triangles[t].A);
                glm::dvec3 Br = sphere_to_rect_coords(triangles[t].B);
                glm::dvec3 Cr = sphere_to_rect_coords(triangles[t].C);

                // Break-insert
                if (point_in_triangle(Pr, Ar, Br, Cr))
                {
                    LOG_WARNING("ah ??");
                    Triangle old = triangles[t];

                    const glm::dvec3 P = vertices[p].coord;

                    // Update / add new adjacent triangles
                    vertices[old.a].triangles.emplace_back(triangles.size() + 1);
                    vertices[old.b].triangles.emplace_back(triangles.size());
                    vertices[old.c].triangles.emplace_back(triangles.size());
                    *std::ranges::find(vertices[old.a].triangles, t) = triangles.size() + 1;

                    //triangles.erase(triangles.begin() + t);

                    // Update / generate triangles
                    triangles[t] = Triangle{.A = old.A, .B = P, .C = old.C, .a = old.a, .b = p, .c = old.c};
                    triangles.emplace_back(Triangle{.A = old.B, .B = P, .C = old.C, .a = old.b, .b = p, .c = old.a});
                    triangles.emplace_back(Triangle{.A = old.C, .B = P, .C = old.B, .a = old.c, .b = p, .c = old.b});

                    break;
                }
            }

            //break; // @todo : remove this
        }
    }

    struct Triangle
    {
        glm::dvec3 A, B, C;
        size_t      a, b, c;
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