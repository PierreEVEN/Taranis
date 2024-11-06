#pragma once
#include "macros.hpp"
#include "scene\components\scene_component.gen.hpp"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine
{

class SceneComponent
{
    REFLECT_BODY();

public:
    void set_position(glm::vec3 in_position)
    {
        position          = std::move(in_position);
        b_transform_dirty = true;
    }

    void set_rotation(glm::quat in_rotation)
    {
        rotation          = std::move(in_rotation);
        b_transform_dirty = true;
    }

    void set_scale(glm::vec3 in_scale)
    {
        scale             = std::move(in_scale);
        b_transform_dirty = true;
    }

    const glm::vec3& get_position() const
    {
        return position;
    }

    const glm::quat& get_rotation() const
    {
        return rotation;
    }

    const glm::vec3& get_scale() const
    {
        return scale;
    }

    const glm::mat4& get_transform()
    {
        if (b_transform_dirty)
        {
            transform         = translate(mat4_cast(rotation) * glm::scale({1}, scale), position);
            b_transform_dirty = false;
        }
        return transform;
    }

private:
    bool      b_transform_dirty = true;
    glm::mat4 transform{1};

    glm::vec3 position{0};
    glm::quat rotation{};
    glm::vec3 scale{1};
};


}