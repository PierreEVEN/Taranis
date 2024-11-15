#pragma once
#include <memory>
#include <utility>

#include "gfx/shaders/shader_compiler.hpp"
#include "queue_family.hpp"

namespace Eng::Gfx
{
class RenderPassInstance;
}

namespace Eng
{
class MaterialAsset;
}

namespace Eng::Gfx
{
class DescriptorSet;
class Pipeline;
class BufferData;
class Mesh;
class Fence;
class Device;

struct Scissor
{
    int32_t  offset_x;
    int32_t  offset_y;
    uint32_t width;
    uint32_t height;
};

class CommandBuffer
{
public:
    static std::shared_ptr<CommandBuffer> create(const std::string& name, std::weak_ptr<Device> device, QueueSpecialization type)
    {
        return std::shared_ptr<CommandBuffer>(new CommandBuffer(name, std::move(device), type, std::this_thread::get_id()));
    }

    CommandBuffer(CommandBuffer&)  = delete;
    CommandBuffer(CommandBuffer&&) = delete;
    ~CommandBuffer();

    VkCommandBuffer raw() const
    {
        return ptr;
    }

    void create_secondaries();
    void begin_secondary(const RenderPassInstance& rp);
    void begin(bool one_time);
    void end() const;
    void merge_secondaries() const;
    void submit(VkSubmitInfo submit_infos, const Fence* optional_fence = nullptr) const;

    void begin_debug_marker(const std::string& name, const std::array<float, 4>& color) const;
    void end_debug_marker() const;

    void draw_procedural(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) const;
    void bind_pipeline(const std::shared_ptr<Pipeline>& pipeline);
    void bind_descriptors(DescriptorSet& descriptors, const Pipeline& pipeline) const;
    void draw_mesh(const Mesh& in_buffer, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
    void draw_mesh(const Mesh& in_buffer, uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
    void set_scissor(const Scissor& scissors) const;
    void push_constant(EShaderStage stage, const Pipeline& pipeline, const BufferData& data) const;

    const std::unordered_map<std::thread::id, std::shared_ptr<CommandBuffer>>& get_secondary_command_buffers() const
    {
        return secondary_command_buffers;
    }

private:
    void reset_stats();

    CommandBuffer(std::string name, std::weak_ptr<Device> device, QueueSpecialization type, std::thread::id thread_id, bool secondary = false);
    VkCommandBuffer                                                     ptr = VK_NULL_HANDLE;
    QueueSpecialization                                                 type;
    std::weak_ptr<Device>                                               device;
    std::thread::id                                                     thread_id;
    std::string                                                         name;
    std::shared_ptr<Pipeline>                                           last_pipeline;
    std::unordered_map<std::thread::id, std::shared_ptr<CommandBuffer>> secondary_command_buffers;
};
} // namespace Eng::Gfx