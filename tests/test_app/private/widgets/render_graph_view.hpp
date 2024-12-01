#pragma once
#include "gfx/ui/ui_window.hpp"

#include <imgui.h>
#include <memory>
#include <vector>
#include <ankerl/unordered_dense.h>

namespace Eng::Gfx
{
class ImageView;
}

namespace Eng::Gfx
{
class RenderPassInstance;
}

class RenderGraphView : public Eng::UiWindow
{
public:
    explicit RenderGraphView(const std::string& name)
        : UiWindow(name)
    {
    }

protected:
    void draw(Eng::Gfx::ImGuiWrapper& ctx) override;

    struct Object
    {
        std::string                        name;
        ImVec2                             pos = {0, 0};
        ImVec2                             res = {0, 0};
        std::weak_ptr<Eng::Gfx::ImageView> image;

    };

    struct Group
    {
        std::string              name;
        ImVec2                   size = {0, 0};
        std::vector<std::string> dependencies;
        std::vector<Object>      attachments;
        int                      stage = 0;
    };

    struct Content
    {
        ankerl::unordered_dense::map<int, ImVec2>        stage_sizes;
        ImVec2                                           size        = {0, 0};
        ImVec2                                           current_pos = {0, 0};
        ankerl::unordered_dense::map<std::string, Group> passes;
    };

    ImVec2 canvas_pos;
    float  zoom;

    static void add_pass_content(Eng::Gfx::ImGuiWrapper& ctx, const std::shared_ptr<Eng::Gfx::RenderPassInstance>& pass, Content& content, int current_stage);
};