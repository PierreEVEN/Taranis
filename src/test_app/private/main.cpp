#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"

#include <gfx/window.hpp>

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/renderer/instance/swapchain_renderer.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/image_view.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "import/assimp_import.hpp"
#include "import/material_import.hpp"
#include "scene/scene.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"

#include <ranges>

using namespace Eng;

namespace Eng::Gfx
{
class ImGuiWrapper;
}


class Viewport
{
public:
    Viewport(const std::shared_ptr<Gfx::RenderPassInstanceBase>& in_render_pass) : render_pass(in_render_pass)
    {
        draw_res = in_render_pass->resolution();
        in_render_pass->set_resize_callback(
            [&](auto)
            {
                return draw_res;
            });
    }

    void draw_ui(Gfx::ImGuiWrapper& imgui)
    {
        if (ImGui::Begin("Viewport"))
        {
            glm::uvec2 new_draw_res = {static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), static_cast<uint32_t>(ImGui::GetContentRegionAvail().y)};

            if (new_draw_res != draw_res)
            {
                render_pass->resize(draw_res);
                draw_res = new_draw_res;
            }
            ImGui::Image(imgui.add_image(render_pass->get_attachments()[0].lock()), ImGui::GetContentRegionAvail());
        }
        ImGui::End();
    }

    std::shared_ptr<Gfx::RenderPassInstanceBase> render_pass;
    glm::uvec2                                   draw_res = {0, 0};
};

class TestFirstPassInterface : public Gfx::RenderPassInterface
{
public:
    TestFirstPassInterface(std::weak_ptr<Gfx::Window> parent_window, Scene& in_scene) : window(std::move(parent_window)), scene(&in_scene)
    {
    }

    void init(const std::weak_ptr<Gfx::Device>& device, const Gfx::RenderPassInstanceBase& render_pass) override
    {
        imgui = std::make_unique<Gfx::ImGuiWrapper>("imgui_renderer", render_pass.get_render_pass(), device, window);
        viewport = std::make_unique<Viewport>(render_pass.get_children()[0]);
    }

    void render(const Gfx::RenderPassInstanceBase& render_pass, const Gfx::CommandBuffer& command_buffer) override
    {
        imgui->begin(render_pass.resolution());

        ImGui::SetNextWindowPos(ImVec2(-4, -4));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(render_pass.resolution().x) + 8.f, static_cast<float>(render_pass.resolution().y) + 8.f));
        if (ImGui::Begin("BackgroundHUD", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
        {
            ImGui::DockSpace(ImGui::GetID("Master dockSpace"), ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);
        }
        ImGui::End();

        viewport->draw_ui(*imgui);

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
        imgui->end(command_buffer);
    }

private:
    std::unique_ptr<Gfx::ImGuiWrapper> imgui;
    std::weak_ptr<Gfx::Window>         window;
    Scene*                             scene;
    std::unique_ptr<Viewport> viewport;
};

class SceneRendererInterface : public Gfx::RenderPassInterface
{
public:
    SceneRendererInterface(Scene& in_scene) : scene(&in_scene)
    {
    }

    void init(const std::weak_ptr<Gfx::Device>&, const Gfx::RenderPassInstanceBase&) override
    {
    }

    void render(const Gfx::RenderPassInstanceBase&, const Gfx::CommandBuffer& command_buffer) override
    {
        scene->draw(command_buffer);
    }

    Scene* scene;
};


class TestApp : public Application
{
public:
    void init(Engine& engine, const std::weak_ptr<Gfx::Window>& in_default_window) override
    {
        default_window = in_default_window;
        auto renderer  = default_window.lock()->set_renderer(
            Gfx::Renderer::create<TestFirstPassInterface>(
                "present_pass",
                {},
                default_window,
                scene)
            ->attach(Gfx::RenderPass::create<SceneRendererInterface>(
                "forward_pass", {
                    Gfx::Attachment::color("color", Gfx::ColorFormat::R8G8B8A8_UNORM, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})),
                    Gfx::Attachment::depth("depth", Gfx::ColorFormat::D24_UNORM_S8_UINT, Gfx::ClearValue::depth_stencil({1, 0}))},
                scene)));

        auto& rp = renderer.lock()->get_children()[0];

        auto material = MaterialImport::from_path(
            "resources/shaders/default_mesh.hlsl",
            Gfx::Pipeline::CreateInfos{.stage_input_override =
                std::vector{
                    Gfx::StageInputOutputDescription{0, 0, Gfx::ColorFormat::R32G32B32_SFLOAT},
                    Gfx::StageInputOutputDescription{1, 12, Gfx::ColorFormat::R32G32_SFLOAT},
                    Gfx::StageInputOutputDescription{2, 20, Gfx::ColorFormat::R32G32B32_SFLOAT},
                    Gfx::StageInputOutputDescription{3, 32, Gfx::ColorFormat::R32G32B32_SFLOAT},
                    Gfx::StageInputOutputDescription{4, 44, Gfx::ColorFormat::R32G32B32A32_SFLOAT},
                }},
            {Gfx::EShaderStage::Vertex, Gfx::EShaderStage::Fragment}, rp->get_render_pass());

        auto present_material = MaterialImport::from_path("resources/shaders/present_pass.hlsl",
                                                          Gfx::Pipeline::CreateInfos{.culling = Gfx::ECulling::None, .depth_test = false},
                                                          {Gfx::EShaderStage::Vertex, Gfx::EShaderStage::Fragment}, renderer.lock()->get_render_pass());
        auto present_material_instance = engine.asset_registry().create<MaterialInstanceAsset>("present material instance", present_material);

        camera = scene.add_component<FpsCameraComponent>("test_cam", rp);
        AssimpImporter importer;
        importer.load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf", scene, material, camera.cast<CameraComponent>());
    }

    void tick_game(Engine&, double delta_second) override
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

    bool                           set_first_pos = false;
    glm::vec2                      last_cursor_pos;
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