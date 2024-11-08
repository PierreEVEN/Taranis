#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/material_asset.hpp"
#include "assets/mesh_asset.hpp"

#include <gfx/window.hpp>

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/shader_module.hpp"
#include "import/gltf_import.hpp"
#include "import/stb_import.hpp"
#include "scene/scene.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"

namespace Engine::Gfx
{
class ImGuiWrapper;
}


const char* VERTEX = "\
              struct VSInput\
{\
\
    [[vk::location(0)]]  float3 pos : POSITION;\
[[vk::location(1)]] float2     uv : TEXCOORD0;\
[[vk::location(2)]] float3        normal: NORMAL0;\
[[vk::location(3)]] float3        tangent : TANGENT0;\
[[vk::location(4)]] float4     color : COLOR0;\
};\
struct VsToFs\
{\
    float4 Pos : SV_Position;\
    float4 vtxpos : POSITION0;\
};\
struct PushConsts\
{\
    float4x4 model;\
    float4x4 camera;\
};\
[[vk::push_constant]] ConstantBuffer<PushConsts> pc;\
VsToFs                                           main(VSInput input)\
{\
    VsToFs Out;\
    Out.Pos    = mul(pc.camera, mul(pc.model, float4(input.pos, 1)));\
    Out.vtxpos = float4(input.pos, 1);\
    return Out;\
}";

const char* FRAGMENT = "\
                     struct VsToFs\
{\
    float4 Pos : SV_Position;\
    float4 vtxpos : POSITION0;\
};\
\
float pow_cord(float val)\
{\
    return pow(abs(sin(val * 50)), 10);\
}\
\
/*[[vk::binding(0)]] SamplerState sSampler;\
[[vk::binding(1)]] Texture2D    sTexture;*/\
\
float4 main(VsToFs input) : SV_TARGET\
{\
    return float4(0.5, 0.6, 0, 1);\
    /*float intens = ((pow_cord(input.vtxpos.x) + pow_cord(input.vtxpos.y) + pow_cord(input.vtxpos.z)) * 0.4 + 0.1) * 0.01;\
    return sTexture.Sample(sSampler, input.vtxpos.xy * 0.01) + float4(float3(intens, intens, intens), 1);*/\
}";


class TestFirstPassInterface : public Engine::Gfx::RenderPassInterface
{
public:
    TestFirstPassInterface(std::weak_ptr<Engine::Gfx::Window> parent_window, Engine::Scene& in_scene) : window(std::move(parent_window)), scene(&in_scene)
    {
    }

    void init(const std::weak_ptr<Engine::Gfx::Device>& device, const Engine::Gfx::RenderPassInstanceBase& render_pass) override
    {
        imgui = std::make_unique<Engine::Gfx::ImGuiWrapper>("imgui_renderer", render_pass.get_render_pass(), device, window);
    }

    void render(const Engine::Gfx::RenderPassInstanceBase& render_pass, const Engine::Gfx::CommandBuffer& command_buffer) override
    {
        scene->draw(command_buffer);

        imgui->begin(render_pass.resolution());

        if (ImGui::Begin("test"))
        {
            ImGui::LabelText("ms", "%lf", Engine::Engine::get().delta_second * 1000);
            ImGui::LabelText("fps", "%lf", 1.0 / Engine::Engine::get().delta_second);

            int idx = 0;
            for (const auto& asset : Engine::Engine::get().asset_registry().all_assets())
            {
                if (auto texture = asset.second.cast<Engine::TextureAsset>())
                {
                    ImGui::Image(imgui->add_image(texture->get_view()), {100, 100});
                    if (idx++ % 10 != 0)
                        ImGui::SameLine();
                }
            }
            ImGui::Dummy({});
            for (const auto& asset : Engine::Engine::get().asset_registry().all_assets())
                ImGui::Text("%s : %s", asset.second->get_class()->name(), asset.second->get_name());
        }
        ImGui::End();

        ImGui::ShowDemoWindow(nullptr);

        imgui->end(command_buffer);
    }

private:
    std::unique_ptr<Engine::Gfx::ImGuiWrapper> imgui;
    std::weak_ptr<Engine::Gfx::Window>         window;
    Engine::Scene*                             scene;
};


