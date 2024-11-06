#pragma once
#include "file_data.hpp"
#include "lexical_analyzer.hpp"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace std::filesystem
{
class path;
}

class FileData;

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

    HeaderParser(const std::shared_ptr<FileData>& header_data, const std::filesystem::path& generated_header_include_path, const std::filesystem::path& header_path);

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

    const std::unordered_map<std::string, ReflectedClass>& get_classes() const
    {
        return reflected_classes;
    }

  private:
    Block tokenized_file;

    void        parse_block(Block& block, const ParserContext& context);
    static bool parse_check_include(FileData::Reader& reader, const std::filesystem::path& desired_path);

    void error(const std::string& message, size_t line, size_t column) const;

    std::filesystem::path                           generated_header_include_path;
    std::filesystem::path                           header_path;
    std::unordered_map<std::string, ReflectedClass> reflected_classes;
    size_t                                          line_after_last_include = 0;
    bool                                            b_found_include         = false;
};