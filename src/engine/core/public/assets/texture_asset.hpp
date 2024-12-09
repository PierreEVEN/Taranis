#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"

#include "gfx/vulkan/image.hpp"

#include <cstdint>
#include <memory>

#include "assets/texture_asset.gen.hpp"

namespace Eng
{
namespace Gfx
{
class BufferData;
class ImageView;
class Image;
} // namespace Gfx

class TextureAsset : public AssetBase
{
    REFLECT_BODY()

public:
    struct CreateInfos
    {
        uint32_t          width         = 1;
        uint32_t          height        = 1;
        uint32_t          depth         = 1;
        Gfx::ColorFormat  format        = Gfx::ColorFormat::UNDEFINED;
        Gfx::GenerateMips generate_mips = Gfx::GenerateMips::none();
        uint32_t          array_size    = 1;
    };

    const std::shared_ptr<Gfx::ImageView>& get_view() const
    {
        return view;
    }

    const Gfx::ColorFormat& get_format() const;

    static TObjectRef<TextureAsset> get_default_asset();

    std::shared_ptr<Gfx::ImageView> get_thumbnail() override
    {
        return view;
    }

    glm::vec3 asset_color() const override
    {
        return {0.5, 0.4, 1};
    }

private:
    friend class AssetRegistry;
    TextureAsset(const std::vector<Gfx::BufferData>& mips, const CreateInfos& create_infos);

    std::shared_ptr<Gfx::Image>     image;
    std::shared_ptr<Gfx::ImageView> view;

    uint32_t    width    = 0;
    uint32_t    height   = 0;
    uint32_t    channels = 0;
    CreateInfos infos;
};
} // namespace Eng
