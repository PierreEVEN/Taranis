#pragma once

#define PACKAGE_ENGINE "Engine"
#define PACKAGE_TRANSIENT "~Transient"
#include <functional>
#include <optional>
#include <string>
#include <filesystem>

namespace Eng
{
class Package;

class PackagePath
{
    friend struct std::hash<Eng::PackagePath>;

  public:
    PackagePath() = default;
    PackagePath(const std::filesystem::path& fs_path);
    PackagePath(const std::string& str_path);
    PackagePath(const char* chr_path);

    static bool is_valid_path(const std::filesystem::path& fs_path);

    bool operator==(const PackagePath&) const = default;

    std::string to_string() const;

    std::string name() const
    {
        return path.empty() ? "" : path.back();
    }

    std::optional<PackagePath> parent() const;

    bool empty() const
    {
        return path.empty();
    }

  private:
    std::vector<std::string> path;
};
} // namespace Eng

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

class PackageRef
{
  public:
    PackageRef() = default;

    PackageRef(std::string package_name, PackagePath path) : internal_package(std::move(package_name)), internal_path(std::move(path))
    {
    }

    operator bool() const
    {
        return !internal_package.empty();
    }

    static PackageRef transient(const std::string& path)
    {
        return {PACKAGE_TRANSIENT, path};
    }

    bool is_transient_package() const
    {
        return internal_package == PACKAGE_TRANSIENT;
    }

    const PackagePath& get_path() const
    {
        return internal_path;
    }

    Package* package() const;

    const std::string& package_name() const
    {
        return internal_package;
    }

    std::string to_string() const;

  private:
    std::string internal_package;
    PackagePath internal_path;
};
}