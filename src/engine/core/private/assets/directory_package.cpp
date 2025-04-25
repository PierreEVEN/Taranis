#include "assets/directory_package.hpp"

namespace Eng
{
TObjectRef<AssetBase> DirectoryPackage::load(const PackagePath& relative_path)
{
    (void)relative_path;
    LOG_WARNING("TODO LOAD PACKAGE");
    return {};
}

void DirectoryPackage::save(const PackagePath& relative_path)
{
    if (auto asset = get_asset(relative_path))
    {
        LOG_WARNING("TODO STORE PACKAGE {}", relative_path.to_string());
    }
    else
        LOG_WARNING("Cannot save asset {} : asset is not loaded", relative_path.to_string());
}

std::vector<PackagePath> DirectoryPackage::get_directory_content(const PackagePath& path) const
{
    std::vector<PackagePath> files;
    auto                     dir = root / ("." + path.to_string());
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