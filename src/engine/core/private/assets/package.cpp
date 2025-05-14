#include "assets/package.hpp"

#include "assets/asset_registry.hpp"

#include <ranges>

namespace Eng {
    ankerl::unordered_dense::map<std::string, std::unique_ptr<Package> > Package::packages;

    Package::~Package() {
        set_asset_registry(nullptr);
    }

    ankerl::unordered_dense::set<PackagePath> Package::get_directory_content(const PackagePath &path) const {
        if (auto it = loaded_directories.find(path); it != loaded_directories.end())
            return it->second;
        return {};
    }

    bool Package::is_directory(const PackagePath &path) const {
        return loaded_directories.contains(path);
    }

    std::vector<PackagePath> Package::scan() const {
        LOG_DEBUG("TODO");
        return {};
    }

    void Package::force_unload() {
        auto asset_copy = loaded_assets;
        for (auto &asset: asset_copy)
            asset.second.destroy();
        loaded_assets.clear();
    }

    Package *Package::get_transient_package() {
        Package *transient_package = get(PACKAGE_TRANSIENT);
        if (!transient_package) {
            create<TransientPackage>(PACKAGE_TRANSIENT);
            transient_package = get(PACKAGE_TRANSIENT);
        }
        assert(transient_package);
        return transient_package;
    }

    Package *Package::get(const std::string &name) {
        if (auto it = packages.find(name); it != packages.end())
            return it->second.get();
        return {};
    }

    std::vector<std::string> Package::get_all_packages() {
        std::vector<std::string> package_names;
        package_names.reserve(packages.size());
        for (const auto &name: packages | std::views::keys)
            package_names.emplace_back(name);
        return package_names;
    }

    TObjectRef<AssetBase> Package::get_asset(const PackagePath &path) {
        auto it = loaded_assets.find(path);
        return it == loaded_assets.end() ? TObjectRef<AssetBase>{} : it->second;
    }

    std::vector<TObjectRef<AssetBase> > Package::get_loaded_assets() const {
        std::vector<TObjectRef<AssetBase> > assets;
        for (const auto &asset: loaded_assets | std::views::values)
            assets.emplace_back(asset);
        return assets;
    }

    void Package::set_asset_registry(std::shared_ptr<AssetRegistry> new_registry) {
        if (asset_registry)
            asset_registry->on_asset_removed.clear_object(this);
        asset_registry = std::move(new_registry);
        if (asset_registry)
            asset_registry->on_asset_removed.add_object(this, &Package::on_asset_unloaded);
    }

    void Package::create_package_internal(Package *package, std::string name,
                                          std::shared_ptr<AssetRegistry> asset_registry) {
        package->package_name = std::move(name);
        package->set_asset_registry(asset_registry ? std::move(asset_registry) : AssetRegistry::global());
        assert(package->asset_registry);
        packages.insert_or_assign(package->package_name, std::unique_ptr<Package>(package));
    }

    void Package::on_asset_loaded_internal(const PackagePath &path, const TObjectRef<AssetBase> &asset_ptr) {
        if (path.empty())
            LOG_FATAL("Cannot register asset with an empty package path : {} : {}", asset_ptr->get_name(),
                  path.to_string());
        ASSERT(asset_ptr, "Cannot register invalid asset {}", path.to_string());
        load_path(path);
        if (!loaded_assets.emplace(path, asset_ptr).second)
            LOG_FATAL("Could not register asset {} : there is already an other package with the same path {}",
                  asset_ptr->get_name(), path.to_string());

    }

    void Package::on_asset_unloaded(AssetBase *asset) {
        PackagePath removed_path = asset->package.get_path();
        unload_path(removed_path);
        if (asset && asset->package.package_name() == get_name())
            loaded_assets.erase(removed_path);
    }

    void Package::unload_path(const PackagePath &path) {
        if (auto parent = path.parent()) {
            if (auto it = loaded_directories.find(*parent); it != loaded_directories.end()) {
                it->second.erase(path);
                if (it->second.empty())
                    unload_path(*parent);
            }
        }
    }

    void Package::load_path(const PackagePath &path) {
        if (auto parent = path.parent()) {
            if (auto it = loaded_directories.find(*parent); it != loaded_directories.end())
                it->second.insert(path);
            else {
                load_path(*parent);
                loaded_directories.emplace(*parent, ankerl::unordered_dense::set<PackagePath>{}).first->second.
                        insert(path);
            }
        }
    }
}
