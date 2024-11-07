#pragma once
#include <cassert>

struct _ObjectPtrData
{
    size_t ptr_count = 0;
    size_t ref_count = 0;
    bool   b_valid   = false;
};

template <typename T> class TObjectPtr
{
    template <typename V> friend class TObjectPtr;

public:
    TObjectPtr() = default;

    explicit TObjectPtr(T* in_object)
    {
        if (in_object)
        {
            object = std::move(in_object);
            data   = new _ObjectPtrData{.ptr_count = 1, .ref_count = 0, .b_valid = true};
        }
    }

    template <typename V>
    TObjectPtr(const TObjectPtr<V>& other)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (other)
        {
            object = other.object;
            data   = other.data;
            ++data->ptr_count;
        }
    }

    ~TObjectPtr()
    {
        if (data)
        {
            if (data->ptr_count > 0)
                --data->ptr_count;
            if (data->ptr_count == 0)
            {
                if (data->b_valid)
                    destroy();
                else if (data->ref_count == 0)
                {
                    delete data;
                    data = nullptr;
                }
            }
        }
    }

    operator bool() const
    {
        return object && data && data->b_valid;
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
            data->b_valid = false;
            --data->ptr_count;
            if (data->ptr_count == 0 && data->ref_count)
                delete data;
            delete object;
            object = nullptr;
            data   = nullptr;
        }
    }

private:
    template <typename V>
    friend class TObjectRef;

    _ObjectPtrData* data   = nullptr;
    T*              object = nullptr;
};

template <typename T, typename... Args> TObjectPtr<T> make_object_ptr(Args&&... args)
{
    return TObjectPtr<T>(new T(std::forward<Args>(args)...));
}


template <typename T> class TObjectRef
{
public:
    TObjectRef() = default;

    TObjectRef(const TObjectRef& in_object)
    {
        if (in_object)
        {
            data = in_object.data;
            ++data->ref_count;
            object = in_object.object;
        }
    }

    template <typename V> TObjectRef(const TObjectPtr<V>& in_object)
    {
        static_assert(std::is_base_of_v<T, V>, "Implicit cast of object ptr are only allowed with parent classes");
        if (in_object)
        {
            data = in_object.data;
            ++data->ref_count;
            object = in_object.object;
        }
    }

    ~TObjectRef()
    {
        if (data)
        {
            assert(data->ref_count > 0);
            --data->ref_count;
            if (data->ref_count == 0 && data->ptr_count == 0)
            {
                delete data;
                data = nullptr;
            }
        }
    }

    operator bool() const
    {
        return object && data && data->b_valid;
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

            data->b_valid = false;
            --data->ref_count;
            if (data->ptr_count == 0 && data->ref_count)
                delete data;
            delete object;
            object = nullptr;
            data   = nullptr;
        }
    }

private:
    _ObjectPtrData* data = nullptr;
    T*              object;
};