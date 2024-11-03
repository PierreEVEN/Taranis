#include "gfx/mesh.hpp"

namespace Engine
{
	Mesh::Mesh(const std::weak_ptr<Device>& in_device, size_t in_vertex_structure_size, EBufferType in_buffer_type) :
		buffer_type(in_buffer_type), vertex_structure_size(in_vertex_structure_size), device(in_device)
	{
	}

	void Mesh::reserve_vertices(size_t vertex_count)
	{
		if (!vertex_buffer)
		{
			vertex_buffer = std::make_unique<Buffer>(device, Buffer::CreateInfos{
				                                         .usage = EBufferUsage::VERTEX_DATA,
				                                         .access = EBufferAccess::CPU_TO_GPU,
				                                         .type = buffer_type,
			                                         }, vertex_structure_size, vertex_count);
		}
		else if (vertex_count > vertex_buffer->get_element_count()) {
			vertex_buffer->resize(vertex_structure_size, vertex_count);
		}
	}

	void Mesh::reserve_indices(size_t index_count, IndexBufferType in_index_buffer_type)
	{
		index_type = in_index_buffer_type;

		size_t size = 0;
		switch (index_type)
		{
		case IndexBufferType::Uint8:
			size = 1;
			break;
		case IndexBufferType::Uint16:
			size = 2;
			break;
		case IndexBufferType::Uint32:
			size = 4;
			break;
		default:
			LOG_FATAL("Unhandled index type");
		}

		if (!index_buffer)
		{
			index_buffer = std::make_unique<Buffer>(device, Buffer::CreateInfos{
				                                        .usage = EBufferUsage::INDEX_DATA,
				                                        .access = EBufferAccess::CPU_TO_GPU,
				                                        .type = buffer_type,
			                                        }, size, index_count);
		}
		else if (index_count > index_buffer->get_element_count() || size != index_buffer->get_stride()) {
			index_buffer->resize(size, index_count);
		}
	}

	void Mesh::set_vertices(size_t start_vertex, const BufferData& vertex_data)
	{
		reserve_vertices(start_vertex + vertex_data.get_element_count());
		vertex_buffer->set_data(start_vertex, vertex_data);
	}

	void Mesh::set_indexed_vertices(size_t start_vertex, const BufferData& vertex_data, size_t start_index,
	                                const BufferData& index_data)
	{
		set_vertices(start_vertex, vertex_data);

		IndexBufferType type;
		switch (index_data.get_stride())
		{
		case 1:
			type = IndexBufferType::Uint8;
			break;
		case 2:
			type = IndexBufferType::Uint16;
			break;
		case 4:
			type = IndexBufferType::Uint32;
			break;
		default:
			LOG_FATAL("Unhandled index buffer size");
		}

		reserve_indices(start_index + index_data.get_element_count(), type);
		index_buffer->set_data(start_index, index_data);
	}
}
