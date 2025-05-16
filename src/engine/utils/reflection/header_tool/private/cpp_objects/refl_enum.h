#pragma once
#include "namespaced_name.h"

namespace Llp
{
class TokenSet;
class Tokenizer;
}
class Enum
{
public:

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
