#pragma once

#include "class.hpp"

#include <cassert>
#include <type_traits>

#include <shared_mutex>

class IObject;

namespace Eng
{
class SceneComponent;
}

namespace Reflection
{
class Class;
}

class IObjectDestructor
{
public:
    virtual      ~IObjectDestructor() = default;
    virtual void destroy() = 0;
};

template <typename T> class TObjectDestructor final : public IObjectDestructor
{
public:
    TObjectDestructor(struct ObjectAllocation* in_allocation) : allocation(in_allocation)
    {
    }

    void destroy() override;

    ObjectAllocation* allocation;
};

struct ObjectAllocation final
{
    size_t                   ptr_count    = 0;
    size_t                   ref_count    = 0;
    void*                    ptr          = nullptr;
    class ObjectAllocator*   allocator    = nullptr;
    const Reflection::Class* object_class = nullptr;
    IObjectDestructor*       destructor   = nullptr;


    ~ObjectAllocation()
    {
        delete destructor;
    }
};

template <typename T>
void TObjectDestructor<T>::destroy()
{
    assert(allocation);
    static_cast<T*>(allocation->ptr)->~T();
    allocation = nullptr;
}

class IObject
{
    friend class ContiguousObjectPool;

    template <typename V> friend class TObjectPtr;
    template <typename V> friend class TObjectRef;

public:
    operator bool() const
    {
        return allocation && allocation->ptr;
    }

    IObject()          = default;
    virtual ~IObject() = default;

    void destroy();

private:
    void free()
    {
        delete allocation;
        allocation = nullptr;
    }

protected:
    ObjectAllocation* allocation = nullptr;
};

template <typename T> struct std::hash<TObjectPtr<T>>;

template <typename T> class TObjectPtr final : public IObject
{
    template <typename V> friend class TObjectPtr;
    template <typename V> friend class TObjectRef;
    friend struct std::hash<TObjectPtr>;

    /*
     * Act like this container was destroyed.
     */
    void destructor_ptr()
    {
        if (*this)
        {
            assert(allocation->ptr_count > 0);
            if (allocation->ptr_count == 1)
            {
                destroy();
                allocation->ptr_count--;
            }
            else
            {
                allocation->ptr_count--;
                allocation = nullptr;
            }
        }
    }

    /**
     * Initialize or replace with other allocation (other could be null)
     */
    void assign_from(ObjectAllocation* other)
    {
        if (other && other->ptr)
        {
            // Other is totally valid, we just needs to increment the ref count
            allocation = other;
            ++allocation->ptr_count;
            if (!allocation->destructor)
                allocation->destructor = new TObjectDestructor<T>(allocation);
        }
        else if (*this)
        {
            // Replace this with an empty allocation (the other ptr was destroyed or other allocation is null)
            destructor_ptr();
        }
        // else this is null and stays null
    }

    void move_from(IObject& other)
    {
        if (other.allocation && other.allocation->ptr)
        {
            // Other is totally valid, we just needs to increment the ref count
            allocation = other.allocation;
            if (!allocation->destructor)
                allocation->destructor = new TObjectDestructor<T>(allocation);
        }
        else if (*this)
        {
            // Replace this with an empty allocation (the other ptr was destroyed or other allocation is null)
            destructor_ptr();
        }
        other.allocation = nullptr;
        // else this is null and stays null
    }

  public:
    TObjectPtr() = default;

    ~TObjectPtr() override
    {
        destructor_ptr();
    }

    /**
     * INIT OPERATORS
     */

    explicit TObjectPtr(T* in_object)
    {
        if (in_object)
        {
            allocation             = new ObjectAllocation{.ptr_count = 1, .ref_count = 0, .ptr = in_object, .allocator = nullptr, .object_class = nullptr};
            allocation->destructor = new TObjectDestructor<T>(allocation);
        }
    }

    explicit TObjectPtr(ObjectAllocation* in_allocation)
    {
        assert(in_allocation->ptr_count == 0);
        assign_from(in_allocation);
    }

    /**
     * COPY OPERATORS
     */

    TObjectPtr(const TObjectPtr& other)
    {
        assign_from(other.allocation);
    }

    template <typename V> TObjectPtr(const TObjectPtr<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        assign_from(other.allocation);
    }

    TObjectPtr& operator=(const TObjectPtr& other)
    {
        assign_from(other.allocation);
        return *this;
    }

    template <typename V> TObjectPtr& operator=(const TObjectPtr<V>& other)
    {
        assign_from(other.allocation);
        return *this;
    }

    /**
     * MOVE OPERATORS
     */

    TObjectPtr(TObjectPtr&& other) noexcept
    {
        move_from(other);
    }

    template <typename V> TObjectPtr(TObjectPtr<V>&& other) noexcept
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        move_from(other);
    }

    /**
     * OTHER OPERATORS
     */

    template <typename V> bool operator==(const TObjectPtr<V>& other) const
    {
        if (!allocation)
            return !other.allocation;
        return allocation == other.allocation && allocation->ptr == other.allocation->ptr;
    }

    template <typename V> bool operator==(const TObjectRef<V>& other) const
    {
        if (!allocation)
            return !other.allocation;
        return allocation == other.allocation && allocation->ptr == other.allocation->ptr;
    }

    T* operator->() const
    {
        assert(*this);
        return static_cast<T*>(allocation->ptr);
    }

    template <typename V> TObjectRef<V> cast() const
    {
        static_assert(Reflection::StaticClassInfos<T>::name, "Cast of non reflected object is not allowed");
        static_assert(Reflection::StaticClassInfos<V>::name, "Cast of non reflected object is not allowed");

        if (*this && static_cast<T*>(allocation->ptr)->cast<V>())
            return TObjectRef<V>(allocation);

        return TObjectRef<V>();
    }
};

