#pragma once
#include <llp/error.hpp>
#include <vector>

struct ParserContext;
namespace Llp
{
class Parser;
}

class NamespacedName
{
public:
    Llp::ParserError try_parse(Llp::Parser& parser, const ParserContext& context);

    // Name without namespace
    [[nodiscard]] const std::string& short_name() const
    {
        return name;
    }
    // Full name with namespace
    [[nodiscard]] std::string cpp_name() const;
    // Only namespace
    [[nodiscard]] std::string cpp_namespace() const;
    // Names separated with "_"
    [[nodiscard]] std::string sanitized_name() const;
    // Only namespace separated with "_"
    [[nodiscard]] std::string sanitized_namespace() const;
    // Starts with "::"
    [[nodiscard]] bool is_global_namespace() const
    {
        return b_is_global_namespace;
    }
    [[nodiscard]] bool has_namespace() const
    {
        return !type_namespace.empty();
    }

private:
    std::string              name;
    std::vector<std::string> type_namespace;
    bool                     b_is_global_namespace = false;
};
