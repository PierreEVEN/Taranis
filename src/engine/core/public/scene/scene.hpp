#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_allocator.hpp"
#include "object_ptr.hpp"

#include <vector>
#include "scene\scene.gen.hpp"

#include <glm/ext/matrix_float4x4.hpp>

namespace Eng::Gfx
{
class RenderPassInstance;
}

namespace Eng::Gfx
{
class Buffer;
}

namespace Eng
{
class CameraComponent;
}

class ContiguousObjectAllocator;

namespace Eng
{
namespace Gfx
{
class CommandBuffer;
}

class SceneComponent;

class Scene final
{
    REFLECT_BODY();
    friend class SceneComponent;

public:
    Scene();

    template <typename T, typename... Args> TObjectRef<T> add_component(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of_v<SceneComponent, T>, "This type is not an SceneComponent");
        ObjectAllocation* alloc = allocator->allocate(T::static_class());
        T*                ptr   = static_cast<T*>(alloc->ptr);
        ptr->scene              = this;
        ptr->name               = new char[name.size() + 1];
        memcpy(const_cast<char*>(ptr->name), name.c_str(), name.size() + 1);
        new(alloc->ptr) T(std::forward<Args>(args)...);
        if (!ptr->name)
            LOG_FATAL("Object {} does not contains any constructor", typeid(T).name())
        TObjectPtr<T> obj_ptr(alloc);
        root_nodes.emplace_back(obj_ptr);
        return obj_ptr;
    }

    void tick(double delta_second);

    void pre_draw(const Gfx::RenderPassInstance& render_pass);
    void pre_submit(const Gfx::RenderPassInstance& render_pass);
    void draw(const Gfx::RenderPassInstance& render_pass, Gfx::CommandBuffer& command_buffer, size_t idx, size_t num_threads);

    template <typename T>
    void for_each(const std::function<void(T&)>& callback) const
    {
        allocator->for_each(callback);
    }

    template <typename T> void for_each_part(const std::function<void(T&)>& callback, size_t part_index, size_t part_count) const
    {
        allocator->for_each_part(callback, part_index, part_count);
    }

    template <typename T> TObjectRef<T> get_component_ref(T* component)
    {
        return allocator->get_ref<T>(component, component->get_class());
    }

    const std::vector<TObjectPtr<SceneComponent>>& get_nodes() const
    {
        return root_nodes;
    }

    void merge(Scene&& other_scene)
    {
        std::lock_guard lk(*merge_queue_mtx);
        scenes_to_merge.push_back(std::move(other_scene));
    }

    void set_active_camera(const TObjectRef<CameraComponent>& camera)
    {
        active_camera = camera;
    }

    const TObjectRef<CameraComponent>& get_active_camera()
    {
        return active_camera;
    }

    std::weak_ptr<Gfx::Buffer> get_scene_buffer() const
    {
        return scene_buffer;
    }


private:
    TObjectRef<CameraComponent> active_camera;

    std::shared_ptr<Gfx::Buffer> scene_buffer;
    glm::mat4                    last_pv;

    std::unique_ptr<std::mutex> merge_queue_mtx;
    std::vector<Scene>          scenes_to_merge;

    std::unique_ptr<ContiguousObjectAllocator> allocator;

    std::vector<TObjectPtr<SceneComponent>> root_nodes;
};
} // namespace Eng