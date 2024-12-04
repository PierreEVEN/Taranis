#pragma once

#include "gfx/types.hpp"
#include "gfx_types/format.hpp"

#include <algorithm>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <ankerl/unordered_dense.h>

namespace Eng::Gfx
{
class CustomPassList;
}

namespace Eng::Gfx
{
class Window;
class RenderPassInstance;
class CommandBuffer;
class Device;

class Attachment
{
public:
    static Attachment slot(const std::string& in_name)
    {
        return Attachment{in_name};
    }

    Attachment& format(const ColorFormat& format)
    {
        color_format = format;
        return *this;
    }

    Attachment& clear_color(const glm::vec4& clear_color)
    {
        clear_color_value = clear_color;
        return *this;
    }

    Attachment& clear_depth(const glm::vec2& clear_depth)
    {
        clear_depth_value = clear_depth;
        return *this;
    }

    bool has_clear() const
    {
        return clear_color_value.has_value() || clear_depth_value.has_value();
    }

    std::string              name;
    ColorFormat              color_format      = ColorFormat::UNDEFINED;
    std::optional<glm::vec4> clear_color_value = {};
    std::optional<glm::vec2> clear_depth_value = {};

private:
    Attachment(std::string in_name) : name(std::move(in_name))
    {
    }
};

class IRenderPass
{
public:
    virtual void init(const RenderPassInstance&)
    {
    }

    virtual void on_create_framebuffer(const RenderPassInstance&)
    {
    }

    virtual void pre_draw(const RenderPassInstance&)
    {
    }

    virtual void draw(const RenderPassInstance&, CommandBuffer&, size_t)
    {
    }

    virtual void pre_submit(const RenderPassInstance&)
    {
    }

    // Get the number of thread used by this renderer (if > 1, parallel rendering will be automatically enabled)
    virtual size_t record_threads()
    {
        return 0;
    }
};

struct RenderPassKey
{
    bool operator==(const RenderPassKey& other) const
    {
        if (b_present != other.b_present)
            return false;
        if (attachments.size() != other.attachments.size())
            return false;
        for (auto a = attachments.begin(), b = other.attachments.begin(); a != attachments.end(); ++a, ++b)
            if (a->color_format != b->color_format || a->has_clear() != b->has_clear())
                return false;
        return true;
    }

    std::vector<Attachment> attachments;
    std::string             name;
    bool                    b_present = false;
};

} // namespace Eng::Gfx
template <> struct std::hash<Eng::Gfx::RenderPassKey>
{
    size_t operator()(const Eng::Gfx::RenderPassKey& val) const noexcept
    {
        auto ite = val.attachments.begin();
        if (ite == val.attachments.end())
            return 0;
        size_t hash = std::hash<uint32_t>()(static_cast<uint32_t>(ite->color_format) + 1);
        for (; ite != val.attachments.end(); ++ite)
        {
            hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(ite->color_format) + 1) * 13;
        }
        return hash;
    }
};

namespace Eng::Gfx
{

class RenderNode
{
    friend class Renderer;

    class IRenderPassInitializer
    {
    public:
        virtual std::shared_ptr<IRenderPass> construct() = 0;
    };

    template <typename T, typename... Args> class TRenderPassInitializer : public IRenderPassInitializer
    {
    public:
        TRenderPassInitializer(Args&&... in_args) : tuple_value(std::tuple<Args...>(std::forward<Args>(in_args)...))
        {
        }

        std::shared_ptr<IRenderPass> construct() override
        {
            return construct_with_tuple(tuple_value, std::index_sequence_for<Args...>());
        }

    private:
        std::shared_ptr<T> construct_internal(Args... in_args)
        {
            return std::make_shared<T>(in_args...);
        }

        template <std::size_t... Is> std::shared_ptr<T> construct_with_tuple(const std::tuple<Args...>& tuple, std::index_sequence<Is...>)
        {
            return construct_internal(std::get<Is>(tuple)...);
        }

        std::tuple<Args...> tuple_value;
    };

    RenderNode() = default;

public:
    using ResizeCallback = std::function<glm::uvec2(glm::uvec2)>;

    RenderPassKey get_key(bool b_present) const
    {
        RenderPassKey key;
        key.name      = name;
        key.b_present = b_present;
        for (const auto& attachment : attachments | std::views::values)
            key.attachments.emplace_back(attachment);
        return key;
    }

    RenderNode& require(std::initializer_list<std::string> required)
    {
        for (const auto& item : required)
            dependencies.insert(item);
        return *this;
    }

    RenderNode& require(const std::string& required)
    {
        dependencies.insert(required);
        return *this;
    }

    RenderNode& with_imgui(bool enable, const std::weak_ptr<Window>& input_window = {})
    {
        b_with_imgui       = enable;
        imgui_input_window = enable ? input_window : std::weak_ptr<Window>{};
        return *this;
    }

    RenderNode& resize_callback(const ResizeCallback& callback)
    {
        resize_callback_ptr = callback;
        return *this;
    }

    RenderNode& operator[](const Attachment& attachment)
    {
        attachments.insert_or_assign(attachment.name, attachment);
        return *this;
    }

    template <typename T, typename... Args> RenderNode& render_pass(Args&&... args)
    {
        render_pass_initializer = std::make_shared<TRenderPassInitializer<T, Args...>>(std::forward<Args>(args)...);
        return *this;
    }

    std::shared_ptr<IRenderPassInitializer>               render_pass_initializer;
    ankerl::unordered_dense::map<std::string, Attachment> attachments;
    ankerl::unordered_dense::set<std::string>             dependencies;
    bool                                                  b_with_imgui = false;
    std::weak_ptr<Window>                                 imgui_input_window;
    std::string                                           name;
    ResizeCallback                                        resize_callback_ptr;
};

class Renderer
{
public:
    Renderer() = default;

    RenderNode& operator[](const std::string& node)
    {
        auto& found = nodes.emplace(node, RenderNode{}).first->second;
        found.name  = node;
        return found;
    }

    const RenderNode&          get_node(const std::string& pass_name) const;
    std::optional<std::string> root_node() const;

    Renderer compile(ColorFormat target_format, const std::weak_ptr<Device>& device) const;

    bool compiled() const
    {
        return b_compiled;
    }

    const std::shared_ptr<CustomPassList>& get_custom_passes() const
    {
        return custom_passes;
    }

  private:

    std::shared_ptr<CustomPassList> custom_passes;

    bool                                                  b_compiled = false;
    ankerl::unordered_dense::map<std::string, RenderNode> nodes;
};

} // namespace Eng::Gfx