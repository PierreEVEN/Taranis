#pragma once
#include "property.hpp"
#include "type.hpp"
#include <iostream>

namespace Reflection {
    class TypeInstance;
}

namespace Reflection {
    class Class : public Type {
    public:
        static Class *get(const TypeId &type_id);

        template<typename C>
        static Class *get() {
            static_assert(StaticTypeInfos<C>::value,
                          "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
            return get(TypeId::create<C>());
        }

        template<typename ClassName>
        static Class *register_class() {
            static_assert(StaticTypeInfos<ClassName>::value,
                          "Failed to register class : not a reflected class. Please add the REFLECT_BODY macro to it.");
            Class *new_class = new Class(TypeId::create<ClassName>(), sizeof(ClassName));

            if constexpr (std::is_default_constructible<ClassName>::value) {
                new_class->construct_at = [](void *ptr) {
                    return new(ptr) ClassName();
                };
            }

            new_class->delete_at = [](void *ptr) {
                static_cast<ClassName *>(ptr)->~ClassName();
            };

            register_class_internal(new_class);
            register_type_internal(new_class);
            return new_class;
        }

        using CastFunc = void*(*)(const Class *, void *);
        using CastFuncConst = const void*(*)(const Class *, const void *);

        struct CastFuncWrapper {
            CastFunc fn;
            CastFuncConst const_fn;
        };

        bool placement_new(void *target) const {
            return construct_at && construct_at(target);
        }

        template<typename T = void>
        T *instantiate() const {
            if (!construct_at)
                return nullptr;
            void *memory = calloc(1, stride());
            if (!construct_at(memory)) {
                free(memory);
                return nullptr;
            }
            return static_cast<T *>(memory);
        }

        void placement_delete(void *alloc) const {
            if (!alloc)
                return;
            assert(delete_at);
            delete_at(alloc);
        }

        void delete_instance(void *alloc) const {
            if (!alloc)
                return;
            assert(delete_at);
            delete_at(alloc);
            free(alloc);
        }

        /**
         * Add function that FromPtr from ThisClass to ParentClass
         */
        template<typename ThisClass, typename ParentClass>
        void add_cast_function() {
            if constexpr (StaticTypeInfos<ParentClass>::is_class) {
                cast_functions.insert_or_assign(TypeId::create<ParentClass>(),
                                                CastFuncWrapper{
                                                    [](const Class *desired_class, void *from_ptr) -> void * {
                                                        return ParentClass::static_class()->cast_to(
                                                            desired_class,
                                                            reinterpret_cast<void *>(static_cast<ParentClass *>(
                                                                static_cast<ThisClass *>(from_ptr))));
                                                    },
                                                    [](const Class *desired_class,
                                                       const void *from_ptr) -> const void * {
                                                        return ParentClass::static_class()->cast_to_const(
                                                            desired_class,
                                                            reinterpret_cast<const void *>(static_cast<const ParentClass
                                                                *>(static_cast<const ThisClass *>(from_ptr))));
                                                    }
                                                });
            }
        }

        /**
         * Cast Ptr to To Object
         * if ThisClass == To, return Ptr, else try to cast to one of the parent class
         */
        void *cast_to(const Class *To, void *Ptr) const {
            if (To == this)
                return Ptr;

            for (const auto &parent: parents) {
                if (auto cast_fn = cast_functions.find(parent->id()); cast_fn != cast_functions.end()) {
                    if (void *ToPtr = cast_fn->second.fn(To, Ptr))
                        return ToPtr;
                }
            }
            return nullptr;
        }

        const void *cast_to_const(const Class *To, const void *Ptr) const {
            if (To == this)
                return Ptr;

            for (const auto &parent: parents) {
                if (auto cast_fn = cast_functions.find(parent->id()); cast_fn != cast_functions.end()) {
                    if (const void *ToPtr = cast_fn->second.const_fn(To, Ptr))
                        return ToPtr;
                }
            }
            return nullptr;
        }

        void add_parent(const TypeId &parent);

        void register_property(const std::string &name, size_t offset, const TypeInstance &type);

        template<typename Base, typename T>
        static bool is_base_of() {
            return Class::is_base_of(Base::static_class(), T::static_class());
        }

        bool is_base_of(const Class *other) const {
            return is_base_of(this, other);
        }


        static ankerl::unordered_dense::map<TypeId, Class *> &get_classes() {
            return get_classes_internal();
        }

        const ankerl::unordered_dense::map<std::string, Property> &get_properties() const {
            return properties;
        }

        const std::vector<Class*>& get_parents() const {
            return parents;
        };

    private:
        static bool is_base_of(const Class *base, const Class *t);

        void on_register_parent_class(Class *new_class);

        Class(TypeId type_id, uint32_t in_type_size) : Type(type_id, in_type_size) {
        }

        static void register_class_internal(Class *inClass);

        std::function<void*(void *)> construct_at = nullptr;
        std::function<void(void *)> delete_at = nullptr;
        std::vector<Class *> parents = {};
        ankerl::unordered_dense::map<std::string, Property> properties;
        ankerl::unordered_dense::map<TypeId, CastFuncWrapper> cast_functions;


        static ankerl::unordered_dense::map<TypeId, std::vector<Class *> > &get_class_waiting_type_registration();

        static ankerl::unordered_dense::map<TypeId, Class *> &get_classes_internal();

        static ankerl::unordered_dense::map<TypeId, Class *> *classes;
        static ankerl::unordered_dense::map<TypeId, std::vector<Class *> > *class_waiting_parent_registration;

        struct PropertyWaitingTypeRegistration {
            Class *owning_class = nullptr;
            std::string name;
            size_t offset = 0;
        };
    };
} // namespace Reflection
