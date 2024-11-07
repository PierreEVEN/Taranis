#pragma once
#include <memory>
#include <unordered_map>

class IObjectPtr;

namespace Reflection
{
class Class;
}

namespace Engine
{

class ObjectPool
{
public:
    ObjectPool(const Reflection::Class* in_object_class) : object_class(in_object_class)
    {
    }

    ObjectPool()             = default;
    ObjectPool(ObjectPool&)  = delete;
    ObjectPool(ObjectPool&&) = delete;

    ~ObjectPool()
    {
        free(components);
    }

    void* allocate();
    void  free(void* ptr);

private:
    void*                    components      = nullptr;
    size_t                   allocation_size = 0;
    size_t                   component_count = 0;
    const Reflection::Class* object_class;

    std::unordered_map<void*, std::unique_ptr<IObjectPtr>> allocation_map;
};


class ObjectAllocator
{
public:
    void* allocate(Reflection::Class* component_class);
    void  free(Reflection::Class* component_class, void* allocation);

private:
    std::unordered_map<Reflection::Class*, ObjectPool> pools;
};
}