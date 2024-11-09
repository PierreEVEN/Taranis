#pragma once
#include "object_ptr.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace std::filesystem
{
class path;
}

namespace Eng
{
namespace Gfx
{
class BufferData;
}

class TextureAsset;

class StbImporter
{
  public:
    static TObjectRef<TextureAsset> load_from_path(const std::filesystem::path& path);
    static TObjectRef<TextureAsset> load_raw(const std::string& file_name, const Gfx::BufferData& raw);
};
} // namespace Eng