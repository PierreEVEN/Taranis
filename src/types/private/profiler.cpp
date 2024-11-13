#include "profiler.hpp"

#include <ranges>

static Profiler* profiler = nullptr;

Profiler& Profiler::get()
{
    if (!profiler)
        profiler = new Profiler();
    return *profiler;
}

void Profiler::next_frame()
{
    if (!b_record)
        return;
    std::unique_lock                   global_lk(global_lock);
    std::shared_ptr<ProfilerFrameData> last = recorded_frames.empty() ? nullptr : recorded_frames.back();
    recorded_frames.emplace_back(std::move(current_frame));
    current_frame        = std::make_shared<ProfilerFrameData>();
    current_frame->start = std::chrono::steady_clock::now();
    if (last)
    {
        for (const auto& [k, v] : last->thread_data)
        {
            ProfilerThreadData data;
            data.markers.reserve(v.markers.size());
            data.events.reserve(v.events.size());
            current_frame->thread_data.emplace(k, std::move(data));
        }
    }
}

void Profiler::start_recording()
{
    if (b_record)
        return;
    std::unique_lock global_lk(global_lock);
    recorded_frames.clear();
    b_record = true;

    current_frame        = std::make_shared<ProfilerFrameData>();
    current_frame->start = std::chrono::steady_clock::now();
}

std::shared_ptr<Profiler::ProfilerFrameData> Profiler::last_frame()
{
    std::shared_lock global_lk(global_lock);
    if (recorded_frames.empty())
        return current_frame;
    return recorded_frames.back();
}

std::vector<std::shared_ptr<Profiler::ProfilerFrameData>> Profiler::all_frames()
{
    std::shared_lock global_lk(global_lock);
    if (!b_record)
        return {};
    return recorded_frames;
}

std::vector<std::shared_ptr<Profiler::ProfilerFrameData>> Profiler::stop_recording()
{
    std::unique_lock                                global_lk(global_lock);
    std::vector<std::shared_ptr<ProfilerFrameData>> frames = recorded_frames;
    frames.emplace_back(current_frame);
    recorded_frames.clear();
    current_frame = nullptr;
    b_record      = false;
    return frames;
}