#include "assets/package_ref.hpp"

#include "logger.hpp"
#include "assets/package.hpp"

#include <filesystem>

namespace Eng
{

PackagePath::PackagePath(const std::filesystem::path& fs_path) : PackagePath(fs_path.lexically_normal().generic_string())
{
}

PackagePath::PackagePath(const std::string& str_path) : PackagePath(str_path.c_str())
{
}

PackagePath::PackagePath(const char* chr_path)
{
    std::string current;
    for (size_t i = 0; chr_path[i] != '\0'; ++i)
    {
        char chr = chr_path[i];

        if (chr == '/')
        {
            if (!current.empty())
            {
                path.emplace_back(current);
                current.clear();
            }
            continue;
        }

        if (!(std::isalnum(chr) || chr == '.' || chr == '_' || chr == '-'))
            LOG_FATAL("Character '{}' not allowed in package path : {}", chr, chr_path)

        if (chr == '.' && chr_path[i + 1] != '\0' && chr_path[i + 1] == '.')
            LOG_FATAL("Double dot '..' is not allowed in path : {}", chr_path)

        current += chr;
    }

    if (!current.empty())
        path.emplace_back(current);
}

std::string PackagePath::to_string() const
{
    std::string str;
    for (const auto& p : path)
        str += "/" + p;
    return str;
}

Package* PackageRef::package() const
{
    if (internal_package == PACKAGE_TRANSIENT)
        return Package::get_transient_package();

    return Package::get(internal_package);
}

std::string PackageRef::to_string() const
{
    return std::format("{}:/{}", internal_package, internal_path.to_string());
}

}