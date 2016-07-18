#pragma once

#include <d3d9.h> 

#include <string>
#include <memory>
#include <cctype>

#include "renderer.hpp"

class Renderer;
class RenderList;

enum FontFlags : std::uint8_t
{
	FONT_DEFAULT      = 0 << 0,
	FONT_BOLD         = 1 << 0,
	FONT_ITALIC       = 1 << 1
};

enum TextFlags : std::uint8_t
{
	TEXT_LEFT         = 0 << 0,
	TEXT_RIGHT        = 1 << 1,
	TEXT_CENTERED_X   = 1 << 2,
	TEXT_CENTERED_Y   = 1 << 3,
	TEXT_CENTERED     = 1 << 2 | 1 << 3,
	TEXT_SHADOW       = 1 << 4,
	TEXT_COLORTAGS    = 1 << 5
};

class Font
	: public std::enable_shared_from_this<Font>
{
public:
	Font(const RendererPtr &renderer, IDirect3DDevice9 *device, const std::string &family, long height, std::uint8_t flags = FONT_DEFAULT);
	~Font();
	
	void draw_text(const RenderListPtr &render_list, Vec2 position, const std::string &text, Color color = 0xffffffff, std::uint8_t flags = TEXT_LEFT);
	Vec2 get_text_extent(const std::string &text);

	std::shared_ptr<Font> make_ptr();

private:
	void create_gdi_font(HDC ctx, HGDIOBJ *gdi_font);
	HRESULT paint_alphabet(HDC ctx, bool measure_only = false);

	IDirect3DDevice9     *device;
	IDirect3DTexture9    *texture;
	long                  tex_width;
	long                  tex_height;
	float                 text_scale;
	float                 tex_coords[128 - 32][4];
	long                  spacing;

	std::string           family;
	long                  height;
	std::uint8_t          flags;

	RendererPtr           renderer;
};