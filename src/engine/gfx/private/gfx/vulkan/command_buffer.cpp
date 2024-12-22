#include <utility>

#include "gfx/vulkan/command_buffer.hpp"

#include "gfx/mesh.hpp"
#include "gfx/vulkan/buffer.hpp"
#include "gfx/vulkan/command_pool.hpp"
#include "gfx/vulkan/descriptor_sets.hpp"
#include "gfx/vulkan/device.hpp"
#include "gfx/vulkan/fence.hpp"
#include "gfx/vulkan/pipeline.hpp"
#include "gfx/vulkan/vk_render_pass.hpp"
#include "jobsys/job_sys.hpp"
#include "profiler.hpp"
#include "gfx/vulkan/compute_pipeline.hpp"
#include "gfx/vulkan/framebuffer.hpp"
#include "gfx/vulkan/pipeline_layout.hpp"

namespace Eng::Gfx
{
class Fence;

CommandBuffer::CommandBuffer(std::string in_name, std::weak_ptr<Device> in_device, QueueSpecialization in_type, std::thread::id thread_id, bool secondary)
    : type(in_type), device(std::move(in_device)), thread_id(thread_id), name(std::move(in_name))
{
    auto allocation = device.lock()->get_queues().get_queue(type)->get_command_pool().allocate(secondary, thread_id);
    ptr             = allocation.first;
    pool_mtx        = allocation.second;
}

SecondaryCommandBuffer::SecondaryCommandBuffer(std::string name, std::weak_ptr<Device> in_device, QueueSpecialization in_type, std::thread::id thread_id)
    : CommandBuffer(std::move(name), std::move(in_device), std::move(in_type), thread_id, true)
{
}

CommandBuffer::~CommandBuffer()
{
    device.lock()->get_queues().get_queue(type)->get_command_pool().free(ptr, thread_id);
}

void CommandBuffer::begin(bool one_time)
{
    if (b_wait_submission)
        return;
    assert(!pool_lock);
    pool_lock = std::make_unique<PoolLockGuard>(pool_mtx);
    assert(!is_recording);
    b_wait_submission                        = true;
    is_recording                             = true;
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = one_time ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : static_cast<VkCommandBufferUsageFlags>(0),
    };

    VK_CHECK(vkBeginCommandBuffer(ptr, &beginInfo), "failed to begin one time command buffer")
    device.lock()->debug_set_object_name(name, ptr);
    reset_stats();
    pool_lock = nullptr;
}

void CommandBuffer::end()
{
    assert(b_wait_submission);
    if (!is_recording)
        return;
    assert(!pool_lock);
    pool_lock    = std::make_unique<PoolLockGuard>(pool_mtx);
    is_recording = false;
    vkEndCommandBuffer(ptr);
    pool_lock = nullptr;
}

void CommandBuffer::submit(VkSubmitInfo submit_infos = {}, const Fence* optional_fence)
{
    b_wait_submission = false;
    if (optional_fence)
        optional_fence->reset();
    submit_infos.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_infos.commandBufferCount = 1;
    submit_infos.pCommandBuffers    = &ptr;
    PoolLockGuard lk(pool_mtx);
    VK_CHECK(device.lock()->get_queues().get_queue(type)->submit(*this, submit_infos, optional_fence), "Failed to submit queue");
}

void CommandBuffer::begin_debug_marker(const std::string& in_name, const std::array<float, 4>& color)
{
    assert(std::this_thread::get_id() == thread_id);
    thread_lock();
    device.lock()->get_instance().lock()->begin_debug_marker(ptr, in_name, color);
    thread_unlock();
}

void CommandBuffer::end_debug_marker() 
{
    assert(std::this_thread::get_id() == thread_id);
    thread_lock();
    device.lock()->get_instance().lock()->end_debug_marker(ptr);
    thread_unlock();
}

