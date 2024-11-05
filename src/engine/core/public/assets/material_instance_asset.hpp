#pragma once
#include "asset_base.hpp"

namespace Engine
{

class MaterialInstanceAsset : public AssetBase
{
    AssetType get_type() const override
    {
        return AssetType::MaterialInstance;
    }
};
}