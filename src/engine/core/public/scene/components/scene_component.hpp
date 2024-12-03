#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_ptr.hpp"
#include "scene/scene.hpp"
#include <glm/gtc/quaternion.hpp>

#include "scene\components\scene_component.gen.hpp"

namespace Eng
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
        assert(name);
        delete[] name;
        name = nullptr;
    }

    virtual void tick(double)
    {
    }

    template <typename T, typename... Args> TObjectRef<T> add_component(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of_v<SceneComponent, T>, "This type is not an SceneComponent");
        // this component could be moved during the next allocation. The TObjectRef will handle this move
        TObjectRef<SceneComponent> this_ref = scene->allocator->get_ref<SceneComponent>(this, get_class());
        if (!this_ref)
            LOG_FATAL("Internal error : failed to current_thread ref to this object");
        ObjectAllocation* alloc = this_ref->scene->allocator->allocate(T::static_class());
        T*                ptr   = static_cast<T*>(alloc->ptr);
        ptr->scene              = this_ref->scene;
        ptr->name               = new char[name.size() + 1];
        memcpy(const_cast<char*>(ptr->name), name.c_str(), name.size() + 1);
        new(alloc->ptr) T(std::forward<Args>(args)...);
        if (!ptr->name)
            LOG_FATAL("Object {} does not contains any constructor", typeid(T).name())
        TObjectPtr<T> obj_ptr(alloc);
        obj_ptr->parent = this_ref;
        this_ref->children.emplace_back(obj_ptr);
        return obj_ptr;
    }

    virtual void set_position(glm::vec3 in_position)
    {
        position = std::move(in_position);
        mark_transform_dirty();
    }

    virtual void set_rotation(glm::quat in_rotation)
    {
        rotation = std::move(in_rotation);
        mark_transform_dirty();
    }

    virtual void set_scale(glm::vec3 in_scale)
    {
        scale = std::move(in_scale);
        mark_transform_dirty();
    }

    const glm::vec3& get_relative_position() const
    {
        return position;
    }

    const glm::quat& get_relative_rotation() const
    {
        return rotation;
    }

    const glm::vec3& get_relative_scale() const
    {
        return scale;
    }

    const glm::mat4& get_world_transform()
    {
        if (b_transform_dirty)
        {
            world_transform = translate(mat4_cast(rotation) * glm::scale({1}, scale), position);
            if (parent)
                world_transform = parent->get_world_transform() * world_transform;
            b_transform_dirty = false;
        }
        return world_transform;
    }

    const std::vector<TObjectPtr<SceneComponent>>& get_nodes() const
    {
        return children;
    }

    const char* get_name() const
    {
        return name;
    }

    Scene& get_scene() const
    {
        return *scene;
    }

protected:
    SceneComponent()
    {
    }

private:
    void mark_transform_dirty()
    {
        b_transform_dirty = true;
        for (const auto& child : children)
            child->mark_transform_dirty();
    }

    const char*                             name;
    Scene*                                  scene;
    TObjectRef<SceneComponent>              parent = {};
    std::vector<TObjectPtr<SceneComponent>> children{};


    bool      b_transform_dirty = true;
    glm::mat4 world_transform{1};

    glm::vec3 position{0};
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale{1};
};
} // namespace Eng