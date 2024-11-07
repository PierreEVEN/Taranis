#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_ptr.hpp"

#include <glm/gtc/quaternion.hpp>
#include "scene\components\scene_component.gen.hpp"

namespace Engine
{
class Scene;

class SceneComponent
{
    REFLECT_BODY();

    friend class Scene;
    SceneComponent(SceneComponent&)  = delete;
    SceneComponent(SceneComponent&&) = delete;

    void internal_tick(double delta_second);

public:
    virtual ~SceneComponent()
    {
        delete[] name;
    }

    virtual void tick(double)
    {
    }

    template <typename T, typename... Args> TObjectRef<T> add_component(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of_v<SceneComponent, T>, "This type is not an SceneComponent");
        T* data     = static_cast<T*>(calloc(1, sizeof(T)));
        data->scene = scene;
        data->name  = new char[name.size() + 1];
        memcpy(const_cast<char*>(data->name), name.c_str(), name.size() + 1);
        new(data) T(std::forward<Args>(args)...);
        if (!data->name)
            LOG_FATAL("Object {} does not contains any constructor", typeid(T).name())
        TObjectPtr<T> ptr(std::move(data));
        children.emplace_back(ptr);
        return ptr;
    }

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

protected:
    explicit SceneComponent()
    {
        std::cout << "construct : " << (size_t)name << "\n";

    };

private:
    const char*  name;
    const Scene* scene;

    bool      b_transform_dirty = true;
    glm::mat4 transform{1};

    glm::vec3                               position{0};
    glm::quat                               rotation{};
    glm::vec3                               scale{1};
    std::vector<TObjectPtr<SceneComponent>> children;
};
}