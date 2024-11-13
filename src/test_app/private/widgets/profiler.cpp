#include "widgets/profiler.hpp"

#include "logger.hpp"

#include <algorithm>
#include <imgui.h>
#include <ranges>

namespace Eng
{
ProfilerWindow::ProfilerWindow(const std::string& name) : UiWindow(name)
{
    Profiler::get().start_recording();
}

void ProfilerWindow::DisplayData::build()
{
    boxes.clear();
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
            start       = frame->start;
            b_set_start = true;
        }

        for (auto& data : frame->thread_data)
        {
            for (const auto& markers : data.second.markers)
            {
                auto ms_x = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(markers.time - start).count()) / 1000000.0;
                boxes.emplace_back(Box{
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
    std::unordered_map<int, double> stage_endings;

    int current_stage = 0;
    for (const auto& thread_data : all_events | std::views::values)
    {
        int max_stage = -1;
        for (const auto& event : thread_data)
        {
            auto ms_x     = std::max(static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.start - start).count()) / 1000000.0, 0.0);
            auto ms_x_end = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.end - start).count()) / 1000000.0;
            int  stage    = 0;

            while (stage_endings.emplace(stage, 0).first->second > ms_x)
                stage++;

            if (stage > max_stage)
                max_stage = stage + 1;
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(stage * 0.1f, 1, 1, r, g, b);
            stage_endings.insert_or_assign(stage, ms_x_end);
            boxes.emplace_back(Box{
                .min_max = {ms_x, ms_x_end},
                .stage = current_stage + stage,
                .color = ImGui::ColorConvertFloat4ToU32({r, g, b, 1}),
                .name = event.name,
                .start = event.start,
                .duration = event.end - event.start,
            });
        }
        current_stage = max_stage + 1;
    }
}

ImVec2 operator+(const ImVec2& a, const ImVec2& b)
{
    return {a.x + b.x, a.y + b.y};
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
    if (ImGui::Button("Select all"))
    {
        size_t i = 0;
        for (const auto& frame : display_data.displayed_frames)
            display_data.selected_frames.insert(i++);
        display_data.build();

    }
    ImGui::SameLine();
    ImGui::Checkbox("Always focus last frame", &b_always_display_last);
    ImGui::SameLine();
    ImGui::DragFloat("Scale", &scale);

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
    size_t i = 0;
    if (ImGui::BeginChild("FrameContainer", {0, 50}, 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::BeginChild("frames", {static_cast<float>(display_data.displayed_frames.size()) * 3, 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            for (const auto& frame : display_data.displayed_frames)
            {
                bool b_set_button = display_data.selected_frames.contains(i);
                if (b_set_button)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5, 0.5, 0, 1));
                if (ImGui::Button(std::format("##but_frame_{}", i).c_str(), {3, 20}))
                {
                    if (display_data.selected_frames.contains(i))
                        display_data.selected_frames.erase(i);
                    else
                        display_data.selected_frames.insert(i);
                    display_data.build();
                }
                if (b_set_button)
                    ImGui::PopStyleColor();
                ImGui::SameLine(0, 0);
                ++i;
            }
        }

        ImGui::EndChild();
    }
    ImGui::EndChild();

    float  total_max  = 0;
    float total_min = FLT_MAX;
    if (ImGui::BeginChild("LogsContainer", {0, 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
    {
        if (ImGui::BeginChild("Logs", {last_max * scale, 0}, 0, ImGuiWindowFlags_HorizontalScrollbar))
        {
            auto dl = ImGui::GetWindowDrawList();

            ImVec2 base = ImGui::GetCursorPos() + ImGui::GetWindowPos();
            for (const auto& box : display_data.boxes)
            {
                float step_height = 10;

                if (box.min_max.x < total_min)
                    total_min = box.min_max.x;
                if (box.min_max.y > total_max)
                    total_max = box.min_max.y;
                auto min = base + ImVec2{static_cast<float>(box.min_max.x * scale), box.stage * step_height};
                auto max = base + ImVec2{static_cast<float>(box.min_max.y * scale), (box.stage + 1) * step_height};
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
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();
    last_max = total_max - total_min;
}
} // namespace Eng