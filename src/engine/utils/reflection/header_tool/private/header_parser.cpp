#include "header_parser.hpp"

#include "llp/native_tokens.hpp"
#include "llp/parser.hpp"

#include <filesystem>
#include <format>

HeaderParser::HeaderParser(const std::string& header_data, std::filesystem::path in_generated_header_include_path, std::filesystem::path in_header_path)
    : generated_header_include_path(std::move(in_generated_header_include_path)), header_path(std::move(in_header_path))
{
    Llp::TokenSet token_set;

    Llp::Tokenizer lexer;
    // Register custom tokens
    token_set.register_token_before<Llp::ScopeOperator, Llp::SymbolToken>("ScopeOp");

    lexer.tokenize(header_data, token_set);
    parse_block(lexer, token_set, {})->exit_on_error(header_path);
}

std::optional<Llp::ParserError> HeaderParser::parse_enum_args(const Llp::Tokenizer& block, Enum& data)
{
    Llp::Parser parser(block);
    while (parser && parser.get_current_token_type() != Llp::NULL_TOKEN)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            if (key->word == "EnumFlags")
                data.enum_flag = true;
            else
                return Llp::ParserError{parser.current_location(), std::format("Unknown word token '{}'", key->word)};
        }
        else
            return Llp::ParserError{parser.current_location(), std::format("Expected word")};
        if (parser && !parser.consume<Llp::ComaToken>())
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }

    return {};
}

