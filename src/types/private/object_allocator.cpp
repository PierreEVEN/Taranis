#include "object_allocator.hpp"

#include "class.hpp"
#include "logger.hpp"
#include "object_ptr.hpp"
#include "profiler.hpp"

ObjectAllocation* ContiguousObjectPool::allocate()
{
    ObjectAllocation* allocation = new ObjectAllocation();
    allocation->object_class     = object_class;
    reserve(component_count + 1);
    allocation->ptr = static_cast<uint8_t*>(memory) + (component_count * stride);
    std::memset(allocation->ptr, 0, stride);
    allocation_map.emplace(allocation->ptr, allocation);
    component_count += 1;
    return allocation;
}

ObjectAllocation* ContiguousObjectPool::find(void* ptr)
{
    if (auto found = allocation_map.find(ptr); found != allocation_map.end())
        return found->second;
    return nullptr;
}

void ContiguousObjectPool::free(void* ptr)
{
    if (auto obj_ptr = allocation_map.find(ptr); obj_ptr != allocation_map.end())
    {
        void* removed_element = obj_ptr->second->ptr;
        obj_ptr->second->ptr  = nullptr;
        allocation_map.erase(obj_ptr);
        component_count--;

        if (component_count > 0)
        {
            memcpy(removed_element, nth(component_count), stride);
            auto moved_elem         = allocation_map.find(nth(component_count));
            moved_elem->second->ptr = removed_element;
            allocation_map.emplace(removed_element, moved_elem->second);
            allocation_map.erase(nth(component_count));
        }

        reserve(component_count);
    }
    else
        LOG_FATAL("Allocation {:x} is not allocated in this pool ({})", reinterpret_cast<size_t>(ptr), object_class->name())
}

void ContiguousObjectPool::merge(ContiguousObjectPool& other)
{
    if (other.component_count == 0)
        return;

    reserve(this->component_count + other.component_count);
    memcpy(nth(component_count), other.nth(0), stride * other.component_count);
    for (const auto& old_alloc : other.allocation_map)
    {
        ObjectAllocation* allocation = old_alloc.second;
        allocation->ptr              = nth(other.nth_ptr(old_alloc.first) + component_count);
        allocation_map.emplace(allocation->ptr, allocation);
    }
    component_count = component_count + other.component_count;
    other.allocation_map.clear();
    other.component_count = 0;
    other.resize(0);
}

void ContiguousObjectPool::reserve(size_t desired_count)
{
    if (desired_count == 0)
        resize(0);

    if (desired_count > allocated_count)
        resize(static_cast<size_t>(std::ceil(static_cast<double>(desired_count) * 1.5)));
}

void ContiguousObjectPool::resize(size_t new_count)
{
    if (new_count < component_count)
        LOG_FATAL("Cannot resize Object pool bellow it's component count");

    if (new_count == 0)
    {
        std::free(memory);
        memory          = nullptr;
        allocated_count = 0;
        component_count = 0;
    }
    else
    {
        if (new_count != allocated_count)
        {
            PROFILER_SCOPE_NAMED(ResizeAllocation, std::format("Allocator resize for {}", object_class->name()));
            void* new_memory = std::realloc(memory, new_count * stride);
            if (!new_memory)
                LOG_FATAL("Failed to allocate memory for object {}", object_class->name())

            if (memory != new_memory)
                move_old_to_new_block(memory, new_memory);
            memory          = new_memory;
            allocated_count = new_count;
        }
    }
}

void ContiguousObjectPool::move_old_to_new_block(void* old, void* new_block)
{
    if (!old)
        return;

    ankerl::unordered_dense::map<void*, ObjectAllocation*> new_alloc_map;

    for (size_t i = 0; i < component_count; ++i)
    {
        void* old_ptr = static_cast<uint8_t*>(old) + (i * stride);
        void* new_ptr = static_cast<uint8_t*>(new_block) + (i * stride);

        if (auto found = allocation_map.find(old_ptr); found != allocation_map.end())
        {
            found->second->ptr = new_ptr;
            new_alloc_map.emplace(new_ptr, found->second);
        }
    }
    allocation_map = new_alloc_map;
}

ContiguousObjectAllocator::ContiguousObjectAllocator()
{
}

ObjectAllocation* ContiguousObjectAllocator::allocate(const Reflection::Class* component_class)
{
    assert(component_class);
    ObjectAllocation* allocation = pools.emplace(component_class, std::make_unique<ContiguousObjectPool>(component_class)).first->second->allocate();
    allocation->allocator        = this;
    return allocation;
}

void ContiguousObjectAllocator::free(const Reflection::Class* component_class, void* allocation)
{
    if (auto pool = pools.find(component_class); pool != pools.end())
        pool->second->free(allocation);
    else
        LOG_FATAL("No object {} was allocated using this allocator", component_class->name())
}

void ContiguousObjectAllocator::merge_with(ContiguousObjectAllocator& other)
{
    for (const auto& pool : other.pools)
        pools.emplace(pool.first, std::make_unique<ContiguousObjectPool>(pool.second->get_class())).first->second->merge(*pool.second);
}

std::vector<ContiguousObjectPool*> ContiguousObjectAllocator::find_pools(const Reflection::Class* parent_class) const
{
    std::vector<ContiguousObjectPool*> found;
    for (const auto& pool : pools)
        if (parent_class->is_base_of(pool.first))
            found.push_back(pool.second.get());
    return found;
}