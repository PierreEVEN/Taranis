#pragma once
#include <glm/vec2.hpp>
#include <memory>
#include <optional>
#include <vulkan/vulkan_core.h>

#include "buffer.hpp"
#include "device.hpp"
#include "gfx/types.hpp"
#include "gfx_types/format.hpp"

namespace Eng::Gfx
{
class CommandBuffer;
class Device;

enum class EImageType
{
    Texture_1D,
    Texture_1D_Array,
    Texture_2D,
    Texture_2D_Array,
    Texture_3D,
    Cubemap,
};

enum class ETextureTransferCapabilities
{
    None = 0x00000,
    CopySource = 0x00001,
    CopyDestination = 0x00002,
};

struct GenerateMips
{
    friend class ImageResource;

    // Keep provided mips
    static GenerateMips none()
    {
        return {-1};
    }

    // Auto
    static GenerateMips max()
    {
        return {0};
    }

    // Auto
    static GenerateMips custom(uint32_t number)
    {
        return {static_cast<int32_t>(number)};
    }

    bool does_generates() const
    {
        return mipmaps > -1;
    }

    uint32_t desired_mip_count() const
    {
        return mipmaps;
    }

private:
    GenerateMips(int32_t c) : mipmaps(c)
    {
    }

    int32_t mipmaps;
};

struct ImageParameter
{
    ColorFormat                  format                = ColorFormat::R8G8B8A8_UNORM;
    EImageType                   image_type            = EImageType::Texture_2D;
    ETextureTransferCapabilities transfer_capabilities = ETextureTransferCapabilities::CopyDestination;
    bool                         is_attachment         = false;
    bool                         can_sample            = true;
    bool                         is_storage            = false;
    GenerateMips                 generate_mips         = GenerateMips::none();
    EBufferType                  buffer_type           = EBufferType::IMMUTABLE;
    uint32_t                     width                 = 1;
    uint32_t                     height                = 1;
    uint32_t                     depth                 = 1;
    uint32_t                     array_size            = 1;
    bool                         read_only             = true;
};

class Image
{
    friend class ImageView;

public:
    class ImageResource : public DeviceResource, public std::enable_shared_from_this<ImageResource>
    {
    public:
        ImageResource(std::string name, std::weak_ptr<Device> device, ImageParameter params, uint32_t mip_count);
        ImageResource(ImageResource&&) = delete;
        ImageResource(ImageResource&)  = delete;
        ~ImageResource();
        void set_data(const std::vector<BufferData>& mips);
        void set_image_layout(const CommandBuffer& command_buffer, VkImageLayout new_layout);
        void generate_mipmaps(uint32_t mipLevels, const CommandBuffer& command_buffer) const;

        VkImage                            ptr          = VK_NULL_HANDLE;
        VkImageLayout                      image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        std::unique_ptr<VmaAllocationWrap> allocation;
        bool                               outdated      = false;
        uint32_t                           layer_cout    = 0;
        uint32_t                           mip_count     = 0;
        GenerateMips                       generate_mips = GenerateMips::none();
        bool                               is_depth      = false;
        VkFormat                           format;
        glm::uvec2                         res;
        uint32_t                           depth = 0;
    };

    static std::shared_ptr<Image> create(const std::string& name, std::weak_ptr<Device> device, const ImageParameter& params, const std::vector<BufferData>& mips)
    {
        auto created_image = std::shared_ptr<Image>(new Image(name, std::move(device), params, static_cast<glm::uint32>(mips.size())));
        for (const auto& image : created_image->images)
            image->set_data(mips);
        return created_image;
    }

    static std::shared_ptr<Image> create(const std::string& name, std::weak_ptr<Device> device, const ImageParameter& params, uint32_t desired_mip_count = 1)
    {
        return std::shared_ptr<Image>(new Image(name, std::move(device), params, desired_mip_count));
    }

    Image(Image&&) = delete;
    Image(Image&)  = delete;
    ~Image();

    std::vector<VkImage> raw() const;
    VkImage              raw_current();

    std::weak_ptr<Device> get_device() const
    {
        return device;
    }

    glm::uvec2 get_extent() const
    {
        return extent;
    }

    bool resize(glm::uvec2 new_size);
    void set_data(glm::uvec2 new_size, const std::vector<BufferData>& mips);

    const std::vector<std::shared_ptr<ImageResource>>& get_resources() const
    {
        return images;
    }

    const std::shared_ptr<ImageResource>& get_resource() const
    {
        return images.size() == 1 ? images[0] : images[device.lock()->get_current_image()];
    }

    const ImageParameter& get_params() const
    {
        return params;
    }

    uint32_t get_mips_count() const;

private:
    Image(std::string name, std::weak_ptr<Device> device, const ImageParameter& params, uint32_t base_mips);
    glm::uvec2                                  extent;
    ImageParameter                              params;
    std::weak_ptr<Device>                       device;
    std::vector<BufferData>                     temp_buffer_data;
    std::vector<std::shared_ptr<ImageResource>> images;
    std::string                                 name;
    uint32_t                                    provided_mips = 1;
};
} // namespace Eng::Gfx