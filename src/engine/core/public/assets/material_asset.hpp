#pragma once
#include "asset_base.hpp"
#include "assets\material_asset.gen.hpp"

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
