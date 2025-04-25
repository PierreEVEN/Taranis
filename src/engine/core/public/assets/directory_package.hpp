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
}