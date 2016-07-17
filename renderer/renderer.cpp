#include "renderer.hpp"

Renderer::Renderer(IDirect3DDevice9 *device, std::size_t max_vertices) :
	device(device), vertex_buffer(nullptr), max_vertices(max_vertices), render_list(std::make_shared<RenderList>(max_vertices)),
	prev_state_block(nullptr), render_state_block(nullptr)
{
	if (!device)
		throw std::exception("Renderer::ctor: Device was nullptr!");

	reacquire();
}

Renderer::~Renderer()
{
	release();
}

void Renderer::reacquire()
{
	throw_if_failed(device->CreateVertexBuffer(max_vertices * sizeof(Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		vertex_definition, D3DPOOL_DEFAULT, &vertex_buffer, nullptr));

	for (int i = 0; i < 2; ++i)
	{
		device->BeginStateBlock();

		device->SetRenderState(D3DRS_ZENABLE, FALSE);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHAREF, 0x08);
		device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

		device->SetRenderState(D3DRS_LIGHTING, FALSE);

		device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		device->SetRenderState(D3DRS_CLIPPING, TRUE);
		device->SetRenderState(D3DRS_CLIPPLANEENABLE, FALSE);
		device->SetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE);
		device->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_FOGENABLE, FALSE);
		device->SetRenderState(D3DRS_COLORWRITEENABLE,
			D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN |
			D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);

		device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		device->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
		device->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
		device->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		device->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		
		device->SetFVF(vertex_definition);
		device->SetTexture(0, nullptr);
		device->SetStreamSource(0, vertex_buffer, 0, sizeof(Vertex));
		device->SetPixelShader(nullptr);

		if (i != 0)
			device->EndStateBlock(&prev_state_block);
		else
			device->EndStateBlock(&render_state_block);
	}
}

void Renderer::release()
{
	safe_release(vertex_buffer);
	safe_release(prev_state_block);
	safe_release(render_state_block);
}

void Renderer::begin()
{
	prev_state_block->Capture();
	render_state_block->Apply();
}

void Renderer::end()
{
	prev_state_block->Apply();
}

void Renderer::draw(const RenderListPtr &render_list)
{
	std::size_t num_vertices = std::size(render_list->vertices);
	if (num_vertices > 0)
	{
		void *data;

		if (num_vertices > max_vertices)
		{
			max_vertices = num_vertices;
			release();
			reacquire();
		}

		throw_if_failed(vertex_buffer->Lock(0, 0, &data, D3DLOCK_DISCARD));
		{
			std::memcpy(data, std::data(render_list->vertices), sizeof(Vertex) * num_vertices);
		}
		vertex_buffer->Unlock();
	}

	std::size_t pos = 0;

	Batch &prev_batch = render_list->batches.front();

	for (const auto &batch : render_list->batches)
	{
		int order = topology_order(batch.topology);
		if (batch.count && order > 0)
		{
			std::uint32_t primitive_count = batch.count;

			if (is_toplogy_list(batch.topology))
				primitive_count /= order;
			else
				primitive_count -= (order - 1);

			device->SetTexture(0, batch.texture);
			device->DrawPrimitive(batch.topology, pos, primitive_count);
			pos += batch.count;
		}
	}
}

void Renderer::draw()
{
	draw(render_list);
	render_list->clear();
}

FontHandle Renderer::create_font(const std::string &family, long size, std::uint8_t flags)
{
	fonts.push_back(std::make_unique<Font>(make_ptr(), device, family.c_str(), size, flags));
	return FontHandle{ fonts.size() - 1 };
}

void Renderer::draw_filled_rect(const RenderListPtr &render_list, const Vec4 &rect, Color color)
{
	Vertex v[]
	{
		{ rect.x,          rect.y,          color },
		{ rect.x + rect.z, rect.y,          color },
		{ rect.x,          rect.y + rect.w, color },

		{ rect.x + rect.z, rect.y,          color },
		{ rect.x + rect.z, rect.y + rect.w, color },
		{ rect.x,          rect.y + rect.w, color }
	};

	add_vertices(render_list, v, D3DPT_TRIANGLELIST);
}

void Renderer::draw_filled_rect(const Vec4 &rect, Color color)
{
	draw_filled_rect(render_list, rect, color);
}

void Renderer::draw_rect(const RenderListPtr &render_list, const Vec4 &rect, float stroke_width, Color color)
{
	draw_filled_rect(render_list, { rect.x, rect.y, rect.z, stroke_width }, color);
	draw_filled_rect(render_list, { rect.x, rect.y + rect.w - stroke_width, rect.z, stroke_width }, color);
	draw_filled_rect(render_list, { rect.x, rect.y, stroke_width, rect.w }, color);
	draw_filled_rect(render_list, { rect.x + rect.z - stroke_width, rect.y, stroke_width, rect.w }, color);
}

void Renderer::draw_rect(const Vec4 &rect, float stroke_width, Color color)
{
	draw_rect(render_list, rect, stroke_width, color);
}

void Renderer::draw_outlined_rect(const RenderListPtr &render_list, const Vec4 &rect, float stroke_width, Color outline_color, Color rect_color)
{
	draw_filled_rect(render_list, rect, rect_color);
	draw_rect(render_list, rect, stroke_width, outline_color);
}

void Renderer::draw_outlined_rect(const Vec4 &rect, float stroke_width, Color outline_color, Color rect_color)
{
	draw_filled_rect(render_list, rect, rect_color);
	draw_rect(render_list, rect, stroke_width, outline_color);
}

void Renderer::draw_line(const RenderListPtr &render_list, const Vec2 &from, const Vec2 &to, Color color)
{
	Vertex v[]
	{
		{ from.x, from.y, color },
		{ to.x,   to.y,   color }
	};

	add_vertices(render_list, v, D3DPT_LINELIST);
}

