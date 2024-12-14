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

static ImVec2 operator*(const ImVec2& a, const float& b)
{
    return {a.x * b, a.y * b};
}

static ImVec2 operator/(const ImVec2& a, const ImVec2& b)
{
    return {a.x / b.x, a.y / b.y};
}

static ImVec2 operator/(const ImVec2& a, const float& b)
{
    return {a.x / b, a.y / b};
}

static constexpr ImVec2 image_padding{20, 20};
static constexpr ImVec2 group_padding{300, 40};

void RenderGraphView::draw(Eng::Gfx::ImGuiWrapper& ctx)
{
    Content content;
    for (const auto& child : ctx.get_current_render_pass()->all_childs())
        add_pass_content(ctx, child.lock(), content, 0);

    ankerl::unordered_dense::map<int, float> stage_x_offsets;

    ImVec2 total_size       = {group_padding.x, 0};
    float  current_offset_x = group_padding.x;
    for (int i = 0; content.stage_sizes.contains(i); ++i)
    {
        ImVec2 idx_size = content.stage_sizes.find(i)->second;
        total_size.x += idx_size.x + group_padding.x;
        if (idx_size.y > total_size.y)
            total_size.y = idx_size.y;

        stage_x_offsets.emplace(i, current_offset_x);
        current_offset_x += idx_size.x + group_padding.x * 2.f;
    }

    ankerl::unordered_dense::map<int, float> stage_y_offsets;
    for (auto& stage : content.passes | std::views::values)
    {
        auto offset           = stage_y_offsets.emplace(stage.stage, -group_padding.y).first;
        stage.y_offset = offset->second;
        offset->second += stage.size.y + group_padding.y * 2.f;
    }

    if (ImGui::IsMouseHoveringRect(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImGui::GetContentRegionAvail()) || !initialized)
    {
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || desired_zoom || !initialized)
        {
            auto& io       = ImGui::GetIO();
            float old_zoom = zoom;
            zoom *= atan(io.MouseWheel) * 0.5f + 1.f;

            if (desired_zoom)
            {
                zoom         = *desired_zoom;
                desired_zoom = {};
            }
            ImVec2 min_zoom = ImGui::GetContentRegionAvail() / (total_size + group_padding * 4.f);
            if (min_zoom.x < 0 || min_zoom.y < 0)
                return;
            initialized = true;
            if (zoom < std::min(min_zoom.x, min_zoom.y))
                zoom = std::min(min_zoom.x, min_zoom.y);

            ImVec2 center               = ImGui::GetMousePos() - ImGui::GetWindowPos();
            ImVec2 cursor_dist_to_start = ImVec2(last_scroll, last_scroll_y) + center;
            ImVec2 dist_not_scaled      = cursor_dist_to_start / old_zoom;
            ImVec2 dist_new_scaled      = dist_not_scaled * zoom - center;

            ImGui::SetNextWindowScroll(dist_new_scaled);
        }
    }

    if (desired_pos)
    {
        ImGui::SetNextWindowScroll(*desired_pos * zoom);
        desired_pos = {};
    }

    if (ImGui::BeginChild("canvas", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        last_scroll   = ImGui::GetScrollX();
        last_scroll_y = ImGui::GetScrollY();

        if (ImGui::BeginChild("Canvas content", {(total_size + group_padding * 4.f) * zoom}))
        {
            auto dl = ImGui::GetWindowDrawList();

            ImVec2 window_pos = ImGui::GetCursorScreenPos() + total_size * 0.05f * (zoom / 2.f);

            for (const auto& group : content.passes)
            {
                ImVec2 idx_size     = content.stage_sizes.find(group.second.stage)->second;
                float  idx_offset_x = stage_x_offsets.find(group.second.stage)->second;

                float offset_y = (total_size.y - idx_size.y) / 2.f;

                auto group_pos = ImVec2{total_size.x - idx_offset_x - idx_size.x, group.second.y_offset + offset_y};

                auto group_min = window_pos + group_pos * zoom;
                auto group_max = window_pos + (group_pos + group.second.size) * zoom;

                if (ImGui::IsMouseHoveringRect(group_min, group_max))
                {
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::Text("Render pass %s", group.second.name.c_str());
                        ImGui::EndTooltip();
                    }
                }

                dl->AddRectFilled(window_pos + group_pos * zoom, window_pos + (group_pos + group.second.size) * zoom, ImGui::ColorConvertFloat4ToU32(ImVec4(0.3f, 0.2f, 0.2f, 1.f)));

                for (const auto& attachment : group.second.attachments)
                {
                    ImVec2 attachment_pos = group_pos + ImVec2{((group.second.size.x - attachment.res.x) / 2.f), attachment.offset_y};
                    auto   min            = window_pos + attachment_pos * zoom;
                    auto   max            = window_pos + (attachment_pos + attachment.res) * zoom;

                    if (ImGui::IsMouseHoveringRect(min, max))
                    {
                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("Attachment %s", attachment.name.c_str());
                            ImGui::EndTooltip();
                        }
                        dl->AddRectFilled(min - image_padding * zoom * 0.5f, max + image_padding * zoom * 0.5f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.4f, 0.2f, 1.f)));
                        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                        {
                            desired_zoom = 1.f;
                            desired_pos  = attachment_pos;
                        }
                    }
                    dl->AddImage(ctx.add_image(attachment.image.lock()), min, max);
                }
            }
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void RenderGraphView::add_pass_content(Eng::Gfx::ImGuiWrapper& ctx, const std::shared_ptr<Eng::Gfx::RenderPassInstance>& pass, Content& content, int current_stage)
{
    if (auto found = content.passes.find(pass->get_definition().render_pass_ref); found != content.passes.end())
    {
        if (current_stage > found->second.stage)
            found->second.stage = current_stage;
        return;
    }
    Group group;
    group.name  = pass->get_definition().render_pass_ref.to_string();
    group.stage = current_stage;

    for (const auto& attachment : pass->get_definition().attachments | std::views::keys)
    {
        auto image = pass->get_image_resource(attachment).lock();
        if (!image)
            continue;

        auto extent = pass->resolution();

        Object object;
        object.image    = image;
        object.offset_y = group.size.y + image_padding.y;
        object.res      = {static_cast<float>(extent.x), static_cast<float>(extent.y)};
        object.name     = attachment;

        if (object.res.x + image_padding.x * 2.f > group.size.x)
            group.size.x = object.res.x + image_padding.x * 2.f;
        group.size.y += object.res.y + image_padding.y * 2.f;
        group.attachments.emplace_back(object);
    }

    auto& stage_dims = content.stage_sizes.emplace(current_stage, ImVec2{0, 0}).first->second;
    if (group.size.x + group_padding.x * 2.f > stage_dims.x)
        stage_dims.x = group.size.x + group_padding.y * 2.f;
    stage_dims.y += group.size.y + group_padding.y;

    content.passes.emplace(pass->get_definition().render_pass_ref, group);

    for (const auto& child : pass->all_childs())
        add_pass_content(ctx, child.lock(), content, current_stage + 1);
}