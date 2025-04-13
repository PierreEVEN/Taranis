#pragma once
#include <string>

namespace Reflection
{

class Type;

class Property
{
public:
    Property(std::string in_name, const Type* in_type, const size_t in_offset) : name(std::move(in_name)), type(in_type), offset(in_offset)
    {
    }

    const char* get_name() const
    {
        return name.c_str();
    }

    const Type* get_type() const
    {
        return type;
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
    const std::string name;
    const Type*       type   = nullptr;
    const size_t      offset = 0;
};
} // namespace Reflection