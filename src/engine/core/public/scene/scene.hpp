#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_allocator.hpp"
#include "object_ptr.hpp"

#include <vector>
#include <glm/ext/matrix_float4x4.hpp>

#include "scene\scene.gen.hpp"

namespace Eng::Gfx
{
class Renderer;
class TemporaryRenderPassInstance;
class CustomPassList;
class RenderNode;
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
    Scene(Scene&& other) = default;

    ~Scene()
    {
        root_nodes.clear();
    }

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

    template <typename T> void for_each(const std::function<void(T&)>& callback) const
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

    void merge(Scene&& other_scene);

    void set_active_camera(const TObjectRef<CameraComponent>& camera)
    {
        active_camera = camera;
    }

    const TObjectRef<CameraComponent>& get_active_camera()
    {
        return active_camera;
    }

    void set_pass_list(const std::weak_ptr<Gfx::CustomPassList>& pass_list);

    std::shared_ptr<Gfx::TemporaryRenderPassInstance> add_custom_pass(const std::vector<std::string>& targets, const Gfx::Renderer& node) const;

    void remove_custom_pass(const std::shared_ptr<Gfx::TemporaryRenderPassInstance>& pass) const;

private:
    std::weak_ptr<Gfx::CustomPassList> custom_passes;

    TObjectRef<CameraComponent> active_camera;

    glm::mat4 last_pv;

    std::unique_ptr<std::mutex> merge_queue_mtx;
    std::vector<Scene>          scenes_to_merge;

    std::vector<TObjectPtr<SceneComponent>>    root_nodes;
    std::unique_ptr<ContiguousObjectAllocator> allocator;
};
} // namespace Eng