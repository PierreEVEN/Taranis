#include "config.hpp"
#include "engine.hpp"
#include "assets/asset_registry.hpp"
#include "assets/texture_asset.hpp"

#include <gfx/window.hpp>

#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "import/stb_import.hpp"

namespace Engine
{
class ImGuiWrapper;
}

class TestFirstPassInterface : public Engine::RenderPassInterface
{
public:
    TestFirstPassInterface(std::weak_ptr<Engine::Window> parent_window) : window(std::move(parent_window))
    {
    }

    void init(const std::weak_ptr<Engine::Device>& device, const Engine::RenderPassInstanceBase& render_pass) override
    {

        text = Engine::StbImporter::load_from_path("./resources/cat.jpeg");

        imgui = std::make_unique<Engine::ImGuiWrapper>("imgui_renderer", render_pass.get_render_pass(), device, window);
    }

    void render(const Engine::RenderPassInstanceBase& render_pass, const Engine::CommandBuffer& command_buffer) override
    {
        imgui->begin(command_buffer, render_pass.resolution());

        if (ImGui::Begin("test"))
        {
            ImGui::LabelText("ms", "%lf", Engine::Engine::get().delta_second * 1000);
            ImGui::LabelText("fps", "%lf", 1.0 / Engine::Engine::get().delta_second);

            ImGui::Image(imgui->add_image(text->get_view()), ImGui::GetContentRegionAvail());
        }
        ImGui::End();

        ImGui::ShowDemoWindow(nullptr);

        imgui->end(command_buffer);
    }

private:
    std::unique_ptr<Engine::ImGuiWrapper> imgui;
    std::weak_ptr<Engine::Window>         window;
    Engine::TextureAsset*                 text;

};

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING | Logger::LOG_LEVEL_TRACE);

    Engine::Config config = {};

    Engine::Engine engine(config);

    const auto main_window = engine.new_window(Engine::WindowConfig{.name = "primary"});
    main_window.lock()->set_renderer(
        Engine::Renderer::create<TestFirstPassInterface>("present_pass", {.clear_color = Engine::ClearValue::color({0.2, 0.2, 0.5, 1})}, main_window)
        ->attach(Engine::RenderPass::create("forward_pass", {Engine::Attachment::color("color", Engine::ColorFormat::R8G8B8A8_UNORM), Engine::Attachment::depth("depth", Engine::ColorFormat::D24_UNORM_S8_UINT)}))
        ->attach(Engine::RenderPass::create("forward_test", {Engine::Attachment::color("color", Engine::ColorFormat::R8G8B8A8_UNORM), Engine::Attachment::color("normal", Engine::ColorFormat::R8G8B8A8_UNORM),
                                                             Engine::Attachment::depth("depth", Engine::ColorFormat::D32_SFLOAT)})));

    const auto secondary_window = engine.new_window(Engine::WindowConfig{.name = "secondary"});
    secondary_window.lock()->set_renderer(
        Engine::Renderer::create<TestFirstPassInterface>("present_pass", {.clear_color = Engine::ClearValue::color({0.8, 0.6, 0.5, 1})}, secondary_window)
        ->attach(Engine::RenderPass::create<TestFirstPassInterface>("forward_pass",
                                                                    {Engine::Attachment::color("color", Engine::ColorFormat::R8G8B8A8_UNORM, Engine::ClearValue::color({0, 0.8, 0.5, 1})),
                                                                     Engine::Attachment::depth("depth", Engine::ColorFormat::D24_UNORM_S8_UINT, Engine::ClearValue::depth_stencil({1, 0}))},
                                                                    secondary_window))
        ->attach(Engine::RenderPass::create("forward_test", {Engine::Attachment::color("color", Engine::ColorFormat::R8G8B8A8_UNORM), Engine::Attachment::color("normal", Engine::ColorFormat::R8G8B8A8_UNORM),
                                                             Engine::Attachment::depth("depth", Engine::ColorFormat::D32_SFLOAT)})));
    engine.run();
}