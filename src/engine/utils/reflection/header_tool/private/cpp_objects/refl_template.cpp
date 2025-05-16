

#include "refl_template.hpp"

#include "custom_ops.hpp"
#include "refl_type.h"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

Llp::ParserError TemplateDeclaration::try_parse(Llp::Parser& parser, const ParserContext& context)
{
    if (!parser.consume<Llp::SymbolToken>('<'))
        return Llp::ParserError{parser.current_location(), "'<' expected"};

    while (!parser.consume<Llp::SymbolToken>('>'))
    {
        Type type_name;
        if (auto error = type_name.try_parse(parser, context))
            return error;
        template_types.emplace_back(type_name);

        if (parser.consume<Llp::EllipsisOperator>())
            b_variadic = true;

        if (!parser.consume<Llp::WordToken>())
            return Llp::ParserError{parser.current_location(), "Expected typename"};
    }
    return {};
}

std::string TemplateDeclaration::gen_values(bool b_with_type) const
{
    std::string arguments;
    char        name = 'A';
    for (size_t i = 0; i < template_types.size(); ++i)
    {
        if (b_with_type)
            arguments += template_types[i].cpp_name() + " ";
        arguments += std::format("{}{}", name++, i < template_types.size() - 1 ? ", " : "");
    }

    if (b_variadic)
    {
        if (!template_types.empty())
            arguments += ", ";
        arguments += "Args";
    }
    return arguments;
}

std::string TemplateDeclaration::gen_declaration() const
{
    return std::format("template <{}>", gen_values(true));
}
