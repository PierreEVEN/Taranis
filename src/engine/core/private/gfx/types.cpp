#include "gfx/types.hpp"

namespace Engine
{

uint8_t get_format_channel_count(ColorFormat format)
{
    switch (format)
    {
    case ColorFormat::UNDEFINED:
        return 0;
    case ColorFormat::R8_UNORM:
        return 1;
    case ColorFormat::R8_SNORM:
        return 1;
    case ColorFormat::R8_USCALED:
        return 1;
    case ColorFormat::R8_SSCALED:
        return 1;
    case ColorFormat::R8_UINT:
        return 1;
    case ColorFormat::R8_SINT:
        return 1;
    case ColorFormat::R8_SRGB:
        return 1;
    case ColorFormat::R8G8_UNORM:
        return 2;
    case ColorFormat::R8G8_SNORM:
        return 2;
    case ColorFormat::R8G8_USCALED:
        return 2;
    case ColorFormat::R8G8_SSCALED:
        return 2;
    case ColorFormat::R8G8_UINT:
        return 2;
    case ColorFormat::R8G8_SINT:
        return 2;
    case ColorFormat::R8G8_SRGB:
        return 2;
    case ColorFormat::R8G8B8_UNORM:
        return 3;
    case ColorFormat::R8G8B8_SNORM:
        return 3;
    case ColorFormat::R8G8B8_USCALED:
        return 3;
    case ColorFormat::R8G8B8_SSCALED:
        return 3;
    case ColorFormat::R8G8B8_UINT:
        return 3;
    case ColorFormat::R8G8B8_SINT:
        return 3;
    case ColorFormat::R8G8B8_SRGB:
        return 3;
    case ColorFormat::B8G8R8_UNORM:
        return 3;
    case ColorFormat::B8G8R8_SNORM:
        return 3;
    case ColorFormat::B8G8R8_USCALED:
        return 3;
    case ColorFormat::B8G8R8_SSCALED:
        return 3;
    case ColorFormat::B8G8R8_UINT:
        return 3;
    case ColorFormat::B8G8R8_SINT:
        return 3;
    case ColorFormat::B8G8R8_SRGB:
        return 3;
    case ColorFormat::R8G8B8A8_UNORM:
        return 4;
    case ColorFormat::R8G8B8A8_SNORM:
        return 4;
    case ColorFormat::R8G8B8A8_USCALED:
        return 4;
    case ColorFormat::R8G8B8A8_SSCALED:
        return 4;
    case ColorFormat::R8G8B8A8_UINT:
        return 4;
    case ColorFormat::R8G8B8A8_SINT:
        return 4;
    case ColorFormat::R8G8B8A8_SRGB:
        return 4;
    case ColorFormat::B8G8R8A8_UNORM:
        return 4;
    case ColorFormat::B8G8R8A8_SNORM:
        return 4;
    case ColorFormat::B8G8R8A8_USCALED:
        return 4;
    case ColorFormat::B8G8R8A8_SSCALED:
        return 4;
    case ColorFormat::B8G8R8A8_UINT:
        return 4;
    case ColorFormat::B8G8R8A8_SINT:
        return 4;
    case ColorFormat::B8G8R8A8_SRGB:
        return 4;
    case ColorFormat::A8B8G8R8_UNORM_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_SNORM_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_USCALED_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_SSCALED_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_UINT_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_SINT_PACK32:
        return 4;
    case ColorFormat::A8B8G8R8_SRGB_PACK32:
        return 4;
    case ColorFormat::R16_UNORM:
        return 1;
    case ColorFormat::R16_SNORM:
        return 1;
    case ColorFormat::R16_USCALED:
        return 1;
    case ColorFormat::R16_SSCALED:
        return 1;
    case ColorFormat::R16_UINT:
        return 1;
    case ColorFormat::R16_SINT:
        return 1;
    case ColorFormat::R16_SFLOAT:
        return 1;
    case ColorFormat::R16G16_UNORM:
        return 2;
    case ColorFormat::R16G16_SNORM:
        return 2;
    case ColorFormat::R16G16_USCALED:
        return 2;
    case ColorFormat::R16G16_SSCALED:
        return 2;
    case ColorFormat::R16G16_UINT:
        return 2;
    case ColorFormat::R16G16_SINT:
        return 2;
    case ColorFormat::R16G16_SFLOAT:
        return 2;
    case ColorFormat::R16G16B16_UNORM:
        return 3;
    case ColorFormat::R16G16B16_SNORM:
        return 3;
    case ColorFormat::R16G16B16_USCALED:
        return 3;
    case ColorFormat::R16G16B16_SSCALED:
        return 3;
    case ColorFormat::R16G16B16_UINT:
        return 3;
    case ColorFormat::R16G16B16_SINT:
        return 3;
    case ColorFormat::R16G16B16_SFLOAT:
        return 3;
    case ColorFormat::R16G16B16A16_UNORM:
        return 4;
    case ColorFormat::R16G16B16A16_SNORM:
        return 4;
    case ColorFormat::R16G16B16A16_USCALED:
        return 4;
    case ColorFormat::R16G16B16A16_SSCALED:
        return 4;
    case ColorFormat::R16G16B16A16_UINT:
        return 4;
    case ColorFormat::R16G16B16A16_SINT:
        return 4;
    case ColorFormat::R16G16B16A16_SFLOAT:
        return 4;
    case ColorFormat::R32_UINT:
        return 1;
    case ColorFormat::R32_SINT:
        return 1;
    case ColorFormat::R32_SFLOAT:
        return 1;
    case ColorFormat::R32G32_UINT:
        return 2;
    case ColorFormat::R32G32_SINT:
        return 2;
    case ColorFormat::R32G32_SFLOAT:
        return 2;
    case ColorFormat::R32G32B32_UINT:
        return 3;
    case ColorFormat::R32G32B32_SINT:
        return 3;
    case ColorFormat::R32G32B32_SFLOAT:
        return 3;
    case ColorFormat::R32G32B32A32_UINT:
        return 4;
    case ColorFormat::R32G32B32A32_SINT:
        return 4;
    case ColorFormat::R32G32B32A32_SFLOAT:
        return 4;
    case ColorFormat::R64_UINT:
        return 1;
    case ColorFormat::R64_SINT:
        return 1;
    case ColorFormat::R64_SFLOAT:
        return 1;
    case ColorFormat::R64G64_UINT:
        return 2;
    case ColorFormat::R64G64_SINT:
        return 2;
    case ColorFormat::R64G64_SFLOAT:
        return 2;
    case ColorFormat::R64G64B64_UINT:
        return 3;
    case ColorFormat::R64G64B64_SINT:
        return 3;
    case ColorFormat::R64G64B64_SFLOAT:
        return 3;
    case ColorFormat::R64G64B64A64_UINT:
        return 4;
    case ColorFormat::R64G64B64A64_SINT:
        return 4;
    case ColorFormat::R64G64B64A64_SFLOAT:
        return 4;
    case ColorFormat::D16_UNORM:
        return 0;
    case ColorFormat::D32_SFLOAT:
        return 0;
    case ColorFormat::D16_UNORM_S8_UINT:
        return 0;
    case ColorFormat::D24_UNORM_S8_UINT:
        return 0;
    case ColorFormat::D32_SFLOAT_S8_UINT:
        return 0;
    default:
        return 0;
    }
}

uint8_t get_format_bytes_per_pixel(ColorFormat format)
{
    switch (format)
    {
    case ColorFormat::UNDEFINED:
        return 0;
    case ColorFormat::R8_UNORM:
        return 1;
    case ColorFormat::R8_SNORM:
        return 1;
    case ColorFormat::R8_USCALED:
        return 1;
    case ColorFormat::R8_SSCALED:
        return 1;
    case ColorFormat::R8_UINT:
        return 1;
    case ColorFormat::R8_SINT:
        return 1;
    case ColorFormat::R8_SRGB:
        return 1;
    case ColorFormat::R8G8_UNORM:
        return 1;
    case ColorFormat::R8G8_SNORM:
        return 1;
    case ColorFormat::R8G8_USCALED:
        return 1;
    case ColorFormat::R8G8_SSCALED:
        return 1;
    case ColorFormat::R8G8_UINT:
        return 1;
    case ColorFormat::R8G8_SINT:
        return 1;
    case ColorFormat::R8G8_SRGB:
        return 1;
    case ColorFormat::R8G8B8_UNORM:
        return 1;
    case ColorFormat::R8G8B8_SNORM:
        return 1;
    case ColorFormat::R8G8B8_USCALED:
        return 1;
    case ColorFormat::R8G8B8_SSCALED:
        return 1;
    case ColorFormat::R8G8B8_UINT:
        return 1;
    case ColorFormat::R8G8B8_SINT:
        return 1;
    case ColorFormat::R8G8B8_SRGB:
        return 1;
    case ColorFormat::B8G8R8_UNORM:
        return 1;
    case ColorFormat::B8G8R8_SNORM:
        return 1;
    case ColorFormat::B8G8R8_USCALED:
        return 1;
    case ColorFormat::B8G8R8_SSCALED:
        return 1;
    case ColorFormat::B8G8R8_UINT:
        return 1;
    case ColorFormat::B8G8R8_SINT:
        return 1;
    case ColorFormat::B8G8R8_SRGB:
        return 1;
    case ColorFormat::R8G8B8A8_UNORM:
        return 1;
    case ColorFormat::R8G8B8A8_SNORM:
        return 1;
    case ColorFormat::R8G8B8A8_USCALED:
        return 1;
    case ColorFormat::R8G8B8A8_SSCALED:
        return 1;
    case ColorFormat::R8G8B8A8_UINT:
        return 1;
    case ColorFormat::R8G8B8A8_SINT:
        return 1;
    case ColorFormat::R8G8B8A8_SRGB:
        return 1;
    case ColorFormat::B8G8R8A8_UNORM:
        return 1;
    case ColorFormat::B8G8R8A8_SNORM:
        return 1;
    case ColorFormat::B8G8R8A8_USCALED:
        return 1;
    case ColorFormat::B8G8R8A8_SSCALED:
        return 1;
    case ColorFormat::B8G8R8A8_UINT:
        return 1;
    case ColorFormat::B8G8R8A8_SINT:
        return 1;
    case ColorFormat::B8G8R8A8_SRGB:
        return 1;
    case ColorFormat::A8B8G8R8_UNORM_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_SNORM_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_USCALED_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_SSCALED_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_UINT_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_SINT_PACK32:
        return 1;
    case ColorFormat::A8B8G8R8_SRGB_PACK32:
        return 1;
    case ColorFormat::R16_UNORM:
        return 2;
    case ColorFormat::R16_SNORM:
        return 2;
    case ColorFormat::R16_USCALED:
        return 2;
    case ColorFormat::R16_SSCALED:
        return 2;
    case ColorFormat::R16_UINT:
        return 2;
    case ColorFormat::R16_SINT:
        return 2;
    case ColorFormat::R16_SFLOAT:
        return 2;
    case ColorFormat::R16G16_UNORM:
        return 2;
    case ColorFormat::R16G16_SNORM:
        return 2;
    case ColorFormat::R16G16_USCALED:
        return 2;
    case ColorFormat::R16G16_SSCALED:
        return 2;
    case ColorFormat::R16G16_UINT:
        return 2;
    case ColorFormat::R16G16_SINT:
        return 2;
    case ColorFormat::R16G16_SFLOAT:
        return 2;
    case ColorFormat::R16G16B16_UNORM:
        return 2;
    case ColorFormat::R16G16B16_SNORM:
        return 2;
    case ColorFormat::R16G16B16_USCALED:
        return 2;
    case ColorFormat::R16G16B16_SSCALED:
        return 2;
    case ColorFormat::R16G16B16_UINT:
        return 2;
    case ColorFormat::R16G16B16_SINT:
        return 2;
    case ColorFormat::R16G16B16_SFLOAT:
        return 2;
    case ColorFormat::R16G16B16A16_UNORM:
        return 2;
    case ColorFormat::R16G16B16A16_SNORM:
        return 2;
    case ColorFormat::R16G16B16A16_USCALED:
        return 2;
    case ColorFormat::R16G16B16A16_SSCALED:
        return 2;
    case ColorFormat::R16G16B16A16_UINT:
        return 2;
    case ColorFormat::R16G16B16A16_SINT:
        return 2;
    case ColorFormat::R16G16B16A16_SFLOAT:
        return 2;
    case ColorFormat::R32_UINT:
        return 4;
    case ColorFormat::R32_SINT:
        return 4;
    case ColorFormat::R32_SFLOAT:
        return 4;
    case ColorFormat::R32G32_UINT:
        return 4;
    case ColorFormat::R32G32_SINT:
        return 4;
    case ColorFormat::R32G32_SFLOAT:
        return 4;
    case ColorFormat::R32G32B32_UINT:
        return 4;
    case ColorFormat::R32G32B32_SINT:
        return 4;
    case ColorFormat::R32G32B32_SFLOAT:
        return 4;
    case ColorFormat::R32G32B32A32_UINT:
        return 4;
    case ColorFormat::R32G32B32A32_SINT:
        return 4;
    case ColorFormat::R32G32B32A32_SFLOAT:
        return 4;
    case ColorFormat::R64_UINT:
        return 8;
    case ColorFormat::R64_SINT:
        return 8;
    case ColorFormat::R64_SFLOAT:
        return 8;
    case ColorFormat::R64G64_UINT:
        return 8;
    case ColorFormat::R64G64_SINT:
        return 8;
    case ColorFormat::R64G64_SFLOAT:
        return 8;
    case ColorFormat::R64G64B64_UINT:
        return 8;
    case ColorFormat::R64G64B64_SINT:
        return 8;
    case ColorFormat::R64G64B64_SFLOAT:
        return 8;
    case ColorFormat::R64G64B64A64_UINT:
        return 8;
    case ColorFormat::R64G64B64A64_SINT:
        return 8;
    case ColorFormat::R64G64B64A64_SFLOAT:
        return 8;
    default:
        return 0;
    }
}

bool is_depth_format(ColorFormat format)
{
    return format == ColorFormat::D24_UNORM_S8_UINT || format == ColorFormat::D32_SFLOAT || format == ColorFormat::D32_SFLOAT_S8_UINT;
}

} // namespace Engine
