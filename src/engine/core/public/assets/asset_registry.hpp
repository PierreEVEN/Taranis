#pragma once
#include "asset_base.hpp"
#include "eventmanager.hpp"
#include "logger.hpp"
#include "object_ptr.hpp"

#include <ranges>
#include <string>
#include <ankerl/unordered_dense.h>

namespace Eng
{
class AssetBase;

class AssetRegistry final
{
    friend class AssetBase;

public:
    DECLARE_DELEGATE_MULTICAST(TOnAssetRemovedEven, Eng::AssetBase*)
    DECLARE_DELEGATE_MULTICAST(TOnAssetAddedEvent, const TObjectRef<Eng::AssetBase>&)

    AssetRegistry();
    ~AssetRegistry();

    template <typename T, typename... Args> TObjectRef<T> create(const std::string& name, AssetFlags flags, Args&&... args)
    {
        std::unique_lock lock(asset_lock);

        AssetBase* data = static_cast<AssetBase*>(calloc(1, T::static_class()->stride()));
        data->name      = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->registry   = this;
        data->flags      = flags;
        data->base_class = T::static_class();
        new(data) T(std::forward<Args>(args)...);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
        if (!data->name)
            LOG_FATAL("Asset {} does not contains any constructor", T::static_class()->name())
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

        ObjectAllocation* allocation = new ObjectAllocation();
        allocation->ptr              = data;
        allocation->object_class     = T::static_class();
        TObjectPtr<AssetBase> object_ptr(allocation);
        object_ptr->this_ref_obj = object_ptr;

        assets.emplace(T::static_class(), ankerl::unordered_dense::map<AssetBase*, TObjectPtr<AssetBase>>{}).first->second.emplace(data, object_ptr);
        on_asset_created.execute(object_ptr);
        return object_ptr.cast<T>();
    }

    TObjectRef<AssetBase> create(const Reflection::Class* base_class, const std::string& name, AssetFlags flags)
    {
        std::unique_lock lock(asset_lock);

        AssetBase* data = static_cast<AssetBase*>(calloc(1, base_class->stride()));
        data->name      = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->registry   = this;
        data->flags      = flags;
        data->base_class = base_class;
        base_class->placement_new(data);
        if (!data->name)
            LOG_FATAL("Asset {} does not contains any constructor", base_class->name())

        ObjectAllocation* allocation = new ObjectAllocation();
        allocation->ptr              = data;
        allocation->object_class     = base_class;
        TObjectPtr<AssetBase> object_ptr(allocation);
        object_ptr->this_ref_obj = object_ptr;

        assets.emplace(base_class, ankerl::unordered_dense::map<AssetBase*, TObjectPtr<AssetBase>>{}).first->second.emplace(data, object_ptr);
        on_asset_created.execute(object_ptr);
        return object_ptr;
    }

    void for_each(const std::function<void(const TObjectPtr<AssetBase>&)>& callback) const
    {
        std::shared_lock lock(asset_lock);
        for (const auto& cl : assets | std::views::values)
            for (const auto& val : cl | std::views::values)
                callback(val);
    }

    template <typename T> void for_each(const std::function<void(T&)>& callback) const
    {
        std::shared_lock lock(asset_lock);
        if (auto cl = assets.find(T::static_class()); cl != assets.end())
            for (const auto& asset : cl->second)
                callback(*asset.second->template cast<T>());
    }

    static std::shared_ptr<AssetRegistry> global();

    TOnAssetAddedEvent  on_asset_created;
    TOnAssetRemovedEven on_asset_removed;

private:
    void unregister_object(const Reflection::Class* object_class, AssetBase* object_ptr);

    static std::shared_ptr<AssetRegistry>                                                                                   default_asset_registry;
    ankerl::unordered_dense::map<const Reflection::Class*, ankerl::unordered_dense::map<AssetBase*, TObjectPtr<AssetBase>>> assets;
    mutable std::shared_mutex                                                                                               asset_lock;
};
} // namespace Eng