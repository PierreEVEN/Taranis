#pragma once
#include "ui_window.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Eng
{
class UiWindow;
}

struct ImGuiContext;

namespace Eng::Gfx
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

class ImGuiWrapper
{
public:
    ImGuiWrapper(std::string name, const std::weak_ptr<VkRendererPass>& render_pass, std::weak_ptr<Device> device, std::weak_ptr<Window> target_window);
    ~ImGuiWrapper();
    void begin(glm::uvec2 draw_res);
    void end(const CommandBuffer& cmd);

    ImTextureID add_image(const std::shared_ptr<ImageView>& image_view);


    template <typename T, typename... Args> std::weak_ptr<T> new_window(const std::string& window_name, Args&&...args)
    {
        auto new_window = std::make_shared<T>(window_name, std::forward<Args>(args)...);
        windows.push_back(new_window);
        return new_window;
    }

private:


    std::vector<std::shared_ptr<UiWindow>> windows;

    std::chrono::steady_clock::time_point last_time;

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
} // namespace Eng::Gfx