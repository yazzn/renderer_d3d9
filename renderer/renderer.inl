#pragma once

template <std::size_t N>
void Renderer::add_vertices(const RenderListPtr &render_list, const Vertex(&vertex_array)[N], ToplogyType topology, IDirect3DTexture9 *texture)
{
	std::size_t num_vertices = std::size(render_list->vertices);
	if (std::empty(render_list->batches) || render_list->batches.back().topology != topology || render_list->batches.back().texture != texture)
	{
		render_list->batches.emplace_back(0, topology, texture);
	}

	render_list->batches.back().count += N;

	render_list->vertices.resize(num_vertices + N);
	std::memcpy(&render_list->vertices[std::size(render_list->vertices) - N], &vertex_array[0], N * sizeof(Vertex));

	switch (topology)
	{
	case D3DPT_LINESTRIP:
	case D3DPT_TRIANGLESTRIP:
		render_list->batches.emplace_back(0, D3DPT_FORCE_DWORD, nullptr);
	default:
		break;
	}
}

template <std::size_t N>
void Renderer::add_vertices(const Vertex(&vertex_array)[N], ToplogyType topology, IDirect3DTexture9 *texture)
{
	add_vertices(render_list, vertex_array, topology, texture);
}

template <typename Ty>
void safe_release(Ty &com_ptr)
{
	static_assert(std::is_pointer<Ty>::value,
		"safe_release: com_ptr is not a pointer!");

	static_assert(std::is_base_of<IUnknown, std::remove_pointer<Ty>::type>::value,
		"safe_release: com_ptr is not a pointer to a com object!");

	if (com_ptr)
	{
		com_ptr->Release();
		com_ptr = nullptr;
	}
}