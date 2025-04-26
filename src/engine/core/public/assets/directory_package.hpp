#pragma once
#include "package.hpp"

namespace Eng
{
class DirectoryPackage : public Package
{
public:
    DirectoryPackage(std::filesystem::path in_root) : root(std::move(in_root))
    {
        create_directories(root);
    }

    TObjectRef<AssetBase> load(const PackagePath& relative_path) override;
    void                  save(const PackagePath& relative_path) override;

    ankerl::unordered_dense::set<PackagePath> get_directory_content(const PackagePath& path) const override;
    bool                                      is_directory(const PackagePath& package) const override;

private:
    ankerl::unordered_dense::set<PackagePath> directories;

    std::filesystem::path root;
};
}