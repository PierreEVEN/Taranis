#pragma once
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
#include <vector>

struct ImGuiContext;

namespace Engine
{
class Window;
class DescriptorSet;
class Sampler;
class ImageView;
class Image;
class CommandBuffer;
class Mesh;
class RenderPassObject;
class Device;
class Pipeline;
} // namespace Engine

namespace Engine
{
class ImGuiWrapper
{
  public:
    ImGuiWrapper(const std::weak_ptr<RenderPassObject>& render_pass, std::weak_ptr<Device> device, std::weak_ptr<Window> target_window);
    ~ImGuiWrapper();
    void draw(const CommandBuffer& cmd, glm::uvec2 draw_res);

  private:
    std::shared_ptr<Mesh>          mesh;
    std::shared_ptr<Pipeline>      imgui_material;
    std::shared_ptr<DescriptorSet> imgui_descriptors;
    std::shared_ptr<Image>         font_texture;
    std::shared_ptr<Sampler>       image_sampler;
    std::shared_ptr<ImageView>     font_texture_view;
    std::weak_ptr<Device>          device;
    std::weak_ptr<Window>          target_window;
    ImGuiContext*                  imgui_context = nullptr;
    std::vector<GLFWcursor*>       cursor_map    = std::vector<GLFWcursor*>(ImGuiMouseCursor_COUNT, nullptr);
};
} // namespace Engine
