#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"
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
#include "gfx/vulkan/vk_render_pass.hpp"
#include "import/assimp_import.hpp"
#include "import/stb_import.hpp"
#include "scene/scene.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"

#include <ranges>

using namespace Eng;

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
    float2 Uvs : TEXCOORD0;\
};\
struct PushConsts\
{\
    float4x4 camera;\
};\
[[vk::push_constant]] ConstantBuffer<PushConsts> pc;\
VsToFs                                           main(VSInput input)\
{\
    VsToFs Out;\
    Out.Pos    = mul(pc.camera, float4(input.pos, 1));\
    Out.Uvs = input.uv;\
    return Out;\
}";

const char* FRAGMENT = "\
                     struct VsToFs\
{\
    float4 Pos : SV_Position;\
    float2 Uvs : TEXCOORD0;\
};\
\
float pow_cord(float val)\
{\
    return pow(abs(sin(val * 50)), 10);\
}\
\
[[vk::binding(0)]] SamplerState sSampler;\
[[vk::binding(1)]] Texture2D    albedo;\
\
float4 main(VsToFs input) : SV_TARGET\
{\
    return albedo.Sample(sSampler, input.Uvs, 1);\
}";

namespace Eng::Gfx
{
class ImGuiWrapper;
}

class TestFirstPassInterface : public Gfx::RenderPassInterface
{
public:
    TestFirstPassInterface(std::weak_ptr<Gfx::Window> parent_window, Scene& in_scene) : window(std::move(parent_window)), scene(&in_scene)
    {
    }

    void init(const std::weak_ptr<Gfx::Device>& device, const Gfx::RenderPassInstanceBase& render_pass) override
    {
        imgui = std::make_unique<Gfx::ImGuiWrapper>("imgui_renderer", render_pass.get_render_pass(), device, window);
    }

    void render(const Gfx::RenderPassInstanceBase& render_pass, const Gfx::CommandBuffer& command_buffer) override
    {
        scene->draw(command_buffer);

        imgui->begin(render_pass.resolution());

        if (ImGui::Begin("test"))
        {
            ImGui::LabelText("ms", "%lf", Engine::get().delta_second * 1000);
            ImGui::LabelText("fps", "%lf", 1.0 / Engine::get().delta_second);

            int idx = 0;
            for (const auto& asset : Engine::get().asset_registry().all_assets() | std::views::values)
            {
                if (auto texture = asset.cast<TextureAsset>())
                {
                    ImGui::Image(imgui->add_image(texture->get_view()), {100, 100});
                    if (idx++ % 10 != 0)
                        ImGui::SameLine();
                }
            }
            ImGui::Dummy({});
            for (const auto& asset : Engine::get().asset_registry().all_assets() | std::views::values)
                ImGui::Text("%s : %s", asset->get_class()->name(), asset->get_name());
        }
        ImGui::End();

        ImGui::ShowDemoWindow(nullptr);

        imgui->end(command_buffer);
    }

private:
    std::unique_ptr<Gfx::ImGuiWrapper> imgui;
    std::weak_ptr<Gfx::Window>         window;
    Scene*                             scene;
};


class TestApp : public Application
{
public:
    void init(Eng::Engine &engine, const std::weak_ptr<Gfx::Window>& in_default_window) override
    {
        default_window = in_default_window;
        auto renderer  = default_window.lock()->set_renderer(
            Gfx::Renderer::create<TestFirstPassInterface>("present_pass", {.clear_color = Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})}, default_window, scene)
            ->attach(Gfx::RenderPass::create("forward_pass", {Gfx::Attachment::color("color", Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                      Gfx::Attachment::depth("depth", Gfx::ColorFormat::D24_UNORM_S8_UINT)})));

