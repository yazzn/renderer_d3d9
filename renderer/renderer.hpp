#pragma once

#include <vector>
#include <d3d9.h>
#include <d3dx9.h>
#include <memory>
#include <exception>

#include "format.h"

using Vec4 = D3DXVECTOR4;
using Vec3 = D3DXVECTOR3;
using Vec2 = D3DXVECTOR2;

using Color = D3DCOLOR;

using ToplogyType = D3DPRIMITIVETYPE;

struct Vertex;
struct Batch;
struct FontHandle;

class RenderList;
class Renderer;

using RenderListPtr = std::shared_ptr<RenderList>;
using RendererPtr = std::shared_ptr<Renderer>;

class Font;

#include "font.hpp"

namespace /* anonymous namespace */
{
	bool is_toplogy_list(D3DPRIMITIVETYPE topology);
	int topology_order(D3DPRIMITIVETYPE topology);
};

void throw_if_failed(HRESULT hr);

template <typename Ty>
void safe_release(Ty &com_ptr);

class Renderer
	: public std::enable_shared_from_this<Renderer>
{
public:
	Renderer(IDirect3DDevice9 *device, std::size_t max_vertices);
	~Renderer();

	void reacquire();
	void release();

	void begin();
	void end();

	void draw(const RenderListPtr &render_list);
	void draw();

	FontHandle create_font(const std::string &family, long size, std::uint8_t flags = 0);

	template <std::size_t N>
	void add_vertices(const RenderListPtr &render_list, const Vertex(&vertex_array)[N], ToplogyType topology, IDirect3DTexture9 *texture = nullptr);

	template <std::size_t N>
	void add_vertices(const Vertex(&vertex_array)[N], ToplogyType topology, IDirect3DTexture9 *texture = nullptr);

	void draw_filled_rect(const RenderListPtr &render_list, const Vec4 &rect, Color color = 0UL);
	void draw_filled_rect(const Vec4 &rect, Color color = 0UL);

	void draw_rect(const RenderListPtr &render_list, const Vec4 &rect, float stroke_width = 1.f, Color color = 0UL);
	void draw_rect(const Vec4 &rect, float stroke_width = 1.f, Color color = 0UL);

	void draw_outlined_rect(const RenderListPtr &render_list, const Vec4 &rect, float stroke_width = 1.f, Color outline_color = 0UL, Color rect_color = 0UL);
	void draw_outlined_rect(const Vec4 &rect, float stroke_width = 1.f, Color outline_color = 0UL, Color rect_color = 0UL);

	void draw_line(const RenderListPtr &render_list, const Vec2 &from, const Vec2 &to, Color color = 0UL);
	void draw_line(const Vec2 &from, const Vec2 &to, Color color = 0UL);

	void draw_circle(const RenderListPtr &render_list, const Vec2 &position, float radius, Color color = 0UL);
	void draw_circle(const Vec2 &position, float radius, Color color = 0UL);

	void draw_pixel(const RenderListPtr &render_list, const Vec2 &position, Color color = 0UL);
	void draw_pixel(const Vec2 &position, Color color = 0UL);

	void draw_pixels(const RenderListPtr &render_list, const Vec2 &position, float square, Color color = 0UL);
	void draw_pixels(const Vec2 &position, float square, Color color = 0UL);

	void draw_radar(const RenderListPtr &render_list, const Vec2 &position, float size = 150.f, float stroke_width = 1.f, Color outline_color = 0UL, Color rect_color = 0UL);
	void draw_radar(const Vec2 &position, float size = 150.f, float stroke_width = 1.f, Color outline_color = 0UL, Color rect_color = 0UL);

	Vec2 get_text_extent(FontHandle font, const std::string &text);

	void draw_text(const RenderListPtr &render_list, FontHandle font, Vec2 pos, const std::string& text, Color color = 0UL, std::uint8_t flags = 0);
	void draw_text(FontHandle font, Vec2 position, const std::string &text, Color color = 0UL, std::uint8_t flags = 0);

	RendererPtr make_ptr();
	RenderListPtr make_render_list();

private:
	IDirect3DDevice9                   *device;
	IDirect3DVertexBuffer9             *vertex_buffer;
	IDirect3DStateBlock9               *prev_state_block;
	IDirect3DStateBlock9               *render_state_block;

	std::size_t                        max_vertices;

	RenderListPtr                      render_list;
	std::vector<std::unique_ptr<Font>> fonts;
};

struct Batch
{
	Batch(std::size_t count, ToplogyType topology, IDirect3DTexture9 *texture = nullptr);

	std::size_t count;
	ToplogyType topology;
	IDirect3DTexture9 *texture;
};

struct FontHandle
{
	FontHandle() = default;
	explicit FontHandle(std::size_t id);

	std::size_t id;
};

constexpr unsigned long vertex_definition = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;

struct Vertex
{
	Vertex() = default;

	Vertex(Vec4 position, Color color, Vec2 tex);
	Vertex(Vec4 position, Color color);
	Vertex(Vec3 position, Color color);
	Vertex(Vec2 position, Color color);

	Vertex(float x, float y, float z, Color color);
	Vertex(float x, float y, Color color);

	Vec4 position;
	Color color;
	Vec2 tex{};
};

class RenderList
	: public std::enable_shared_from_this<RenderList>
{
public:
	RenderList() = delete;
	RenderList(std::size_t max_vertices);

	RenderListPtr make_ptr();
	void clear();

protected:
	friend class Renderer;

	std::vector<Vertex>	vertices;
	std::vector<Batch>	batches;
};

#include "renderer.inl"