#pragma once
#include "tools/debug_draw.hpp"

#include <numbers>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using SphereCoord = glm::dvec2;

template <> struct std::formatter<glm::dvec3>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const glm::dvec3& p, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "({}, {}, {})", p.x, p.y, p.z);
    }
};


class HalfEdgeMeshStructure
{
    class Vertex_V;
    class HalfEdge_V;
    class Face_V;

public:
    using Vertex   = Vertex_V*;
    using HalfEdge = HalfEdge_V*;
    using Face     = Face_V*;

    ~HalfEdgeMeshStructure()
    {
        for (const auto& vertex : vertices)
            delete vertex;
        for (const auto& hedge : hedges)
            delete hedge;
        for (const auto& face : faces)
            delete face;
    }

    Vertex add_vertex(const glm::dvec3& position)
    {
        Vertex v    = new Vertex_V();
        v->position = position;
        register_vertex(v);
        return v;
    }

    Face add_face(const std::vector<Vertex>& face_vertices)
    {
        Face f = new Face_V();

        std::vector<HalfEdge> new_hedge;
        for (const auto& v : face_vertices)
        {
            HalfEdge h = new HalfEdge_V();
            h->origin  = v;
            h->face    = f;
            new_hedge.emplace_back(h);
        }

        for (size_t i = 0; i < new_hedge.size(); ++i)
        {
            HalfEdge current  = new_hedge[i];
            current->previous = new_hedge[(i + new_hedge.size() - 1) % new_hedge.size()];
            current->next     = new_hedge[(i + 1) % new_hedge.size()];

            // Iterate over all the half hedges of the next point
            for (const auto& next_h : current->next->origin->half_edges)
            {
                if (next_h->next->origin == current->origin)
                {
                    // If we're going back to the first edge origin, it's our twin
                    assert(!current->twin); // Note : we should never find multiple twins half edges
                    current->twin = next_h;
                    next_h->twin  = current;
                }
            }

        }

        // Only if success
        for (size_t i = 0; i < new_hedge.size(); ++i)
        {
            face_vertices[i]->add_hedge(new_hedge[i]);
            register_hedge(new_hedge[i]);
        }
        register_face(f);
        f->hedge = new_hedge[0];
        return f;
    }

    void remove_face(Face f)
    {
        bool b_start = false;
        for (auto h = f->hedge; h != f->hedge || !b_start; h = h->next, b_start = true)
        {
            if (h->twin)
                h->twin->twin = nullptr;
            h->origin->remove_edge(h);

            unregister_hedge(h);
        }
        unregister_face(f);
    }

    void remove_vertex(Vertex v)
    {
        ankerl::unordered_dense::set<Face> deleted_faces;
        for (const auto& h : v->half_edges)
            deleted_faces.insert(h->face);

        for (const auto& f : deleted_faces)
            remove_face(f);
        unregister_vertex(v);
    }

    const std::vector<Face>& get_faces() const
    {
        return faces;
    }

    const std::vector<Vertex>& get_vertices() const
    {
        return vertices;
    }

    const std::vector<HalfEdge>& get_hedges() const
    {
        return hedges;
    }

    struct PublicMeshData
    {
        glm::dvec3 position;
    };

    void compile_mesh_data(std::vector<PublicMeshData>& out_vertices, std::vector<uint32_t>& out_faces) const
    {
        out_vertices.reserve(vertices.size());
        out_faces.reserve(faces.size() * 3);
        for (const auto& vertex : vertices)
            out_vertices.emplace_back(vertex->position);

        for (const auto& face : faces)
        {
            bool b_init = false;
            for (auto cur = face->hedge; cur != face->hedge || !b_init; cur = cur->next, b_init = true)
                out_faces.emplace_back(cur->origin->index);
        }
    }

private:
    std::vector<Vertex>   vertices;
    std::vector<HalfEdge> hedges;
    std::vector<Face>     faces;

    void register_vertex(Vertex v)
    {
        v->index = vertices.size();
        vertices.emplace_back(v);
    }

    void unregister_vertex(Vertex v)
    {
        assert(v->index != UINT64_MAX);
        assert(v->index < vertices.size());

        if (vertices.size() == 1)
        {
            vertices.clear();
            return;
        }

        // Swap back
        vertices[v->index]        = vertices.back();
        vertices[v->index]->index = v->index;
        vertices.pop_back();
        v->index = UINT64_MAX;
    }

    void register_face(Face f)
    {
        f->index = faces.size();
        faces.emplace_back(f);
    }

    void unregister_face(Face f)
    {
        assert(f->index != UINT64_MAX);
        assert(f->index < faces.size());

        if (faces.size() == 1)
        {
            faces.clear();
            return;
        }

        // Swap back
        faces[f->index]        = faces.back();
        faces[f->index]->index = f->index;
        faces.pop_back();
        f->index = UINT64_MAX;
    }

    void register_hedge(HalfEdge e)
    {
        e->index = hedges.size();
        hedges.emplace_back(e);
    }

    void unregister_hedge(HalfEdge e)
    {
        assert(e->index != UINT64_MAX);
        assert(e->index < hedges.size());

        if (hedges.size() == 1)
        {
            hedges.clear();
            return;
        }

        // Swap back
        hedges[e->index]        = hedges.back();
        hedges[e->index]->index = e->index;
        hedges.pop_back();
        e->index = UINT64_MAX;
    }

    class HalfEdge_V
    {
        friend class HalfEdgeMeshStructure;