void CommandBuffer::draw_procedural(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) const
{
    assert(std::this_thread::get_id() == thread_id);
    vkCmdDraw(ptr, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::bind_pipeline(const std::shared_ptr<Pipeline>& pipeline)
{
    assert(std::this_thread::get_id() == thread_id);
    if (last_pipeline == pipeline)
        return;
    last_pipeline = pipeline;
    if (pipeline->infos().options.line_width != 1.0f)
        vkCmdSetLineWidth(ptr, pipeline->infos().options.line_width);
    vkCmdBindPipeline(ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->raw());
}

void CommandBuffer::bind_descriptors(const DescriptorSet& descriptors, const Pipeline& pipeline) const
{
    assert(std::this_thread::get_id() == thread_id);
    vkCmdBindDescriptorSets(ptr, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.get_layout()->raw(), 0, 1, &descriptors.raw_current(), 0, nullptr);
}

void CommandBuffer::draw_mesh(const Mesh& in_mesh, uint32_t instance_count, uint32_t first_instance) const
{
    assert(std::this_thread::get_id() == thread_id);
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
    assert(std::this_thread::get_id() == thread_id);
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
    assert(std::this_thread::get_id() == thread_id);
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

void CommandBuffer::set_viewport(const Viewport& in_viewport) const
{
    assert(std::this_thread::get_id() == thread_id);
    const VkViewport viewport{
        .x = in_viewport.x,
        .y = in_viewport.y,
        .width = in_viewport.width,
        .height = in_viewport.height,
        .minDepth = in_viewport.min_depth,
        .maxDepth = in_viewport.max_depth,
    };
    vkCmdSetViewport(ptr, 0, 1, &viewport);
}

void CommandBuffer::push_constant(EShaderStage stage, const Pipeline& pipeline, const BufferData& data) const
{
    assert(std::this_thread::get_id() == thread_id);
    vkCmdPushConstants(ptr, pipeline.get_layout()->raw(), static_cast<VkShaderStageFlags>(stage), 0, static_cast<uint32_t>(data.get_byte_size()), data.data());
}

void CommandBuffer::begin_render_pass(const RenderPassRef& pass_name, const VkRenderPassBeginInfo& begin_infos, bool parallel_rendering)
{
    thread_lock();
    vkCmdBeginRenderPass(ptr, &begin_infos, parallel_rendering ? VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS : VK_SUBPASS_CONTENTS_INLINE);
    render_pass_name = pass_name;
    thread_unlock();
}

void CommandBuffer::bind_compute_pipeline(const ComputePipeline& pipeline) const
{
    vkCmdBindPipeline(ptr, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.raw());
}

void CommandBuffer::bind_descriptors(const DescriptorSet& descriptors, const ComputePipeline& pipeline) const
{
    vkCmdBindDescriptorSets(ptr, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.get_layout()->raw(), 0, 1, &descriptors.raw_current(), 0, 0);
}

void CommandBuffer::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) const
{
    vkCmdDispatch(ptr, x, y, z);
}

void CommandBuffer::end_render_pass()
{
    PROFILER_SCOPE(EndRenderPass);
    thread_lock();
    if (!secondary_command_buffers.empty())
    {
        PROFILER_SCOPE(ExecuteSecondaryCommandBuffers);
        std::vector<VkCommandBuffer> p_command_buffers;
        for (const auto& sec : secondary_command_buffers)
            p_command_buffers.emplace_back(sec->raw());
        vkCmdExecuteCommands(ptr, static_cast<uint32_t>(p_command_buffers.size()), p_command_buffers.data());
        secondary_command_buffers.clear();
    }
    vkCmdEndRenderPass(ptr);
    render_pass_name = {};
    thread_unlock();
}

void CommandBuffer::reset_stats()
{
    last_pipeline = nullptr;
}

void SecondaryCommandBuffer::begin(bool one_time)
{
    assert(!one_time);
    if (b_wait_submission)
        return;
    assert(!is_recording);
    b_wait_submission = true;
    is_recording      = true;
    VkCommandBufferInheritanceInfo inheritance{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass = framebuffer->get_render_pass_resource().lock()->raw(),
        .framebuffer = framebuffer->raw(),
    };

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritance,
    };

    VK_CHECK(vkBeginCommandBuffer(raw(), &beginInfo), "failed to begin secondary command buffer");

    render_pass_name = parent->render_pass_name;

    get_device().lock()->debug_set_object_name(get_name(), raw());
    reset_stats();
}

void SecondaryCommandBuffer::end()
{
    if (!is_recording)
        return;
    CommandBuffer::end();
    b_wait_submission = false;
    std::lock_guard lk(parent->secondary_vector_mtx);
    parent->secondary_command_buffers.insert(shared_from_this());
}

void CommandBuffer::thread_lock()
{
    assert(!pool_lock);
    pool_lock = std::make_unique<PoolLockGuard>(pool_mtx);
}

void CommandBuffer::thread_unlock()
{
    pool_lock = nullptr;
}

} // namespace Eng::Gfx