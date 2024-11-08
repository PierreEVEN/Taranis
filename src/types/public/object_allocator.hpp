#pragma once
#include "logger.hpp"
#include "object_ptr.hpp"

#include <memory>
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
    virtual const ObjectAllocation* allocate(const Reflection::Class* component_class) = 0;
    virtual void                    free(const Reflection::Class* component_class, void* allocation) = 0;
};


class ContiguousObjectPool
{
    template <typename T>
    friend class TObjectIterator;

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
    void              free(void* ptr);

    void* nth(size_t i) const
    {
        return static_cast<uint8_t*>(memory) + i * stride;
    }

private:
    void reserve(size_t desired_count);
    void resize(size_t new_count);
    void move_old_to_new_block(void* old, void* new_block);

    void*                    memory          = nullptr;
    size_t                   allocated_count = 0;
    size_t                   component_count = 0;
    const Reflection::Class* object_class;
    const size_t             stride;

    std::unordered_map<void*, ObjectAllocation*> allocation_map;
};

template <typename T> class TObjectIterator
{
public:
    TObjectIterator(const std::vector<ContiguousObjectPool*>& in_classes) : classes(in_classes)
    {
        class_iterator  = classes.begin();
        if (class_iterator != classes.end())
            this_pool_count = (*class_iterator)->component_count;
    }

    T& operator*() const
    {
        return *static_cast<T*>((*class_iterator)->nth(index));
    }

    T* operator->()
    {
        return static_cast<T*>((*class_iterator)->nth(index));
    }

    TObjectIterator& operator++()
    {
        ++index;
        if (index >= this_pool_count)
        {
            ++class_iterator;
            if (class_iterator != classes.end())
                this_pool_count = (*class_iterator)->component_count;
            index = 0;
        }
        return *this;
    }

    operator bool() const
    {
        return class_iterator != classes.end();
    }

private:
    size_t                                             index           = 0;
    size_t                                             this_pool_count = 0;
    std::vector<ContiguousObjectPool*>::const_iterator class_iterator;
    std::vector<ContiguousObjectPool*>                 classes;
};

class ContiguousObjectAllocator : public ObjectAllocator
{
public:
    ObjectAllocation* allocate(const Reflection::Class* component_class) override;
    void              free(const Reflection::Class* component_class, void* allocation) override;

    template <typename T, typename... Args> TObjectPtr<T> construct(Args&&... args)
    {
        ObjectAllocation* allocation = allocate(T::static_class());
        new(allocation->ptr) T(std::forward<Args>(args)...);
        return TObjectPtr<T>(allocation);
    }

    template <typename T>
    TObjectIterator<T> iter() const
    {
        return TObjectIterator<T>(find_pools(T::static_class()));
    }

private:
    std::vector<ContiguousObjectPool*> find_pools(const Reflection::Class* parent_class) const;

    std::unordered_map<const Reflection::Class*, std::unique_ptr<ContiguousObjectPool>> pools;
};