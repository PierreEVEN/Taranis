#pragma once
#include "../../../build/reflection/core/public/assets/asset_base.gen.hpp"

#include <cassert>
#include <iostream>
#include <type_traits>

namespace Reflection
{
class Class;
}

struct ObjectAllocation
{
    size_t                   ptr_count    = 0;
    size_t                   ref_count    = 0;
    void*                    ptr          = nullptr;
    class ObjectAllocator*   allocator    = nullptr;
    const Reflection::Class* object_class = nullptr;
};

class IObject
{
    friend class ContiguousObjectPool;

public:
    operator bool() const
    {
        return allocation && allocation->ptr;
    }

    IObject()          = default;
    virtual ~IObject() = default;

    void destroy();

protected:
    void destructor_ptr()
    {
        if (*this)
        {
            assert(allocation->ptr_count > 0);
            allocation->ptr_count--;
            if (allocation->ptr_count == 0)
                destroy();
            else
                allocation = nullptr;
        }
    }

    void destructor_ref()
    {
        if (*this)
        {
            assert(allocation->ptr_count > 0);
            allocation->ref_count--;
            if (allocation->ref_count == 0 && allocation->ptr_count == 0)
                free();
            else
                allocation = nullptr;
        }
    }

    virtual void internal_delete_ptr() = 0;

    ObjectAllocation* allocation = nullptr;

private:
    void free()
    {
        delete allocation;
        allocation = nullptr;
    }
};


template <typename T> class TObjectPtr final : public IObject
{
    template <typename V> friend class TObjectPtr;
    template <typename V> friend class TObjectRef;

public:
    TObjectPtr() = default;

    explicit TObjectPtr(T* in_object)
    {
        if (in_object)
            allocation = new ObjectAllocation{.ptr_count = 1, .ref_count = 0, .ptr = in_object, .allocator = nullptr, .object_class = nullptr};
    }

    explicit TObjectPtr(ObjectAllocation* in_allocation)
    {
        allocation            = in_allocation;
        allocation->ptr_count = 1;
    }

    TObjectPtr(const TObjectPtr& other)
    {
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
    }

    TObjectPtr(const TObjectPtr&& other) noexcept
    {
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
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

    template <typename V> TObjectPtr(const TObjectPtr<V>&& other) noexcept
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
    }

    template <typename V> TObjectRef<V> cast() const
    {
        static_assert(Reflection::StaticClassInfos<T>::name, "Cast of non reflected object is not allowed");
        static_assert(Reflection::StaticClassInfos<V>::name, "Cast of non reflected object is not allowed");

        if (*this && static_cast<T*>(allocation->ptr)->cast<V>())
        {
            TObjectRef<V> other;
            other.allocation = allocation;
            ++allocation->ptr_count;
            return other;
        }

        return TObjectRef<V>();
    }


    ~TObjectPtr() override
    {
        destructor_ptr();
    }

    TObjectPtr& operator=(const TObjectPtr<T>& other)
    {
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
        return *this;
    }

    template <typename V> TObjectPtr& operator=(const TObjectPtr<V>& other)
    {
        if (other)
        {
            allocation = other.allocation;
            ++allocation->ptr_count;
        }
        return *this;
    }

    T* operator->() const
    {
        return static_cast<T*>(allocation->ptr);
    }

protected:
    void internal_delete_ptr() override
    {
        static_cast<T*>(allocation->ptr)->~T();
    }
};

template <typename T, typename... Args> TObjectPtr<T> make_object_ptr(Args&&... args)
{
    return TObjectPtr<T>(new T(std::forward<Args>(args)...));
}


template <typename T> class TObjectRef final : public IObject
{
public:
    TObjectRef() = default;

    TObjectRef(const TObjectRef&& in_object) noexcept
    {
        if (in_object)
        {
            allocation = in_object.allocation;
            ++allocation->ref_count;
        }
    }

    template <typename V> TObjectRef(const TObjectPtr<V>&& in_object) noexcept
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (in_object)
        {
            allocation = in_object.allocation;
            ++allocation->ref_count;
        }
    }

    TObjectRef(const TObjectRef& in_object)
    {
        if (in_object)
        {
            allocation = in_object.allocation;
            ++allocation->ref_count;
        }
    }

    template <typename V> TObjectRef<V> cast() const
    {
        static_assert(Reflection::StaticClassInfos<T>::name, "Cast of non reflected object is not allowed");
        static_assert(Reflection::StaticClassInfos<V>::name, "Cast of non reflected object is not allowed");

        if (*this && static_cast<T*>(allocation->ptr)->cast<V>())
        {
            TObjectRef<V> other;
            other.allocation = allocation;
            ++allocation->ptr_count;
            return other;
        }

        return TObjectRef<V>();
    }

    template <typename V> TObjectRef(const TObjectPtr<V>& in_object)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (in_object)
        {
            allocation = in_object.allocation;
            ++allocation->ref_count;
        }
    }

    ~TObjectRef() override
    {
        destructor_ref();
    }

    TObjectRef& operator=(const TObjectRef<T>& other)
    {
        if (other)
        {
            allocation = other.allocation;
            allocation->ref_count++;
        }
        return *this;
    }

    TObjectRef& operator=(const TObjectPtr<T>& other)
    {
        if (other)
        {
            allocation = other.allocation;
            allocation->ref_count++;
        }
        return *this;
    }

    template <typename V> TObjectRef& operator=(const TObjectRef<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (other)
        {
            allocation = other.allocation;
            allocation->ref_count++;
        }
        return *this;
    }

    template <typename V> TObjectRef& operator=(const TObjectPtr<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (other)
        {
            allocation = other.allocation;
            allocation->ref_count++;
        }
        return *this;
    }

    T* operator->() const
    {
        return static_cast<T*>(allocation->ptr);
    }

protected:
    void internal_delete_ptr() override
    {
        static_cast<T*>(allocation->ptr)->~T();
    }
};