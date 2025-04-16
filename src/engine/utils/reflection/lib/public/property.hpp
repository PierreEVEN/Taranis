#pragma once
#include "type.hpp"

#include <string>

namespace Reflection
{
class Type;

class Property
{
public:
    Property(std::string in_name, size_t in_offset, TypeInstance in_type) : name(std::move(in_name)), offset(in_offset), type(std::move(in_type))
    {
    }

    const char* get_name() const
    {
        return name.c_str();
    }

    const TypeInstance& get_type_instance() const
    {
        return type;
    }

    size_t get_offset() const
    {
        return offset;
    }

    template <typename T = void> T* ptr(void* object_ptr) const
    {
        return reinterpret_cast<T*>(reinterpret_cast<size_t>(object_ptr) + offset);
    }

    template <typename T = void> void write(void* object_ptr, T* valuePtr) const
    {
        T* ptr = reinterpret_cast<T*>(reinterpret_cast<size_t>(object_ptr) + offset);
        *ptr   = *valuePtr;
    }

private:
    const std::string  name;
    const size_t       offset = 0;
    const TypeInstance type;
};
} // namespace Reflection