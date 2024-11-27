#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Reflection
{

template <typename RClass> struct StaticClassInfos
{
    constexpr static bool Reflected = false;
};

class Class
{

  public:
    static Class* get(const std::string& type_name);

    template <typename C> static const Class* get()
    {
        static_assert(StaticClassInfos<C>::value, "Failed to get class : not a reflected class. Please declare this class as a reflected class.");
        return get(StaticClassInfos<C>::name);
    }

    template <typename ClassName> static Class* RegisterClass(const std::string& inClassName)
    {
        static_assert(StaticClassInfos<ClassName>::value, "Failed to register class : not a reflected class. Please declare this class as a reflected class.");
        Class* new_class = new Class(inClassName, sizeof(ClassName));
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

    using CastFunc = std::function<void*(const Class*, void*)>;

    /**
     * Add function that FromPtr from ThisClass to ParentClass
     */
    template <typename ThisClass, typename ParentClass> void add_cast_function()
    {
        if constexpr (StaticClassInfos<ParentClass>::value)
        {
            cast_functions.insert_or_assign(Class::make_type_id<ParentClass>(), CastFunc(
                                                                                    [](const Class* desired_class, void* from_ptr) -> void*
                                                                                    {
                                                                                        return ParentClass::static_class()->cast_to(desired_class,
                                                                                                                                    reinterpret_cast<void*>(static_cast<ParentClass*>(static_cast<ThisClass*>(from_ptr))));
                                                                                    }));
        }
    }

    /**
     * Cast Ptr to To Object
     * if ThisClass == To, return Ptr, else try to cast to one of the parent class
     */
    void* cast_to(const Class* To, void* Ptr) const
    {
        if (To == this)
            return Ptr;

        for (const auto& parent : parents)
        {
            if (const auto cast_fn = cast_functions.find(parent->get_id()); cast_fn != cast_functions.end())
                if (void* ToPtr = cast_fn->second(To, Ptr))
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

    Class(std::string in_type_name, size_t in_type_size) : type_size(in_type_size), type_name(std::move(in_type_name)), type_id(std::hash<std::string>{}(type_name))
    {
    }

    static void register_class_internal(Class* inClass);

    std::string                          type_name;
    std::vector<Class*>                  parents = {};
    std::unordered_map<size_t, CastFunc> cast_functions;

    size_t type_size = 0;
    size_t type_id   = 0;
};
} // namespace Reflection