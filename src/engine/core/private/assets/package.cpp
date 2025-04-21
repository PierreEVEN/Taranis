#include "assets/package.hpp"

#include <ranges>

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

ankerl::unordered_dense::map<std::string, std::unique_ptr<Package>> Package::packages;
Package* Package::get(const std::string& name)
{
    if (auto it = packages.find(name); it != packages.end())
        return it->second.get();
    return {};
}

std::vector<std::string> Package::get_all_packages()
{
    std::vector<std::string> package_names;
    package_names.reserve(packages.size());
    for (const auto& name : packages | std::views::keys)
        package_names.emplace_back(name);
    return package_names;
}

TObjectPtr<AssetBase> DirectoryPackage::load(const PackagePath& relative_path)
{
    (void)relative_path;
    LOG_WARNING("TODO LOAD PACKAGE");
    return {};
}

void DirectoryPackage::store(TObjectRef<AssetBase> object, const PackagePath& relative_path)
{
    LOG_WARNING("TODO STORE PACKAGE");
    (void)object;
    (void)relative_path;
}

std::vector<PackagePath> DirectoryPackage::get_directory_content(const PackagePath& path) const
{
    std::vector<PackagePath> files;
    auto                     dir  = root / ("." + path.to_string());
    if (!exists(dir) || !std::filesystem::is_directory(dir))
        return {};
    for (const auto& entry : std::filesystem::directory_iterator(dir))
        files.emplace_back(relative(entry.path(), root));
    return files;
}

bool DirectoryPackage::is_directory(const PackagePath& package) const
{
    return std::filesystem::is_directory(root / ("." + package.to_string()));
}

std::vector<PackagePath> DirectoryPackage::scan_dir(const std::filesystem::path& path, const std::filesystem::path& root)
{
    std::vector<PackagePath> files;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_directory())
            scan_dir(path, root);
        else
            files.emplace_back(relative(entry.path(), root));
    }
    return files;
}
}