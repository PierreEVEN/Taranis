#pragma once
#include <iostream>
#include <string>
#include <ankerl/unordered_dense.h>
#include <vector>

namespace Reflection
{

template <typename RClass> struct StaticClassInfos
{
    constexpr static bool value = false;
};

class Class
{

public:
    static Class* get(const std::string& type_name);

    template <typename C> static const Class* get()
    {
        static_assert(StaticClassInfos<C>::value, "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
        return get(StaticClassInfos<C>::name);
    }

    template <typename ClassName> static Class* RegisterClass(const std::string& in_class_name)
    {
        static_assert(StaticClassInfos<ClassName>::value, "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
        Class* new_class = new Class(in_class_name, sizeof(ClassName));
        register_class_internal(new_class);
        return new_class;
    }

    size_t get_id() const
    {
        return type_id;
    }

    const char* name() const
    {
        return type_name.c_str();
    }

    size_t stride() const
    {
        return type_size;
    }

    template <typename Type> static size_t make_type_id()
    {
        return std::hash<std::string>{}(StaticClassInfos<Type>::name);
    }

    using CastFunc      = void*(*)(const Class*, void*);
    using CastFuncConst = const void*(*)(const Class*, const void*);

    struct CastFuncWrapper
    {
        CastFunc      fn;
        CastFuncConst const_fn;
    };

    /**
     * Add function that FromPtr from ThisClass to ParentClass
     */
    template <typename ThisClass, typename ParentClass> void add_cast_function()
    {
        if constexpr (StaticClassInfos<ParentClass>::value)
        {
            cast_functions.insert_or_assign(Class::make_type_id<ParentClass>(),
                                            CastFuncWrapper{[](const Class* desired_class, void* from_ptr) -> void* {
                                                                return ParentClass::static_class()->cast_to(desired_class, reinterpret_cast<void*>(static_cast<ParentClass*>(static_cast<ThisClass*>(from_ptr))));
                                                            },
                                                            [](const Class* desired_class, const void* from_ptr) -> const void* {
                                                                return ParentClass::static_class()->cast_to_const(
                                                                    desired_class, reinterpret_cast<const void*>(static_cast<const ParentClass*>(static_cast<const ThisClass*>(from_ptr))));
                                                            }});
        }
    }

    /**
     * Cast Ptr to To Object
     * if ThisClass == To, return Ptr, else try to cast to none of the parent class
     */
    void* cast_to(const Class* To, void* Ptr) const
    {
        if (To == this)
            return Ptr;

        for (const auto& parent : parents)
        {
            if (auto cast_fn = cast_functions.find(parent->get_id()); cast_fn != cast_functions.end())
                if (void* ToPtr = cast_fn->second.fn(To, Ptr))
                    return ToPtr;
        }
        return nullptr;
    }

    const void* cast_to_const(const Class* To, const void* Ptr) const
    {
        if (To == this)
            return Ptr;

        for (const auto& parent : parents)
        {
            if (auto cast_fn = cast_functions.find(parent->get_id()); cast_fn != cast_functions.end())
                if (const void* ToPtr = cast_fn->second.const_fn(To, Ptr))
                    return ToPtr;
        }
        return nullptr;
    }

    void add_parent(const std::string& parent);

    template <typename Base, typename T> static bool is_base_of()
    {
        return Class::is_base_of(Base::static_class(), T::static_class());
    }

    bool is_base_of(const Class* other) const
    {
        return is_base_of(this, other);
    }

private:
    static bool is_base_of(const Class* base, const Class* t);

    void on_register_parent_class(Class* new_class);

    Class(std::string in_type_name, size_t in_type_size) : type_name(std::move(in_type_name)), type_size(in_type_size), type_id(std::hash<std::string>{}(type_name))
    {
    }

    static void register_class_internal(Class* inClass);

    std::string                                           type_name;
    std::vector<Class*>                                   parents = {};
    ankerl::unordered_dense::map<size_t, CastFuncWrapper> cast_functions;

    size_t type_size = 0;
    size_t type_id   = 0;
};
} // namespace Reflection