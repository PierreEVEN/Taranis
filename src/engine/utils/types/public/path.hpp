#pragma once
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>

namespace std::filesystem
{
class path;
}

class Path
{
public:
    Path() = default;
    Path(const std::filesystem::path& fs_path);
    Path(const std::string& mount_point, const std::filesystem::path& fs_path);
    Path(const std::wstring& source);
    Path(const std::string& mount_point, const std::wstring& source);
    Path(std::string mount_point, const std::vector<std::wstring>& source);

    bool is_web() const
    {
        return path_mount_point == "http" || path_mount_point == "https://";
    }

    bool is_absolute() const
    {
        return path_mount_point.empty();
    }

    bool exists() const;

    static void register_mount_point(const std::string& mount_point, const Path& location);

    const std::string& mount_point() const
    {
        return path_mount_point;
    }

    std::string to_string() const;

    std::wstring to_wstring() const;

    std::wstring resolve() const;

    std::string resolve_simple() const;

    std::optional<Path> parent() const;

private:
    static std::string  from_wstring(const std::wstring& base);
    static std::wstring from_string(const std::string& base);

    static ankerl::unordered_dense::map<std::string, Path> mount_points;

    std::string               path_mount_point;
    std::vector<std::wstring> path;
};