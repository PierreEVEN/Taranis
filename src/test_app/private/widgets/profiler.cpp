#include "widgets/profiler.hpp"

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
    boxes.clear();

    std::chrono::steady_clock::time_point start = displayed_frames[0]->start;

    std::unordered_map<std::thread::id, std::vector<Profiler::ProfilerEvent>> all_events;

    for (const auto& frame : displayed_frames)
    {
        for (auto& data : frame->declared_thread_data)
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

    for (const auto& thread_data : all_events)
    {
        for (const auto& event : thread_data.second)
        {
            auto ms_x     = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.start - start).count()) / 1000000.0;
            auto ms_x_end = static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(event.end - start).count()) / 1000000.0;
            boxes.emplace_back(Box{
                .min_max = {ms_x, ms_x_end},
                .stage = 0,
                .color = ImGui::ColorConvertFloat4ToU32({1, 1, 0, 1}),
                .name = event.name,
                .start = event.start,
                .duration = event.end - event.start,
            });
        }
    }
}

void ProfilerWindow::draw(Gfx::ImGuiWrapper& ctx)
{
    auto last = Profiler::get().last_frame();

    display_data = {.displayed_frames = {last}};
    display_data.build();


    

}
} // namespace Eng