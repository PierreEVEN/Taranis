#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace std::filesystem
{
class path;
}

namespace Engine
{
class TextureAsset;

class StbImporter
{
  public:
    static TextureAsset* load_from_path(const std::filesystem::path& path);
    static TextureAsset* load_raw(const std::string& file_name, const std::vector<uint8_t>& path);
};
} // namespace Engine