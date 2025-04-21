#include "path.hpp"

#include "logger.hpp"

#include <filesystem>

Path::Path(const std::filesystem::path& fs_path) : Path(fs_path.wstring())
{
}

Path::Path(const std::string& in_mount_point, const std::filesystem::path& fs_path) : Path(in_mount_point, fs_path.wstring())
{
}

Path::Path(const std::wstring& source)
{
    std::wstring current;

    for (size_t i = 0; i < source.length(); ++i)
    {
        const auto& chr = source[i];

        // Detect '..'
        if (chr == '.')
        {
            if (i + 1 < source.length() && source[i + 1] == L'.' && (i + 2 < source.length() && (source[i + 2] == L'/' || source[i + 2] == L'\\') || i + 2 == source.length()))
            {
                LOG_FATAL(".. is not allowed in {}, moving up in Path is not permitted.", from_wstring(source));
            }
        }

        // Detect '://'
        if (chr == L':')
        {

            if (i + 1 < source.length() && (source[i + 1] == L'/' || source[i + 1] == L'\\') &&
                i + 2 < source.length() && (source[i + 2] == L'/' || source[i + 2] == L'\\'))
            {
                path_mount_point = from_wstring(current);
                current.clear();
                i += 2;
                continue;
            }
        }
        // Detect '/'
        else if (chr == L'/' || chr == L'\\')
        {
            if (!current.empty())
                path.emplace_back(current);
            current.clear();
            continue;
        }

        current += chr;
    }
    if (!current.empty())
        path.emplace_back(current);
}

Path::Path(const std::string& in_mount_point, const std::wstring& source) : Path(source)
{
    if (path_mount_point.empty())
        path_mount_point = in_mount_point;
}

Path::Path(std::string in_mount_point, const std::vector<std::wstring>& source) : path_mount_point(std::move(in_mount_point)), path(source)
{
}

bool Path::exists() const
{
    return std::filesystem::exists(resolve());
}

void Path::register_mount_point(const std::string& mount_point, const Path& location)
{
    mount_points.insert_or_assign(mount_point, location);
}

std::wstring Path::resolve() const
{
    std::wstring formated_path;
    for (const auto& i : path)
        formated_path += L"/" + i;

    if (is_absolute())
    {
        return formated_path;
    }

    auto it = mount_points.find(path_mount_point);
    if (it == mount_points.end())
        LOG_FATAL("Failed to resolve path {}. There is no mount point named {}", from_wstring(formated_path), path_mount_point);

    return it->second.resolve() + formated_path;
}

std::string Path::to_string() const
{
    return from_wstring(to_wstring());
}

std::wstring Path::to_wstring() const
{
    std::wstring formated_path = is_absolute() ? L"" : from_string(path_mount_point + "://");
    for (const auto& i : path)
        formated_path += L"/" + i;
    return formated_path;
}

std::string Path::resolve_simple() const
{
    return from_wstring(resolve());
}

std::optional<Path> Path::parent() const
{
    if (path.empty())
    {
        return {};
    }

    std::vector<std::wstring> cpy = path;
    cpy.pop_back();
    return Path(path_mount_point, cpy);
}

std::string Path::from_wstring(const std::wstring& base)
{
    std::string str;
    size_t      size;
    str.resize(base.length());
    wcstombs_s(&size, &str[0], str.size() + 1, base.c_str(), base.size());
    return str;
}

std::wstring Path::from_string(const std::string& base)
{
    std::wstring wstr;
    size_t       size;
    wstr.resize(base.length());
    mbstowcs_s(&size, &wstr[0], wstr.size() + 1, base.c_str(), base.size());
    return wstr;
}