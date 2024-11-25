#pragma once
#include "command_pool.hpp"

#include <memory>
#include <utility>

#include "queue_family.hpp"
#include "shader_module.hpp"

#include <unordered_set>

namespace std
{
class shared_mutex;
}

namespace Eng::Gfx
{
class SecondaryCommandBuffer;
class VkRendererPass;
class Framebuffer;
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

struct Viewport
{
    float x = 0;
    float y = 0;
    float width;
    float height;
    float min_depth = 0.f;
    float max_depth = 1.f;
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
    virtual ~CommandBuffer();

    VkCommandBuffer raw() const
    {
        return ptr;
    }

    virtual void begin(bool one_time);
    virtual void end();
    void         submit(VkSubmitInfo submit_infos, const Fence* optional_fence = nullptr);

    void begin_debug_marker(const std::string& name, const std::array<float, 4>& color) const;
    void end_debug_marker() const;

    void draw_procedural(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) const;
    void bind_pipeline(const std::shared_ptr<Pipeline>& pipeline);
    void bind_descriptors(DescriptorSet& descriptors, const Pipeline& pipeline) const;
    void draw_mesh(const Mesh& in_buffer, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
    void draw_mesh(const Mesh& in_buffer, uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
    void set_scissor(const Scissor& scissors) const;
    void set_viewport(const Viewport& viewport) const;
    void push_constant(EShaderStage stage, const Pipeline& pipeline, const BufferData& data) const;
    void end_render_pass();

    const std::weak_ptr<Device>& get_device() const
    {
        return device;
    }


    QueueSpecialization get_specialization() const
    {
        return type;
    }

    const std::string& get_name() const
    {
        return name;
    }

protected:
    friend class SecondaryCommandBuffer;
    void                                                        reset_stats();
    std::mutex                                                  secondary_vector_mtx;
    std::unordered_set<std::shared_ptr<SecondaryCommandBuffer>> secondary_command_buffers;
    std::unique_ptr<PoolLockGuard>                              pool_lock;

  private:
    CommandBuffer(std::string name, std::weak_ptr<Device> device, QueueSpecialization type, std::thread::id thread_id, bool secondary = false);
    VkCommandBuffer           ptr = VK_NULL_HANDLE;
    CommandPool::Mutex        pool_mtx;
    QueueSpecialization       type;
    std::weak_ptr<Device>     device;
    std::thread::id           thread_id;
    std::string               name;
    std::shared_ptr<Pipeline> last_pipeline;
    bool                      is_recording      = false;
    bool                      b_wait_submission = false;
};

class SecondaryCommandBuffer : public CommandBuffer, public std::enable_shared_from_this<SecondaryCommandBuffer>
{
public:
    static std::shared_ptr<SecondaryCommandBuffer> create(const std::string& name, const std::weak_ptr<CommandBuffer>& parent, const std::weak_ptr<Framebuffer>& framebuffer,
                                                          std::thread::id    thread_id)
    {
        return std::shared_ptr<SecondaryCommandBuffer>(new SecondaryCommandBuffer(name, parent, framebuffer, thread_id));
    }

    void begin(bool one_time) override;
    void end() override;

private:
    SecondaryCommandBuffer(const std::string& name, const std::weak_ptr<CommandBuffer>& parent, const std::weak_ptr<Framebuffer>& framebuffer, std::thread::id thread_id);
    std::weak_ptr<Framebuffer>   framebuffer;
    std::weak_ptr<CommandBuffer> parent;
};

} // namespace Eng::Gfx