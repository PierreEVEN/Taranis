#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"

#include <gfx/window.hpp>

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "import/gltf_import.hpp"
#include "import/stb_import.hpp"
#include "scene/scene.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"

namespace Engine::Gfx
{
class ImGuiWrapper;
}

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
        imgui->begin(render_pass.resolution());

        if (ImGui::Begin("test"))
        {
            ImGui::LabelText("ms", "%lf", Engine::Engine::get().delta_second * 1000);
            ImGui::LabelText("fps", "%lf", 1.0 / Engine::Engine::get().delta_second);

            int idx = 0;
            for (const auto& asset : Engine::Engine::get().asset_registry().all_assets())
            {
                if (Engine::TextureAsset* texture = asset->cast<Engine::TextureAsset>())
                {
                    ImGui::Image(imgui->add_image(texture->get_view()), {100, 100});
                    if (idx++ % 10 != 0)
                        ImGui::SameLine();
                }
            }
            ImGui::Dummy({});
            for (const auto& asset : Engine::Engine::get().asset_registry().all_assets())
                ImGui::Text("%s : %s", asset->get_class()->name(), asset->get_name());
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
    void init(Engine::Engine&, const std::weak_ptr<Engine::Gfx::Window>& default_window) override
  {
      default_window.lock()->set_renderer(Engine::Gfx::Renderer::create<TestFirstPassInterface>("present_pass", {.clear_color = Engine::Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})}, default_window, scene)
                                              ->attach(Engine::Gfx::RenderPass::create("forward_pass", {Engine::Gfx::Attachment::color("color", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                        Engine::Gfx::Attachment::depth("depth", Engine::Gfx::ColorFormat::D24_UNORM_S8_UINT)}))
                                              ->attach(Engine::Gfx::RenderPass::create("forward_test", {Engine::Gfx::Attachment::color("color", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                        Engine::Gfx::Attachment::color("normal", Engine::Gfx::ColorFormat::R8G8B8A8_UNORM),
                                                                                                        Engine::Gfx::Attachment::depth("depth", Engine::Gfx::ColorFormat::D32_SFLOAT)})));

        Engine::StbImporter::load_from_path("./resources/cat.jpeg");
        //Engine::GltfImporter::load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf");

        auto camera = scene.add_component<Engine::CameraComponent>("test_cam");

        for (int i = 0; i < 1000000; ++i)
            scene.add_component<Engine::MeshComponent>("Iter mesh");

        scene.add_component<Engine::MeshComponent>("test mesh");
        scene.add_component<Engine::MeshComponent>("test mesh1");
        scene.add_component<Engine::MeshComponent>("test mesh2");
        scene.add_component<Engine::SceneComponent>("test scene comp");
        scene.add_component<Engine::MeshComponent>("test mesh3");
        scene.add_component<Engine::MeshComponent>("test mesh4");

        LOG_WARNING("finished add");

    }

    void tick_game(Engine::Engine&, double delta_second) override
    {
        scene.tick(delta_second);
    }

    Engine::Scene scene;
};

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING | Logger::LOG_LEVEL_TRACE);

    ContiguousObjectAllocator allocator;

    TObjectPtr<ParentA> foo = allocator.construct<ParentA>(10);
    foo.destroy();



    Engine::Config config = {};

    Engine::Engine engine(config);
    engine.run<TestApp>(Engine::Gfx::WindowConfig{.name = "primary"});
}