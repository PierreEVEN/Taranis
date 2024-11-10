#include "assets/texture_asset.hpp"
#include "config.hpp"
#include "engine.hpp"
#include "object_allocator.hpp"
#include "test_reflected_header.hpp"
#include "assets/material_instance_asset.hpp"
#include "assets/mesh_asset.hpp"
#include "assets/sampler_asset.hpp"
#include <gfx/window.hpp>
#include "gfx/renderer/definition/renderer.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/renderer/instance/swapchain_renderer.hpp"
#include "gfx/shaders/shader_compiler.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/command_buffer.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "import/assimp_import.hpp"
#include "import/material_import.hpp"
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

class GBufferResolveInterface : public Gfx::RenderPassInterface
{
public:
    void init(const std::weak_ptr<Gfx::Device>&, const Gfx::RenderPassInstanceBase& render_pass) override
    {
        auto base_mat = MaterialImport::from_path("resources/shaders/gbuffer_resolve.hlsl", Gfx::Pipeline::CreateInfos{.culling = Gfx::ECulling::None}, {Gfx::EShaderStage::Vertex, Gfx::EShaderStage::Fragment},
                                                  render_pass.get_render_pass());
        material = Engine::get().asset_registry().create<MaterialInstanceAsset>("gbuffer-resolve", base_mat);
        sampler  = Engine::get().asset_registry().create<SamplerAsset>("gbuffer-sampler");

        auto attachments = render_pass.get_children()[0]->get_attachments();

        material->set_sampler("sSampler", sampler);
        material->get_descriptor_resource()->bind_image("gbuffer_position", attachments[0].lock());
        material->get_descriptor_resource()->bind_image("gbuffer_albedo_m", attachments[1].lock());
        material->get_descriptor_resource()->bind_image("gbuffer_normal_r", attachments[2].lock());
        material->get_descriptor_resource()->bind_image("gbuffer_depth", attachments[3].lock());
    }

    void render(const Gfx::RenderPassInstanceBase&, const Gfx::CommandBuffer& command_buffer) override
    {
        command_buffer.bind_pipeline(*material->get_base_resource());
        command_buffer.bind_descriptors(*material->get_descriptor_resource(), *material->get_base_resource());
        command_buffer.draw_procedural(6, 0, 1, 0);
    }

    TObjectRef<MaterialInstanceAsset> material;
    TObjectRef<SamplerAsset>          sampler;
};

struct Attachment
{
    static Attachment slot(const std::string& in_name)
    {
        return Attachment{in_name};
    }

    Attachment& format(const Gfx::ColorFormat& format)
    {
        color_format = format;
        return *this;
    }


    Attachment& clear_color(const glm::vec4& clear_color)
    {
        clear_color_value = clear_color;
        return *this;
    }

    Attachment& clear_depth(const glm::vec2& clear_depth)
    {
        clear_depth_value = clear_depth;
        return *this;
    }

    std::string              name;
    Gfx::ColorFormat         color_format      = Gfx::ColorFormat::UNDEFINED;
    std::optional<glm::vec4> clear_color_value = {};
    std::optional<glm::vec2> clear_depth_value = {};

private:
    Attachment(std::string in_name) : name(std::move(in_name))
    {
    }
};

class IRenderPass
{

};

class TestRenderPass : public IRenderPass
{
public:
    TestRenderPass(int)
    {
    }
};

struct RenderPassKey
{
    using Elem = std::pair<Gfx::ColorFormat, bool>;

    std::vector<Elem> pass_data;

    void sort()
    {
        std::ranges::sort(pass_data,
                          [](const Elem& a, const Elem& b)
                          {
                              if (a.first == b.first)
                                  return a.second > b.second;

                              return static_cast<int>(a.first) > static_cast<int>(b.first);
                          });
    }

    bool operator==(RenderPassKey& other)
    {
        if (pass_data.size() != other.pass_data.size())
            return false;
        for (auto a = pass_data.begin(), b = other.pass_data.begin(); a != pass_data.end(); ++a, ++b)
            if (a->second != b->second || a->first != b->first)
                return false;
        return true;
    }
};

template <> struct std::hash<RenderPassKey>
{
    size_t operator()(const RenderPassKey& val) const noexcept
    {
        auto ite = val.pass_data.begin();
        if (ite == val.pass_data.end())
            return 0;
        size_t hash = std::hash<int32_t>()(static_cast<uint32_t>(ite->first) + 1);
        for (; ite != val.pass_data.end(); ++ite)
        {
            hash ^= std::hash<int32_t>()(static_cast<uint32_t>(ite->first) + 1) * 13;
        }
        return hash;
    }
};

