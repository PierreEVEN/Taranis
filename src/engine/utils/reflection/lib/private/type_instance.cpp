#include "type_instance.hpp"

#include "type.hpp"

namespace Reflection
{
bool TypeSpecializationDescription::operator==(const TypeSpecializationDescription& other) const
{
    auto ita = other.types.begin();
    auto itb = types.begin();
    for (; ita != other.types.end() && itb != types.end(); ++ita, ++itb)
        if (*ita != *itb)
            return false;
    return ita == other.types.end() && itb == types.end();
}

std::string TypeInstance::display() const
{
    std::string text;
    if (is_const())
        text += "const ";
    text += base_type->name();

    if (template_specialization)
    {
        text += '<';
        auto it = template_specialization->get_types().begin();
        while (it != template_specialization->get_types().end())
        {
            text += it->display();
            ++it;
            if (it != template_specialization->get_types().end())
                text += ", ";
        }
        text += '>';
    }

    for (uint8_t i = 0; i < flags; ++i)
        text += '*';
    if (is_ref())
        text += '&';
    return text;
}
} // namespace Reflection