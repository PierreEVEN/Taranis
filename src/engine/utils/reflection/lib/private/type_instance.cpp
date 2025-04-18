#include "type_instance.hpp"

#include "type.hpp"

namespace Reflection
{

std::string TypeInstance::display() const
{
    std::string text;
    if (is_const())
        text += "const ";
    text += base_id.name();

    for (uint8_t i = 0; i < get_ptr_indirections(); ++i)
        text += '*';
    if (is_ref())
        text += '&';
    return text;
}
} // namespace Reflection