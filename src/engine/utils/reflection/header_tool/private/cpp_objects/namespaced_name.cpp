
#include "namespaced_name.h"

#include "custom_ops.hpp"
#include "refl_type.h"
#include "src/engine/utils/reflection/header_tool/private/header_parser.hpp"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

std::string NamespacedName::cpp_name() const
{
    const auto ns = cpp_namespace();
    return ns.empty() ? name : ns + "::" + name;
}

std::string NamespacedName::sanitized_name() const
{
    const auto ns = sanitized_namespace();
    return ns.empty() ? name : ns + "_" + name;
}

std::string NamespacedName::cpp_namespace() const
{
    std::string ns;
    for (size_t i = 0; i < type_namespace.size(); ++i)
        ns += type_namespace[i] + (i < type_namespace.size() - 1 ? "::" : "");
    return ns;
}

std::string NamespacedName::sanitized_namespace() const
{
    std::string ns;
    for (size_t i = 0; i < type_namespace.size(); ++i)
        ns += type_namespace[i] + (i < type_namespace.size() - 1 ? "_" : "");
    return ns;
}

Llp::ParserError NamespacedName::try_parse(Llp::Parser& parser, const ParserContext& context)
{
    if (parser.consume<Llp::ScopeOperator>())
        b_is_global_namespace = true;

    if (!b_is_global_namespace)
        for (const auto& ns : context.namespace_stack)
            type_namespace.emplace_back(ns);

    do
    {
        if (!name.empty())
            type_namespace.emplace_back(name);
        if (auto found_name = parser.consume<Llp::WordToken>())
            name            = found_name->word;
        else
            return Llp::ParserError{parser.current_location(), "Expected word here"};
    } while (parser.consume<Llp::ScopeOperator>());

    if (Type::builtin_types.contains(name))
        type_namespace.clear();

    return {};
}
