#pragma once
#include "asset_base.hpp"
#include "logger.hpp"

#include <memory>
#include <string>
#include <unordered_set>

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
        data->name      = name;
        data->type_size = sizeof(T);
        data->base_ptr  = reinterpret_cast<void*>(data);

        new(data) T(std::forward<Args>(args)...);
        assets.insert(data);
        return data;
    }

private:
    void destroy_object(AssetBase* asset)
  {
        if (asset && asset->type_size)
      {
            assets.erase(asset);
            void*  base = asset->base_ptr;
            size_t size = asset->type_size;
            delete asset;
            memset(base, 0, size);
        }
    }

    std::unordered_set<AssetBase*> assets;
};
}