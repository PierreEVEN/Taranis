#include "gfx/mesh.hpp"

namespace Engine
{
	Mesh::Mesh(const std::weak_ptr<Device>& in_device, size_t in_vertex_structure_size, EBufferType buffer_type) :
		vertex_structure_size(in_vertex_structure_size), device(in_device)
	{
	}

	void Mesh::reserve_vertices(size_t vertex_count)
	{
		if (!vertex_buffer)
		{
			vertex_buffer = std::make_unique<Buffer>(device, Buffer::CreateInfos{
				                                         .usage = EBufferUsage::VERTEX_DATA,
				                                         .access = EBufferAccess::CPU_TO_GPU,
				                                         .type = EBufferType::IMMEDIATE,
				                                         .element_count = vertex_count,
				                                         .stride = vertex_structure_size,
			                                         });
		}
		vertex_buffer->resize(vertex_structure_size, vertex_count);
	}

	void Mesh::reserve_indices(size_t index_count)
	{
		if (!index_buffer)
		{
			index_buffer = std::make_unique<Buffer>(device, Buffer::CreateInfos{
														 .usage = EBufferUsage::INDEX_DATA,
														 .access = EBufferAccess::CPU_TO_GPU,
														 .type = EBufferType::IMMEDIATE,
														 .element_count = index_count,
														 .stride = vertex_structure_size,
				});
		}
		index_buffer->resize(vertex_structure_size, index_count);
	}

	void Mesh::set_vertices(size_t start_vertex, const BufferData& vertex_data, size_t start_index,
	                        const BufferData& index_data)
	{
	}

	void Mesh::set_indexed_vertices(size_t start_vertex, const BufferData& vertex_data, size_t start_index,
	                                const BufferData& index_data)
	{
	}
}