std::optional<Llp::ParserError> HeaderParser::parse_enum_body(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, Enum& data)
{
    Llp::Parser parser(block);
    while (parser && parser.get_current_token_type() != Llp::NULL_TOKEN)
    {
        if (auto key = parser.consume<Llp::WordToken>())
        {
            data.fields.emplace_back(key->word);
            if (parser.consume<Llp::EqualsToken>())
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
            return Llp::ParserError{parser.current_location(), std::format("Expected enum field name, got {}", parser.get_current_token_name(token_set))};
        if (parser && !parser.consume<Llp::ComaToken>())
            return Llp::ParserError{parser.current_location(), "Expected coma token"};
    }
    return {};
}

std::optional<Llp::ParserError> HeaderParser::parse_block(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, const ParserContext& context)
{
    for (Llp::Parser parser(block); parser;)
    {
        if (parser.consume<Llp::SymbolToken>('#'))
        {
                if (parser.consume<Llp::WordToken>("pragma") && parser.consume<Llp::WordToken>("once"))
                    line_after_last_include = parser.current_location().line + 2;
        }
        else if (parser.get_current_token_type() == Llp::TTokenType<Llp::WordToken>::id)
        {
            const std::string& word = parser.consume<Llp::WordToken>()->word;

            // Class declaration
            if (word == "class")
            {
                if (auto class_name = parser.consume<Llp::WordToken>())
                {
                    // Skip final keyword
                    parser.consume<Llp::WordToken>("final");

                    std::vector<std::string> parents;
                    // : parent
                    if (parser.consume<Llp::SymbolToken>(':'))
                    {
                        do
                        {
                            // Skip fields
                            parser.consume<Llp::WordToken>("public") || parser.consume<Llp::WordToken>("protected") || parser.consume<Llp::WordToken>("private");

                            std::string parent;

                            do
                            {
                                if (auto parent_class = parser.consume<Llp::WordToken>())
                                {
                                    if (!parent.empty())
                                        parent += "::";
                                    parent += parent_class->word;

                                    if (parser.consume<Llp::SymbolToken>('<'))
                                    {
                                        parent += '<';
                                        size_t template_level = 1;
                                        do
                                        {
                                            if (parser.consume<Llp::SymbolToken>('<'))
                                            {
                                                parent += '<';
                                                template_level++;
                                            }
                                            else if (auto* template_str = parser.consume<Llp::WordToken>())
                                                parent += template_str->word;
                                            else if (parser.consume<Llp::SymbolToken>('>'))
                                            {
                                                parent += '>';
                                                template_level--;
                                            }

                                        } while (parser && template_level != 0);
                                    }
                                }
                                else
                                    return Llp::ParserError{parser.current_location(), "Expected class name"};
                            } while (parser.consume<Llp::ScopeOperator>());

                            if (!parent.empty())
                                parents.push_back(parent);

                        } while (parser.consume<Llp::ComaToken>());
                    }
                    if (auto* class_block = parser.consume<Llp::BraceBlockToken>())
                        if (auto error = parse_block(class_block->content, context.push_class(Class{class_name->word, parents, {}})))
                            return error;
                }
            }
            else if (word == "using")
            {
                if (parser.consume<Llp::WordToken>("namespace"))
                    return Llp::ParserError{parser.current_location(), "'using namespace ...;' is forbidden in headers !"};
            }
            else if (word == "namespace")
            {
                std::vector<std::string> added_namespace_stack;
                bool                     b_failed = false;
                do
                {
                    parser.consume<Llp::ScopeOperator>();
                    if (auto namespace_name = parser.consume<Llp::WordToken>())
                        added_namespace_stack.push_back(namespace_name->word);
                    else
                    {
                        b_failed = true;
                        break;
                    }

                } while (parser.get_current_token_type() != Llp::TTokenType<Llp::BraceBlockToken>::id);
                if (!b_failed)
                {
                    ParserContext new_context = context;
                    for (const auto& elem : added_namespace_stack)
                        new_context = new_context.push_namespace(elem);
                    if (auto* class_block = parser.consume<Llp::BraceBlockToken>())
                        if (auto error = parse_block(class_block->content, token_set, new_context))
                            return error;
                }
            }
            if (word == "RPROPERTY")
            {
                // expect RPROPERTY(...)
                if (context.class_stack.empty())
                    return Llp::ParserError{parser.current_location(), "RPROPERTY() should not be used outside class context"};
                if (!parser.consume<Llp::ParenthesisBlockToken>())
                    return Llp::ParserError{parser.current_location(), "'(args...)' expected after RPROPERTY"};

                // Try parsing a type
                Type type;
                if (auto error = type.try_parse(parser, context))
                    return error;

                // Property name
                auto property_name = parser.consume<Llp::WordToken>();
                if (!property_name)
                    return Llp::ParserError{parser.current_location(), "Expected property name"};

                // Register property
                if (context.class_stack.back()->properties.contains(property_name->word))
                    return Llp::ParserError{parser.current_location(), std::format("Duplicated property ", property_name->word)};
                context.class_stack.back()->properties.emplace(property_name->word, type);
            }
            if (word == "REFLECT_BODY")
            {
                if (context.class_stack.empty())
                    return Llp::ParserError{parser.current_location(), "REFLECT_BODY() should not be declared outside class context"};

                std::string absolute_class_name;
                for (const auto& elem : context.class_stack)
                    absolute_class_name += "::" + elem->name;
                if (reflected_classes.contains(absolute_class_name))
                    return Llp::ParserError{parser.current_location(), "Cannot implement multiple REFLECT_BODY() for the same class"};
                reflected_classes.emplace(absolute_class_name, Class{context, parser.current_location().line + 1});
            }
            if (word == "RENUM")
            {
                Enum enum_data;
                if (auto args = parser.consume<Llp::ParenthesisBlockToken>())
                {
                    if (auto error = parse_enum_args(args->content, enum_data))
                        return error;
                }
                else
                    return Llp::ParserError{parser.current_location(), "'(args...)' expected after RENUM"};
                if (!parser.consume<Llp::WordToken>("enum"))
                    return Llp::ParserError{parser.current_location(), "'enum' word expected"};
                bool scoped = parser.consume<Llp::WordToken>("class");

                if (auto enum_name = parser.consume<Llp::WordToken>())
                {
#if _WIN32
                    std::string enum_type = "int";
#else
                    std::string enum_type = "";
#endif
                    if (parser.consume<Llp::SymbolToken>(':'))
                    {
                        if (auto t    = parser.consume<Llp::WordToken>())
                            enum_type = t->word;
                        else
                            return Llp::ParserError{parser.current_location(), "Expected enum type after ':'"};
                    }

                    if (auto enum_content = parser.consume<Llp::BraceBlockToken>())
                    {
                        enum_data.context   = context;
                        enum_data.type      = enum_type;
                        enum_data.scoped    = scoped;
                        enum_data.enum_name = enum_name->word;
                        if (auto error = parse_enum_body(enum_content->content, token_set, enum_data))
                            return error;
                        reflected_enums.emplace(enum_name->word, enum_data);
                    }
                }
                else
                    return Llp::ParserError{parser.current_location(), "Expected enum name"};
            }
        }
        else if (parser.get_current_token_type() == Llp::TTokenType<Llp::IncludeToken>::id)
        {
            line_after_last_include = parser.current_location().line + 2;
            std::string& include    = parser.consume<Llp::IncludeToken>()->path;
            if (std::filesystem::path(include).lexically_normal() == generated_header_include_path)
                b_found_include = true;
        }
        else if (parser.get_current_token_type() == Llp::TTokenType<Llp::BraceBlockToken>::id)
        {
            if (auto* class_block = parser.consume<Llp::BraceBlockToken>())
                if (auto error = parse_block(class_block->content, token_set, context))
                    return error;
        }
        else
            ++parser;
    }
    return {};
}

bool HeaderParser::parse_check_include(Llp::TextReader& reader, const std::filesystem::path& desired_path)
{
    bool        started = false;
    std::string include;

    while (reader)
    {
        if (*reader == '"' || *reader == '<' || *reader == '>')
        {
            ++reader;
            if (started)
                break;

            started = true;
            continue;
        }
        if (started)
            include += *reader;
        ++reader;
    }

    if (!started)
        return false;

    return std::filesystem::path(include).lexically_normal() == desired_path.lexically_normal();
}