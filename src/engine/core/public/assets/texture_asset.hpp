#pragma once
#include "asset_base.hpp"

#include <cstdint>
#include <memory>

namespace Engine
{
class BufferData;
class ImageView;
class Image;

class TextureAsset : public AssetBase
{
public:
    struct CreateInfos
    {
        uint32_t width    = 0;
        uint32_t height   = 0;
        uint32_t channels = 0;
    };

    const std::shared_ptr<ImageView>& get_view() const
    {
        return view;
    }

private:
    friend class AssetRegistry;
    TextureAsset(const BufferData& data, CreateInfos create_infos);
    ~TextureAsset();

    std::shared_ptr<Image>     image;
    std::shared_ptr<ImageView> view;

    uint32_t    width    = 0;
    uint32_t    height   = 0;
    uint32_t    channels = 0;
    CreateInfos infos;
};
}