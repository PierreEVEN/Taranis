#include "widgets/render_graph_view.hpp"

#include "gfx/renderer/instance/render_pass_instance.hpp"
#include "gfx/ui/ImGuiWrapper.hpp"
#include "gfx/vulkan/image_view.hpp"

static ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
    return {a.x + b.x, a.y + b.y};
}
static ImVec2 operator-(const ImVec2& a, const ImVec2& b)
{
    return {a.x - b.x, a.y - b.y};
}

static ImVec2 operator*(const ImVec2& a, const ImVec2& b)
{
    return {a.x * b.x, a.y * b.y};
}

static ImVec2 operator*(const ImVec2& a, const float& b)
{
    return {a.x * b, a.y * b};
}

static ImVec2 operator-=(ImVec2& a, const ImVec2& b)
{
    a.x -= b.x;
    a.y -= b.y;
    return a;
};


void RenderGraphView::draw(Eng::Gfx::ImGuiWrapper& ctx)
{

    if (ImGui::BeginChild("canvas", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        Content content;
        for (const auto& child : ctx.get_current_render_pass()->all_childs())
            add_pass_content(ctx, child.lock(), content, 0);

        ankerl::unordered_dense::map<int, float> stage_positions;

        ImVec2 total_size = {0, 0};
        float  last_pos   = 0;
        for (int i = 0; content.stage_sizes.contains(i); ++i)
        {
            ImVec2 idx_size = content.stage_sizes.find(i)->second;
            last_pos -= idx_size.x;
            total_size.x += idx_size.x;
            if (idx_size.y > total_size.y)
                total_size.y = idx_size.y;

            stage_positions.emplace(i, last_pos);
        }

        if (ImGui::BeginChild("Canvas content", {total_size * 1.2f}))
        {
            auto dl = ImGui::GetWindowDrawList();

            ImVec2 window_pos = ImGui::GetCursorPos();

            for (const auto& group : content.passes)
            {
                ImVec2 idx_size     = content.stage_sizes.find(group.second.stage)->second;
                float  idx_offset_x = stage_positions.find(group.second.stage)->second;

                auto group_pos = window_pos + ImVec2{total_size.x, total_size.y / 2} - ImVec2{idx_offset_x - idx_size.x, idx_size.y / 2};

                LOG_DEBUG("{} : {}x{}", group.first, group_pos.x, group_pos.y);

                dl->AddRectFilled(group_pos, group_pos + group.second.size, ImGui::ColorConvertFloat4ToU32(ImVec4(0.2, 0.2, 0.2, 1)));

                for (const auto& attachment : group.second.attachments)
                {
                    dl->AddImage(ctx.add_image(attachment.image.lock()), group_pos + attachment.pos, group_pos + attachment.pos + attachment.res);
                }
            }

            ImGui::EndChild();
        }

        ImGui::EndChild();
    }
}

void RenderGraphView::add_pass_content(Eng::Gfx::ImGuiWrapper& ctx, const std::shared_ptr<Eng::Gfx::RenderPassInstance>& pass, Content& content, int current_stage)
{
    if (content.passes.contains(pass->get_definition().name))
        return;

    Group group;
    group.name  = pass->get_definition().name;
    group.stage = current_stage;

    for (const auto& attachment : pass->get_definition().attachments | std::views::keys)
    {
        auto image = pass->get_attachment(attachment).lock();
        if (!image)
            continue;

        auto extent = pass->resolution();

        Object object;
        object.image = image;
        object.pos   = {0, group.size.y};
        object.res   = {static_cast<float>(extent.x), static_cast<float>(extent.y)};
        object.name  = attachment;

        if (extent.x > group.size.x)
            group.size.x = extent.x;
        group.size.y += extent.y;
        group.attachments.emplace_back(object);
    }

    auto& dims = content.stage_sizes.emplace(current_stage, ImVec2{0, 0}).first->second;
    if (group.size.x > dims.x)
        dims.x = group.size.x;
    dims.y += group.size.y;

    content.passes.emplace(pass->get_definition().name, group);

    for (const auto& child : pass->all_childs())
    {
        add_pass_content(ctx, child.lock(), content, current_stage + 1);
    }
}