#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"
#include "package_ref.hpp"

#include <string>
#include <vector>

namespace Eng
{
class Package;
class AssetRegistry;

class Package
{
    friend class AssetFactory;

public:
    virtual ~Package();

    template <typename T, typename... Args> static void create(std::shared_ptr<AssetRegistry> custom_registry, std::string in_name, Args&&... args)
    {
        Package::create_package_internal(new T(std::forward<Args>(args)...), std::move(in_name), std::move(custom_registry));
    }

    template <typename T, typename... Args> static void create(std::string in_name, Args&&... args)
    {
        Package::create_package_internal(new T(std::forward<Args>(args)...), std::move(in_name), nullptr);
    }

    virtual TObjectRef<AssetBase>    load(const PackagePath& relative_path) = 0;
    virtual void                     save(const PackagePath& relative_path) = 0;
    virtual std::vector<PackagePath> get_directory_content(const PackagePath& path = "") const = 0;
    virtual bool                     is_directory(const PackagePath& package) const = 0;
    virtual std::vector<PackagePath> scan() const = 0;

    void force_unload();

    static Package*                 get_transient_package();
    static Package*                 get(const std::string& name);
    static std::vector<std::string> get_all_packages();

    TObjectRef<AssetBase> get_asset(const PackagePath& path);

    const std::string& get_name() const
    {
        return package_name;
    }

    std::vector<TObjectRef<AssetBase>> get_loaded_assets() const;

    AssetRegistry& get_asset_registry() const
    {
        if (!asset_registry)
            LOG_FATAL("Package '{}' does not point to a valid asset registry which should never happen !", package_name)
        return *asset_registry;
    }

protected:
    static void create_package_internal(Package* package, std::string name, std::shared_ptr<AssetRegistry> asset_registry);

    void on_asset_loaded_internal(const PackagePath& path, const TObjectRef<AssetBase>& asset_ptr);

private:
    void set_asset_registry(std::shared_ptr<AssetRegistry> new_registry);
  void on_asset_registry_removed(AssetBase* asset);

    static ankerl::unordered_dense::map<std::string, std::unique_ptr<Package>> packages;

    ankerl::unordered_dense::map<PackagePath, TObjectRef<AssetBase>> loaded_assets;
    std::shared_ptr<AssetRegistry>                                   asset_registry;
    std::string                                                      package_name;
};

class TransientPackage : public Package
{
public:
    TObjectRef<AssetBase> load(const PackagePath&) override
    {
        LOG_ERROR("Cannot load from a transient package");
        return {};
    }

    void save(const PackagePath&) override
    {
        LOG_ERROR("Cannot save into a transient package");
    }

    std::vector<PackagePath> get_directory_content(const PackagePath&) const override
    {
        return {};
    }

    bool is_directory(const PackagePath&) const override
    {
        return false;
    }

    std::vector<PackagePath> scan() const override
    {
        return {};
    }
};
} // namespace Eng