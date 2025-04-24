#pragma once
#include "object_ptr.hpp"

#include <string>
#include <vector>

#define PACKAGE_ENGINE "Engine"

namespace Eng
{
class Package;
class AssetRegistry;
class AssetBase;

class PackagePath
{
    friend struct std::hash<Eng::PackagePath>;

public:
    PackagePath() = default;
    PackagePath(const std::filesystem::path& fs_path);
    PackagePath(const std::string& str_path);
    PackagePath(const char* chr_path);

    bool operator==(const PackagePath&) const = default;

    std::string to_string() const;

    std::string name() const
    {
        return path.empty() ? "" : path.back();
    }

private:
    std::vector<std::string> path;
};

class PackageRef
{
public:
    PackageRef() = default;

    PackageRef(std::string package_name, PackagePath path) : internal_package(std::move(package_name)), internal_path(std::move(path))
    {
    }

    operator bool() const
    {
        return !is_transient_package();
    }

    static PackageRef transient()
    {
        return {};
    }

    bool is_transient_package() const
    {
        return internal_package.empty();
    }

    const PackagePath& get_path() const
    {
        return internal_path;
    }

    Package* package() const;

    std::string to_string() const;

private:
    std::string internal_package;
    PackagePath internal_path;
};
}

template <> struct std::hash<Eng::PackagePath>
{
    size_t operator()(const Eng::PackagePath& ctx) const noexcept
    {
        size_t hash = 0;
        for (const auto& item : ctx.path)
            hash += std::hash<std::string>()(item);
        return hash;
    }
};

namespace Eng
{
class Package
{
    friend class AssetFactory;

public:
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

    static Package*                 get_transient_package();
    static Package*                 get(const std::string& name);
    static std::vector<std::string> get_all_packages();

    TObjectRef<AssetBase> get_asset(const PackagePath& path)
    {
        auto it = loaded_assets.find(path);
        return it == loaded_assets.end() ? TObjectRef<AssetBase>{} : it->second;
    }

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

class DirectoryPackage : public Package
{
public:
    DirectoryPackage(std::filesystem::path in_root) : root(std::move(in_root))
    {
        create_directories(root);
    }

    TObjectRef<AssetBase> load(const PackagePath& relative_path) override;
    void                  save(const PackagePath& relative_path) override;

    std::vector<PackagePath> scan() const override
    {
        return scan_dir(root, root);
    }

    std::vector<PackagePath> get_directory_content(const PackagePath& path) const override;
    bool                     is_directory(const PackagePath& package) const override;

private:
    static std::vector<PackagePath> scan_dir(const std::filesystem::path& path, const std::filesystem::path& root);

    std::filesystem::path root;
};
} // namespace Eng