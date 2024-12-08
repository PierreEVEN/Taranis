#include "gfx/renderer/definition/renderer.hpp"

#include "gfx/vulkan/swapchain.hpp"
#include "logger.hpp"

#include <ranges>

namespace Eng::Gfx
{
const RenderNode& Renderer::get_node(const std::string& pass_name) const
{
    if (auto found = nodes.find(pass_name); found != nodes.end())
        return found->second;
    LOG_FATAL("There is no render pass named {}", pass_name);
}

std::optional<RenderPassGenericId> Renderer::root_node() const
{
    ankerl::unordered_dense::set<RenderPassGenericId> roots;
    for (const auto& node_name : nodes | std::views::keys)
        roots.insert(node_name);

    for (const auto& node : nodes | std::views::values)
        for (const auto& require : node.dependencies)
            roots.erase(require);

    if (roots.empty())
    {
        if (nodes.empty())
            LOG_ERROR("Renderer has no root render pass (renderer is empty)");
        else
            LOG_ERROR("Renderer has no root render pass (maybe circular dependency)");
        return {};
    }

    if (roots.size() != 1)
    {
        LOG_ERROR("Renderer have multiple roots");
        for (const auto& root : roots)
            LOG_WARNING("\t'{}", root);
        return {};
    }
    return *roots.begin();
}

static uint64_t UNIQUE_PASS_ID = 0;

Renderer Renderer::compile(ColorFormat target_format, const std::weak_ptr<Device>& device) const
{
    Renderer copy      = *this;
    copy.custom_passes = std::make_shared<CustomPassList>(device);

    assert(!copy.b_compiled);
    copy.b_compiled = true;

    auto root = copy.root_node();
    if (!root)
        LOG_FATAL("Failed to compile renderer, no root");

    for (auto& node : copy.nodes)
        node.second.render_pass_ref.id = ++UNIQUE_PASS_ID;

    if (target_format == ColorFormat::UNDEFINED)
        return copy;

    auto& attachment = copy[*root].attachments;

    if (attachment.empty())
        copy[*root][Attachment::slot("present").format(target_format)];
    else if (attachment.size() == 1)
        attachment.begin()->second.color_format = target_format;
    else
        LOG_FATAL("Failed to compiler renderer : the root node can only contain one attachment")
    return copy;
}
} // namespace Eng::Gfx