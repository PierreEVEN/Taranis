#pragma once
#include <optional>
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>
#include <llp/tokens.hpp>

struct ParserContext;

namespace Llp
{
class Parser;
}

class Type
{
public:
    const std::string& name_short() const
    {
        return name;
    }

    const std::vector<Type>& get_template_args() const
    {
        return template_args;
    }

    bool is_template() const
    {
        return !template_args.empty();
    }

    bool is_const() const
    {
        return b_is_const;
    }

    bool is_ref() const
    {
        return b_is_ref;
    }

    bool ptr_indirections() const
    {
        return n_ptr_indirections;
    }

    std::string cpp_name(bool b_with_decorations = false) const;
    std::string sanitized_name(bool b_with_decorations = false) const;

    std::string cpp_namespace() const;
    std::string sanitized_namespace() const;

    std::optional<Llp::ParserError> try_parse(Llp::Parser& parser, const ParserContext& context);

private:
    std::string              name;
    std::vector<std::string> type_namespace;

    std::vector<Type> template_args;
    bool              b_is_const         = false;
    bool              b_is_ref           = false;
    uint32_t          n_ptr_indirections = 0;
};

class Class : public Type
{
    std::optional<Llp::ParserError> try_parse(Llp::Parser& parser, const ParserContext& context);

public:
    size_t                                          implementation_line;
    std::vector<Type>                               parents;
    ankerl::unordered_dense::map<std::string, Type> properties;
};


struct Enum
{
    friend class HeaderParser;

    std::string name(bool b_with_decorations = false) const;

    std::string cpp_namespace() const;
    std::string sanitized_namespace() const;

    const std::vector<std::string>& get_fields() const
    {
        return fields;
    }

    const std::string& get_type() const
    {
        return type;
    }

    bool is_scoped() const
    {
        return scoped;
    }

    bool is_enum_flag() const
    {
        return enum_flag;
    }

  private:
    std::string              enum_name;
    bool                     scoped;
    std::string              type;
    std::vector<std::string> fields;
    bool                     enum_flag = false;
};
