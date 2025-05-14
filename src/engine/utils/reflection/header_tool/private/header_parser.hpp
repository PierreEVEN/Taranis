#pragma once
#include "llp/file_data.hpp"

#include <filesystem>
#include <ankerl/unordered_dense.h>
#include <vector>
#include <filesystem>

struct ParserContext;

namespace Llp
{
class Parser;
}

namespace Llp
{
struct ParserError;
class TokenizedBlock;
}

class FileReader;

struct TypeDefinition
{
  private:
    std::string                 name;
    std::vector<TypeDefinition> template_args;

  public:
    bool     b_is_global_namespace = false;
    bool     is_const              = false;
    bool     is_ref                = false;
    uint32_t ptr_indirections      = 0;

    const std::string& name_short() const
    {
        return name;
    }

    const std::vector<TypeDefinition>& get_template_args() const
    {
        return template_args;
    }

    std::string full_name_string() const;

    std::optional<Llp::ParserError> try_parse(Llp::Parser& parser, const ParserContext& context);
};

struct ClassDefinition
{
    std::string                                               name;
    std::vector<std::string>                                  parents;
    ankerl::unordered_dense::map<std::string, TypeDefinition> properties;
};

struct ParserContext
{
    std::vector<std::string>                      namespace_stack;
    std::vector<std::shared_ptr<ClassDefinition>> class_stack;

    ParserContext push_namespace(const std::string& last_namespace) const
    {
        ParserContext copy = *this;
        copy.namespace_stack.push_back(last_namespace);
        return copy;
    }

    ParserContext push_class(const ClassDefinition& last_class) const
    {
        ParserContext copy = *this;
        copy.class_stack.push_back(std::make_shared<ClassDefinition>(last_class));
        return copy;
    }
};

class HeaderParser
{
public:

    HeaderParser(const std::string& header_data, std::filesystem::path generated_header_include_path, std::filesystem::path header_path);

    struct ReflectedClass
    {
        ParserContext context;
        size_t        implementation_line;

        std::string              class_path() const;
        std::string              sanitized_class_path() const;
        std::vector<std::string> get_parent_paths() const;
        std::string              class_name() const;
        std::string              namespace_path() const;

        ankerl::unordered_dense::map<std::string, TypeDefinition> properties() const
        {
            if (context.class_stack.empty())
                return {};
            return context.class_stack.back()->properties;
        }
    };

    struct ReflectedEnum
    {
        friend class HeaderParser;
        ParserContext context;

        std::string enum_path() const;
        std::string sanitized_enum_path() const;
        std::string namespace_path() const;

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

    std::optional<size_t> get_include_line_to_add() const
    {
        return b_found_include ? std::optional<size_t>{} : line_after_last_include;
    }

    const ankerl::unordered_dense::map<std::string, ReflectedClass>& get_classes() const
    {
        return reflected_classes;
    }

    const ankerl::unordered_dense::map<std::string, ReflectedEnum>& get_enums() const
    {
        return reflected_enums;
    }

private:
    std::optional<Llp::ParserError>        parse_block(const Llp::TokenizedBlock& block, const ParserContext& context);
    static std::optional<Llp::ParserError> parse_enum_args(const Llp::TokenizedBlock& block, ReflectedEnum& data);
    static std::optional<Llp::ParserError> parse_enum_body(const Llp::TokenizedBlock& block, ReflectedEnum& data);
    static bool                            parse_check_include(Llp::TextReader& reader, const std::filesystem::path& desired_path);

    void error(const std::string& message, size_t line, size_t column) const;

    std::filesystem::path                                     generated_header_include_path;
    std::filesystem::path                                     header_path;
    ankerl::unordered_dense::map<std::string, ReflectedClass> reflected_classes;
    ankerl::unordered_dense::map<std::string, ReflectedEnum>  reflected_enums;
    size_t                                                    line_after_last_include = 1;
    bool                                                      b_found_include         = false;
};