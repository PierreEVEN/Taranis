#pragma once

#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#ifdef ENABLE_PROFILER
#define PROFILER_MARKER(name) Profiler::get().add_marker({#name})
#define PROFILER_SCOPE(name)  Profiler::EventRecorder __profiler_event__##name(#name)
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
        ProfilerFrameData() = default;
        std::unordered_map<std::thread::id, ProfilerThreadData> declared_thread_data;
        std::mutex                                              other_thread_lock;
        std::unordered_map<std::thread::id, ProfilerThreadData> other_thread_data;
    };

    static Profiler& get();

    void next_frame();

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

    class EventRecorder
    {
      public:
        EventRecorder(std::string in_name) : name(std::move(in_name)), start(std::chrono::steady_clock::now())
        {
        }

        ~EventRecorder()
        {
            end = std::chrono::steady_clock::now();
            Profiler::get().add_event({name, start, end});
        }

        std::string                           name;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
    };

    void                                            start_recording();
    std::shared_ptr<ProfilerFrameData>              last_frame();
    std::vector<std::shared_ptr<ProfilerFrameData>> stop_recording();
    void declare_worker_thread(std::thread::id id, const std::string& name)
    {
        worker_threads.emplace(id, name);
    }

  private:
    ProfilerThreadData& get_thread_data() const
    {
        auto thread_id = std::this_thread::get_id();
        if (auto found = current_frame->declared_thread_data.find(thread_id); found != current_frame->declared_thread_data.end())
            return found->second;
        std::lock_guard lk(current_frame->other_thread_lock);
        return current_frame->other_thread_data.emplace(thread_id, ProfilerThreadData{}).first->second;
    }

    std::unordered_map<std::thread::id, std::string> worker_threads;

    bool                                            b_record = false;
    std::shared_ptr<ProfilerFrameData>              current_frame;
    std::vector<std::shared_ptr<ProfilerFrameData>> recorded_frames;
};