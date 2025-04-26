#include "assets/directory_package.hpp"

#include "serialization.hpp"

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
        if (auto serializer = Reflection::Serializer::get(asset->get_class()->id()))
            LOG_WARNING("TODO STORE PACKAGE {}", relative_path.to_string());
        else
            LOG_ERROR("Cannot save {} : no serializer for {}", relative_path.to_string(), asset->get_class()->name());
    }
    else
        LOG_WARNING("Cannot save asset {} : asset is not loaded", relative_path.to_string());
}

ankerl::unordered_dense::set<PackagePath> DirectoryPackage::get_directory_content(const PackagePath& path) const
{
    auto content = Package::get_directory_content(path);
    auto dir     = root / ("." + path.to_string());
    if (!exists(dir) || !std::filesystem::is_directory(dir))
        return content;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
        if (PackagePath::is_valid_path(entry.path()))
            content.emplace(relative(entry.path(), root));
    return content;
}

bool DirectoryPackage::is_directory(const PackagePath& package) const
{
    return Package::is_directory(package) || std::filesystem::is_directory(root / ("." + package.to_string()));
}
}