#pragma once
#include "llp/file_data.hpp"
#include "llp/lexical_analyzer.hpp"

#include <filesystem>
#include <memory>
#include <ankerl/unordered_dense.h>
#include <vector>

namespace std::filesystem
{
class path;
}

class FileReader;

struct ClassDefinition
{
    std::string              name;
    std::vector<std::string> parents;
};

class HeaderParser
{
  public:
    struct ParserContext
    {
        std::vector<std::string>     namespace_stack;
        std::vector<ClassDefinition> class_stack;

        ParserContext push_namespace(const std::string& last_namespace) const
        {
            ParserContext copy = *this;
            copy.namespace_stack.push_back(last_namespace);
            return copy;
        }

        ParserContext push_class(const ClassDefinition& last_class) const
        {
            ParserContext copy = *this;
            copy.class_stack.push_back(last_class);
            return copy;
        }
    };

    HeaderParser(const std::shared_ptr<FileReader>& header_data, std::filesystem::path generated_header_include_path, std::filesystem::path header_path);

    struct ReflectedClass
    {
        ParserContext context;
        size_t        implementation_line;

        std::string              class_path() const;
        std::string              sanitized_class_path() const;
        std::vector<std::string> get_parent_paths() const;
        std::string              class_name() const;
        std::string              namespace_path() const;
    };

    std::optional<size_t> get_include_line_to_add() const
    {
        return b_found_include ? std::optional<size_t>{} : line_after_last_include;
    }

    const ankerl::unordered_dense::map<std::string, ReflectedClass>& get_classes() const
    {
        return reflected_classes;
    }

  private:
    TokenizerBlock tokenized_file;

    void        parse_block(TokenizerBlock& block, const ParserContext& context);
    static bool parse_check_include(TextReader& reader, const std::filesystem::path& desired_path);

    void error(const std::string& message, size_t line, size_t column) const;

    std::filesystem::path                           generated_header_include_path;
    std::filesystem::path                           header_path;
    ankerl::unordered_dense::map<std::string, ReflectedClass> reflected_classes;
    size_t                                          line_after_last_include = 0;
    bool                                            b_found_include         = false;
};