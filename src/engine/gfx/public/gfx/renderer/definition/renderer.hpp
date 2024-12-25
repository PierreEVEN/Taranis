#pragma once

#include "logger.hpp"
#include "render_pass_id.hpp"
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
class RenderPassInstanceBase;
class CustomPassList;
}

namespace Eng::Gfx
{
class Window;
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
    virtual void init(const RenderPassInstanceBase&)
    {
    }

    virtual void on_create_framebuffer(const RenderPassInstanceBase&)
    {
    }

    virtual void pre_draw(const RenderPassInstanceBase&)
    {
    }

    virtual void draw(const RenderPassInstanceBase&, CommandBuffer&, size_t)
    {
    }

    virtual void pre_submit(const RenderPassInstanceBase&)
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
        if (b_present != other.b_present || b_reversed_z != other.b_reversed_z)
            return false;
        if (attachments.size() != other.attachments.size())
            return false;
        for (auto a = attachments.begin(), b = other.attachments.begin(); a != attachments.end(); ++a, ++b)
            if (a->color_format != b->color_format || a->has_clear() != b->has_clear())
                return false;
        return true;
    }

    std::vector<Attachment> attachments;
    RenderPassRef           render_pass_ref;
    bool                    b_reversed_z = false;
    bool                    b_present    = false;
    bool                    reverse_cull = false;
};

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
        if (b_is_compute_pass)
            LOG_FATAL("Cannot create Render Pass key for compute passes")
        
        RenderPassKey key;
        key.render_pass_ref = render_pass_ref;
        key.b_present       = b_present;
        key.b_reversed_z    = reversed_logarithmic_depth;
        key.reverse_cull    = b_flip_culling;
        for (const auto& attachment : attachments_sorted)
            key.attachments.emplace_back(attachment);
        return key;
    }

    RenderNode& require(std::initializer_list<RenderPassGenericId> required)
    {
        for (const auto& item : required)
            dependencies.insert(item);
        return *this;
    }

    RenderNode& require(const RenderPassGenericId& required)
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

    RenderNode& reversed_log_z(bool enabled)
    {
        reversed_logarithmic_depth = enabled;
        return *this;
    }

    RenderNode& flip_culling(bool flip)
    {
        b_flip_culling = flip;
        return *this;
    }

    RenderNode& compute_pass(bool is_compute_pass)
    {
        b_is_compute_pass = is_compute_pass;
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
    ankerl::unordered_dense::set<RenderPassGenericId>     dependencies;
    bool                                                  b_with_imgui = false;
    std::weak_ptr<Window>                                 imgui_input_window;
    RenderPassRef                                         render_pass_ref;
    ResizeCallback                                        resize_callback_ptr;
    bool                                                  reversed_logarithmic_depth = false;
    bool                                                  b_flip_culling             = false;
    bool                                                  b_is_compute_pass;
    std::vector<Attachment>                               attachments_sorted;

    const Attachment* find_attachment_by_name(const std::string& attachment_name) const
    {
        if (auto found = attachments.find(attachment_name); found != attachments.end())
            return &found->second;
        return nullptr;
    }

  private:
    ankerl::unordered_dense::map<std::string, Attachment> attachments;
};

class Renderer
{
public:
    Renderer() = default;

    RenderNode& operator[](const RenderPassGenericId& node)
    {
        auto& found           = nodes.emplace(node, RenderNode{}).first->second;
        found.render_pass_ref = RenderPassRef(node, 0);
        return found;
    }

    const RenderNode&                  get_node(const RenderPassGenericId& pass_name) const;
    std::optional<RenderPassGenericId> root_node() const;

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

    bool                                                          b_compiled = false;
    ankerl::unordered_dense::map<RenderPassGenericId, RenderNode> nodes;
};

} // namespace Eng::Gfx