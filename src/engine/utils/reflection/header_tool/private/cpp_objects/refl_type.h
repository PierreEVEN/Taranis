#pragma once
#include "namespaced_name.h"
#include "ankerl/unordered_dense.h"

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

    static ankerl::unordered_dense::set<std::string> builtin_types;
private:
    NamespacedName namespaced_name;

    std::vector<Type> template_args;
    bool              b_is_const         = false;
    bool              b_is_ref           = false;
    uint32_t          n_ptr_indirections = 0;
};
