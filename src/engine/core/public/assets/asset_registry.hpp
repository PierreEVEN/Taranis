#pragma once
#include "asset_base.hpp"
#include "logger.hpp"
#include "object_allocator.hpp"
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
    AssetRegistry();
    ~AssetRegistry();

    template <typename T, typename... Args> TObjectRef<T> create(const std::string& name, AssetFlags flags, Args&&... args)
    {
        std::unique_lock lock(asset_lock);

        AssetBase* data = static_cast<AssetBase*>(calloc(1, T::static_class()->stride()));
        data->name      = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->registry = this;
        data->flags    = flags;
        new(data) T(std::forward<Args>(args)...);
        if (!data->name)
            LOG_FATAL("Asset {} does not contains any constructor", T::static_class()->name())

        ObjectAllocation* allocation = new ObjectAllocation();
        allocation->ptr              = data;
        allocation->object_class     = T::static_class();
        TObjectPtr<AssetBase> object_ptr(allocation);
        object_ptr->this_ref_obj = object_ptr;

        assets.emplace(T::static_class(), ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>>{}).first->second.emplace(data, object_ptr);

        return object_ptr.cast<T>();
    }

    TObjectRef<AssetBase> create(const Reflection::Class* base_class, const std::string& name, AssetFlags flags)
    {
        std::unique_lock lock(asset_lock);

        AssetBase* data = static_cast<AssetBase*>(calloc(1, base_class->stride()));
        data->name      = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->registry = this;
        data->flags    = flags;
        base_class->placement_new(data);
        if (!data->name)
            LOG_FATAL("Asset {} does not contains any constructor", base_class->name())

        ObjectAllocation* allocation = new ObjectAllocation();
        allocation->ptr              = data;
        allocation->object_class     = base_class;
        TObjectPtr<AssetBase> object_ptr(allocation);
        object_ptr->this_ref_obj = object_ptr;

        assets.emplace(base_class, ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>>{}).first->second.emplace(data, object_ptr);

        return object_ptr;
    }

    void for_each(const std::function<void(const TObjectPtr<AssetBase>&)>& callback) const
    {
        std::shared_lock lock(asset_lock);
        for (const auto& cl : assets | std::views::values)
            for (const auto& asset : cl)
                callback(asset.second);
    }

    template <typename T> void for_each(const std::function<void(T&)>& callback) const
    {
        std::shared_lock lock(asset_lock);
        if (auto cl = assets.find(T::static_class()); cl != assets.end())
            for (const auto& asset : cl->second)
                callback(*asset.second->cast<T>());
    }

    static std::shared_ptr<AssetRegistry> global();

private:
    void unregister_object(const Reflection::Class* object_class, void* object_ptr)
    {
        if (auto cl = assets.find(object_class); cl != assets.end())
            cl->second.erase(object_ptr);
    }

    static std::shared_ptr<AssetRegistry>                                                                       default_asset_registry;
    ankerl::unordered_dense::map<const Reflection::Class*, ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>>> assets;
    mutable std::shared_mutex                                                                                   asset_lock;
};
} // namespace Eng