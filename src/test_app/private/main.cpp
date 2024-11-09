#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"
#include "assets/mesh_asset.hpp"
#include <gfx/window.hpp>
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/renderer/instance/swapchain_renderer.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "import/assimp_import.hpp"
#include "scene/scene.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"
#include "widgets/content_browser.hpp"
#include "widgets/viewport.hpp"

using namespace Eng;

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
        scene = std::make_shared<Scene>();

        default_window = in_default_window;
        auto renderer  = default_window.lock()->set_renderer(
            Gfx::Renderer::create(
                "present_pass",
                {})
            ->attach(Gfx::RenderPass::create<SceneRendererInterface>(
                "forward_pass", {
                    Gfx::Attachment::color("color", Gfx::ColorFormat::R8G8B8A8_UNORM, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})),
                    Gfx::Attachment::depth("depth", Gfx::ColorFormat::D32_SFLOAT, Gfx::ClearValue::depth_stencil({0, 0}))},
                *scene)));

        renderer.lock()->imgui_context()->new_window<Viewport>("Viewport", renderer.lock()->get_children()[0], scene);
        renderer.lock()->imgui_context()->new_window<ContentBrowser>("Content browser", engine.asset_registry());

        auto& rp = renderer.lock()->get_children()[0];
        camera   = scene->add_component<FpsCameraComponent>("test_cam", rp);
        AssimpImporter importer;
        importer.load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf", *scene, camera.cast<CameraComponent>(), rp->get_render_pass());
        //importer.load_from_path("./resources/models/samples/Bistro_v5_2/BistroExterior.fbx", *scene, camera.cast<CameraComponent>(), rp->get_render_pass());
        //importer.load_from_path("./resources/models/samples/Bistro_v5_2/BistroInterior_Wine.fbx", *scene, camera.cast<CameraComponent>(), rp->get_render_pass());
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
        if (glfwGetKey(glfw_ptr, GLFW_KEY_D))
        {
            mov_vec.y = 1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_A))
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

        scene->tick(delta_second);
    }

    bool                           set_first_pos = false;
    glm::vec2                      last_cursor_pos;
    TObjectRef<FpsCameraComponent> camera;
    std::weak_ptr<Gfx::Window>     default_window;
    std::shared_ptr<Scene>         scene;
};

int main()
{
    Logger::get().enable_logs(Logger::LOG_LEVEL_DEBUG | Logger::LOG_LEVEL_ERROR | Logger::LOG_LEVEL_FATAL | Logger::LOG_LEVEL_INFO | Logger::LOG_LEVEL_WARNING);

    ContiguousObjectAllocator allocator;

    TObjectPtr<ParentA> foo = allocator.construct<ParentA>(10);
    foo.destroy();

    Config config = {};

    Engine engine(config);
    engine.run<TestApp>(Gfx::WindowConfig{.name = "primary"});
}