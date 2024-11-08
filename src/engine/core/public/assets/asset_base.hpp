#pragma once

#include "assets\asset_base.gen.hpp"

namespace Engine
{
class AssetBase
{
    REFLECT_BODY()

public:
    AssetBase(AssetBase&)  = delete;
    AssetBase(AssetBase&&) = delete;

    virtual ~AssetBase();

    const char* get_name() const
    {
        return name;
    }

protected:
    AssetBase() = default;

private:
    friend class AssetRegistry;
    char*          name;
    AssetRegistry* registry;
};
} // namespace Engine