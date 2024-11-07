#pragma once
#include "logger.hpp"
#include "macros.hpp"
#include "object_ptr.hpp"

#include <vector>
#include "scene\scene.gen.hpp"

namespace Engine
{
namespace Gfx
{
class CommandBuffer;
}

class SceneComponent;

class Scene
{
    REFLECT_BODY();

public:
    template <typename T, typename... Args> TObjectRef<T> add_component(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of_v<SceneComponent, T>, "This type is not an SceneComponent");
        T* data     = static_cast<T*>(calloc(1, sizeof(T)));
        data->scene = this;
        data->name  = new char[name.size() + 1];
        memcpy(const_cast<char*>(data->name), name.c_str(), name.size() + 1);
        new (data) T(std::forward<Args>(args)...);
        if (!data->name)
            LOG_FATAL("Object {} does not contains any constructor", typeid(T).name())
        TObjectPtr<T> ptr(std::move(data));
        root_nodes.emplace_back(ptr);
        return ptr;
    }

    void tick(double delta_second);

    void draw(Gfx::CommandBuffer& command_buffer);

private:
    std::vector<TObjectPtr<SceneComponent>> root_nodes;
};
} // namespace Engine