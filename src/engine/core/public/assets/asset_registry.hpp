#pragma once
#include "asset_base.hpp"
#include "logger.hpp"
#include "object_ptr.hpp"

#include <string>
#include <unordered_set>

namespace Eng
{
class AssetBase;

class AssetRegistry
{
    friend class AssetBase;

public:
    AssetRegistry() = default;
  ~AssetRegistry();

    template <typename T, typename... Args> TObjectRef<T> create(std::string name, Args&&... args)
    {
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
        TObjectPtr<T>     object_ptr(allocation);
        TObjectPtr<T> copy = object_ptr;
        assets.emplace(data, copy);
        return object_ptr;
    }

    const std::unordered_map<void*, TObjectPtr<AssetBase>>& all_assets() const
    {
        return assets;
    }



private:
    std::unordered_map<void*, TObjectPtr<AssetBase>> assets;
};
} // namespace Eng