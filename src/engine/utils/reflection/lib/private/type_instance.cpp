#include "type_instance.hpp"

#include "type.hpp"

namespace Reflection
{
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
            text += Type::get_type(*it)->name();
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