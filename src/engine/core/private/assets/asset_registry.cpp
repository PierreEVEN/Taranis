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
        for (auto& asset : cl | std::views::values)
            asset.destroy();
}
} // namespace Eng