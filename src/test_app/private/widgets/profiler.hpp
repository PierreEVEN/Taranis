#pragma once
#include "gfx/ui/ui_window.hpp"

#include <profiler.hpp>
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

        std::vector<std::shared_ptr<Profiler::ProfilerFrameData>> displayed_frames;
        std::vector<Box>                                          boxes;

        void build();
    };


    DisplayData display_data;

    void draw(Gfx::ImGuiWrapper& ctx) override;
};
}