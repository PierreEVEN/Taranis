#pragma once
#include <cassert>

class IObject
{
    friend class ContiguousObjectPool;

    operator bool() const
    {
        return allocation && allocation->ptr;
    }

protected:
    void decrement_ptr()
    {
        if (*this)
        {
            assert(allocation->ptr_count > 0);
            allocation->ptr_count--;
            if (allocation->dereferenced())
            {

            }
        }
    }

    void decrement_ref()
    {
        if (*this)
        {
        }
    }

    virtual void internal_delete_ptr() = 0;

    ObjectAllocation* allocation = nullptr;
};

template <typename T> class TObjectPtr : public IObject
{
    template <typename V> friend class TObjectPtr;

public:
    TObjectPtr() = default;

    explicit TObjectPtr(T* in_object)
    {
        if (in_object)
            allocation = new ObjectAllocation{.ptr_count = 1, .ref_count = 0, .ptr = in_object, .allocator = nullptr, .object_class = nullptr};
    }

    explicit TObjectPtr(ObjectAllocation* in_allocation)
    {
        allocation = in_allocation;
    }

    template <typename V>
    TObjectPtr(const TObjectPtr<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
    }

    ~TObjectPtr()
    {
        if (allocation)
        {
            allocation->decrement_ptr();
            if (allocation->should_delete_internal_object())
            {
                static_cast<T*>(allocation->ptr)->~T();
                allocation->free();
            }
        }
    }


    T* operator->() const
    {
        return static_cast<T*>(allocation->ptr);
    }

    void destroy()
    {
        if (*this)
        {
        }
    }

private:
    template <typename V>
    friend class TObjectRef;
};

template <typename T, typename... Args> TObjectPtr<T> make_object_ptr(Args&&... args)
{
    return TObjectPtr<T>(new T(std::forward<Args>(args)...));
}


template <typename T> class TObjectRef : public IObject
{
public:
    TObjectRef() = default;

    TObjectRef(const TObjectRef& in_object)
    {
        if (in_object)
        {
            allocation = in_object.data;
            ++allocation->ref_count;
            object = in_object.object;
        }
    }

    template <typename V> TObjectRef(const TObjectPtr<V>& in_object)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (in_object)
        {
            allocation = in_object.data;
            ++allocation->ref_count;
            object = in_object.object;
        }
    }

    ~TObjectRef()
    {
        if (allocation)
        {
            assert(allocation->ref_count > 0);
            --allocation->ref_count;
            if (allocation->ref_count == 0 && allocation->ptr_count == 0)
            {
                delete allocation;
                allocation = nullptr;
            }
        }
    }

    operator bool() const
    {
        return object && allocation && allocation->b_valid;
    }

    T& operator*() const
    {
        return *object;
    }

    T* operator->() const
    {
        return object;
    }

    void destroy()
    {
        if (*this)
        {

            allocation->b_valid = false;
            --allocation->ref_count;
            if (allocation->ptr_count == 0 && allocation->ref_count == 0)
                delete allocation;
            delete object;
            object     = nullptr;
            allocation = nullptr;
        }
    }

private:
    T* object;
};