    public:
        HalfEdge twin     = nullptr;
        HalfEdge next     = nullptr;
        HalfEdge previous = nullptr;
        Vertex   origin   = nullptr;
        Face     face     = nullptr;

    private:
        HalfEdge_V() = default;
        size_t index = SIZE_MAX;
    };

    class Vertex_V
    {
        friend class HalfEdgeMeshStructure;

    public:
        glm::dvec3            position;
        std::vector<HalfEdge> half_edges;

    private:
        Vertex_V() = default;

        void add_hedge(HalfEdge h)
        {
            half_edges.emplace_back(h);
        }

        void remove_edge(HalfEdge h)
        {
            for (int64_t i = static_cast<int64_t>(half_edges.size()) - 1; i >= 0; --i)
                if (half_edges[i] == h)
                {
                    half_edges.erase(half_edges.begin() + i);
                    break;
                }
        }

        size_t index = SIZE_MAX;
    };

    class Face_V
    {
        friend class HalfEdgeMeshStructure;

    public:
        HalfEdge hedge = nullptr;

    private:
        Face_V() = default;
        size_t index = SIZE_MAX;
    };
};


/**
 * Approximately evenly distributed points on a sphere : https://www.johndcook.com/blog/2023/08/12/fibonacci-lattice/
 * @tparam Point_T Custom point data
 */
//template <typename Point_T>
class PlanetSampleMesh
{
public:
    PlanetSampleMesh(size_t n_points)
    {
        for (size_t i = 0; i < n_points; ++i)
        {
            glm::dvec3 pos =
                sphere_to_rect_coords({acos(1 - 2.0 * static_cast<double>(i) / static_cast<double>(n_points)), std::fmod(2 * std::numbers::pi * static_cast<double>(i) / std::numbers::phi, 2.0 * std::numbers::pi)});

            Eng::DebugDraw::get().add_segment(pos * 6000.0, pos * 6500.0, {0, 0, 1}, 100);
            mesh.add_vertex(pos);
        }

        triangulate();
    }

    void compile_mesh_data(std::vector<HalfEdgeMeshStructure::PublicMeshData>& out_vertices, std::vector<uint32_t>& out_faces) const
    {
        return mesh.compile_mesh_data(out_vertices, out_faces);
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
        const glm::dvec3 N  = cross(normalize(P), AC);
        const double     a  = dot(AB, N);
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

    bool test_delaunay(HalfEdgeMeshStructure::Face f)
    {
        auto A = f->hedge->origin;
        auto B = f->hedge->next->origin;
        auto C = f->hedge->previous->origin;

        auto ABprime = f->hedge->twin;

        auto N = ABprime->previous->origin;

        auto AB = B->position - A->position;
        auto AC = C->position - A->position;

        glm::dvec3 G = cross(AB, AC) / (length(AB) * length(AC));

        if (length(G - A->position) > length(G - N->position))
        {
            Eng::DebugDraw::get().add_segment(A->position * 6000.0, B->position * 6000.0, {0, 1, 1}, 100);
            mesh.remove_face(f);
            mesh.remove_face(ABprime->face);
            //auto F1 = mesh.add_face({N, C, A});
            //auto F2 = mesh.add_face({C, N, B});

            /*
            if (!test_delaunay(F1->hedge->next->twin->face))
                ;//if (!test_delaunay(F1->hedge->previous->twin->face))
                    ;/*
                    if (!test_delaunay(F2->hedge->next->twin->face))
                        if (!test_delaunay(F2->hedge->previous->twin->face))
                            ;*/
            return true;
        }
        return false;
    }

    void triangulate()
    {
        size_t vertex_count = mesh.get_vertices().size();
        // Initialize with octahedron triangles
        auto V0 = mesh.add_vertex({0, 0, 1});
        auto V1 = mesh.add_vertex({0, 1, 0});
        auto V2 = mesh.add_vertex({1, 0, 0});
        auto V3 = mesh.add_vertex({0, -1, 0});
        auto V4 = mesh.add_vertex({-1, 0, 0});
        auto V5 = mesh.add_vertex({0, 0, -1});
        mesh.add_face({V0, V2, V1});
        mesh.add_face({V0, V3, V2});
        mesh.add_face({V0, V4, V3});
        mesh.add_face({V0, V1, V4});
        mesh.add_face({V1, V2, V5});
        mesh.add_face({V2, V3, V5});
        mesh.add_face({V3, V4, V5});
        mesh.add_face({V4, V1, V5});

        for (size_t i = 0; i < vertex_count; ++i)
        {
            const auto& P = mesh.get_vertices()[i];
            // Search the concerned triangle and split it
            for (const auto& face : mesh.get_faces())
            {
                auto A = face->hedge->origin;
                auto B = face->hedge->next->origin;
                auto C = face->hedge->previous->origin;

                // Break-insert
                if (point_in_triangle(P->position, A->position, B->position, C->position))
                {
                    mesh.remove_face(face);
                    auto F1 = mesh.add_face({A, B, P});
                    auto F2 = mesh.add_face({B, C, P});
                    auto F3 = mesh.add_face({C, A, P});
                    test_delaunay(F1);
                    test_delaunay(F2);
                    test_delaunay(F3);
                    break;
                }
            }
        }

        for (const auto& triangle : mesh.get_faces())
        {
            auto a = triangle->hedge;
            for (int i = 0; i < 3; ++i, a = a->next)
                Eng::DebugDraw::get().add_segment(a->origin->position * 6005.0, a->next->origin->position * 6005.0, {1, 1, 0}, 100);
        }
    }

    HalfEdgeMeshStructure mesh;
};