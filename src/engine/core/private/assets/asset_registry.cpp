#include "assets/asset_registry.hpp"

namespace Engine
{
AssetRegistry::~AssetRegistry()
{
    std::unordered_map<void*, TObjectPtr<AssetBase>> map_copy = assets;
    for (auto& asset : map_copy)
        asset.second.destroy();
}
} // namespace Engine