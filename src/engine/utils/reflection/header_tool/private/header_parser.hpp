#pragma once
#include "cpp_objects/refl_class.h"
#include "cpp_objects/refl_enum.h"
#include "cpp_objects/refl_template.hpp"

#include <ankerl/unordered_dense.h>
#include <filesystem>
#include <llp/token.hpp>
#include <vector>

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

    std::optional<TemplateDeclaration> last_template_declaration;

    std::filesystem::path                                             generated_header_include_path;
    std::filesystem::path                                             header_path;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<Class>> reflected_classes;
    ankerl::unordered_dense::map<std::string, Enum>                   reflected_enums;
    size_t                                                            line_after_last_include = 1;
    bool                                                              b_found_include         = false;
};