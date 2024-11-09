#pragma once
#include "asset_base.hpp"
#include "object_ptr.hpp"

#include "assets/texture_asset.gen.hpp"
#include "gfx/vulkan/image.hpp"

#include <cstdint>
#include <memory>

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
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;
    };

    const std::shared_ptr<Gfx::ImageView>& get_view() const
    {
        return view;
    }

    const Gfx::ColorFormat& get_format() const;

    static TObjectRef<TextureAsset> get_default_asset();

  private:

    friend class AssetRegistry;
    TextureAsset(const Gfx::BufferData& data, CreateInfos create_infos);

    std::shared_ptr<Gfx::Image>     image;
    std::shared_ptr<Gfx::ImageView> view;

    uint32_t    width    = 0;
    uint32_t    height   = 0;
    uint32_t    channels = 0;
    CreateInfos infos;
};
} // namespace Eng