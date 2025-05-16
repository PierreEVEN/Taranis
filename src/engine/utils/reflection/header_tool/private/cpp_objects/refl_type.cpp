
#include "refl_type.h"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

ankerl::unordered_dense::set<std::string> Type::builtin_types = {"bool", "void", "char", "wchar_t", "short", "float", "double", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t"};

std::string Type::cpp_name(bool b_with_decorations) const
{
    std::string full_name = (b_with_decorations && b_is_const ? "const " : "") + namespaced_name.cpp_name();
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
    std::string full_name = (b_with_decorations && b_is_const ? "const_" : "") + namespaced_name.sanitized_name();
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

Llp::ParserError Type::try_parse(Llp::Parser& parser, const ParserContext& context)
{
    if (parser.consume<Llp::WordToken>("const"))
        b_is_const = true;

    if (auto error = namespaced_name.try_parse(parser, context))
        return error;

    if (parser.consume<Llp::SymbolToken>('<'))
    {
        do
        {
            Type type;
            if (auto error = type.try_parse(parser, context))
                return error;
            template_args.push_back(type);
        } while (parser.consume<Llp::SymbolToken>(','));
        if (!parser.consume<Llp::SymbolToken>('>'))
            return Llp::ParserError{parser.current_location(), "'>' expected"};
    }
    while (parser.consume<Llp::SymbolToken>('*'))
        ++n_ptr_indirections;
    if (parser.consume<Llp::SymbolToken>('&'))
        b_is_ref = true;

    return {};
}
