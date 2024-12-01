#pragma once
#include "ui_window.hpp"

#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/vec2.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <ankerl/unordered_dense.h>
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
    void mouse_button_callback(int button, int action, int mods);
    void scroll_callback(double xoffset, double yoffset);
    void key_callback(int keycode, int scancode, int action, int mods);
    void update_mouse_cursor() const;
    void update_mouse_data() const;
    void update_gamepads();
    void windows_focus_callback(int focused) const;
    void cursor_pos_callback(double x, double y);
    void update_key_modifiers() const;
    void cursor_enter_callback(int entered);
    void char_callback(unsigned int c);

    ImGuiWrapper(std::string name, const std::string& render_pass, std::weak_ptr<Device> device, std::weak_ptr<Window> target_window);
    ~ImGuiWrapper();
    void begin(glm::uvec2 draw_res);
    void prepare_all_window();
    void end(CommandBuffer& cmd);

    ImTextureID add_image(const std::shared_ptr<ImageView>& image_view);

    template <typename T, typename... Args> std::weak_ptr<T> new_window(const std::string& window_name, Args&&... args)
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

    ImVec2 LastValidMousePos = {0, 0};

    ImTextureID                                                                                                      max_texture_id = 0;
    ankerl::unordered_dense::map<std::shared_ptr<ImageView>, std::pair<ImTextureID, std::shared_ptr<DescriptorSet>>> per_image_descriptor;
    ankerl::unordered_dense::map<ImTextureID, std::shared_ptr<ImageView>>                                            per_image_ids;
    std::string                                                                                                      name;
};
} // namespace Eng::Gfx