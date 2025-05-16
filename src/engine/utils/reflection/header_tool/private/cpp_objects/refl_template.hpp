#pragma once
#include <llp/error.hpp>
#include <vector>

class Type;
struct ParserContext;
namespace Llp
{
class Parser;
}
class TemplateDeclaration
{
  public:
    Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context);

    [[nodiscard]] bool is_variadic() const
    {
        return b_variadic;
    }

    [[nodiscard]] const std::vector<Type>& get_types() const
    {
        return template_types;
    }

    [[nodiscard]] std::string gen_values(bool b_with_type) const;
    [[nodiscard]] std::string gen_declaration() const;

  private:
    bool              b_variadic = false;
    std::vector<Type> template_types;
};
