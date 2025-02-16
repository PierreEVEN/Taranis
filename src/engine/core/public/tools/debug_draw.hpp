#pragma once
#include "spinlock.hpp"

#include <mutex>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace Eng
{
class SceneView;

namespace Gfx
{
class CommandBuffer;
}


class DebugDraw
{
public:
    static DebugDraw& get();

    void add_segment(const glm::vec3& a, const glm::vec3& b, const glm::vec3& color = {1, 0, 0}, float duration = 0)
    {
        std::lock_guard lk(rw_lock);
        stored_segments.emplace_back(WireframePoint{a, color}, duration);
        stored_segments.emplace_back(WireframePoint{b, color}, duration);
    }

    void draw(Gfx::CommandBuffer& command_buffer, const SceneView& view);

private:
    friend class Engine;

    void destroy();

    void flush();

    struct WireframePoint
    {
        glm::vec3 position;
        glm::vec3 color;
    };

    struct StoredSegments
    {
        WireframePoint point;
        float          remaining_duration;
    };

    std::vector<StoredSegments> stored_segments;

    std::vector<WireframePoint> faces;

    Spinlock rw_lock;
};
}