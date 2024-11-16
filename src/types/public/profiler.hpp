#pragma once

#include "logger.hpp"

#include <cassert>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef ENABLE_PROFILER
#define PROFILER_MARKER(name) Profiler::get().add_marker({#name})
#define PROFILER_SCOPE(name)  Profiler::EventRecorder __profiler_event__##name(#name)
#define PROFILER_SCOPE_NAMED(name, string_name) Profiler::EventRecorder __profiler_event__##name(string_name)
#else
#define PROFILER_MARKER(name)
#define PROFILER_SCOPE(name)
#endif

class Profiler
{
public:
    class ProfilerMarker
    {
    public:
        ProfilerMarker(std::string in_name) : time(std::chrono::steady_clock::now()), name(std::move(in_name))
        {
        }

        std::chrono::steady_clock::time_point time;
        std::string                           name;
    };

    class ProfilerEvent
    {
    public:
        ProfilerEvent(std::string in_name, std::chrono::steady_clock::time_point in_start, std::chrono::steady_clock::time_point in_end) : start(in_start), end(in_end), name(std::move(in_name))
        {
        }

        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        std::string                           name;
    };

    class ProfilerThreadData
    {
    public:
        std::vector<ProfilerEvent>  events;
        std::vector<ProfilerMarker> markers;
    };

    class ProfilerFrameData
    {
    public:
        ProfilerFrameData() : threads_lock(std::make_unique<std::shared_mutex>())
        {
            min   = std::chrono::steady_clock::now();
            start = std::chrono::steady_clock::now();
        }

        std::chrono::steady_clock::time_point                   start;
        std::chrono::steady_clock::time_point                   end;
        std::chrono::steady_clock::time_point                   min;
        std::unique_ptr<std::shared_mutex>                      threads_lock;
        std::unordered_map<std::thread::id, ProfilerThreadData> thread_data;
    };

    static Profiler& get();

    void next_frame();

    void add_marker(const ProfilerMarker& marker) const
    {
        std::shared_lock lk1(global_lock);
        if (!b_record || !current_frame)
            return;
        auto&            data = get_thread_data();
        std::shared_lock lk(*current_frame->threads_lock);
        data.markers.push_back(marker);
    }

    void add_event(const ProfilerEvent& event) const
    {
        std::shared_lock lk1(global_lock);
        if (!b_record || !current_frame)
            return;
        auto&            data = get_thread_data();
        std::shared_lock lk(*current_frame->threads_lock);
        data.events.push_back(event);
    }

    class EventRecorder
    {
    public:
        EventRecorder(std::string in_name) : name(std::move(in_name)), start(std::chrono::steady_clock::now())
        {
        }

        ~EventRecorder()
        {
            end = std::chrono::steady_clock::now();
            get().add_event({name, start, end});
        }

        std::string                           name;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
    };

    void                                            start_recording();
    std::shared_ptr<ProfilerFrameData>              last_frame();
    std::vector<std::shared_ptr<ProfilerFrameData>> all_frames();
    void                                            stop_recording();

private:
    ProfilerThreadData& get_thread_data() const
    {
        auto thread_id = std::this_thread::get_id();
        {
            std::shared_lock lk(*current_frame->threads_lock);
            if (auto found = current_frame->thread_data.find(thread_id); found != current_frame->thread_data.end())
                return found->second;
        }
        std::unique_lock lk(*current_frame->threads_lock);
        return current_frame->thread_data.emplace(thread_id, ProfilerThreadData{}).first->second;
    }

    mutable std::shared_mutex                       global_lock;
    bool                                            b_record = false;
    std::shared_ptr<ProfilerFrameData>              current_frame;
    std::vector<std::shared_ptr<ProfilerFrameData>> recorded_frames;
};