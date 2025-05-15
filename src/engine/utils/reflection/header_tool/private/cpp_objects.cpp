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

    if (builtin_types.contains(name))
        type_namespace.clear();

    return {};
}

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
        if (!builtin_types.contains(type.name().short_name()))
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

Llp::ParserError Enum::try_parse(Llp::Parser& parser, const ParserContext& context, const Llp::TokenSet& token_set)
{
    auto args = parser.consume<Llp::ParenthesisBlockToken>();
    if (!args)
        return Llp::ParserError{parser.current_location(), "'(args...)' expected after RENUM"};
    if (auto error = try_parse_args(args->content))
        return error;

    if (!parser.consume<Llp::WordToken>("enum"))
        return Llp::ParserError{parser.current_location(), "'enum' word expected"};

    scoped = parser.consume<Llp::WordToken>("class");

    if (auto error = enum_name.try_parse(parser, context))
        return error;

#if _WIN32
    type = "int";
#else
    type = "";
#endif
    if (parser.consume<Llp::SymbolToken>(':'))
    {
        if (auto t = parser.consume<Llp::WordToken>())
            type   = t->word;
        else
            return Llp::ParserError{parser.current_location(), "Expected enum type after ':'"};
    }

    auto enum_content = parser.consume<Llp::BraceBlockToken>();
    if (!enum_content)
        return Llp::ParserError{parser.current_location(), "Expected enum content {}"};
    if (auto error = try_parse_body(enum_content->content, token_set))
        return error;

    return {};
}

Llp::ParserError Enum::try_parse_args(Llp::Tokenizer& block)
{
    Llp::Parser parser(block);
    while (parser && parser.get_current_token_type() != Llp::NULL_TOKEN)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            if (key->word == "EnumFlags")
                b_flag_enum_flags = true;
            else
                return Llp::ParserError{parser.current_location(), std::format("Unknown word token '{}'", key->word)};
        }
        else
            return Llp::ParserError{parser.current_location(), std::format("Expected word")};
        if (parser && !parser.consume<Llp::SymbolToken>(','))
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }
    return {};
}

Llp::ParserError Enum::try_parse_body(Llp::Tokenizer& tokenizer, const Llp::TokenSet& token_set)
{
    Llp::Parser parser(tokenizer);
    while (parser)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            fields.emplace_back(key->word);
            if (parser.consume<Llp::SymbolToken>('='))
            {
                if (!parser.consume<Llp::IntegerToken>())
                    return Llp::ParserError{parser.current_location(), "Expected number here"};
                if (parser.consume<Llp::SymbolToken>('<'))
                {
                    if (!parser.consume<Llp::SymbolToken>('<'))
                        return Llp::ParserError{parser.current_location(), "Expected symbol '<'"};

                    if (!parser.consume<Llp::IntegerToken>())
                        return Llp::ParserError{parser.current_location(), "Expected number here"};
                }
            }
        }
        else
            return Llp::ParserError{
                parser.current_location(),
                std::format("Expected enum field name, got {}", parser.get_current_token_name(token_set))
            };
        if (parser && !parser.consume<Llp::SymbolToken>(','))
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }
    return {};
}