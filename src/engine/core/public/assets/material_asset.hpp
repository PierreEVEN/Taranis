#pragma once
#include "asset_base.hpp"

namespace Engine
{
	class MaterialAsset : public AssetBase
	{

            AssetType get_type() const override
            {
                return AssetType::Material;
            }

	};
}