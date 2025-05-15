#pragma once
#include "cpp_objects.hpp"
#include "llp/file_data.hpp"

#include <filesystem>
#include <ankerl/unordered_dense.h>
#include <vector>
#include <llp/token.hpp>

/******* CUSTOM TOKENS *******/
namespace Llp
{
/*####[ :: ]####*/
DECLARE_LEXER_TOKEN(ScopeOperator)

static std::unique_ptr<ScopeOperator> consume(const TokenSet&, Location& in_location, const std::string& source, ParserError&)
    {
        if (source[in_location.get_index()] == ':' && source[in_location.get_index() + 1] == ':')
        {
            ++++in_location;
            return std::make_unique<ScopeOperator>(in_location);
        }
        return nullptr;
    }

    [[nodiscard]] std::string to_string(const TokenSet&, bool) const override
    {
        return "::";
    }
};
}

struct ParserContext
{
    std::vector<std::string>            namespace_stack;
    std::vector<std::shared_ptr<Class>> class_stack;

    [[nodiscard]] ParserContext push_namespace(const std::string& last_namespace) const
    {
        ParserContext copy = *this;
        copy.namespace_stack.push_back(last_namespace);
        return copy;
    }

    [[nodiscard]] ParserContext push_class(const Class& last_class) const
    {
        ParserContext copy = *this;
        copy.class_stack.push_back(std::make_shared<Class>(last_class));
        return copy;
    }
};

class HeaderParser
{
public:
    HeaderParser(const std::string& header_data, std::filesystem::path generated_header_include_path, std::filesystem::path header_path);

    [[nodiscard]] std::optional<size_t> get_include_line_to_add() const
    {
        return b_found_include ? std::optional<size_t>{} : line_after_last_include;
    }

    [[nodiscard]] const ankerl::unordered_dense::map<std::string, std::shared_ptr<Class>>& get_classes() const
    {
        return reflected_classes;
    }

    [[nodiscard]] const ankerl::unordered_dense::map<std::string, Enum>& get_enums() const
    {
        return reflected_enums;
    }

private:
    [[nodiscard]] Llp::ParserError parse_block(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, const ParserContext& context);

    std::filesystem::path                                             generated_header_include_path;
    std::filesystem::path                                             header_path;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<Class>> reflected_classes;
    ankerl::unordered_dense::map<std::string, Enum>                   reflected_enums;
    size_t                                                            line_after_last_include = 1;
    bool                                                              b_found_include         = false;
};