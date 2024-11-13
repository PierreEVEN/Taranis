#pragma once
#include "gfx/ui/ui_window.hpp"

#include <profiler.hpp>
#include <unordered_set>
#include <glm/vec2.hpp>


namespace Eng
{
class ProfilerWindow : public UiWindow
{
public:
    explicit ProfilerWindow(const std::string& name);

protected:
    struct DisplayData
    {
        struct Box
        {
            glm::dvec2                            min_max;
            int                                   stage = 0;
            uint32_t                              color;
            std::string                           name;
            std::chrono::steady_clock::time_point start;
            std::chrono::steady_clock::duration   duration;
        };

        std::unordered_set<size_t>                                selected_frames;
        std::vector<std::shared_ptr<Profiler::ProfilerFrameData>> displayed_frames;
        std::vector<Box>                                          boxes;

        void build();
    };

    bool b_record = true;
    bool b_always_display_last = true;

    
    DisplayData display_data;

    void draw(Gfx::ImGuiWrapper& ctx) override;
};
}