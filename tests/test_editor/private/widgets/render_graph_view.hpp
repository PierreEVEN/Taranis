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
        float                              offset_y;
        ImVec2                             res = {0, 0};
        std::weak_ptr<Eng::Gfx::ImageView> image;

    };

    struct Group
    {
        std::string name;
        // This group total size (should be smaller than the total stage size
        ImVec2                   size = {0, 0};
        std::vector<std::string> dependencies;
        std::vector<Object>      attachments;
        float                    y_offset = 0;
        int                      stage    = 0;
    };

    struct Content
    {
        // This is the total size of each stage (none stage contains multiple groups)
        ankerl::unordered_dense::map<int, ImVec2>     stage_sizes;
        ImVec2                                        size        = {0, 0};
        ImVec2                                        current_pos = {0, 0};
        ankerl::unordered_dense::map<uint64_t, Group> passes;
    };

    std::optional<ImVec2> desired_pos;
    std::optional<float>  desired_zoom;
    float                 last_scroll   = 0;
    float                 last_scroll_y = 0;
    float                 zoom          = 0;
    bool                  initialized   = false;

    static void add_pass_content(Eng::Gfx::ImGuiWrapper& ctx, const std::shared_ptr<Eng::Gfx::RenderPassInstance>& pass, Content& content, int current_stage);
};