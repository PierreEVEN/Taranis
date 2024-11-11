#pragma once


#include "gfx/types.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

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

    Attachment& format(const Gfx::ColorFormat& format)
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
    Gfx::ColorFormat         color_format      = Gfx::ColorFormat::UNDEFINED;
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
    virtual void init(const RenderPassInstance& render_pass)
    {
    }
    virtual void render(const RenderPassInstance& render_pass, const CommandBuffer& command_buffer)
    {
    }
};

class TestRenderPass : public IRenderPass
{
public:
    TestRenderPass(int)
    {
    }
};

struct RenderPassKey
{
    using Elem = std::pair<Gfx::ColorFormat, bool>;

    bool operator==(RenderPassKey& other)
    {
        if (b_present != other.b_present)
            return false;
        if (pass_data.size() != other.pass_data.size())
            return false;
        for (auto a = pass_data.begin(), b = other.pass_data.begin(); a != pass_data.end(); ++a, ++b)
            if (a->second != b->second || a->first != b->first)
                return false;
        return true;
    }

    std::vector<Elem> pass_data;
    std::string       name;
    bool              b_present = false;
};

} // namespace Eng::Gfx
template <> struct std::hash<Eng::Gfx::RenderPassKey>
{
    size_t operator()(const Eng::Gfx::RenderPassKey& val) const noexcept
    {
        auto ite = val.pass_data.begin();
        if (ite == val.pass_data.end())
            return 0;
        size_t hash = std::hash<uint32_t>()(static_cast<uint32_t>(ite->first) + 1);
        for (; ite != val.pass_data.end(); ++ite)
        {
            hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(ite->first) + 1) * 13;
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
    RenderPassKey get_key(bool b_present) const
    {
        RenderPassKey key;
        key.name      = name;
        key.b_present = b_present;
        for (const auto& attachment : attachments)
            key.pass_data.emplace_back(attachment.second.color_format, attachment.second.has_clear());
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
        b_with_imgui = enable;
        imgui_input_window = enable ? input_window : nullptr;
        return *this;
    }

    RenderNode& add_attachment(const std::string& in_name, const Attachment& attachment)
    {
        attachments.emplace(in_name, attachment);
        return *this;
    }

    RenderNode& operator[](const Attachment& attachment)
    {
        attachments.emplace(attachment.name, attachment);
        return *this;
    }

    template <typename T, typename... Args> RenderNode& render_pass(Args&&... args)
    {
        render_pass_initializer = std::make_shared<TRenderPassInitializer<T, Args...>>(std::forward<Args>(args)...);
        return *this;
    }

    std::shared_ptr<IRenderPassInitializer>     render_pass_initializer;
    std::unordered_map<std::string, Attachment> attachments;
    std::unordered_set<std::string>             dependencies;
    bool                                        b_with_imgui = false;
    std::weak_ptr<Window>                       imgui_input_window;
    std::string                                 name;
};

class Renderer
{
public:
    Renderer() = default;

    RenderNode& operator[](const std::string& node)
    {
        if (auto found = nodes.find(node); found != nodes.end())
            return found->second;

        auto& found = nodes.emplace(node, RenderNode{}).first->second;
        found.name  = node;
        return found;
    }

    const RenderNode&          get_node(const std::string& pass_name) const;
    std::optional<std::string> root_node() const;

private:
    std::unordered_map<std::string, RenderNode> nodes;
};


} // namespace Eng::Gfx