class TestApp : public Engine::Application
{
public:
    void init(Engine::Engine& engine, const std::weak_ptr<Engine::Gfx::Window>& default_window) override
    {

        default_window.lock()->set_renderer(Engine::Gfx::Renderer::create<TestFirstPassInterface>("present_pass", {.clear_color = Engine::Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})}, default_window, scene)
                                            ->attach(Engine::Gfx::RenderPass::create("forward_pass", {Engine::Gfx::Attachment::color("color", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                      Engine::Gfx::Attachment::depth("depth", Engine::Gfx::ColorFormat::D24_UNORM_S8_UINT)}))
                                            ->attach(Engine::Gfx::RenderPass::create("forward_test", {Engine::Gfx::Attachment::color("color", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                      Engine::Gfx::Attachment::color("normal", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                      Engine::Gfx::Attachment::depth("depth", Engine::Gfx::ColorFormat::D32_SFLOAT)})));
        Engine::StbImporter::load_from_path("./resources/cat.jpeg");

        auto rp = engine.get_device().lock()->find_or_create_render_pass(Engine::Gfx::Renderer::create("present_pass", {.clear_color = Engine::Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})})
                                                                         ->init_for_swapchain(*default_window.lock()->get_surface()->get_swapchain())
                                                                         ->get_infos());

        Engine::Gfx::ShaderCompiler compiler;
        const auto                  vertex_code   = compiler.compile_raw(VERTEX, "main", Engine::Gfx::EShaderStage::Vertex, "internal://test_mat");
        const auto                  fragment_code = compiler.compile_raw(FRAGMENT, "main", Engine::Gfx::EShaderStage::Fragment, "internal://test_mat");
        auto                        test_mat      = Engine::Gfx::Pipeline::create("test material", engine.get_device(), rp,
                                                      std::vector{Engine::Gfx::ShaderModule::create(engine.get_device(), vertex_code.get()), Engine::Gfx::ShaderModule::create(engine.get_device(), fragment_code.get())},
                                                      Engine::Gfx::Pipeline::CreateInfos{.culling = Engine::Gfx::ECulling::None,
                                                                                         .alpha_mode = Engine::Gfx::EAlphaMode::Translucent,
                                                                                         .depth_test = false,
                                                                                         .stage_input_override = std::vector{
                                                                                             Engine::Gfx::StageInputOutputDescription{0, 0, Engine::Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                             Engine::Gfx::StageInputOutputDescription{1, 12, Engine::Gfx::ColorFormat::R32G32_SFLOAT},
                                                                                             Engine::Gfx::StageInputOutputDescription{2, 20, Engine::Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                             Engine::Gfx::StageInputOutputDescription{3, 32, Engine::Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                             Engine::Gfx::StageInputOutputDescription{4, 44, Engine::Gfx::ColorFormat::R32G32B32A32_SFLOAT},
                                                                                         }});

        auto mat_test               = engine.asset_registry().create<Engine::MaterialAsset>("test mat", test_mat);
        auto test_material_instance = engine.asset_registry().create<Engine::MaterialInstanceAsset>("test mat instance", mat_test);

        Engine::GltfImporter::load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf");
        for (const auto& asset : engine.asset_registry().all_assets())
            if (auto mesh = asset.second.cast<Engine::MeshAsset>())
                scene.add_component<Engine::MeshComponent>(mesh->get_name(), mesh, test_material_instance);

        auto camera = scene.add_component<Engine::CameraComponent>("test_cam");
    }

    void tick_game(Engine::Engine&, double delta_second) override
    {
        scene.tick(delta_second);
    }

    Engine::Scene scene;
};

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    ContiguousObjectAllocator allocator;

    TObjectPtr<ParentA> foo = allocator.construct<ParentA>(10);
    foo.destroy();

    Engine::Config config = {};

    Engine::Engine engine(config);
    engine.run<TestApp>(Engine::Gfx::WindowConfig{.name = "primary"});
}