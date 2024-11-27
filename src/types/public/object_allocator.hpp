#pragma once
#include "logger.hpp"
#include "object_ptr.hpp"

#include <memory>
#include <ranges>
#include <unordered_map>

struct ObjectAllocation;
class IObjectPtr;

namespace Reflection
{
class Class;
}

class ObjectAllocator
{
  public:
    virtual const ObjectAllocation* allocate(const Reflection::Class* component_class)               = 0;
    virtual void                    free(const Reflection::Class* component_class, void* allocation) = 0;
};

class ContiguousObjectPool
{
  public:
    ContiguousObjectPool(const Reflection::Class* in_object_class) : object_class(in_object_class), stride(object_class->stride())
    {
    }

    ContiguousObjectPool(ContiguousObjectPool&)  = delete;
    ContiguousObjectPool(ContiguousObjectPool&&) = delete;

    ~ContiguousObjectPool()
    {
        std::free(memory);
    }

    ObjectAllocation* allocate();
    ObjectAllocation* find(void* ptr);
    void              free(void* ptr);

    void* nth(size_t i) const
    {
        return static_cast<uint8_t*>(memory) + i * stride;
    }

    size_t nth_ptr(void* ptr) const
    {
        return (static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(memory)) / stride;
    }

    void merge(ContiguousObjectPool& other);

    const Reflection::Class* get_class() const
    {
        return object_class;
    }

    size_t size() const
    {
        return component_count;
    }

  private:
    void reserve(size_t desired_count);
    void resize(size_t new_count);
    void move_old_to_new_block(void* old, void* new_block);

    const Reflection::Class* object_class;
    void*                    memory          = nullptr;
    size_t                   allocated_count = 0;
    size_t                   component_count = 0;
    const size_t             stride;

    std::unordered_map<void*, ObjectAllocation*> allocation_map;
};

template <typename T> class TObjectIterator
{
  public:
    TObjectIterator(const std::vector<ContiguousObjectPool*>& in_classes) : classes(in_classes)
    {
        if (classes.empty())
            return;
        current_class   = classes[0];
        this_pool_count = current_class->size();
    }

    T& operator*() const
    {
        return *static_cast<T*>(current_class->nth(index));
    }

    T* operator->()
    {
        return static_cast<T*>(current_class->nth(index));
    }

    TObjectIterator& operator++()
    {
        ++index;
        if (index >= this_pool_count)
        {
            ++current_class_index;
            if (current_class_index < classes.size())
            {
                current_class   = classes[current_class_index];
                this_pool_count = current_class->size();
            }
            else
                this_pool_count = 0;
            index = 0;
        }
        return *this;
    }

    operator bool() const
    {
        return index < this_pool_count && current_class_index < classes.size();
    }

  private:
    size_t                             index               = 0;
    size_t                             this_pool_count     = 0;
    size_t                             current_class_index = 0;
    ContiguousObjectPool*              current_class       = nullptr;
    std::vector<ContiguousObjectPool*> classes;
};

template <typename T> class TObjectIteratorPart
{
  public:
    TObjectIteratorPart(const std::vector<ContiguousObjectPool*>& in_classes, size_t first_pool_start, size_t last_pool_end) : classes(in_classes), index(first_pool_start), end(last_pool_end)
    {
        if (classes.empty())
            return;
        current_class   = classes[0];
        this_pool_count = current_class->size();
    }

    T& operator*() const
    {
        return *static_cast<T*>(current_class->nth(index));
    }

    T* operator->()
    {
        return static_cast<T*>(current_class->nth(index));
    }

    TObjectIteratorPart& operator++()
    {
        ++index;
        if (index >= this_pool_count)
        {
            ++current_class_index;
            if (current_class_index < classes.size())
            {
                current_class   = classes[current_class_index];
                this_pool_count = current_class->size();
            }
            else
                this_pool_count = 0;
            index = 0;
        }
        return *this;
    }

    operator bool() const
    {
        return index < this_pool_count && (current_class_index != classes.size() - 1 || index < end);
    }

  private:
    size_t                             index               = 0;
    size_t                             end                 = 0;
    size_t                             this_pool_count     = 0;
    size_t                             current_class_index = 0;
    ContiguousObjectPool*              current_class       = nullptr;
    std::vector<ContiguousObjectPool*> classes;
};

class ContiguousObjectAllocator : public ObjectAllocator
{
  public:
    ContiguousObjectAllocator();

    ObjectAllocation* allocate(const Reflection::Class* component_class) override;
    void              free(const Reflection::Class* component_class, void* allocation) override;

    template <typename T, typename... Args> TObjectPtr<T> construct(Args&&... args)
    {
        ObjectAllocation* allocation = allocate(T::static_class());
        new (allocation->ptr) T(std::forward<Args>(args)...);
        return TObjectPtr<T>(allocation);
    }

    template <typename T> void for_each(const std::function<void(T&)>& callback)
    {
        for (auto ite = TObjectIterator<T>(find_pools(T::static_class())); ite; ++ite)
            callback(*ite);
    }

    template <typename T> void for_each_part(const std::function<void(T&)>& callback, size_t part_index, size_t part_count)
    {
        for (ContiguousObjectPool* pool : find_pools(T::static_class()))
        {
            size_t components_per_chunk = static_cast<size_t>(static_cast<double>(pool->size()) / static_cast<double>(part_count));
            size_t start                = part_index * components_per_chunk;
            size_t end                  = part_index == part_count - 1 ? pool->size() : (part_index + 1) * components_per_chunk;

            for (size_t i = start; i < end; ++i)
                callback(*static_cast<T*>(pool->nth(i)));
        }
    }

    template <typename T> TObjectRef<T> get_ref(T* object, const Reflection::Class* static_class)
    {
        if (auto found = pools.find(static_class); found != pools.end())
            if (auto* allocation = found->second->find(object))
                return TObjectRef<T>(allocation);
        return {};
    }

    void merge_with(ContiguousObjectAllocator& other);

  private:
    std::vector<ContiguousObjectPool*> find_pools(const Reflection::Class* parent_class) const;

    std::unordered_map<const Reflection::Class*, std::unique_ptr<ContiguousObjectPool>> pools;
};