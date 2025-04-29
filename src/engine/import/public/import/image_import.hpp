#pragma once
#include "object_ptr.hpp"
#include "assets/asset_base.hpp"

#include <string>
#include <filesystem>


namespace Eng
{
class PackageRef;

namespace Gfx
{
class BufferData;
}

class TextureAsset;

class ImageImport
{
  public:
    static TObjectRef<TextureAsset> load_from_path(const std::filesystem::path& path, const PackageRef& package);
    static TObjectRef<TextureAsset> load_raw(const std::string& file_name, const Gfx::BufferData& raw, const PackageRef& package);
};
} // namespace Eng