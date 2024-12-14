#include "widgets/viewport.hpp"

#include "profiler.hpp"
#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "scene_outliner.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>

Viewport::Viewport(const std::string& name, const std::weak_ptr<Eng::Gfx::RenderPassInstance>& in_render_pass, const std::shared_ptr<Eng::Scene> in_scene) : UiWindow(name), render_pass(in_render_pass), scene(in_scene)
{
    b_enable_menu_bar = true;
    draw_res          = render_pass.lock()->resolution();
    render_pass.lock()->set_resize_callback(
        [&](auto base)
        {
            if (draw_res.x == 0 || draw_res.y == 0)
                return base;
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
        ImGui::Dummy({ImGui::GetContentRegionAvail().x - 75, 0});
        ImGui::Text("%d fps", static_cast<int>(ImGui::GetIO().Framerate));

        ImGui::EndMenuBar();
    }

    if (ImGui::GetContentRegionAvail().x <= 0 || ImGui::GetContentRegionAvail().y <= 0)
        return;

    glm::uvec2 new_draw_res = {static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), static_cast<uint32_t>(ImGui::GetContentRegionAvail().y)};

    if (new_draw_res != draw_res && new_draw_res.x != 0 && new_draw_res.y != 0)
    {
        draw_res = new_draw_res;
        render_pass.lock()->create_or_resize(draw_res, draw_res);
    }

    for (const auto& attachment : render_pass.lock()->get_definition().attachments)
        if (!is_depth_format(attachment.second.color_format))
        {
            ImGui::Image(ctx.add_image(render_pass.lock()->get_image_resource(attachment.first).lock()), ImGui::GetContentRegionAvail());
            break;
        }
}