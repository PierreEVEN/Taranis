#include "refl_class.h"

#include "refl_type.h"
#include "src/engine/utils/reflection/header_tool/private/header_parser.hpp"

#include <llp/native_tokens.hpp>
#include <llp/parser.hpp>

Llp::ParserError Class::try_parse(Llp::Parser& parser, const ParserContext& context, const Llp::TokenSet&)
{
    // Name
    if (auto error = namespaced_name.try_parse(parser, context))
        return error;

    if (parser.consume<Llp::WordToken>("final"))
        b_final = true;

    if (parser.consume<Llp::SymbolToken>(';'))
    {
        b_is_forward_declaration = true;
        return {};
    }

    if (parser.consume<Llp::SymbolToken>(':'))
    {
        do
        {
            // Skip private / public fields
            parser.consume<Llp::WordToken>("private") || parser.consume<Llp::WordToken>("protected") || parser.consume<Llp::WordToken>("public");
            Type type;
            if (auto error = type.try_parse(parser, context))
                return error;
        } while (parser.consume<Llp::SymbolToken>(','));
    }

    if (!parser.get<Llp::BraceBlockToken>())
        return Llp::ParserError{parser.current_location(), "Brace block '{}' expected after class declaration"};

    b_is_forward_declaration = false;
    return {};
}

Llp::ParserError Class::make_reflected(Llp::Parser& parser)
{
    if (implementation_line != 0)
        return Llp::ParserError{
            parser.current_location(), "Cannot implement multiple REFLECT_BODY() for the same class"
        };
    implementation_line = parser.current_location().get_line();
    auto args           = parser.consume<Llp::ParenthesisBlockToken>();
    if (!args)
        return Llp::ParserError{
            parser.current_location(), "() expected"
        };

    return {};
}

Llp::ParserError Class::parse_property(Llp::Parser& parser, const ParserContext& context)
{
    if (!parser.consume<Llp::ParenthesisBlockToken>())
        return Llp::ParserError{parser.current_location(), "'(args...)' expected after RPROPERTY"};

    // Try parsing a type
    Type type;
    if (auto error = type.try_parse(parser, context))
        return error;

    // Requires "::" if not in global namespace
    if (!context.namespace_stack.empty() && !type.name().is_global_namespace())
    {
        if (!Type::builtin_types.contains(type.name().short_name()))
            return Llp::ParserError{parser.current_location(), "Cannot determine the type of a property that doesn't have an explicit path and is not declared in the global scope (and is not a builtin types)"};
    }

    // Property name
    auto property_name = parser.consume<Llp::WordToken>();
    if (!property_name)
        return Llp::ParserError{parser.current_location(), "Expected property name"};

    // Register property
    if (properties.contains(property_name->word))
        return Llp::ParserError{
            parser.current_location(), std::format("Duplicated property ", property_name->word)
        };
    properties.emplace(property_name->word, type);
    return {};
}
