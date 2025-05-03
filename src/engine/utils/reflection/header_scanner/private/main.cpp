#include "../../../../../../../../.xmake/packages/u/unordered_dense/v4.5.0/4c8261380ae444c6a2528b3a30dc2a73/include/ankerl/unordered_dense.h"

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

class TimeCache
{
public:
    std::time_t get_time_for(const std::filesystem::path& file_path, bool is_dep_file)
    {
        if (auto it = time_cache.find(file_path); it != time_cache.end())
            return it->second;
        if (is_dep_file)
            auto data = nlohmann::from_json(""); //TODO : handle depfile
        else
            std::time_t modification_time = get_last_modification_time(file_path);
        time_cache.emplace(file_path, modification_time);
        return modification_time;
    }

    static std::time_t get_last_modification_time(const std::filesystem::path& file_path) {
        // Check if the file exists
        if (std::filesystem::exists(file_path)) {
            // Get the last write time (modification time) of the file
            auto ftime = std::filesystem::last_write_time(file_path);

            // Convert to system clock time
            auto sctp = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));

            return sctp;
        }

        // If file doesn't exist, return 0
        return 0;
    }

private:
    ankerl::unordered_dense::map<std::filesystem::path, std::time_t> time_cache;
};


int main(int argc, char** argv)
{
    if (argc != 5)
    {
        std::cerr << "[Header tool] Syntax error. Expected 'header_tool <scanned_header> <target_source> <target_header> <include_path>'. Got " << argc << " params \n";
        return -1;
    }

    struct HeaderData {
        std::filesystem::path header_path;
        std::filesystem::path dep_path;
        std::filesystem::path cpp_path;
        std::filesystem::path object_path;
        std::vector<std::string> arguments;
    };
    std::filesystem::path header_tool_path;

    std::vector<HeaderData> headers;

    TimeCache time_cache;

    for (const auto& tool : headers)
    {
        if (!exists(tool.object_path))
        {
            auto last_o_time = time_cache.get_time_for(tool.object_path, false);
            auto last_d_time = time_cache.get_time_for(tool.dep_path, true);

            if (last_d_time == 0 || last_d_time > last_o_time)
                continue;

            // TODO run header tool on current header : header_tool file.gen.cpp ...

            if (!exists(tool.object_path))
                continue;
        }
        auto last_o_time = time_cache.get_time_for(tool.object_path, false);
        auto last_d_time = time_cache.get_time_for(tool.dep_path, true);

        if (last_d_time == 0 || last_d_time > last_o_time)
            continue;

        // TODO compile here : /usr/bin/gcc ...
    }
    return 0;
}