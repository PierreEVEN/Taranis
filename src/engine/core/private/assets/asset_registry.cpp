#include "assets/asset_registry.hpp"

#include "object_allocator.hpp"

namespace Eng {
    AssetRegistry::AssetRegistry() {
    }

    AssetRegistry::~AssetRegistry() {
        std::unique_lock lock(asset_lock);
        if (!assets.empty())
            LOG_WARNING("Some asset have not been correctly removed from asset registry");
        auto assets_copy = assets;
        for (auto &cl: assets_copy | std::views::values)
            for (auto &val: cl) {
                LOG_WARNING("\t- Force late unload of {}", val.first->get_package().to_string());
                on_asset_removed.execute(val.first);
                val.second.destroy();
            }
        assets.clear();
    }

    std::shared_ptr<AssetRegistry> AssetRegistry::default_asset_registry;

    std::shared_ptr<AssetRegistry> AssetRegistry::global() {
        if (!default_asset_registry)
            default_asset_registry = std::make_shared<AssetRegistry>();
        return default_asset_registry;
    }

    void AssetRegistry::unregister_object(const Reflection::Class *object_class, AssetBase *object_ptr) {
        if (auto cl = assets.find(object_class); cl != assets.end()) {
            if (auto it = cl->second.find(object_ptr); it != cl->second.end()) {
                on_asset_removed.execute(object_ptr);
                cl->second.erase(it);
                if (cl->second.empty())
                    assets.erase(object_class);
            }
        }
    }
} // namespace Eng
