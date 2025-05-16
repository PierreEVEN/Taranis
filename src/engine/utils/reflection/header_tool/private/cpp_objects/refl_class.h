#pragma once
#include "namespaced_name.h"
#include "refl_template.hpp"
#include "refl_type.h"

#include <ankerl/unordered_dense.h>

namespace Llp
{
class TokenSet;
}
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

    [[nodiscard]] bool is_forward_declaration() const
    {
        return b_is_forward_declaration;
    }
    [[nodiscard]] size_t get_implementation_line() const
    {
        return implementation_line;
    }
    [[nodiscard]] const ankerl::unordered_dense::map<std::string, Type>& get_properties() const
    {
        return properties;
    }
    [[nodiscard]] const std::vector<Type>& get_parents() const
    {
        return parents;
    }
    [[nodiscard]] bool is_template() const
    {
        return b_is_template;
    }
    [[nodiscard]] TemplateDeclaration get_template_declaration() const
    {
        return template_declaration;
    }

    void set_template_arguments(const TemplateDeclaration& in_template_declaration)
    {
        b_is_template        = true;
        template_declaration = in_template_declaration;
    }

  private:
    NamespacedName                                  namespaced_name;
    bool                                            b_is_forward_declaration = true;
    bool                                            b_final                  = false;
    size_t                                          implementation_line      = 0;
    std::vector<Type>                               parents;
    ankerl::unordered_dense::map<std::string, Type> properties;
    bool                                            b_is_template = false;
    TemplateDeclaration                             template_declaration;
};
