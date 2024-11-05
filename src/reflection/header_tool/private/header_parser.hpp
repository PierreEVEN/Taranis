#pragma once
#include "file_data.hpp"

#include <memory>
#include <vector>

class Include;

namespace std::filesystem
{
class path;
}

class FileData;

class HeaderParser
{
public:
    HeaderParser(const std::shared_ptr<FileData>& header_data, const std::filesystem::path& relative_path);

    struct ReflectedClass
    {
        std::string class_name;
        size_t      implementation_line;
    };

    std::optional<size_t> get_include_line_to_add() const
    {
        return b_found_include ? std::optional<size_t>{} : line_after_last_include;
    }

    const std::vector<ReflectedClass>& get_classes() const
    {
        return reflected_classes;
    }

private:
    static bool parse_check_include(FileData::Reader& reader, const std::filesystem::path& desired_path);
    size_t                      line_after_last_include = 0;
    std::vector<ReflectedClass> reflected_classes;
    bool b_found_include = false;
};