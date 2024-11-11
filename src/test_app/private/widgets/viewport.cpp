#include "widgets/viewport.hpp"

#include "scene_outliner.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"

#include <imgui.h>
#include <glm/vec2.hpp>

Viewport::Viewport(const std::string& name, const std::shared_ptr<Eng::Gfx::RenderPassInstanceBase>& in_render_pass, const std::shared_ptr<Eng::Scene> in_scene) : UiWindow(name), render_pass(in_render_pass), scene(in_scene)
{
    b_enable_menu_bar = true;
    draw_res = render_pass->resolution();
    render_pass->set_resize_callback(
        [&](auto)
        {
            return draw_res;
        });
}

void Viewport::draw(Eng::Gfx::ImGuiWrapper& ctx)
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("Windows"))
        {
            if (ImGui::MenuItem("Outliner"))
                ctx.new_window<SceneOutliner>("Scene outliner", scene);
            ImGui::EndMenu();
        }



        ImGui::EndMenuBar();
    }


    glm::uvec2 new_draw_res = {static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), static_cast<uint32_t>(ImGui::GetContentRegionAvail().y)};

    if (new_draw_res != draw_res)
    {
        draw_res = new_draw_res;
        render_pass->try_resize(draw_res);
    }
    ImGui::Image(ctx.add_image(render_pass->get_attachments()[0].lock()), ImGui::GetContentRegionAvail());
}