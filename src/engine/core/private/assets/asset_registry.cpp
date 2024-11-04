#include "assets/asset_registry.hpp"

#include "logger.hpp"

namespace Engine
{
AssetRegistry::~AssetRegistry()
{
    std::unordered_set<AssetBase*> asset_copy = assets;
    for (const auto& asset : asset_copy)
    {
        destroy_object(asset);
    }
}
}