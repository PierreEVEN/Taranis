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


static constexpr ImVec2 image_padding{10, 10};
static constexpr ImVec2 group_padding{20, 20};

void RenderGraphView::draw(Eng::Gfx::ImGuiWrapper& ctx)
{
    if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
    {
        auto& io        = ImGui::GetIO();
        float old_scale = zoom;
        zoom *= atan(io.MouseWheel) * 0.5f + 1;
        if (zoom > 1)
            zoom = 1;
    }

    if (ImGui::BeginChild("canvas", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        Content content;
        for (const auto& child : ctx.get_current_render_pass()->all_childs())
            add_pass_content(ctx, child.lock(), content, 0);

        ankerl::unordered_dense::map<int, float> stage_x_offsets;

        ImVec2 total_size       = {0, 0};
        float  current_offset_x = group_padding.x;
        for (int i = 0; content.stage_sizes.contains(i); ++i)
        {
            ImVec2 idx_size = content.stage_sizes.find(i)->second;
            total_size.x += idx_size.x;
            if (idx_size.y > total_size.y)
                total_size.y = idx_size.y;

            stage_x_offsets.emplace(i, current_offset_x);
            current_offset_x += idx_size.x + group_padding.x * 2;
        }

        ankerl::unordered_dense::map<int, float> stage_y_offsets;
        for (auto& stage : content.passes)
        {
            auto offset = stage_y_offsets.emplace(stage.second.stage, group_padding.y).first;
            stage.second.y_offset = offset->second;
            offset->second += stage.second.size.y;
        }

        if (ImGui::BeginChild("Canvas content", {total_size * 1.1f * zoom}))
        {
            auto dl = ImGui::GetWindowDrawList();

            ImVec2 window_pos = ImGui::GetCursorScreenPos() + total_size * 0.05f * (zoom / 2.f);

            for (const auto& group : content.passes)
            {
                ImVec2 idx_size     = content.stage_sizes.find(group.second.stage)->second;
                float  idx_offset_x = stage_x_offsets.find(group.second.stage)->second;

                auto group_pos = ImVec2{total_size.x, total_size.y / 2} - ImVec2{idx_offset_x + idx_size.x, group.second.y_offset + idx_size.y / 2};

                dl->AddRectFilled(window_pos + group_pos * zoom, window_pos + (group_pos + group.second.size) * zoom, ImGui::ColorConvertFloat4ToU32(ImVec4(0.5f, 0.2f, 0.2f, 1)));

                for (const auto& attachment : group.second.attachments)
                {
                    ImVec2 attachment_pos = group_pos + ImVec2{((group.second.size.x - attachment.res.x) / 2), attachment.offset_y};
                    dl->AddRectFilled(window_pos + attachment_pos * zoom, window_pos + (attachment_pos + attachment.res) * zoom, ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.8f, 0.2f, 1)));
                    dl->AddImage(ctx.add_image(attachment.image.lock()), window_pos + attachment_pos * zoom, window_pos + (attachment_pos + attachment.res) * zoom);
                }
            }
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void RenderGraphView::add_pass_content(Eng::Gfx::ImGuiWrapper& ctx, const std::shared_ptr<Eng::Gfx::RenderPassInstance>& pass, Content& content, int current_stage)
{
    if (auto found = content.passes.find(pass->get_definition().name); found != content.passes.end())
    {
        if (current_stage > found->second.stage)
            found->second.stage = current_stage;
        return;
    }
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
        object.image    = image;
        object.offset_y = group.size.y + image_padding.y;
        object.res      = {static_cast<float>(extent.x), static_cast<float>(extent.y)};
        object.name     = attachment;

        if (object.res.x + image_padding.x * 2 > group.size.x)
            group.size.x = object.res.x + image_padding.x * 2;
        group.size.y += object.res.y + image_padding.y * 2;
        group.attachments.emplace_back(object);
    }

    auto& stage_dims = content.stage_sizes.emplace(current_stage, ImVec2{0, 0}).first->second;
    if (group.size.x + group_padding.x * 2 > stage_dims.x)
        stage_dims.x = group.size.x + group_padding.y * 2;
    stage_dims.y += group.size.y + group_padding.y;

    content.passes.emplace(pass->get_definition().name, group);

    for (const auto& child : pass->all_childs())
        add_pass_content(ctx, child.lock(), content, current_stage + 1);
}