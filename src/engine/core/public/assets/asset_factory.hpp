#pragma once
#include "asset_base.hpp"
#include "asset_registry.hpp"
#include "object_ptr.hpp"

namespace Eng
{
class AssetFactory final
{
public:
    AssetFactory() = delete;

    template <typename AssetT, typename... Args> static TObjectRef<AssetT> instantiate_new(const std::string& name, PackageRef package_ref, Args&&... args)
    {
        static_assert(std::is_base_of_v<AssetBase, AssetT>, "This type is not an asset");

        Package* package = package_ref.package();
        if (!package)
            LOG_FATAL("Cannot instantiate asset : unknown package {}", package_ref.to_string())

        AssetRegistry& registry = package->get_asset_registry();
        AssetFlags     flags    = AssetFlags::NONE;
        if (package_ref.is_transient_package())
            flags |= AssetFlags::TRANSIENT;

        TObjectRef<AssetT> asset = registry.create<AssetT>(name, flags, std::forward<Args>(args)...);
        asset->package           = package_ref;
        package->on_asset_loaded_internal(package_ref.get_path(), asset.cast<AssetBase>());
        return asset;
    }
};
}