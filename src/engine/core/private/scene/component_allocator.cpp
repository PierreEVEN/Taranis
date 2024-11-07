#include "scene/component_allocator.hpp"

#include "class.hpp"
#include "logger.hpp"

namespace Engine
{

void* ObjectPool::allocate()
{




}

void ObjectPool::free(void* ptr)
{

    if (auto offset = allocation_map.find(ptr); offset != allocation_map.end())
    {

    }
    else
        LOG_FATAL("Allocation {:x} is not allocated in this pool ({})", reinterpret_cast<size_t>(ptr), object_class->name())

}
}