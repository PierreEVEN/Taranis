#pragma once
#include "asset_base.hpp"

namespace Engine
{
class MaterialAsset : public AssetBase
{
    REFLECT_BODY()

    AssetType get_type() const override
    {
        return AssetType::Material;
    }

};
}