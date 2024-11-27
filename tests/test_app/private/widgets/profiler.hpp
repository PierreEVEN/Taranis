#pragma once
#include "gfx/ui/ui_window.hpp"

#include <glm/vec2.hpp>
#include <imgui.h>
#include <profiler.hpp>
#include <unordered_set>

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

        struct ThreadGroup
        {
            int              num_stages = 0;
            double           min        = DBL_MAX;
            double           max        = DBL_MIN;
            std::vector<Box> boxes;
        };

        double                                           global_min = DBL_MAX;
        double                                           global_max = DBL_MIN;
        std::unordered_map<std::thread::id, ThreadGroup> threads;

        std::optional<double> target_width;
        std::optional<double> target_start;

        void build();
    };

    void draw_selection();

    struct Frames
    {
        bool  select_frame         = false;
        bool  first_frame_selected = false;
        float frames_zoom          = 5;
        void  draw(DisplayData& display_data);
    };

    struct Selection
    {
        float last_scroll   = 0;
        float last_scroll_y = 0;
        float scale         = 10;
        void  draw(DisplayData& display_data);
    };

    Frames    frames;
    Selection selection;

    bool b_record              = true;
    bool b_always_display_last = false;

    DisplayData display_data;

    void draw(Gfx::ImGuiWrapper& ctx) override;
};
} // namespace Eng