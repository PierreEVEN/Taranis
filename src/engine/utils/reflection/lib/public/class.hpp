#pragma once
#include "type.hpp"
#include <iostream>

namespace Reflection
{
class Property;

class Class : public Type
{

public:
    static Class* get(const char* type_name);
    static Class* get(const TypeId& type_id);

    template <typename C> static const Class* get()
    {
        static_assert(StaticTypeInfos<C>::value, "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
        return get(StaticTypeInfos<C>::name);
    }

    template <typename ClassName> static Class* register_class()
    {
        static_assert(StaticTypeInfos<ClassName>::value, "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
        Class* new_class = new Class(StaticTypeInfos<ClassName>::name, sizeof(ClassName));
        register_class_internal(new_class);
        register_type_internal(new_class);
        return new_class;
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
        if constexpr (StaticTypeInfos<ParentClass>::value)
        {
            cast_functions.insert_or_assign(Type::make_type_id<ParentClass>(),
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
            if (auto cast_fn = cast_functions.find(parent->id()); cast_fn != cast_functions.end())
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
            if (auto cast_fn = cast_functions.find(parent->id()); cast_fn != cast_functions.end())
                if (const void* ToPtr = cast_fn->second.const_fn(To, Ptr))
                    return ToPtr;
        }
        return nullptr;
    }

    void add_parent(const TypeId& parent);

    void register_property(const std::string& name, TypeId type_id, size_t offset);

    template <typename Base, typename T> static bool is_base_of()
    {
        return Class::is_base_of(Base::static_class(), T::static_class());
    }

    bool is_base_of(const Class* other) const
    {
        return is_base_of(this, other);
    }


    static ankerl::unordered_dense::map<TypeId, Class*>& get_classes()
    {
        return get_classes();
    }

private:
    static bool is_base_of(const Class* base, const Class* t);

    void on_register_parent_class(Class* new_class);

    Class(std::string in_type_name, size_t in_type_size) : Type(std::move(in_type_name), in_type_size)
    {
    }

    static void register_class_internal(Class* inClass);

    std::vector<Class*>                                   parents = {};
    std::unordered_map<std::string, Property*>            properties;
    ankerl::unordered_dense::map<TypeId, CastFuncWrapper> cast_functions;

    static ankerl::unordered_dense::map<TypeId, std::vector<Class*>>& get_class_waiting_type_registration();
    static ankerl::unordered_dense::map<TypeId, Class*>&              get_classes_internal();
    static ankerl::unordered_dense::map<TypeId, Class*>*              classes;
    static ankerl::unordered_dense::map<TypeId, std::vector<Class*>>* class_waiting_parent_registration;

    struct PropertyWaitingTypeRegistration
    {
        Class*      owning_class = nullptr;
        std::string name;
        size_t      offset = 0;
    };

    static ankerl::unordered_dense::map<TypeId, std::vector<PropertyWaitingTypeRegistration>>& get_properties_waiting_type_registration();
    static ankerl::unordered_dense::map<TypeId, std::vector<PropertyWaitingTypeRegistration>>* properties_waiting_type_registration;
};
} // namespace Reflection