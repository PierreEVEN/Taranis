#pragma once
#include "type.hpp"

#include <optional>
#include <string>

namespace Reflection
{
class Type;

class TypeInstance
{
public:
    TypeInstance(const Type* in_base, size_t in_size) : size(in_size), base_type(in_base)
    {
    }

    TypeInstance& set_const()
    {
        is_const = true;
        return *this;
    }

    TypeInstance& set_ref()
    {
        is_ref = true;
        return *this;
    }

    TypeInstance& set_ptr_indirections(uint8_t indirections)
    {
        ptr_indirections = indirections;
        return *this;
    }

    TypeInstance& set_template_specialization(const TypeSpecializationDescription& specialization)
    {
        template_specialization = specialization;
        return *this;
    }

    size_t stride() const
    {
        return size;
    }

    const Type* base() const
    {
        return base_type;
    }

    std::string display() const
    {
        std::string text;
        if (is_const)
            text += "const ";
        text += base_type->name();

        if (template_specialization)
        {
            text += '<';
            auto it = template_specialization->get_types().begin();
            while (it != template_specialization->get_types().end())
            {
                text += Type::get_type(*it)->name();
                ++it;
                if (it != template_specialization->get_types().end())
                    text += ", ";
            }
            text += '>';
        }

        for (uint8_t i = 0; i < ptr_indirections; ++i)
            text += '*';
        if (is_ref)
            text += '&';
        return text;
    }

private:
    bool                                         is_const         = false;
    bool                                         is_ref           = false;
    uint8_t                                      ptr_indirections = 0;
    std::optional<TypeSpecializationDescription> template_specialization;
    size_t                                       size      = 0;
    const Type*                                  base_type = nullptr;
};

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