#include "assets/material_asset.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/sampler_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx_types/format.hpp"
#include "import/assimp_import.hpp"
#include "widgets/profiler.hpp"
#include "scene/components/camera_component.hpp"
#include "scene/components/mesh_component.hpp"
#include "scene/scene.hpp"
#include "scene/scene_view.hpp"
#include "scene/components/directional_light_component.hpp"
#include "widgets/content_browser.hpp"
#include "widgets/scene_outliner.hpp"
#include "widgets/viewport.hpp"
#include <gfx/window.hpp>

using namespace Eng;

class SceneGBuffers : public Gfx::IRenderPass
{
public:
    SceneGBuffers(const std::shared_ptr<Scene>& in_scene) : scene(in_scene)
    {
    }

    void pre_draw(const Gfx::RenderPassInstance& rp) override
    {
        scene->get_active_camera()->get_view().pre_draw(rp);
    }

    void draw(const Gfx::RenderPassInstance& rp, Gfx::CommandBuffer& command_buffer, size_t thread_index) override
    {
        scene->get_active_camera()->get_view().draw(*scene, rp, command_buffer, thread_index, record_threads());
    }

    size_t record_threads() override
    {
        return std::thread::hardware_concurrency() * 3;
    }

    void pre_submit(const Gfx::RenderPassInstance&) override
    {
        scene->get_active_camera()->get_view().pre_submit();
    }

    std::shared_ptr<Scene> scene;
};

class GBufferResolveInterface : public Gfx::IRenderPass
{
public:
    GBufferResolveInterface(const std::shared_ptr<Scene>& in_scene) : scene(in_scene)
    {
    }

    void init(const Gfx::RenderPassInstance&) override
    {
        auto base_mat = Engine::get().asset_registry().create<MaterialAsset>("resolve_mat");
        base_mat->set_shader_code("gbuffer_resolve");

        sampler  = Engine::get().asset_registry().create<SamplerAsset>("gbuffer-sampler");
        material = Engine::get().asset_registry().create<MaterialInstanceAsset>("gbuffer-resolve", base_mat, false);
        material->set_sampler("sSampler", sampler);
    }

    void on_create_framebuffer(const Gfx::RenderPassInstance& render_pass) override
    {
        auto dep      = render_pass.get_dependency("gbuffers").lock();
        auto resource = material->get_descriptor_resource("gbuffer_resolve");

        if (!resource)
            return LOG_ERROR("Cannot current_thread descriptor resource for pass {}", "gbuffer_resolve");

        resource->bind_image("gbuffer_position", dep->get_attachment("position").lock());
        resource->bind_image("gbuffer_albedo_m", dep->get_attachment("albedo-m").lock());
        resource->bind_image("gbuffer_normal_r", dep->get_attachment("normal-r").lock());
        resource->bind_image("gbuffer_depth", dep->get_attachment("depth").lock());
    }

    void draw(const Gfx::RenderPassInstance&, Gfx::CommandBuffer& command_buffer, size_t) override
    {
        scene->for_each<CameraComponent>(
            [&](const CameraComponent& object)
            {
                command_buffer.push_constant(Gfx::EShaderStage::Fragment, *material->get_base_resource(command_buffer.render_pass()), Gfx::BufferData{object.get_position()});
            });

        command_buffer.bind_pipeline(material->get_base_resource(command_buffer.render_pass()));
        command_buffer.bind_descriptors(*material->get_descriptor_resource(command_buffer.render_pass()), *material->get_base_resource(command_buffer.render_pass()));
        command_buffer.draw_procedural(6, 0, 1, 0);
    }

    TObjectRef<MaterialInstanceAsset> material;
    TObjectRef<SamplerAsset>          sampler;
    std::shared_ptr<Scene>            scene;
};

class PresentPass : public Gfx::IRenderPass
{
public:
    PresentPass(const std::shared_ptr<Scene>& in_scene) : scene(in_scene)
    {
    }

