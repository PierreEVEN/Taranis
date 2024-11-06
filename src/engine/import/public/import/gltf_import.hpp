#pragma once

namespace std::filesystem
{
class path;
}

namespace Engine
{
class TextureAsset;

class GltfImporter
{
  public:
    static void load_from_path(const std::filesystem::path& path);
};
}