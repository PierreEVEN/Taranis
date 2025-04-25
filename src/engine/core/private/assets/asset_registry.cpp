#include "assets/asset_registry.hpp"

#include "object_allocator.hpp"

namespace Eng
{
AssetRegistry::AssetRegistry()
{
}

AssetRegistry::~AssetRegistry()
{
    std::unique_lock lock(asset_lock);
    auto             assets_copy = assets;
    for (auto& cl : assets_copy | std::views::values)
        for (auto& asset : cl)
            asset.second.destroy();
}

std::shared_ptr<AssetRegistry> AssetRegistry::default_asset_registry;

std::shared_ptr<AssetRegistry> AssetRegistry::global()
{
    if (!default_asset_registry)
        default_asset_registry = std::make_shared<AssetRegistry>();
    return default_asset_registry;
}
} // namespace Eng