class RenderNode
{
    friend class Renderer;

    class IRenderPassInitializer
    {
    public:
        virtual std::shared_ptr<IRenderPassInitializer> construct() = 0;
    };

    template <typename T, typename... Args> class TRenderPassInitializer : public IRenderPassInitializer
    {
    public:
        TRenderPassInitializer(Args&&... in_args) : tuple_value(std::tuple<Args...>(std::forward<Args>(in_args)...))
        {
        }

        std::shared_ptr<IRenderPassInitializer> construct() override
        {
            construct_with_tuple(tuple_value, std::index_sequence_for<Args...>());
            return {};
        }

    private:
        std::shared_ptr<T> construct_internal(Args... in_args)
        {
            return std::make_shared<T>(in_args...);
        }

        template <std::size_t... Is>
        std::shared_ptr<T> construct_with_tuple(const std::tuple<Args...>& tuple, std::index_sequence<Is...>)
        {
            return construct_internal(std::get<Is>(tuple)...);
        }

        std::tuple<Args...> tuple_value;
    };


    RenderNode() = default;

public:
    RenderNode& require(std::initializer_list<std::string> required)
    {
        for (const auto& item : required)
            dependencies.insert(item);
        return *this;
    }

    RenderNode& require(const std::string& required)
    {
        dependencies.insert(required);
        return *this;
    }

    RenderNode& with_imgui(bool enable)
    {
        b_with_imgui = enable;
        return *this;
    }

    RenderNode& add_attachment(const std::string& name, Attachment attachment)
    {
        attachments.emplace(name, attachment);
        return *this;
    }

    RenderNode& operator[](Attachment attachment)
    {
        attachments.emplace(attachment.name, attachment);
        return *this;
    }

    template <typename T, typename... Args> RenderNode& render_pass(Args&&... args)
    {
        render_pass_initializer = std::make_shared<TRenderPassInitializer<T, Args...>>(std::forward<Args>(args)...);
        return *this;
    }


    std::shared_ptr<IRenderPassInitializer>     render_pass_initializer;
    std::unordered_map<std::string, Attachment> attachments;
    std::unordered_set<std::string>             dependencies;
    bool                                        b_with_imgui = false;
};


class Renderer
{
public:
    Renderer() = default;

    RenderNode& operator[](const std::string& node)
    {
        if (auto found = nodes.find(node); found != nodes.end())
            return found->second;
        return nodes.emplace(node, RenderNode{}).first->second;
    }

private:
    std::unordered_map<std::string, RenderNode> nodes;
};

void build()
{
    Renderer renderer;

    renderer["gbuffers"]
        [Attachment::slot("position").format(Gfx::ColorFormat::R32G32B32A32_SFLOAT)]
        [Attachment::slot("albedo-m").format(Gfx::ColorFormat::R8G8B8A8_UNORM)]
        [Attachment::slot("normal-r").format(Gfx::ColorFormat::R8G8B8A8_UNORM)]
        [Attachment::slot("depth").format(Gfx::ColorFormat::D32_SFLOAT)];

    renderer["gbuffer_resolve"]
        .require("gbuffers")
        [Attachment::slot("target").format(Gfx::ColorFormat::R8G8B8A8_UNORM)];

    renderer["present"]
        .require("gbuffer_resolve")
        .with_imgui(true)
        [Attachment::slot("target")]
        .render_pass<TestRenderPass>(5);
}


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
            ->attach(Gfx::RenderPass::create<GBufferResolveInterface>("gbuffers-resolve", {
                                                                          Gfx::Attachment::color("target", Gfx::ColorFormat::R8G8B8A8_UNORM, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1}))})
                ->attach(Gfx::RenderPass::create<SceneRendererInterface>("gbuffers", {
                                                                             Gfx::Attachment::color("position", Gfx::ColorFormat::R32G32B32A32_SFLOAT, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})),
                                                                             Gfx::Attachment::color("albedo-m", Gfx::ColorFormat::R8G8B8A8_UNORM, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})),
                                                                             Gfx::Attachment::color("normal-r", Gfx::ColorFormat::R8G8B8A8_UNORM, Gfx::ClearValue::color({0.2, 0.2, 0.5, 1})),
                                                                             Gfx::Attachment::depth("depth", Gfx::ColorFormat::D32_SFLOAT, Gfx::ClearValue::depth_stencil({0, 0}))},
                                                                         *scene))));

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