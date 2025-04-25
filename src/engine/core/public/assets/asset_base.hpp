#pragma once

#include "object_ptr.hpp"
#include "package_ref.hpp"

#include <glm/vec3.hpp>
#include "assets/asset_base.gen.hpp"

namespace Eng
{

namespace Gfx
{
class ImageView;
}

RENUM(EnumFlags)

enum class AssetFlags
{
    NONE = 0,
    TRANSIENT = 1,
};

class AssetBase
{
    REFLECT_BODY()

    friend class AssetRegistry;
    friend class Package;
    friend class AssetFactory;

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

    AssetFlags get_flags() const
    {
        return flags;
    }

    const PackageRef& get_package() const
    {
        return package;
    }

protected:
    AssetBase() = default;

private:
    /// SET FROM FACTORY
    TObjectRef<AssetBase> this_ref_obj;
    char*                 name;
    AssetRegistry*        registry;
    AssetFlags            flags;
    PackageRef            package;
    /// SET FROM FACTORY
};
} // namespace Eng