#include "widgets/profiler.hpp"

#include "logger.hpp"

#include <algorithm>
#include <imgui.h>

namespace Eng
{
ProfilerWindow::ProfilerWindow(const std::string& name) : UiWindow(name)
{
    Profiler::get().start_recording();
}

void ProfilerWindow::DisplayData::build()
{
    threads.clear();
    global_min = DBL_MAX;
    global_max = DBL_MIN;
    if (displayed_frames.empty())
        return;

    bool                                  b_set_start = false;
    std::chrono::steady_clock::time_point start;

    std::unordered_map<std::thread::id, std::vector<Profiler::ProfilerEvent>> all_events;

    size_t i = 0;
    for (const auto& frame : displayed_frames)
    {
        if (!selected_frames.contains(i++))
            continue;

        if (!b_set_start)
        {
            start       = frame->min;
            b_set_start = true;
        }

        for (auto& data : frame->thread_data)
        {
            auto& thread = threads.emplace(data.first, ThreadGroup{}).first->second;

            for (const auto& markers : data.second.markers)
            {
                auto ms_x = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(markers.time - start).count()) / 1000000.0;
                if (ms_x > thread.max)
                    thread.max = ms_x;

                if (ms_x < thread.min)
                    thread.min = ms_x;
                thread.boxes.emplace_back(Box{
                    .min_max = {ms_x, ms_x},
                    .stage = 0,
                    .color = ImGui::ColorConvertFloat4ToU32({1, 1, 0, 1}),
                    .name = markers.name, .start = markers.time,
                    .duration = std::chrono::steady_clock::duration(0),
                });
            }
            for (const auto& events : data.second.events)
            {
                all_events.emplace(data.first, std::vector<Profiler::ProfilerEvent>{}).first->second.emplace_back(events);
            }
            if (thread.max > global_max)
                global_max = thread.max;

            if (thread.min < global_min)
                global_min = thread.min;
        }
    }

    // Sort events by start time
    for (auto& thread_data : all_events)
        std::ranges::sort(thread_data.second,
                          [](const Profiler::ProfilerEvent& a, const Profiler::ProfilerEvent& b)
                          {
                              return a.start < b.start;
                          });

    // This is the timecode of the last item for this level
    for (const auto& thread_data : all_events)
    {
        struct StageData
        {
            double ending = 0;
            bool   flip   = false;
        };
        std::vector<StageData> stage_data;

        auto get_stage_data = [&stage_data](size_t stage) -> StageData& {
            if (stage >= stage_data.size())
                stage_data.resize(stage + 1);
            return stage_data[stage];
        };

        auto& thread    = threads.emplace(thread_data.first, ThreadGroup{}).first->second;
        int   max_stage = -1;
        for (const auto& event : thread_data.second)
        {
            auto ms_x        = std::max(static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.start - start).count()) / 1000000.0, 0.0);
            auto ms_x_end    = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.end - start).count()) / 1000000.0;
            int  stage_index = 0;

            if (ms_x_end > thread.max)
                thread.max = ms_x_end;

            if (ms_x < thread.min)
                thread.min = ms_x;

            while (get_stage_data(stage_index).ending > ms_x)
                stage_index++;
            auto& found_stage_data  = get_stage_data(stage_index);
            found_stage_data.ending = ms_x_end;

            if (stage_index > max_stage)
                max_stage = stage_index;
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(stage_index * 0.05f, found_stage_data.flip ? 0.8f : 0.65f, 1, r, g, b);
            found_stage_data.flip   = !found_stage_data.flip;
            found_stage_data.ending = ms_x_end;
            thread.boxes.emplace_back(Box{
                .min_max = {ms_x, ms_x_end},
                .stage = stage_index,
                .color = ImGui::ColorConvertFloat4ToU32({r, g, b, 1}),
                .name = event.name,
                .start = event.start,
                .duration = event.end - event.start,
            });
        }
        thread.num_stages = max_stage + 1;
        if (thread.max > global_max)
            global_max = thread.max;

        if (thread.min < global_min)
            global_min = thread.min;
    }
}

ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
    return {a.x + b.x, a.y + b.y};
}

