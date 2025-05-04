
#include <ankerl/unordered_dense.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <vector>

class DepCache
{
  public:
    std::time_t get_time_for(const std::filesystem::path& file_path, bool is_dep_file)
    {
        if (auto it = time_cache.find(file_path); it != time_cache.end())
            return it->second;

        std::time_t modification_time = 0;
        if (is_dep_file)
        {
            if (exists(file_path))
            {
                try
                {
                    std::vector<std::filesystem::path> deps;
                    if (!parse_dep_file(file_path, deps))
                        exit(-1);
                    for (const auto& dep : deps)
                    {
                        auto dep_time = get_last_modification_time(file_path);
                        if (dep_time > modification_time)
                            modification_time = dep_time;
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Failed to parse depend file '" << file_path << "' : " << e.what() << "\n";
                    exit(-1);
                }
            }
        }
        else
            modification_time = get_last_modification_time(file_path);
        time_cache.emplace(file_path, modification_time);
        return modification_time;
    }

    static std::time_t get_last_modification_time(const std::filesystem::path& file_path)
    {
        // Check if the file exists
        if (std::filesystem::exists(file_path))
        {
            // Get the last write time (modification time) of the file
            auto ftime = std::filesystem::last_write_time(file_path);

            // Convert to system clock time
            auto sctp = std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ftime));

            return sctp;
        }

        // If file doesn't exist, return 0
        return 0;
    }

    bool is_up_to_date(const std::filesystem::path& dep_path, const std::filesystem::path& object_path)
    {
        auto last_o_time = get_time_for(object_path, false);
        auto last_d_time = get_time_for(dep_path, true);

        return last_d_time != 0 && last_d_time < last_o_time;
    }

  private:
    static bool parse_dep_file(const std::filesystem::path& file_path, std::vector<std::filesystem::path>& deps)
    {
        std::ifstream f(file_path);
        char          c;
        int           step = 0;
        while (f.get(c))
        {
            while (std::isspace(c) && f.get(c))
            {
            }

            switch (step)
            {
            case 0:
            {
                if (c == '{')
                    step++;
                continue;
            }
            case 1:
            {
                if (c == 'f' && f.get(c) && c == 'i' && f.get(c) && c == 'l' && f.get(c) && c == 'e' && f.get(c) && c == 's')
                    step++;
                else
                    return true;
                continue;
            }
            case 2:
            {
                if (c == '=')
                    step++;
                else
                {
                    std::cerr << "Cannot read dep file " << file_path << " : expected '='\n";
                    return false;
                }
                continue;
            }
            case 3:
            {
                if (c == '{')
                    step++;
                else
                {
                    std::cerr << "Cannot read dep file " << file_path << " : expected '{'\n";
                    return false;
                }
                continue;
            }
            case 4:
            {
                std::string current;
                bool        b_done   = false;
                bool        b_record = false;
                do
                {
                    if (c == '"')
                    {
                        if (b_record)
                        {
                            b_record = false;
                            deps.emplace_back(current);
                        }
                        else
                        {
                            current.clear();
                            b_record = true;
                        }
                    }
                    else if (c == '}')
                    {
                        b_done = true;
                        break;
                    }
                    else
                        current += c;
                } while (f.get(c));
                if (!b_done)
                    std::cerr << "Failed to read dep file " << file_path << " : parse failed\n";
                return b_done;
            }
            default:;
                std::cerr << "unexpected step\n";
                return false;
            }
            std::cerr << "unexpected char while parsing dep file '" << file_path << "' : '" << c << "'\n";
            return false;
        }
        return true;
    }

    ankerl::unordered_dense::map<std::filesystem::path, std::time_t> time_cache;
};

