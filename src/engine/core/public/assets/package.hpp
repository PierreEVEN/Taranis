#pragma once
#include "object_ptr.hpp"

#include <string>
#include <vector>

#define PACKAGE_ENGINE "Engine"

namespace Eng
{
class AssetBase;


class PackagePath
{
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

class Package
{
public:
    template <typename T, typename... Args> static void create(std::string in_name, Args&&... args)
    {
        T* package    = new T(std::forward<Args>(args)...);
        package->name = std::move(in_name);
        packages.insert_or_assign(package->name, std::unique_ptr<T>(package));
    }

    static Package* get(const std::string& name);

    virtual TObjectPtr<AssetBase>    load(const PackagePath& relative_path) = 0;
    virtual void                     store(TObjectRef<AssetBase> object, const PackagePath& relative_path) = 0;
    virtual std::vector<PackagePath> get_directory_content(const PackagePath& path = "") const = 0;
    virtual bool                     is_directory(const PackagePath& package) const = 0;
    virtual std::vector<PackagePath> scan() const = 0;

    static std::vector<std::string> get_all_packages();

    const std::string& get_name() const
    {
        return name;
    }

private:
    static ankerl::unordered_dense::map<std::string, std::unique_ptr<Package>> packages;

    std::string name;
};

class DirectoryPackage : public Package
{
public:
    DirectoryPackage(std::filesystem::path in_root) : root(std::move(in_root))
    {
        create_directories(root);
    }

    TObjectPtr<AssetBase> load(const PackagePath& relative_path) override;
    void                  store(TObjectRef<AssetBase> object, const PackagePath& relative_path) override;

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