template <typename T, typename... Args> TObjectPtr<T> make_object_ptr(Args&&... args)
{
    return TObjectPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T> struct std::hash<TObjectRef<T>>;

template <typename T> class TObjectRef final : public IObject
{
    template <typename V> friend class TObjectPtr;
    template <typename V> friend class TObjectRef;
    friend struct std::hash<TObjectRef>;

    void destructor_ref()
    {
        if (*this)
        {
            assert(allocation->ref_count > 0);
            allocation->ref_count--;
            if (allocation->ref_count == 0 && allocation->ptr_count == 0)
                free();
            else
                allocation = nullptr;
        }
    }

    void assign_from(ObjectAllocation* other)
    {
        if (other && other->ptr)
        {
            // Other is totally valid, we just needs to increment the ref count
            allocation = other;
            ++allocation->ref_count;
            assert(allocation->ptr_count > 0);
        }
        else if (*this)
        {
            // Replace this with an empty allocation (the other ptr was destroyed or other allocation is null)
            destructor_ref();
        }
        // else this is null and stays null
    }

    void move_from(IObject& other)
    {
        if (other.allocation && other.allocation->ptr)
        {
            // Other is totally valid, we just needs to increment the ref count
            allocation = other.allocation;
            assert(allocation->ptr_count > 0);
        }
        else if (*this)
        {
            // Replace this with an empty allocation (the other ptr was destroyed or other allocation is null)
            destructor_ref();
        }
        other.allocation = nullptr;
        // else this is null and stays null
    }

  public:
    /**
     * INIT OPERATORS
     */

    TObjectRef() = default;

    ~TObjectRef() override
    {
        destructor_ref();
    }

    explicit TObjectRef(ObjectAllocation* in_allocation)
    {
        assert(in_allocation->ptr && in_allocation->ptr_count > 0);
        allocation = in_allocation;
        allocation->ref_count++;
    }

    /**
     * COPY OPERATORS
     */

    TObjectRef(const TObjectRef& in_object)
    {
        assign_from(in_object.allocation);
    }

    template <typename V> TObjectRef(const TObjectPtr<V>& in_object)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        assign_from(in_object.allocation);
    }

    TObjectRef& operator=(const TObjectRef& other)
    {
        assign_from(other.allocation);
        return *this;
    }

    TObjectRef& operator=(const TObjectPtr<T>& other)
    {
        assign_from(other.allocation);
        return *this;
    }

    template <typename V> TObjectRef& operator=(const TObjectRef<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        assign_from(other.allocation);
        return *this;
    }

    template <typename V> TObjectRef& operator=(const TObjectPtr<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        assign_from(other.allocation);
        return *this;
    }

    /**
     * MOVE OPERATORS
     */

    TObjectRef(TObjectRef&& in_object) noexcept
    {
        move_from(in_object);
    }

    template <typename V> TObjectRef(TObjectRef<V>&& in_object) noexcept
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        move_from(in_object);
    }

    /**
     * OTHER OPERATORS
     */

    template <typename V> bool operator==(const TObjectPtr<V>& other) const
    {
        if (!allocation)
            return !other.allocation;
        return allocation == other.allocation && allocation->ptr == other.allocation->ptr;
    }

    template <typename V> bool operator==(const TObjectRef<V>& other) const
    {
        if (!allocation)
            return !other.allocation;
        return allocation == other.allocation && allocation->ptr == other.allocation->ptr;
    }


    template <typename V> TObjectRef<V> cast() const
    {
        static_assert(Reflection::StaticClassInfos<T>::name, "Cast of non reflected object is not allowed");
        static_assert(Reflection::StaticClassInfos<V>::name, "Cast of non reflected object is not allowed");

        if (*this && static_cast<T*>(allocation->ptr)->cast<V>())
            return TObjectRef<V>(allocation);
        return TObjectRef<V>();
    }

    T* operator->() const
    {
        assert(*this);
        return static_cast<T*>(allocation->ptr);
    }
};

template <typename T> struct std::hash<TObjectPtr<T>>
{
    size_t operator()(const TObjectPtr<T>& val) const noexcept
    {
        if (!val)
            return 0;
        return std::hash<void*>()(val.allocation);
    }
};

template <typename T> struct std::hash<TObjectRef<T>>
{
    size_t operator()(const TObjectRef<T>& val) const noexcept
    {
        if (!val)
            return 0;
        return std::hash<void*>()(val.allocation);
    }
};