void Renderer::draw_line(const Vec2 &from, const Vec2 &to, Color color)
{
	draw_line(render_list, from, to, color);
}

void Renderer::draw_radar(const RenderListPtr &render_list, const Vec2 &position, float size /* = 150.f */, float stroke_width /* = 1.f */, Color outline_color /* = 0UL */, Color rect_color /* = 0UL */)
{
	//draw_outlined_rect(Rect{ x, y, size, size }, strokeWidth, outlineColor, radarColor);
	//draw_filled_rect(Rect{ x + size / 2.f - strokeWidth / 2.f, y + strokeWidth, strokeWidth, size - 2 * strokeWidth }, outlineColor);
	//draw_filled_rect(Rect{ x + strokeWidth, y + size / 2.f - strokeWidth / 2.f, size - 2 * strokeWidth, strokeWidth }, outlineColor);
}

void Renderer::draw_radar(const Vec2 &position, float size /* = 150.f */, float stroke_width /* = 1.f */, Color outline_color /* = 0UL */, Color rect_color /* = 0UL */)
{
	return draw_radar(render_list, position, size, stroke_width, outline_color, rect_color);
}

void Renderer::draw_circle(const RenderListPtr &render_list, const Vec2 &position, float radius, Color color /* = 0UL */)
{
	const int segments = 24;

	Vertex v[segments + 1];

	for (int i = 0; i <= segments; i++)
	{
		float theta = 2.f * D3DX_PI * static_cast<float>(i) / static_cast<float>(segments);

		v[i] = Vertex
		{
			position.x + radius * std::cos(theta),
			position.y + radius * std::sin(theta),
			color
		};
	}

	add_vertices(render_list, v, D3DPT_LINESTRIP);
}

void Renderer::draw_circle(const Vec2 &position, float radius, Color color /* = 0UL */)
{
	draw_circle(render_list, position, radius, color);
}

void Renderer::draw_pixel(const RenderListPtr &render_list, const Vec2 &position, Color color /* = 0UL */)
{
	draw_filled_rect(render_list, { position.x, position.y, 1.f, 1.f }, color);
}

void Renderer::draw_pixel(const Vec2 &position, Color color /* = 0UL */)
{
	draw_pixel(render_list, position, color);
}

void Renderer::draw_pixels(const RenderListPtr &render_list, const Vec2 &position, float square, Color color /* = 0UL */)
{
	draw_filled_rect(render_list, { position.x - 0.5f * square, position.y - 0.5f * square, square, square }, color);
}

void Renderer::draw_pixels(const Vec2 &position, float square, Color color /* = 0UL */)
{
	draw_pixels(render_list, position, square, color);
}

Vec2 Renderer::get_text_extent(FontHandle font, const std::string &text)
{
	return fonts[font.id]->get_text_extent(text.c_str());
}


void Renderer::draw_text(const RenderListPtr &render_list, FontHandle font, Vec2 position, const std::string &text, Color color, std::uint8_t flags)
{
	if (font.id < 0 || font.id >= std::size(fonts))
		throw std::exception(fmt::format("Renderer::draw_text: Bad font handle (identifier: {})!", font.id).c_str());

	fonts[font.id]->draw_text(render_list, { position.x, position.y }, text.c_str(), color, flags);
}

void Renderer::draw_text(FontHandle font, Vec2 position, const std::string &text, Color color, std::uint8_t flags)
{
	draw_text(render_list, font, position, text, color, flags);
}

RendererPtr Renderer::make_ptr()
{
	return shared_from_this();
}

RenderListPtr Renderer::make_render_list()
{
	return std::make_shared<RenderList>(max_vertices);
}

Batch::Batch(std::size_t count, ToplogyType topology, IDirect3DTexture9 *texture /*= nullptr*/) :
	count(count), topology(topology), texture(texture)
{
}

FontHandle::FontHandle(std::size_t id) :
	id(id)
{
}

Vertex::Vertex(Vec4 position, Color color) : position(position), color(color)
{
}

Vertex::Vertex(Vec4 position, Color color, Vec2 tex) : position(position), color(color), tex(tex)
{
}

Vertex::Vertex(Vec3 position, Color color) : position(position, 1.f), color(color)
{
}

Vertex::Vertex(Vec2 position, Color color) : position(position.x, position.y, 1.f, 1.f), color(color)
{
}

Vertex::Vertex(float x, float y, float z, Color color) : position(x, y, z, 1.f), color(color)
{
}

Vertex::Vertex(float x, float y, Color color) : position(x, y, 1.f, 1.f), color(color)
{
}

RenderList::RenderList(std::size_t max_vertices)
{
	vertices.reserve(max_vertices);
}

RenderListPtr RenderList::make_ptr()
{
	return shared_from_this();
}

void RenderList::clear()
{
	vertices.clear();
	batches.clear();
}

namespace /* anonymous namespace */
{
	bool is_toplogy_list(D3DPRIMITIVETYPE topology)
	{
		return topology == D3DPT_POINTLIST || topology == D3DPT_LINELIST || topology == D3DPT_TRIANGLELIST;
	}

	int topology_order(D3DPRIMITIVETYPE topology)
	{
		switch (topology)
		{
		case D3DPT_POINTLIST:
			return 1;
		case D3DPT_LINELIST:
		case D3DPT_LINESTRIP:
			return 2;
		case D3DPT_TRIANGLELIST:
		case D3DPT_TRIANGLESTRIP:
		case D3DPT_TRIANGLEFAN:
			return 3;
		default:
			return 0;
		}
	}
};

void throw_if_failed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception(fmt::format("Crucial Direct3D 9 operation failed! Code: %X", hr).c_str());
	}
}