    void init(const Gfx::RenderPassInstance& rp) override
    {
        rp.imgui()->new_window<Viewport>("Viewport", rp.get_dependency("gbuffer_resolve"), scene);
        rp.imgui()->new_window<ContentBrowser>("Content browser", Engine::get().asset_registry());
        rp.imgui()->new_window<SceneOutliner>("Scene outliner", scene);
        rp.imgui()->new_window<ProfilerWindow>("Profiler");
    }

    std::shared_ptr<Scene> scene;
};

class TestApp : public Application
{
public:

    void init(Engine& engine, const std::weak_ptr<Gfx::Window>& in_default_window) override
    {
        scene = std::make_shared<Scene>();

        default_window = in_default_window;

        Gfx::Renderer renderer;

        renderer["gbuffers"]
            .render_pass<SceneGBuffers>(scene)
            [Gfx::Attachment::slot("position").format(Gfx::ColorFormat::R32G32B32A32_SFLOAT).clear_color({0, 0, 0, 0})]
            [Gfx::Attachment::slot("albedo-m").format(Gfx::ColorFormat::R8G8B8A8_UNORM).clear_color({0.5f, 0.5f, 0.8f, 0.0f})]
            [Gfx::Attachment::slot("normal-r").format(Gfx::ColorFormat::R8G8B8A8_UNORM).clear_color({0, 0, 0, 1.0f})]
            [Gfx::Attachment::slot("depth").format(Gfx::ColorFormat::D32_SFLOAT).clear_depth({0.0f, 0.0f})];

        renderer["gbuffer_resolve"]
            .require("gbuffers")
            .render_pass<GBufferResolveInterface>(scene)
            [Gfx::Attachment::slot("target").format(Gfx::ColorFormat::R8G8B8A8_UNORM)];

        renderer["present"]
            .require("gbuffer_resolve")
            .with_imgui(true, default_window)
            .render_pass<PresentPass>(scene)
            [Gfx::Attachment::slot("target")];

        scene->set_pass_list(default_window.lock()->set_renderer(renderer));
        camera = scene->add_component<FpsCameraComponent>("test_cam");
        camera->activate();
        auto directional_light = scene->add_component<DirectionalLightComponent>("Directional light");
        directional_light->enable_shadow(ELightType::Movable);


        std::shared_ptr<AssimpImporter> importer = std::make_shared<AssimpImporter>();
        engine.jobs().schedule(
            [&, importer]
            {
                scene->merge(importer->load_from_path("./resources/models/samples/Sponza/glTF/Sponza.gltf"));
            });
        engine.jobs().schedule(
            [&, importer]
            {
                //scene->merge(importer->load_from_path("./resources/models/samples/Bistro_v5_2/BistroExterior.fbx"));
            });
        engine.jobs().schedule(
            [&, importer]
            {
                //scene->merge(importer->load_from_path("./resources/models/samples/Bistro_v5_2/BistroInterior_Wine.fbx"));
            });
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
            mov_vec.y = -1;
        }
        if (glfwGetKey(glfw_ptr, GLFW_KEY_A))
        {
            mov_vec.y = 1;
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
        else if (glfwGetMouseButton(glfw_ptr, GLFW_MOUSE_BUTTON_2))
        {
            glfwSetInputMode(glfw_ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            float dx = static_cast<float>(pos_x) - last_cursor_pos.x;
            float dy = static_cast<float>(pos_y) - last_cursor_pos.y;

            camera->set_yaw(camera->get_yaw() - dx * 0.01f);
            camera->set_pitch(camera->get_pitch() + dy * 0.01f);
        }
        else
            glfwSetInputMode(glfw_ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

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

    Config config = {.gfx = {.enable_validation_layers = false}};
    Engine engine(config);
    engine.run<TestApp>(Gfx::WindowConfig{.name = "primary"});
}