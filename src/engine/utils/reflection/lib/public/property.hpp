#pragma once
#include <string>

namespace Reflection
{

class Type;

class Property
{
public:
    Property(std::string in_name, const Type* in_type, const size_t in_offset, const bool in_is_const, const bool in_is_ref, const uint8_t in_ptr_indirections) : property_is_const(in_is_const), property_is_ref(in_is_ref), property_ptr_indirections(in_ptr_indirections), name(std::move(in_name)), type(in_type), offset(in_offset)
    {
    }

    std::string display_type() const
    {
        std::string text;
        if (property_is_const)
            text += "const ";
        text += type->name();
        for (uint8_t i = 0; i < property_ptr_indirections; ++i)
            text += '*';
        if (property_is_ref)
            text += '&';
        return text;
    }

    const char* get_name() const
    {
        return name.c_str();
    }

    const Type* get_type() const
    {
        return type;
    }

    bool is_const() const
    {
        return property_is_const;
    }

    bool is_ref() const
    {
        return property_is_ref;
    }

    uint8_t ptr_indirections() const
    {
        return property_ptr_indirections;
    }

    const size_t get_offset() const
    {
        return offset;
    }

    template <typename T = void> T* read(void* object_ptr)
    {
        return reinterpret_cast<T*>(reinterpret_cast<size_t>(object_ptr) + offset);
    }

    template <typename T = void> void write(void* object_ptr, T* valuePtr)
    {
        T* ptr = reinterpret_cast<T*>(reinterpret_cast<size_t>(object_ptr) + offset);
        *ptr   = *valuePtr;
    }

private:
    const bool        property_is_const         = false;
    const bool        property_is_ref           = false;
    const uint8_t     property_ptr_indirections = 0;
    const std::string name;
    const Type*       type   = nullptr;
    const size_t      offset = 0;
};
} // namespace Reflection