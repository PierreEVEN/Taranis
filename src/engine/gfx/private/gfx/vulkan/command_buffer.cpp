#include <utility>

#include "gfx/vulkan/command_buffer.hpp"

#include "profiler.hpp"
#include "gfx/mesh.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_pool.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"

namespace Eng::Gfx
{
class Fence;

CommandBuffer::CommandBuffer(std::string in_name, std::weak_ptr<Device> in_device, QueueSpecialization in_type, std::thread::id thread_id, bool secondary)
    : type(in_type), device(std::move(in_device)), thread_id(thread_id), name(
                                                                                                                                      std::move(
                                                                                                                                          in_name))
{
    ptr = device.lock()->get_queues().get_queue(type)->get_command_pool().allocate(secondary, thread_id);
}

CommandBuffer::~CommandBuffer()
{
    device.lock()->get_queues().get_queue(type)->get_command_pool().free(ptr, thread_id);
}

void CommandBuffer::begin(bool one_time)
{
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : static_cast<VkCommandBufferUsageFlags>(0),
    };

    VK_CHECK(vkBeginCommandBuffer(ptr, &beginInfo), "failed to begin one time command buffer")
    device.lock()->debug_set_object_name(name, ptr);
    reset_stats();
}

void CommandBuffer::create_secondaries()
{
    if (secondary_command_buffers.empty())
    {
        for (const auto& worker : JobSystem::get().get_workers())
        {
            std::shared_ptr<CommandBuffer> new_sec = std::shared_ptr<CommandBuffer>(new CommandBuffer(name, device, type, worker->thread_id(), true));
            device.lock()->debug_set_object_name(name + "_secondary", new_sec->raw());
            secondary_command_buffers.emplace(worker->thread_id(), new_sec);
        }
    }
}

void CommandBuffer::begin_secondary(const RenderPassInstance& rp)
{
    VkCommandBufferInheritanceInfo inheritance{
        .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass  = rp.get_render_pass_resource().lock()->raw(),
        .framebuffer = rp.get_current_framebuffer().lock()->raw(),
    };

    const VkCommandBufferBeginInfo beginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance,
    };

    VK_CHECK(vkBeginCommandBuffer(raw(), &beginInfo), "failed to begin secondary command buffer");
    reset_stats();
}

void CommandBuffer::end() const
{
    vkEndCommandBuffer(ptr);
}

void CommandBuffer::merge_secondaries() const
{
    std::vector<VkCommandBuffer> p_command_buffers;
    for (const auto& sec : secondary_command_buffers)
        p_command_buffers.emplace_back(sec.second->raw());
    vkCmdExecuteCommands(ptr, static_cast<uint32_t>(p_command_buffers.size()), p_command_buffers.data());
}

void CommandBuffer::submit(VkSubmitInfo submit_infos = {}, const Fence* optional_fence) const
{
    if (optional_fence)
        optional_fence->reset();
    submit_infos.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_infos.commandBufferCount = 1;
    submit_infos.pCommandBuffers    = &ptr;
    VK_CHECK(device.lock()->get_queues().get_queue(type)->submit(*this, submit_infos, optional_fence), "Failed to submit queue");
}

void CommandBuffer::begin_debug_marker(const std::string& in_name, const std::array<float, 4>& color) const
{
    device.lock()->get_instance().lock()->begin_debug_marker(ptr, in_name, color);
}

void CommandBuffer::end_debug_marker() const
{
    device.lock()->get_instance().lock()->end_debug_marker(ptr);
}

