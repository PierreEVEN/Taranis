#pragma once

#include "logger.hpp"
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <ankerl/unordered_dense.h>
#include <vector>

#ifdef ENABLE_PROFILER
#define PROFILER_MARKER(name)                   Profiler::get().add_marker({#name})
#define PROFILER_SCOPE(name)                    Profiler::EventRecorder __profiler_event__##name(#name)
#define PROFILER_SCOPE_NAMED(name, string_name) Profiler::EventRecorder __profiler_event__##name(string_name)
#else
#define PROFILER_MARKER(name)
#define PROFILER_SCOPE(name)
#define PROFILER_SCOPE_NAMED(name, string_name)
#endif


class Profiler final
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

    struct ThreadData final
    {
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

        std::chrono::steady_clock::time_point                                      start;
        std::chrono::steady_clock::time_point                                      end;
        std::chrono::steady_clock::time_point                                      min;
        std::unique_ptr<std::shared_mutex>                                         threads_lock;
        ankerl::unordered_dense::map<std::thread::id, std::shared_ptr<ThreadData>> thread_data;
    };

    void add_marker(const ProfilerMarker& marker) const
    {
        if (!b_record)
            return;
        get_thread_data().markers.push_back(marker);
    }

    void add_event(const ProfilerEvent& event) const
    {
        if (!b_record)
            return;
        get_thread_data().events.push_back(event);
    }

    struct FrameWrapper
    {
        FrameWrapper(Profiler* in_profiler) : profiler(in_profiler)
        {
            profiler->global_lock.lock();
        }

        ~FrameWrapper()
        {
            profiler->global_lock.unlock();
        }

        const std::vector<std::shared_ptr<ProfilerFrameData>>* operator->() const
        {
            return &profiler->recorded_frames;
        }

        Profiler* profiler;
    };

    void         set_record(bool enabled);
    void         clear();
    FrameWrapper frames()
    {
        return {this};
    }
    void         next_frame();

    static Profiler& get();

private:
    static ThreadData& get_thread_data();

    std::chrono::steady_clock::time_point           record_start;
    std::mutex                                      global_lock;
    bool                                            b_record = false;
    std::vector<std::shared_ptr<ProfilerFrameData>> recorded_frames;
};