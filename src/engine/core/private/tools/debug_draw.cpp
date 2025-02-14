#include "tools/debug_draw.hpp"

#include "engine.hpp"
#include "object_ptr.hpp"
#include "assets/asset_registry.hpp"
#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx_types/format.hpp"
#include "scene/scene_view.hpp"

Eng::DebugDraw instance;

TObjectRef<Eng::MaterialAsset>                         debug_material;
TObjectRef<Eng::MaterialInstanceAsset>                 debug_material_instance;
std::shared_ptr<Eng::Gfx::Mesh>                        debug_mesh_lines;
std::unique_ptr<std::chrono::steady_clock::time_point> last_clear;

namespace Eng
{
DebugDraw& DebugDraw::get()
{
    return instance;
}

void DebugDraw::draw(Gfx::CommandBuffer& command_buffer, const SceneView& view)
{
    std::lock_guard lk(rw_lock);
    if (!debug_material_instance)
    {

        auto device    = Engine::get().get_device().lock();
        debug_material = Engine::get().asset_registry().create<MaterialAsset>("DebugMaterial");
        debug_material->update_options({
            .culling = Gfx::ECulling::None,
            .topology = Gfx::ETopology::Lines,
        });
        debug_material->set_shader_code("debug_draw", std::vector{
                                            StageInputOutputDescription{0, 0, Gfx::ColorFormat::R32G32B32_SFLOAT},
                                            StageInputOutputDescription{1, 12, Gfx::ColorFormat::R32G32B32_SFLOAT}
                                        });
        debug_material_instance = Engine::get().asset_registry().create<MaterialInstanceAsset>("DebugMaterialInst", debug_material);

        debug_mesh_lines = Gfx::Mesh::create("DebugMeshLine", device, sizeof(WireframePoint), Gfx::EBufferType::IMMEDIATE);
    }

    std::vector<WireframePoint> vertices;
    vertices.reserve(stored_segments.size());
    for (const auto& segment : stored_segments)
        vertices.emplace_back(segment.point);

    if (vertices.empty())
        return;

    debug_mesh_lines->set_vertices(0, Gfx::BufferData(vertices));

    auto mat = debug_material->get_permutation(debug_material->get_default_permutation()).lock()->get_resource(command_buffer.render_pass());
    command_buffer.bind_pipeline(mat);
    debug_material_instance->set_scene_data(command_buffer.render_pass(), view.get_view_buffer());
    command_buffer.bind_descriptors(*debug_material_instance->get_descriptor_resource(command_buffer.render_pass()), *mat);
    command_buffer.draw_mesh(*debug_mesh_lines);
}

void DebugDraw::destroy()
{
    debug_material.destroy();
    debug_material_instance.destroy();
    debug_mesh_lines = nullptr;
    last_clear       = nullptr;
    stored_segments.clear();
}

void DebugDraw::flush()
{
    std::lock_guard lk(rw_lock);
    faces.clear();

    const auto now = std::chrono::steady_clock::now();

    if (!last_clear)
        last_clear = std::make_unique<std::chrono::steady_clock::time_point>(now);

    const float elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(now - *last_clear).count()) / 1000000000.f;
    *last_clear          = now;

    for (int64_t i = static_cast<int64_t>(stored_segments.size()) - 1; i >= 0; --i)
    {
        stored_segments[i].remaining_duration -= elapsed;
        if (stored_segments[i].remaining_duration <= 0)
            stored_segments.erase(stored_segments.begin() + i);
    }
}
} // namespace Eng