        Gfx::ShaderCompiler compiler;
        const auto                  vertex_code   = compiler.compile_raw(VERTEX, "main", Gfx::EShaderStage::Vertex, "internal://test_mat");
        const auto                  fragment_code = compiler.compile_raw(FRAGMENT, "main", Gfx::EShaderStage::Fragment, "internal://test_mat");
        auto                        test_mat      = Gfx::Pipeline::create("test material", engine.get_device(), renderer.lock()->get_render_pass(),
                                                                          std::vector{Gfx::ShaderModule::create(engine.get_device(), vertex_code.get()), Gfx::ShaderModule::create(engine.get_device(), fragment_code.get())},
                                                                                  Gfx::Pipeline::CreateInfos{.culling = Gfx::ECulling::None,
                                                                                                                     .alpha_mode = Gfx::EAlphaMode::Opaque,
                                                                                                                     .depth_test = false,
                                                                                                                     .stage_input_override = std::vector{
                                                                                                                         Gfx::StageInputOutputDescription{0, 0, Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                                                         Gfx::StageInputOutputDescription{1, 12, Gfx::ColorFormat::R32G32_SFLOAT},
                                                                                                                         Gfx::StageInputOutputDescription{2, 20, Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                                                         Gfx::StageInputOutputDescription{3, 32, Gfx::ColorFormat::R32G32B32_SFLOAT},
                                                                                                                         Gfx::StageInputOutputDescription{4, 44, Gfx::ColorFormat::R32G32B32A32_SFLOAT},
                                                                                                                     }});

        auto gltf_mat = engine.asset_registry().create<MaterialAsset>("GltfMat", test_mat);

        camera = scene.add_component<FpsCameraComponent>("test_cam", renderer);
        AssimpImporter importer;
        importer.load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf", scene, gltf_mat, camera.cast<CameraComponent>());
    }

    void tick_game(Eng::Engine&, double delta_second) override
    {
        auto glfw_ptr = default_window.lock()->raw();

        glm::vec3 mov_vec{0};

        if (glfwGetKey(glfw_ptr, GLFW_KEY_W))
        {
            mov_vec.x = 1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_S))
        {
            mov_vec.x = -1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_A))
        {
            mov_vec.y = 1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_D))
        {
            mov_vec.y = -1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_SPACE))
        {
            mov_vec.z = 1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_LEFT_SHIFT))
        {
            mov_vec.z = -1;
        }

        double pos_x, pos_y;

        glfwGetCursorPos(glfw_ptr, &pos_x, &pos_y);

        if (!set_first_pos)
        {
            set_first_pos   = true;
            last_cursor_pos = {pos_x, pos_y};
        }
        else
        {

            float dx = static_cast<float>(pos_x) - last_cursor_pos.x;
            float dy = static_cast<float>(pos_y) - last_cursor_pos.y;

            camera->set_yaw(camera->get_yaw() + dx * 0.01f);
            camera->set_pitch(camera->get_pitch() + dy * 0.01f);
        }

        last_cursor_pos = {pos_x, pos_y};

        glm::vec3 forward = camera->get_rotation() * glm::vec3(1, 0, 0);
        glm::vec3 right   = camera->get_rotation() * glm::vec3(0, 1, 0);
        glm::vec3 up      = camera->get_rotation() * glm::vec3(0, 0, 1);

        glm::vec3 vec = mov_vec.x * forward + mov_vec.y * right + mov_vec.z * up;

        camera->set_position(camera->get_position() + static_cast<float>(delta_second) * 500.f * vec);

        // LOG_WARNING("cam : {} :: {}   /   {}, {}, {}", camera->get_pitch(), camera->get_yaw(), forward.x, forward.y, forward.z);

        scene.tick(delta_second);
    }

    bool                                   set_first_pos = false;
    glm::vec2                              last_cursor_pos;
    TObjectRef<FpsCameraComponent> camera;
    std::weak_ptr<Gfx::Window>     default_window;
    Scene                          scene;
};

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    ContiguousObjectAllocator allocator;

    TObjectPtr<ParentA> foo = allocator.construct<ParentA>(10);
    foo.destroy();

    Config config = {};

    Eng::Engine engine(config);
    engine.run<TestApp>(Gfx::WindowConfig{.name = "primary"});
}