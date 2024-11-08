#pragma once
#include <cassert>
#include <memory>
#include <unordered_map>

class ObjectAllocator;
struct ObjectAllocation;
class IObjectPtr;

namespace Reflection
{
class Class;
}


struct ObjectAllocation
{
    size_t             ptr_count    = 0;
    size_t             ref_count    = 0;
    void*              ptr          = nullptr;
    ObjectAllocator*   allocator    = nullptr;
    Reflection::Class* object_class = nullptr;

    void decrement_ref()
    {
        assert(ref_count != 0);
        --ref_count;
    }

    void decrement_ptr()
    {
        assert(ptr_count != 0);
        --ptr_count;
    }

    bool dereferenced() const
    {
        return ptr_count == 0 && ref_count == 0;
    }

    void free();
};

class ObjectAllocator
{
public:
    virtual const ObjectAllocation* allocate(Reflection::Class* component_class) = 0;
    virtual void                    free(Reflection::Class* component_class, void* allocation) = 0;
};


class ContiguousObjectPool
{
public:
    ContiguousObjectPool(const Reflection::Class* in_object_class) : object_class(in_object_class)
    {
    }

    ContiguousObjectPool()                       = default;
    ContiguousObjectPool(ContiguousObjectPool&)  = delete;
    ContiguousObjectPool(ContiguousObjectPool&&) = delete;

    ~ContiguousObjectPool()
    {
        free(memory);
    }

    ObjectAllocation* allocate();
    void              free(void* ptr);

private:
    void reserve(size_t desired_count);
    void resize(size_t new_count);
    void move_old_to_new_block(void* old, void* new_block);

    void*                    memory          = nullptr;
    size_t                   allocated_count = 0;
    size_t                   component_count = 0;
    const Reflection::Class* object_class;

    std::unordered_map<void*, ObjectAllocation*> allocation_map;
};

class ContiguousObjectAllocator : public ObjectAllocator
{
public:
    ObjectAllocation* allocate(Reflection::Class* component_class) override;
    void              free(Reflection::Class* component_class, void* allocation) override;

private:
    std::unordered_map<Reflection::Class*, std::unique_ptr<ContiguousObjectPool>> pools;
};