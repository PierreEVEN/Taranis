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
        for (auto& val : cl)
        {
            if (!val.second)
                LOG_DEBUG("FINAL NOT VALId ??? : {:x}", (size_t)val.first);
            else
                LOG_DEBUG("? {}", val.second->get_name());
            on_asset_removed.execute(val.second);
            val.second.destroy();
        }
    assets.clear();
}

std::shared_ptr<AssetRegistry> AssetRegistry::default_asset_registry;

std::shared_ptr<AssetRegistry> AssetRegistry::global()
{
    if (!default_asset_registry)
        default_asset_registry = std::make_shared<AssetRegistry>();
    return default_asset_registry;
}

void AssetRegistry::unregister_object(const Reflection::Class* object_class, AssetBase* object_ptr)
{
    if (auto cl = assets.find(object_class); cl != assets.end())
    {
        if (auto it = cl->second.find(object_ptr); it != cl->second.end())
        {
            if (!it->second)
                LOG_DEBUG("NOT VALID ??? : {} (caused by the recursive destructor call : it's not correctly unregistered from the registry)", object_ptr ? object_ptr->get_name() : "///");
            else
                LOG_DEBUG("? {}", it->second->get_name());
            on_asset_removed.execute(it->second);
            cl->second.erase(it);
            if (cl->second.empty())
                assets.erase(object_class);
        }
    }
}
} // namespace Eng