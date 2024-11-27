#include "gfx/ui/ui_window.hpp"

#include "logger.hpp"

#include <imgui.h>
#include <ankerl/unordered_dense.h>

ankerl::unordered_dense::map<std::string, size_t> named_windows;

namespace Eng
{
UiWindow::~UiWindow()
{
    named_windows.find(name)->second--;
}

UiWindow::UiWindow(std::string in_name) : name(std::move(in_name))
{
    index = named_windows.emplace(name, 0).first->second++;
}

void UiWindow::draw_internal(Gfx::ImGuiWrapper& ctx)
{
    if (b_open)
    {
        int flags = 0;
        if (b_enable_menu_bar)
            flags |= ImGuiWindowFlags_MenuBar;

        std::string window_name = name + "##" + std::to_string(index);
        if (ImGui::Begin(window_name.c_str(), &b_open, flags))
        {
            draw(ctx);
        }
        ImGui::End();
    }
}
} // namespace Eng