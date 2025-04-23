#pragma once

#include "object_ptr.hpp"
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

class PackageRef
{
public:
    PackageRef() = default;

    PackageRef(std::string package_name, std::filesystem::path path) : package(std::move(package_name)), internal_path(std::move(path))
    {
    }

    operator bool() const
    {
        return !is_transient_package();
    }

    static PackageRef transient()
    {
        return {};
    }

    bool is_transient_package() const
    {
        return package.empty();
    }

private:
    std::string           package;
    std::filesystem::path internal_path;
};

class AssetBase
{
    REFLECT_BODY()

    friend class AssetRegistry;

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

  protected:
    AssetBase() = default;

private:
    TObjectRef<AssetBase> this_ref_obj;
    char*                 name;
    AssetRegistry*        registry;

    AssetFlags flags;

    PackageRef package;
};
} // namespace Eng