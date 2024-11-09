#pragma once
#include "gfx/types.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <optional>
#include <string>

namespace Eng::Gfx
{
class ClearValue
{
  public:
    static ClearValue none()
    {
        return {{}, {}};
    }

    static ClearValue color(glm::vec4 color)
    {
        return {color, {}};
    }

    static ClearValue depth_stencil(glm::vec2 depth_stencil)
    {
        return {{}, depth_stencil};
    }

    bool is_none() const
    {
        return !is_color() && !is_depth_stencil();
    }

    bool is_color() const
    {
        return color_val.has_value();
    }

    bool is_depth_stencil() const
    {
        return depth_stencil_val.has_value();
    }

    glm::vec4 color() const
    {
        return *color_val;
    }

    glm::vec2 depth_stencil() const
    {
        return *depth_stencil_val;
    }

  private:
    ClearValue(const std::optional<glm::vec4>& col, std::optional<glm::vec2> depth) : color_val(col), depth_stencil_val(depth)
    {
    }

    std::optional<glm::vec4> color_val         = {};
    std::optional<glm::vec2> depth_stencil_val = {};
};

class Attachment
{
  public:
    ColorFormat get_format() const
    {
        return format;
    }

    ClearValue clear_value() const
    {
        return clear_value_val;
    }

    bool is_depth() const
    {
        return is_depth_format(format);
    }

    static Attachment depth(std::string name, ColorFormat format, ClearValue clear_value = ClearValue::none())
    {
        Attachment attachment(std::move(name), clear_value);
        attachment.format = format;
        return attachment;
    }

    static Attachment color(std::string name, ColorFormat format, ClearValue clear_value = ClearValue::none())
    {
        Attachment attachment(std::move(name), clear_value);
        attachment.format = format;
        return attachment;
    }

    bool operator==(const Attachment& other) const
    {
        assert(format != ColorFormat::UNDEFINED);
        return get_format() == other.get_format() && clear_value().is_none() == other.clear_value().is_none();
    }

    const std::string& get_name() const
    {
        return name;
    }

  private:
    Attachment(std::string in_name, const ClearValue& in_clear_value) : name(std::move(in_name)), clear_value_val(in_clear_value)
    {
    }

    std::string name;
    ColorFormat format = ColorFormat::UNDEFINED;
    ClearValue  clear_value_val;
};

} // namespace Eng::Gfx