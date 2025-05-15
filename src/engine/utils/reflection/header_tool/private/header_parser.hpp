#pragma once
#include "cpp_objects.hpp"
#include "llp/file_data.hpp"

#include <filesystem>
#include <ankerl/unordered_dense.h>
#include <vector>
#include <llp/token.hpp>

struct ParserContext;

namespace Llp
{
class Parser;
}


class FileReader;

struct ParserContext
{
    std::vector<std::string>            namespace_stack;
    std::vector<std::shared_ptr<Class>> class_stack;

    ParserContext push_namespace(const std::string& last_namespace) const
    {
        ParserContext copy = *this;
        copy.namespace_stack.push_back(last_namespace);
        return copy;
    }

    ParserContext push_class(const Class& last_class) const
    {
        ParserContext copy = *this;
        copy.class_stack.push_back(std::make_shared<Class>(last_class));
        return copy;
    }
};

/******* CUSTOM TOKENS *******/
namespace Llp
{
/*####[ :: ]####*/
DECLARE_LEXER_TOKEN(ScopeOperator)

    static std::unique_ptr<ScopeOperator> consume(Tokenizer&, Location& in_location, const std::string& source, std::optional<ParserError>&)
    {
        if (source[in_location.index] == ':' && source[in_location.index + 1] == ':')
        {
            ++++in_location;
            return std::make_unique<ScopeOperator>(in_location);
        }
        return nullptr;
    }

    std::string to_string(const TokenSet&, bool) const override
    {
        return "::";
    }
};
}

class HeaderParser
{
public:
    HeaderParser(const std::string& header_data, std::filesystem::path generated_header_include_path, std::filesystem::path header_path);

    std::optional<size_t> get_include_line_to_add() const
    {
        return b_found_include ? std::optional<size_t>{} : line_after_last_include;
    }

    const ankerl::unordered_dense::map<std::string, Class>& get_classes() const
    {
        return reflected_classes;
    }

    const ankerl::unordered_dense::map<std::string, Enum>& get_enums() const
    {
        return reflected_enums;
    }

private:
    std::optional<Llp::ParserError> parse_block(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, const ParserContext& context);

    static std::optional<Llp::ParserError> parse_enum_args(const Llp::Tokenizer& block, Enum& data);
    static std::optional<Llp::ParserError> parse_enum_body(const Llp::Tokenizer& block, const Llp::TokenSet& token_set, Enum& data);
    static bool                            parse_check_include(Llp::TextReader& reader, const std::filesystem::path& desired_path);

    std::filesystem::path                            generated_header_include_path;
    std::filesystem::path                            header_path;
    ankerl::unordered_dense::map<std::string, Class> reflected_classes;
    ankerl::unordered_dense::map<std::string, Enum>  reflected_enums;
    size_t                                           line_after_last_include = 1;
    bool                                             b_found_include         = false;
};