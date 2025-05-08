#pragma once
#include <filesystem>
#include <optional>
#include <vector>

#include "llp/tokens.hpp"


class DependParser {
public:
    explicit DependParser(const std::filesystem::path& path);

    [[nodiscard]] std::time_t get_last_write_time() const;
private:

    std::optional<Llp::ParserError> parse(const std::filesystem::path& path);
    std::vector<std::filesystem::path> dependencies;
};
