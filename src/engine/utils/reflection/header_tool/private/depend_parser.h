#pragma once
#include <filesystem>
#include <optional>
#include <vector>

#include "llp/tokens.hpp"


class DependParser {
public:
    DependParser(const std::filesystem::path& path);

private:

    std::optional<Llp::ParserError> parse(const std::filesystem::path& path);
    std::vector<std::filesystem::path> dependencies;
};
