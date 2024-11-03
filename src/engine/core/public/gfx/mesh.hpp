#pragma once
#include <memory>

#include "vulkan/buffer.hpp"

namespace Engine
{
	class Device;
	class Buffer;

	enum class IndexBufferType
	{
		Uint8,
		Uint16,
		Uint32
	};

	class Mesh
	{
	public:

		Mesh(const std::weak_ptr<Device>& device, size_t vertex_structure_size, EBufferType buffer_type);
		void reserve_vertices(size_t vertex_count);
		void reserve_indices(size_t index_count, IndexBufferType index_buffer_type);
		void set_vertices(size_t start_vertex, const BufferData& vertex_data);
		void set_indexed_vertices(size_t start_vertex, const BufferData& vertex_data, size_t start_index,
		                          const BufferData& index_data);

		Buffer* get_vertices() const { return vertex_buffer.get(); }
		Buffer* get_indices() const { return index_buffer.get(); }
		const IndexBufferType& get_index_buffer_type() const { return index_type; }

	private:
		EBufferType buffer_type;
		size_t vertex_structure_size = 0;
		IndexBufferType index_type;
		std::weak_ptr<Device> device;
		std::unique_ptr<Buffer> vertex_buffer;
		std::unique_ptr<Buffer> index_buffer;
	};
}