class Options
{
  public:
    bool parse(int argc, char** argv)
    {
        // Parse arguments
        std::vector<std::string> args;
        args.reserve(argc - 1);
        for (int i = 1; i < argc; i++)
            args.emplace_back(argv[i]);

        for (auto arg = args.cbegin(); arg != args.cend(); ++arg)
        {
            if ((*arg)[0] != '-')
            {
                std::cerr << "Expected command parameter (-f, -t, -c)\n";
                return false;
            }
            if (arg->size() != 2)
            {
                std::cerr << "Invalid argument: " << (*arg) << ". expected one character after '-'\n";
                return false;
            }

            switch ((*arg)[1])
            {
            case 'f': // input file : .hpp / include_path / .hpp.d / .gen.hpp / .gen.cpp / .gen.cpp.o
            {
                File f;
                if (!expect(arg, args.cend()))
                    return false;
                f.header_path = *arg;
                if (!expect(arg, args.cend()))
                    return false;
                f.include_path = *arg;
                if (!expect(arg, args.cend()))
                    return false;
                f.depend_path = *arg;
                if (!expect(arg, args.cend()))
                    return false;
                f.gen_hpp_path = *arg;
                if (!expect(arg, args.cend()))
                    return false;
                f.gen_cpp_path = *arg;
                if (!expect(arg, args.cend()))
                    return false;
                f.gen_object_path = *arg;
                files.emplace_back(f);
                break;
            }
            case 't': // header tool path
            {
                if (!expect(arg, args.cend()))
                    return false;
                header_tool_path = *arg;
                break;
            }
            case 'c': // compiler and every other args
            {
                while (++arg != args.cend())
                    compile_args.emplace_back(*arg);
                break;
            }
            default:
            {
                std::cerr << "Unhandled argument " << *arg << "\n";
                return false;
            }
            }
            if (arg == args.cend())
                break;
        }

        if (!exists(header_tool_path))
        {
            std::cerr << "Header tool at '" << header_tool_path << "' does not exists\n";
            return false;
        }

        return true;
    }

    struct File
    {
        std::filesystem::path header_path;
        std::filesystem::path include_path;
        std::filesystem::path depend_path;
        std::filesystem::path gen_hpp_path;
        std::filesystem::path gen_cpp_path;
        std::filesystem::path gen_object_path;
    };
    std::vector<File>        files;
    std::filesystem::path    header_tool_path;
    std::vector<std::string> compile_args;

  private:
    static bool expect(std::vector<std::string>::const_iterator& it, const std::vector<std::string>::const_iterator& end)
    {
        const std::string& last = *it;
        if (++it == end)
        {
            std::cerr << "Error : expected argument after '" << last << "'\n";
            return false;
        }
        return true;
    }
};

int main(int argc, char** argv)
{
    Options opts;
    if (!opts.parse(argc, argv))
        return -1;

    DepCache dep_cache;
    // Generate .gen.hpp first (required to compile .gen.cpp then)
    for (const auto& file : opts.files)
    {
        // If header was not modified, then skip it
        if (dep_cache.is_up_to_date(file.depend_path, file.gen_object_path))
        {
            std::cout << file.header_path << " UP TO DATE\n";
            continue;
        }

        // Run header tool to generate .gen.hpp and .gen.cpp files
        std::cout << "GENERATE " << file.header_path << "\n";
        if (const auto code = std::system(std::format("{} {} {} {} {}", opts.header_tool_path.string(), file.header_path.string(), file.gen_cpp_path.string(), file.gen_hpp_path.string(), file.include_path.string()).c_str()))
        {
            std::cerr << "Failed to generate " << file.gen_cpp_path << " : Exit " << code << "\n";
            return -1;
        }
    }

    // Recompile outdated .gen.cpp files
    for (const auto& file : opts.files)
    {
        // If there is no .gen.cpp
        if (!std::filesystem::exists(file.gen_cpp_path))
            continue;

        // if .gen.cpp.o is up to date : skip it
        if (dep_cache.is_up_to_date(file.depend_path, file.gen_object_path))
            continue;

        std::string cmd;
        for (const auto& arg : opts.compile_args)
            cmd += arg + " ";

        std::cout << "COMPILE " << file.header_path << "\n";
        if (const auto code = std::system(cmd.c_str()))
        {
            std::cerr << "Failed to compile " << file.gen_cpp_path << " : Exit " << code << "\n";
            return -1;
        }
    }

    return 0;
}