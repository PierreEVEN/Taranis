#include "assets/asset_registry.hpp"

namespace Eng
{
AssetRegistry::~AssetRegistry()
{
    ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>> map_copy = assets;
    for (auto& asset : map_copy)
        asset.second.destroy();
}
} // namespace Eng