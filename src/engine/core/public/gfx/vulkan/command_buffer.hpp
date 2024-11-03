#pragma once
#include <memory>

#include "queue_family.hpp"
#include "gfx/shaders/shader_compiler.hpp"

namespace Engine
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
		CommandBuffer(std::weak_ptr<Device> device, QueueSpecialization type);
		CommandBuffer(CommandBuffer&) = delete;
		CommandBuffer(CommandBuffer&&) = delete;
		~CommandBuffer();
		VkCommandBuffer raw() const { return ptr; }

		void begin(bool one_time) const;
		void end() const;
		void submit(VkSubmitInfo submit_infos, const Fence* optional_fence = nullptr) const;

		void draw_procedural(uint32_t vertex_count, uint32_t first_vertex, uint32_t instance_count, uint32_t first_instance) const;
		void bind_pipeline(const Pipeline& pipeline) const;
		void bind_descriptors(DescriptorSet& descriptors, const Pipeline& pipeline) const;
		void draw_mesh(const Mesh& in_buffer, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
		void draw_mesh(const Mesh& in_buffer, uint32_t first_index, uint32_t vertex_offset, uint32_t index_count, uint32_t instance_count = 1, uint32_t first_instance = 0) const;
		void set_scissor(const Scissor& scissors) const;
		void push_constant(EShaderStage stage, const Pipeline& pipeline, const BufferData& data) const;

	private:
		VkCommandBuffer ptr = VK_NULL_HANDLE;
		QueueSpecialization type;
		std::weak_ptr<Device> device;
		std::thread::id thread_id;
	};
}
