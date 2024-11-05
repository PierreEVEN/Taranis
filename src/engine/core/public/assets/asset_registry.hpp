#pragma once
#include "asset_base.hpp"
#include "logger.hpp"

#include <memory>
#include <string>
#include <unordered_set>
#include "assets\asset_registry.gen.hpp"

namespace Engine
{
class AssetBase;

class AssetRegistry
{
    friend class AssetBase;

public:
    AssetRegistry& get() const;
    AssetRegistry() = default;
    ~AssetRegistry();

    template <typename T, typename... Args> T* create(std::string name, Args&&... args)
    {
        static_assert(std::is_base_of_v<AssetBase, T>, "This type is not an asset");
        T* data         = static_cast<T*>(calloc(1, sizeof(T)));
        data->name      = new char[name.size() + 1];
        memcpy(data->name, name.c_str(), name.size() + 1);
        data->type_size = sizeof(T);
        data->base_ptr  = reinterpret_cast<void*>(data);
        new(data) T(std::forward<Args>(args)...);
        assets.insert(data);
        return data;
    }

    const std::unordered_set<AssetBase*>& all_assets() const
    {
        return assets;
    }

private:
    void destroy_object(AssetBase* asset)
    {
        if (asset && asset->type_size)
        {
            assets.erase(asset);
            void*  base = asset->base_ptr;
            size_t size = asset->type_size;
            delete[] asset->name;
            delete asset;
            memset(base, 0, size);
        }
    }

    std::unordered_set<AssetBase*> assets;
};
}
