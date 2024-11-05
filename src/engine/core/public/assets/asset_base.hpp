#pragma once
#include "class.hpp"

#include <string>
#include "assets\asset_base.gen.hpp"

namespace Engine
{
enum class AssetType
{
    Mesh,
    Texture,
    Material,
    MaterialInstance
};


class AssetBase
{
    REFLECT_BODY()
  public:
    AssetBase(AssetBase&)  = delete;
    AssetBase(AssetBase&&) = delete;
    virtual ~AssetBase()   = default;

    const char* get_name() const
    {
        return name;
    }

    void destroy();

    virtual AssetType get_type() const = 0;

protected:
    AssetBase() = default;

private:
    friend class AssetRegistry;
    char*  name;
    void*  base_ptr;
    size_t type_size;
};
}
