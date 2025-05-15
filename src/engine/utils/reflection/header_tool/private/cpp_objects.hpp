#pragma once
#include <optional>
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>
#include <llp/error.hpp>

struct ParserContext;

namespace Llp
{
class TokenSet;
class Tokenizer;
class Parser;
}

class NamespacedName
{
public:
    Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context);

    // Name without namespace
    [[nodiscard]] const std::string& short_name() const { return name; }
    // Full name with namespace
    [[nodiscard]] std::string cpp_name() const;
    // Only namespace
    [[nodiscard]] std::string cpp_namespace() const;
    // Names separated with "_"
    [[nodiscard]] std::string sanitized_name() const;
    // Only namespace separated with "_"
    [[nodiscard]] std::string sanitized_namespace() const;
    // Starts with "::"
    [[nodiscard]] bool is_global_namespace() const { return b_is_global_namespace; }
    [[nodiscard]] bool has_namespace() const { return !type_namespace.empty(); }

private:
    std::string              name;
    std::vector<std::string> type_namespace;
    bool                     b_is_global_namespace = false;
};

class Type
{
public:
    [[nodiscard]] const NamespacedName& name() const
    {
        return namespaced_name;
    }

    [[nodiscard]] const std::vector<Type>& get_template_args() const
    {
        return template_args;
    }

    [[nodiscard]] bool is_template() const
    {
        return !template_args.empty();
    }

    [[nodiscard]] bool is_const() const
    {
        return b_is_const;
    }

    [[nodiscard]] bool is_ref() const
    {
        return b_is_ref;
    }

    [[nodiscard]] bool ptr_indirections() const
    {
        return n_ptr_indirections;
    }

    [[nodiscard]] std::string cpp_name(bool b_with_decorations = false) const;
    [[nodiscard]] std::string sanitized_name(bool b_with_decorations = false) const;

    Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context);

private:
    NamespacedName namespaced_name;

    std::vector<Type> template_args;
    bool              b_is_const         = false;
    bool              b_is_ref           = false;
    uint32_t          n_ptr_indirections = 0;
};

class Class
{
public:
    Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context, const Llp::TokenSet& token_set);

    // Consume () arguments from REFLECT_BODY
    Llp::ParserError make_reflected(Llp::Parser& parser);


    // Consume () arguments and the following property from RPROPERT
    Llp::ParserError parse_property(Llp::Parser& parser, const ParserContext& context);

    [[nodiscard]] const NamespacedName& name() const
    {
        return namespaced_name;
    }

    [[nodiscard]] bool                                                   is_forward_declaration() const { return b_is_forward_declaration; }
    [[nodiscard]] size_t                                                 get_implementation_line() const { return implementation_line; }
    [[nodiscard]] const ankerl::unordered_dense::map<std::string, Type>& get_properties() const { return properties; }
    [[nodiscard]] const std::vector<Type>&                               get_parents() const { return parents; }
    [[nodiscard]] bool                                                   is_template() const { return b_is_template; }
    [[nodiscard]] size_t                                                 get_template_arguments() const { return template_arguments; }

private:
    NamespacedName                                  namespaced_name;
    bool                                            b_is_forward_declaration = true;
    bool                                            b_final                  = false;
    size_t                                          implementation_line      = 0;
    std::vector<Type>                               parents;
    ankerl::unordered_dense::map<std::string, Type> properties;
    bool                                            b_is_template      = false;
    size_t                                          template_arguments = 0;
};

struct Enum
{
    friend class HeaderParser;

    [[nodiscard]] Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context, const Llp::TokenSet& token_set);

    [[nodiscard]] const NamespacedName& name() const
    {
        return enum_name;
    }

    [[nodiscard]] const std::vector<std::string>& get_fields() const
    {
        return fields;
    }

    [[nodiscard]] const std::string& get_type() const
    {
        return type;
    }

    [[nodiscard]] bool is_scoped() const
    {
        return scoped;
    }

    [[nodiscard]] bool is_enum_flag() const
    {
        return b_flag_enum_flags;
    }

private:
    [[nodiscard]] Llp::ParserError try_parse_args(Llp::Tokenizer& tokenizer);
    [[nodiscard]] Llp::ParserError try_parse_body(Llp::Tokenizer& tokenizer, const Llp::TokenSet& token_set);
    NamespacedName                 enum_name;
    bool                           scoped;
    std::string                    type;
    std::vector<std::string>       fields;
    bool                           b_flag_enum_flags = false;
};
