#pragma once
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <unordered_map>
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
class VkRendererPass;
class Device;
class Pipeline;
} // namespace Engine

namespace Engine
{
class ImGuiWrapper
{
  public:
    ImGuiWrapper(std::string name, const std::weak_ptr<VkRendererPass>& render_pass, std::weak_ptr<Device> device, std::weak_ptr<Window> target_window);
    ~ImGuiWrapper();
    void draw(const CommandBuffer& cmd, glm::uvec2 draw_res);

    ImTextureID add_image(const std::shared_ptr<ImageView>& image_view);

  private:
    std::shared_ptr<Mesh>          mesh;
    std::shared_ptr<Pipeline>      imgui_material;
    std::shared_ptr<DescriptorSet> imgui_font_descriptor;
    std::shared_ptr<Image>         font_texture;
    std::shared_ptr<Sampler>       image_sampler;
    std::shared_ptr<ImageView>     font_texture_view;
    std::weak_ptr<Device>          device;
    std::weak_ptr<Window>          target_window;
    ImGuiContext*                  imgui_context = nullptr;
    std::vector<GLFWcursor*>       cursor_map    = std::vector<GLFWcursor*>(ImGuiMouseCursor_COUNT, nullptr);

    ImTextureID                                                                                            max_texture_id = 0;
    std::unordered_map<std::shared_ptr<ImageView>, std::pair<ImTextureID, std::shared_ptr<DescriptorSet>>> per_image_descriptor;
    std::unordered_map<ImTextureID, std::shared_ptr<ImageView>>                                            per_image_ids;
    std::string                                                                                            name;
};
} // namespace Engine