#include "gfx/ui/ui_window.hpp"

#include <imgui.h>


namespace Eng
{
UiWindow::UiWindow(std::string in_name) : name(std::move(in_name))
{
}

void UiWindow::draw_internal(Gfx::ImGuiWrapper& ctx)
{
    if (b_open)
    {
        int flags = 0;
        if (b_enable_menu_bar)
            flags |= ImGuiWindowFlags_MenuBar;

        if (ImGui::Begin(name.c_str(), &b_open, flags))
        {
            draw(ctx);
        }
        ImGui::End();
    }
}
}