#pragma once
#include <string>

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
        RegisterClass_Internal(new_class);
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

  private:
    Class(std::string in_type_name, size_t in_type_size) : type_size(in_type_size), type_name(std::move(in_type_name)), type_id(std::hash<std::string>{}(type_name))
    {
        
    }

    static void RegisterClass_Internal(Class* inClass);

    Class* parent = nullptr;

    size_t      type_size = 0;
    std::string type_name;
    size_t      type_id = 0;
};
}

#define CONCAT_MACRO_TWO_PARAMS_(x, y)             x##y
#define CONCAT_MACRO_TWO_PARAMS(x, y)              CONCAT_MACRO_TWO_PARAMS_(x, y)
#define CONCAT_MACRO_THREE_PARAMS_(x, y, z)        x##y##z
#define CONCAT_MACRO_THREE_PARAMS(x, y, z)         CONCAT_MACRO_THREE_PARAMS_(x, y, z)
#define CONCAT_MACRO_FOUR_PARAMS_(w, x, y, z)      w##x##y##z
#define CONCAT_MACRO_FOUR_PARAMS(w, x, y, z)       CONCAT_MACRO_FOUR_PARAMS_(w, x, y, z)
#define CONCAT_MACRO_FIVE_PARAMS_(v, w, x, y, z)   v##w##x##y##z
#define CONCAT_MACRO_FIVE_PARAMS(v, w, x, y, z)    CONCAT_MACRO_FIVE_PARAMS_(v, w, x, y, z)
#define CONCAT_MACRO_SIX_PARAMS_(u, v, w, x, y, z) v##w##x##y##z
#define CONCAT_MACRO_SIX_PARAMS(u, v, w, x, y, z)  CONCAT_MACRO_SIX_PARAMS_(u, v, w, x, y, z)


#define REFL_DECLARE_TYPENAME(Type)                \
    template <> struct Reflection::StaticClassInfos<Type>          \
    {                                              \
        constexpr static bool value = true;        \
        constexpr static const char* name  = #Type; \
    };

#define REFL_DECLARE_CLASS(className)                                           \
  public:                                                                       \
    friend void     CONCAT_MACRO_TWO_PARAMS(_Refl_Register_Item_, className)(); \
    friend void     _Refl_Register_Class();                                     \
    static const Reflection::Class*  static_class();                                           \
    virtual const Reflection::Class* get_class() const

#define REFL_REGISTER_CLASS(ClassName) Reflection::Class::RegisterClass<ClassName>(#ClassName);

#define REFLECT(...)
#define RPROPERTY(...)
#define RCONSTRUCTOR(...)
#define RFUNCTION(...)
#define REFLECT_BODY() CONCAT_MACRO_FOUR_PARAMS(_REFLECTION_BODY_, _REFL_FILE_UNIQUE_ID_, _LINE_, __LINE__)