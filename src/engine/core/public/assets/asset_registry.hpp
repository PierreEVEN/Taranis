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

class AssetRegistry
{
    friend class AssetBase;

public:
    AssetRegistry();
    ~AssetRegistry();

    template <typename T, typename... Args> TObjectRef<T> create(std::string name, Args&&... args)
    {
        std::unique_lock lock(asset_lock);
        static_assert(std::is_base_of_v<AssetBase, T>, "This type is not an asset");
        T* data    = static_cast<T*>(calloc(1, sizeof(T)));
        data->name = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->registry = this;
        new(data) T(std::forward<Args>(args)...);
        if (!data->name)
            LOG_FATAL("Asset {} does not contains any constructor", typeid(T).name())
        ObjectAllocation* allocation = new ObjectAllocation();
        allocation->ptr              = data;
        allocation->object_class     = T::static_class();
        TObjectPtr<T> object_ptr(allocation);
        object_ptr->this_ref_obj = object_ptr;
        assets.emplace(T::static_class(), ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>>{}).first->second.emplace(data, object_ptr);
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

private:
    ankerl::unordered_dense::map<const Reflection::Class*, ankerl::unordered_dense::map<void*, TObjectPtr<AssetBase>>> assets;
    mutable std::shared_mutex                                                                                          asset_lock;
};
} // namespace Eng