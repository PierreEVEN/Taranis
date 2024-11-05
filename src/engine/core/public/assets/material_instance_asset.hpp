#pragma once
#include "asset_base.hpp"

namespace Engine
{

class MaterialInstanceAsset : public AssetBase
{
    REFLECT_BODY()
    AssetType get_type() const override
    {
        return AssetType::MaterialInstance;
    }
};
}