#include "assets/package.hpp"

#include "assets/asset_registry.hpp"

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

Package* PackageRef::package() const
{
    if (internal_package.empty())
        return Package::get_transient_package();

    return Package::get(internal_package);
}

std::string PackageRef::to_string() const
{
    return std::format("{}:/{}", internal_package, internal_path.to_string());
}

ankerl::unordered_dense::map<std::string, std::unique_ptr<Package>> Package::packages;

Package* Package::get_transient_package()
{
    Package* transient_package = get("");
    if (!transient_package)
    {
        create<TransientPackage>("");
        transient_package = get("");
    }
    assert(transient_package);
    return transient_package;
}

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

std::vector<TObjectRef<AssetBase>> Package::get_loaded_assets() const
{
    std::vector<TObjectRef<AssetBase>> assets;
    for (const auto& asset : loaded_assets | std::views::values)
        assets.emplace_back(asset);
    return assets;
}

void Package::create_package_internal(Package* package, std::string name, std::shared_ptr<AssetRegistry> asset_registry)
{
    package->package_name   = std::move(name);
    package->asset_registry = asset_registry ? std::move(asset_registry) : AssetRegistry::global();
    assert(package->asset_registry);
    packages.insert_or_assign(package->package_name, std::unique_ptr<Package>(package));
}

void Package::on_asset_loaded_internal(const PackagePath& path, const TObjectPtr<AssetBase>& asset_ptr)
{
    loaded_assets.emplace(path, asset_ptr);
}
}