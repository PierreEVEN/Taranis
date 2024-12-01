#include "profiler.hpp"

#include <iostream>

struct LocalDataContainer
{
    LocalDataContainer() : thread_data(std::make_shared<Profiler::ThreadData>()), this_thread(std::this_thread::get_id())
    {
    }

    std::mutex                            data_mutex;
    std::shared_ptr<Profiler::ThreadData> thread_data;
    std::thread::id                       this_thread;
};

static Profiler                                         profiler;
static std::mutex                                       all_threads_lock;
static std::vector<std::shared_ptr<LocalDataContainer>> all_threads;
static thread_local std::shared_ptr<LocalDataContainer> local_thread_data;


void Profiler::set_record(bool enabled)
{
    std::lock_guard lk(global_lock);
    if (enabled == b_record)
        return;
    b_record = enabled;
    if (b_record)
        record_start = std::chrono::steady_clock::now();
}

void Profiler::clear()
{
    std::lock_guard lk(global_lock);
    recorded_frames.clear();
}


void Profiler::next_frame()
{
    std::lock_guard lk(global_lock);
    if (!b_record)
        return;

    auto recorded_frame = std::make_shared<ProfilerFrameData>();

    std::lock_guard lk_all(all_threads_lock);

    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    for (auto& thread : all_threads)
    {
        std::lock_guard             lk_thread(thread->data_mutex);
        std::shared_ptr<ThreadData> new_thread_data = nullptr;

        if (!thread->thread_data->markers.empty() || !thread->thread_data->events.empty())
        {
            new_thread_data = std::make_shared<ThreadData>();
            new_thread_data->markers.reserve(thread->thread_data->markers.size());
            new_thread_data->events.reserve(thread->thread_data->events.size());
        }

        recorded_frame->thread_data.emplace(thread->this_thread, thread->thread_data);
        recorded_frame->start = record_start;
        recorded_frame->end   = end_time;
        thread->thread_data   = new_thread_data ? new_thread_data : std::make_shared<ThreadData>();
    }

    recorded_frames.emplace_back(recorded_frame);
    record_start = end_time;
}

Profiler& Profiler::get()
{
    return profiler;
}

Profiler::ThreadData& Profiler::get_thread_data()
{
    if (!local_thread_data)
    {
        local_thread_data = std::make_shared<LocalDataContainer>();
        std::lock_guard lk(all_threads_lock);
        all_threads.emplace_back(local_thread_data);
    }
    return *local_thread_data->thread_data;
}