void ProfilerWindow::Frames::draw(DisplayData& display_data)
{
    ImGui::Text("Frame");
    ImGui::SameLine();
    if (ImGui::Button("Select all"))
    {
        for (size_t i = 0; i < display_data.displayed_frames.size(); ++i)
            display_data.selected_frames.insert(i);
        display_data.build();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear selection"))
    {
        display_data.selected_frames.clear();
        display_data.build();
    }
    if (ImGui::BeginChild("FrameContainer", {0, 200}, 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::BeginChild("frames", {std::max(static_cast<float>(display_data.displayed_frames.size()) * 2, ImGui::GetContentRegionAvail().x), 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            auto   dl    = ImGui::GetWindowDrawList();
            ImVec2 base  = ImGui::GetCursorPos() + ImGui::GetWindowPos();
            ImVec2 avail = ImGui::GetContentRegionAvail();

            size_t i = 0;
            for (const auto& frame : display_data.displayed_frames)
            {
                auto  duration_ms = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(frame->end - frame->start).count()) / 1000.f;
                float duration    = duration_ms * frames_zoom;
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(0.33f - std::min(duration / 120 * 0.5f, 1.f), 1, 1, r, g, b);
                auto min_top = base + ImVec2{i * 2.f, 0};
                auto min     = base + ImVec2{i * 2.f, avail.y - duration};
                auto max     = base + ImVec2((i + 1) * 2.f, avail.y);
                if (ImGui::IsMouseHoveringRect(min_top, max))
                {
                    dl->AddRectFilled(min_top, max, ImGui::ColorConvertFloat4ToU32({1, 0, 0, 0.5f}));
                    if (ImGui::BeginTooltip())
                    {
                        ImGui::Text("frame %d : %fms", i, duration_ms);
                        ImGui::EndPopup();
                    }
                    r = 1;
                    g = 1;
                    b = 0;
                    if (ImGui::IsMouseDown(ImGuiButtonFlags_MouseButtonLeft))
                    {
                        if (!select_frame)
                        {
                            first_frame_selected = display_data.selected_frames.contains(i);
                            if (!ImGui::IsKeyDown(ImGuiKey_LeftShift) && !ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
                                display_data.selected_frames.clear();
                        }
                        select_frame = true;
                        if (first_frame_selected)
                            display_data.selected_frames.erase(i);
                        else
                            display_data.selected_frames.insert(i);
                    }
                    else
                    {
                        if (select_frame)
                            display_data.build();
                        select_frame = false;
                    }
                }
                if (display_data.selected_frames.contains(i))
                    dl->AddRectFilled(min_top, max, ImGui::ColorConvertFloat4ToU32({0.7f, 0.7f, 0.7f, 0.3f}));
                dl->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32({r, g, b, 1}));
                ++i;
            }
            dl->AddLine(base + ImVec2{0, avail.y - 1000 / 30.f * frames_zoom}, base + ImVec2{avail.x, avail.y - 1000 / 30.f * frames_zoom}, ImGui::ColorConvertFloat4ToU32({0.5f, 0.3f, 0.3f, 1}), 1);
            dl->AddLine(base + ImVec2{0, avail.y - 1000 / 60.f * frames_zoom}, base + ImVec2{avail.x, avail.y - 1000 / 60.f * frames_zoom}, ImGui::ColorConvertFloat4ToU32({0.5f, 0.5f, 0.3f, 1}), 1);
            dl->AddLine(base + ImVec2{0, avail.y - 1000 / 120.f * frames_zoom}, base + ImVec2{avail.x, avail.y - 1000 / 120.f * frames_zoom}, ImGui::ColorConvertFloat4ToU32({0.3f, 0.5f, 0.3f, 1}), 1);
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void ProfilerWindow::Selection::draw(const DisplayData& display_data)
{
    float old_scale = scale;
    if (ImGui::BeginChild("SelectionContainer", {0, 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImGui::GetWindowPos() + ImGui::GetContentRegionAvail()))
        {
            if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || true)
            {
                auto& io = ImGui::GetIO();
                scale *= atan(io.MouseWheel) + 1;
                if (scale < 0.0001)
                    scale = 0.0001f;
                double center               = ImGui::GetMousePos().x - ImGui::GetWindowPos().x;
                double cursor_dist_to_start = ImGui::GetScrollX() + center;
                double dist_not_scaled      = cursor_dist_to_start / old_scale;
                double dist_new_scaled      = dist_not_scaled * static_cast<double>(scale) - center;

                ImGui::SetScrollX(static_cast<float>(dist_new_scaled));
            }
        }

        float width = std::max(static_cast<float>(display_data.global_max - display_data.global_min) * scale, ImGui::GetContentRegionAvail().x);
        if (ImGui::BeginChild("Selection", {width, 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            auto   dl   = ImGui::GetWindowDrawList();
            ImVec2 base = ImGui::GetCursorScreenPos();

            float current_height = 0;
            for (const auto& thread : display_data.threads)
            {
                constexpr float step_height = 10;
                for (const auto& box : thread.second.boxes)
                {
                    auto min = base + ImVec2{static_cast<float>(box.min_max.x * scale), box.stage * step_height + current_height};
                    auto max = base + ImVec2{static_cast<float>(box.min_max.y * scale), (box.stage + 1) * step_height + current_height};
                    dl->AddRectFilled(min, max, box.color);

                    if (ImGui::IsMouseHoveringRect(min, max))
                    {
                        dl->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32({1, 1, 1, 1}));

                        if (ImGui::BeginTooltip())
                        {
                            ImGui::Text("%s : %fms", box.name.c_str(), static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(box.duration).count()) / 1000.f);
                            ImGui::EndPopup();
                        }
                    }
                }
                current_height += thread.second.num_stages * step_height;
                current_height += 10;
                dl->AddLine({base.x, base.y + current_height}, {base.x + width, base.y + current_height}, ImGui::ColorConvertFloat4ToU32({0.5, 0.5, 0.5, 1}), 2);
                current_height += 10;
            }
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
}

void ProfilerWindow::draw(Gfx::ImGuiWrapper&)
{
    if (ImGui::Button("clear"))
    {
        Profiler::get().stop_recording();
        display_data.selected_frames.clear();
        display_data.displayed_frames.clear();
        display_data.build();
        Profiler::get().start_recording();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Record", &b_record);
    ImGui::SameLine();
    ImGui::Checkbox("Always focus last frame", &b_always_display_last);
    ImGui::SameLine();

    if (b_record)
    {
        Profiler::get().start_recording();

        if (auto frame = Profiler::get().last_frame())
            display_data.displayed_frames.emplace_back(frame);

        if (b_always_display_last && !display_data.displayed_frames.empty())
        {
            display_data.selected_frames = {display_data.displayed_frames.size() - 1};
            display_data.build();
        }
    }
    else
    {
        Profiler::get().stop_recording();
    }

    ImGui::Separator();
    frames.draw(display_data);
    ImGui::Separator();
    selection.draw(display_data);
}
} // namespace Eng