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

        if (!(std::isalnum(chr) || chr == '_' || chr == '-'))
            LOG_FATAL("Character '{}' not allowed in package path : {}", chr, chr_path)

        current += chr;
    }

    if (!current.empty())
        path.emplace_back(current);
}

bool PackagePath::is_valid_path(const std::filesystem::path& fs_path)
{
    for (const auto& item : fs_path)
    {
        if (item.string() == ".")
            continue;
        for (const auto& chr : item.string())
            if (!(isalnum(chr) || chr == '-' || chr == '_'))
                return false;
    }
    return true;
}

std::string PackagePath::to_string() const
{
    std::string str;
    for (const auto& p : path)
        str += "/" + p;
    return str;
}

std::optional<PackagePath> PackagePath::parent() const
{
    if (path.empty())
        return {};
    PackagePath path_copy = *this;
    path_copy.path.pop_back();
    return path_copy;
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