#pragma once

#include "class.hpp"

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

#define REFL_DECLARE_TYPENAME(Type)                       \
    template <> struct Reflection::StaticClassInfos<Type> \
    {                                                     \
        constexpr static bool        value = true;        \
        constexpr static const char* name  = #Type;       \
    };

#define REFL_DECLARE_CLASS(className)                                                                \
  public:                                                                                            \
    friend void                      CONCAT_MACRO_TWO_PARAMS(_Refl_Register_Function_, className)(); \
    friend void                      _Refl_Register_Class();                                         \
    static const Reflection::Class*  static_class();                                                 \
    virtual const Reflection::Class* get_class() const;                                              \
    template <typename T> T*         cast()                                                          \
    {                                                                                                \
        if constexpr (Reflection::StaticClassInfos<T>::value)                                        \
            return reinterpret_cast<T*>(get_class()->cast_to(T::static_class(), this));              \
        else                                                                                         \
            return nullptr;                                                                          \
    }

#define REFL_REGISTER_CLASS(ClassName) Reflection::Class::RegisterClass<ClassName>(#ClassName);

#define REFLECT(...)
#define RPROPERTY(...)
#define RCONSTRUCTOR(...)
#define RFUNCTION(...)
#define REFLECT_BODY() CONCAT_MACRO_FOUR_PARAMS(_REFLECTION_BODY_, _REFL_FILE_UNIQUE_ID_, _LINE_, __LINE__)