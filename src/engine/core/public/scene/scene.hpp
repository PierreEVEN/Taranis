#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_allocator.hpp"
#include "object_ptr.hpp"

#include <vector>
#include "scene\scene.gen.hpp"

class ContiguousObjectAllocator;

namespace Engine
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

    void draw(const Gfx::CommandBuffer& command_buffer);

    template <typename T> TObjectIterator<T> iterate()
    {
        return allocator->iter<T>();
    }

  private:
    std::unique_ptr<ContiguousObjectAllocator> allocator;

    std::vector<TObjectPtr<SceneComponent>> root_nodes;


};
} // namespace Engine