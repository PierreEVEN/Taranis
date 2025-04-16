#pragma once
#include "hash.hpp"
#include "type_id.hpp"

#include <initializer_list>
#include <vector>

namespace Reflection
{
class Type;

class TypeSpecializationDescription
{
public:
    TypeSpecializationDescription() = default;

    TypeSpecializationDescription(std::initializer_list<TypeId> in_types) : types(in_types)
    {
    }

    void push(const TypeId& type)
    {
        types.emplace_back(type);
    }

    bool operator==(const TypeSpecializationDescription& other) const
    {
        auto ita = other.types.begin();
        auto itb = types.begin();
        for (; ita != other.types.end() && itb != types.end(); ++ita, ++itb)
            if (*ita != *itb)
                return false;
        return ita == other.types.end() && itb == types.end();
    }

    const std::vector<TypeId>& get_types() const
    {
        return types;
    }

private:
    std::vector<TypeId> types;
};

class TypeSpecialization
{
  public:
    template <typename T> void push();

    TypeSpecialization() = default;

    //@TODO : HANDLE TYPE SPECIALIZATION (we should be able to handle Type<Type<>> recursively
    TypeSpecialization(size_t in_size) : size(in_size)
    {
    }

    size_t stride() const
    {
        return size;
    }

    const std::vector<Type*>& get_args() const
    {
        return arguments;
    }

  private:
    size_t             size = 0;
    std::vector<Type*> arguments;
};
} // namespace Reflection

namespace std
{
template <> struct hash<Reflection::TypeSpecializationDescription>
{
    size_t operator()(const Reflection::TypeSpecializationDescription& c) const noexcept
    {
        size_t result = 0;
        for (const auto& type : c.get_types())
            hash_combine(result, type);
        return result;
    }
};

} // namespace std