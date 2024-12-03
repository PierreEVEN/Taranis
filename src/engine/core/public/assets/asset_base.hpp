#pragma once

#include "object_ptr.hpp"
#include "assets/asset_base.gen.hpp"

namespace Eng
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

    const TObjectRef<AssetBase>& this_ref() const
    {
        return this_ref_obj;
    }

protected:
    AssetBase() = default;

private:
    TObjectRef<AssetBase> this_ref_obj;
    friend class AssetRegistry;
    char*          name;
    AssetRegistry* registry;
};
} // namespace Eng