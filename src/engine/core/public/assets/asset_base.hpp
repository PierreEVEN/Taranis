#pragma once

#include "object_ptr.hpp"

#include <glm/vec3.hpp>
#include "assets/asset_base.gen.hpp"

namespace Eng::Gfx
{
class ImageView;
}

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

    virtual std::shared_ptr<Gfx::ImageView> get_thumbnail()
    {
        return nullptr;
    }

    virtual glm::vec3 asset_color() const
    {
        return {1, 1, 1};
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
