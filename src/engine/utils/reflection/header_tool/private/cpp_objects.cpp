#include "cpp_objects.hpp"

#include "header_parser.hpp"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

static ankerl::unordered_dense::set<std::string> builtin_types = {
    "bool",
    "void",
    "char",
    "wchar_t",
    "short",
    "float",
    "double",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t"
};

std::string Type::cpp_name(bool b_with_decorations) const
{
    std::string full_name = (b_with_decorations && b_is_const ? "const " : "") + cpp_namespace();
    full_name             = full_name.empty() ? name : full_name + "::" + name;
    if (!template_args.empty())
    {
        full_name += '<';
        for (size_t i = 0; i < template_args.size(); ++i)
            full_name += i == template_args.size() - 1 ? template_args[i].cpp_name() : template_args[i].cpp_name() + ", ";
        full_name += '>';
    }
    for (size_t i = 0; i < n_ptr_indirections; ++i)
        full_name += '*';
    if (b_with_decorations && b_is_ref)
        full_name += '&';
    return full_name;
}

std::string Type::sanitized_name(bool b_with_decorations) const
{
    std::string full_name = (b_with_decorations && b_is_const ? "const_" : "") + sanitized_namespace();
    full_name             = full_name.empty() ? name : full_name + "_" + name;
    if (!template_args.empty())
    {
        full_name += '_';
        for (size_t i = 0; i < template_args.size(); ++i)
            full_name += i == template_args.size() - 1 ? template_args[i].sanitized_name() : template_args[i].sanitized_name() + "_";
        full_name += '_';
    }
    for (size_t i = 0; i < n_ptr_indirections; ++i)
        full_name += "_PTR";
    if (b_with_decorations && b_is_ref)
        full_name += "_REF";
    return full_name;
}

std::string Type::cpp_namespace() const
{
    std::string ns;
    for (size_t i = 0; i < type_namespace.size(); ++i)
        ns += type_namespace[i] + (i < type_namespace.size() - 1 ? "::" : "");
    return ns;
}

std::string Type::sanitized_namespace() const
{
    std::string ns;
    for (size_t i = 0; i < type_namespace.size(); ++i)
        ns += type_namespace[i] + (i < type_namespace.size() - 1 ? "_" : "");
    return ns;
}

std::optional<Llp::ParserError> Type::try_parse(Llp::Parser& parser, const ParserContext& context)
{
    if (parser.consume<Llp::WordToken>("const"))
        b_is_const = true;

    // Ignore first "::"
    bool b_is_global_namespace = false;
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

    if (parser.consume<Llp::SymbolToken>('<'))
    {
        do
        {
            Type type;
            if (auto error = type.try_parse(parser, context))
                return error;
            template_args.push_back(type);
        } while (parser.consume<Llp::ComaToken>());
        if (!parser.consume<Llp::SymbolToken>('>'))
            return Llp::ParserError{parser.current_location(), "'>' expected"};
    }
    while (parser.consume<Llp::SymbolToken>('*'))
        ++n_ptr_indirections;
    if (parser.consume<Llp::SymbolToken>('&'))
        b_is_ref = true;

    // Requires "::" if not in global namespace
    if (!context.namespace_stack.empty() && !b_is_global_namespace)
    {
        if (!builtin_types.contains(name))
            return Llp::ParserError{parser.current_location(), "Cannot determine the type of a property that doesn't have an explicit path and is not declared in the global scope (and is not a builtin types)"};
    }
    return {};
}

std::optional<Llp::ParserError> Class::try_parse(Llp::Parser& parser, const ParserContext& context)
{

}