void CommandBuffer::draw_procedural(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) const
{
    vkCmdDraw(ptr, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::bind_pipeline(const std::shared_ptr<Pipeline>& pipeline)
{
    if (last_pipeline == pipeline)
        return;
    last_pipeline = pipeline;
    if (pipeline->infos().line_width != 1.0f)
        vkCmdSetLineWidth(ptr, pipeline->infos().line_width);
    vkCmdBindPipeline(ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->raw());
}

void CommandBuffer::bind_descriptors(DescriptorSet& descriptors, const Pipeline& pipeline) const
{
    std::lock_guard lk(descriptors.test_mtx);
    vkCmdBindDescriptorSets(ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout(), 0, 1, &descriptors.raw_current(), 0, nullptr);
}

void CommandBuffer::draw_mesh(const Mesh& in_mesh, uint32_t instance_count, uint32_t first_instance) const
{
    if (const auto& vertices = in_mesh.get_vertices())
    {
        constexpr VkDeviceSize offsets[]     = {0};
        const auto             vertex_buffer = vertices->raw_current();
        vkCmdBindVertexBuffers(ptr, 0, 1, &vertex_buffer, offsets);
        if (const auto& indices = in_mesh.get_indices())
        {
            VkIndexType index_buffer_type;
            switch (in_mesh.get_index_buffer_type())
            {
            case IndexBufferType::Uint8:
                index_buffer_type = VK_INDEX_TYPE_UINT8_KHR;
                break;
            case IndexBufferType::Uint16:
                index_buffer_type = VK_INDEX_TYPE_UINT16;
                break;
            case IndexBufferType::Uint32:
                index_buffer_type = VK_INDEX_TYPE_UINT32;
                break;
            default:
                LOG_FATAL("Unhandled index type")
            }
            vkCmdBindIndexBuffer(ptr, indices->raw_current(), 0, index_buffer_type);
            vkCmdDrawIndexed(ptr, static_cast<uint32_t>(indices->get_element_count()), instance_count, 0, 0, first_instance);
        }
        else
        {
            vkCmdDraw(ptr, static_cast<uint32_t>(vertices->get_element_count()), instance_count, 0, first_instance);
        }
    }
}

void CommandBuffer::draw_mesh(const Mesh& in_mesh, uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, uint32_t instance_count, uint32_t first_instance) const
{
    if (const auto& vertices = in_mesh.get_vertices())
    {
        constexpr VkDeviceSize offsets[]     = {0};
        const auto             vertex_buffer = vertices->raw_current();
        vkCmdBindVertexBuffers(ptr, 0, 1, &vertex_buffer, offsets);
        if (const auto& indices = in_mesh.get_indices())
        {
            VkIndexType index_buffer_type;
            switch (in_mesh.get_index_buffer_type())
            {
            case IndexBufferType::Uint8:
                index_buffer_type = VK_INDEX_TYPE_UINT8_KHR;
                break;
            case IndexBufferType::Uint16:
                index_buffer_type = VK_INDEX_TYPE_UINT16;
                break;
            case IndexBufferType::Uint32:
                index_buffer_type = VK_INDEX_TYPE_UINT32;
                break;
            default:
                LOG_FATAL("Unhandled index type")
            }
            vkCmdBindIndexBuffer(ptr, indices->raw_current(), 0, index_buffer_type);
            vkCmdDrawIndexed(ptr, index_count, instance_count, first_index, static_cast<int32_t>(vertex_offset), first_instance);
        }
        else
        {
            vkCmdDraw(ptr, static_cast<uint32_t>(vertices->get_element_count()), instance_count, vertex_offset, first_instance);
        }
    }
}

void CommandBuffer::set_scissor(const Scissor& scissors) const
{
    const VkRect2D vk_scissor{
        .offset =
        VkOffset2D{
            .x = scissors.offset_x,
            .y = scissors.offset_y,
        },
        .extent =
        VkExtent2D{
            .width = scissors.width,
            .height = scissors.height,
        },
    };
    vkCmdSetScissor(ptr, 0, 1, &vk_scissor);
}

void CommandBuffer::push_constant(EShaderStage stage, const Pipeline& pipeline, const BufferData& data) const
{
    vkCmdPushConstants(ptr, pipeline.get_layout(), static_cast<VkShaderStageFlags>(stage), 0, static_cast<uint32_t>(data.get_byte_size()), data.data());
}

void CommandBuffer::reset_stats()
{
    last_pipeline = nullptr;
}
} // namespace Eng::Gfx