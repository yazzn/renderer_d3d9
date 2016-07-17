#include "font.hpp" 

Font::Font(const RendererPtr &renderer, IDirect3DDevice9 *device, const std::string &family, long height, std::uint8_t flags)
	: renderer(renderer), device(device), family(family), height(height), flags(flags), spacing(0), texture(nullptr)
{
	HDC gdi_ctx           = nullptr;

	HGDIOBJ gdi_font      = nullptr;
	HGDIOBJ prev_gdi_font = nullptr;
	HBITMAP bitmap        = nullptr;
	HGDIOBJ prev_bitmap   = nullptr;

	text_scale = 1.0f;

	gdi_ctx = CreateCompatibleDC(nullptr);
	SetMapMode(gdi_ctx, MM_TEXT);
	
	create_gdi_font(gdi_ctx, &gdi_font);
	
	if (!gdi_font) {
		throw std::runtime_error("Font::ctor(): Failed to create GDI font!");
	}

	prev_gdi_font = SelectObject(gdi_ctx, gdi_font);

	tex_width = tex_height = 128;

	HRESULT hr = S_OK;
	while (D3DERR_MOREDATA == (hr = paint_alphabet(gdi_ctx, true)))
	{
		tex_width *= 2;
		tex_height *= 2;
	}

	if (FAILED(hr))
		throw std::runtime_error("Font::ctor(): Failed to paint alphabet!");

	D3DCAPS9 d3dCaps;
	device->GetDeviceCaps(&d3dCaps);

	if (tex_width > static_cast<long>(d3dCaps.MaxTextureWidth))
	{
		text_scale = static_cast<float>(d3dCaps.MaxTextureWidth) / tex_width;
		tex_width = tex_height = d3dCaps.MaxTextureWidth;

		bool first_iteration = true;

		do
		{
			if (!first_iteration)
				text_scale *= 0.9f;

			DeleteObject(SelectObject(gdi_ctx, prev_gdi_font));

			create_gdi_font(gdi_ctx, &gdi_font);

			if (!gdi_font) {
				throw std::runtime_error("Font::ctor(): Failed to create GDI font!");
			}

			prev_gdi_font = SelectObject(gdi_ctx, gdi_font);

			first_iteration = false;
		} while (D3DERR_MOREDATA == (hr = paint_alphabet(gdi_ctx, true)));
	}

	if (FAILED(device->CreateTexture(tex_width, tex_height, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, &texture, nullptr)))
		throw std::runtime_error("Font::ctor(): Failed to create texture!");

	DWORD *bitmap_bits;

	BITMAPINFO bitmap_ctx {};
	bitmap_ctx.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bitmap_ctx.bmiHeader.biWidth       = tex_width;
	bitmap_ctx.bmiHeader.biHeight      = -tex_height;
	bitmap_ctx.bmiHeader.biPlanes      = 1;
	bitmap_ctx.bmiHeader.biCompression = BI_RGB;
	bitmap_ctx.bmiHeader.biBitCount    = 32;

	bitmap = CreateDIBSection(gdi_ctx, &bitmap_ctx, DIB_RGB_COLORS, reinterpret_cast<void**>(&bitmap_bits), nullptr, 0);

	prev_bitmap = SelectObject(gdi_ctx, bitmap);

	SetTextColor(gdi_ctx, RGB(255, 255, 255));
	SetBkColor(gdi_ctx, 0x00000000);
	SetTextAlign(gdi_ctx, TA_TOP);

	if (FAILED(paint_alphabet(gdi_ctx, false)))
		throw std::runtime_error("Font::ctor(): Failed to paint alphabet!");

	D3DLOCKED_RECT locked_rect;
	texture->LockRect(0, &locked_rect, nullptr, 0);

	std::uint8_t *dst_row = static_cast<std::uint8_t *>(locked_rect.pBits);
	BYTE alpha;

	for (long y = 0; y < tex_height; y++)
	{
		std::uint16_t *dst = reinterpret_cast<std::uint16_t *>(dst_row);
		for (long x = 0; x < tex_width; x++)
		{
			alpha = ((bitmap_bits[tex_width*y + x] & 0xff) >> 4);
			if (alpha > 0)
			{
				*dst++ = ((alpha << 12) | 0x0fff);
			}
			else
			{
				*dst++ = 0x0000;
			}
		}
		dst_row += locked_rect.Pitch;
	}

	if (texture)
		texture->UnlockRect(0);

	SelectObject(gdi_ctx, prev_bitmap);
	SelectObject(gdi_ctx, prev_gdi_font);
	DeleteObject(bitmap);
	DeleteObject(gdi_font);
	DeleteDC(gdi_ctx);
}

Font::~Font()
{
	safe_release(texture);
}

void Font::create_gdi_font(HDC ctx, HGDIOBJ *gdi_font)
{
	int character_height = -MulDiv(height, static_cast<int>(GetDeviceCaps(ctx, LOGPIXELSY) * text_scale), 72);

	DWORD bold = (flags & FONT_BOLD) ? FW_BOLD : FW_NORMAL;
	DWORD italic = (flags & FONT_ITALIC) ? TRUE : FALSE;
	
	*gdi_font = CreateFontA(character_height, 0, 0, 0, bold, italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, family.c_str());
}

long Font::paint_alphabet(HDC ctx, bool measure_only /*= false*/)
{
	SIZE size;
	char chr[2] = "x";

	if (0 == GetTextExtentPoint32(ctx, chr, 1, &size))
		return E_FAIL;

	spacing = static_cast<long>(ceil(size.cy * 0.3f));

	long x = spacing;
	long y = 0;

	for (char c = 32; c < 127; c++)
	{
		chr[0] = c;
		if (0 == GetTextExtentPoint32(ctx, chr, 1, &size))
			return E_FAIL;

		if (x + size.cx + spacing > tex_width)
		{
			x = spacing;
			y += size.cy + 1;
		}

		if (y + size.cy > tex_height)
			return D3DERR_MOREDATA;

		if (!measure_only)
		{
			if (0 == ExtTextOut(ctx, x + 0, y + 0, ETO_OPAQUE, nullptr, chr, 1, nullptr))
				return E_FAIL;

			tex_coords[c - 32][0] = (static_cast<float>(x + 0 - spacing))       / tex_width;
			tex_coords[c - 32][1] = (static_cast<float>(y + 0 + 0))             / tex_height;
			tex_coords[c - 32][2] = (static_cast<float>(x + size.cx + spacing)) / tex_width;
			tex_coords[c - 32][3] = (static_cast<float>(y + size.cy + 0))       / tex_height;
		}

		x += size.cx + (2 * spacing);
	}

	return S_OK;
}

Vec2 Font::get_text_extent(const std::string &text)
{
	float row_width = 0.f;
	float row_height = (tex_coords[0][3] - tex_coords[0][1])* tex_height;
	float width = 0.f;
	float height = row_height;

	for (const auto &c : text)
	{
		if (c == '\n')
		{
			row_width = 0.f;
			height += row_height;
		}

		if (c < ' ')
			continue;

		float tx1 = tex_coords[c - 32][0];
		float tx2 = tex_coords[c - 32][2];

		row_width += (tx2 - tx1) * tex_width - 2.f * spacing;

		if (row_width > width)
			width = row_width;
	}

	return { width, height };
}

void Font::draw_text(const RenderListPtr &render_list, Vec2 pos, const std::string &text, Color color, std::uint8_t flags)
{
	std::size_t num_to_skip = 0;

	if (flags & (TEXT_RIGHT | TEXT_CENTERED))
	{
		Vec2 size = get_text_extent(text);
		
		if (flags & TEXT_RIGHT)
			pos.x -= size.x;
		else if (flags & TEXT_CENTERED_X)
			pos.x -= 0.5f * size.x;

		if (flags & TEXT_CENTERED_Y)
			pos.y -= 0.5f * size.y;
	}

	pos.x -= spacing;

	float start_x = pos.x;

	for (const auto &c : text)
	{
		if (num_to_skip > 0 && num_to_skip-- > 0)
			continue;

		if (flags & TEXT_COLORTAGS && c == '{') // format: {#aarrggbb} or {##rrggbb}, {#aarrggbb} will inherit alpha from color argument.
		{
			std::size_t index = &c - &text[0];
			if (std::size(text) > index + 11)
			{
				std::string color_str = text.substr(index, 11);
				if (color_str[1] == '#')
				{
					bool alpha = false;
					if ((alpha = color_str[10] == '}') || color_str[8] == '}')
					{
						num_to_skip += alpha ? 10 : 8;
						color_str.erase(std::remove_if(std::begin(color_str), std::end(color_str), [](char c) { return !std::isalnum(c); }), std::end(color_str));
						color = std::stoul(alpha ? color_str : "ff" + color_str, nullptr, 16);
						continue;
					}
				}
			}
		}

		if (c == '\n')
		{
			pos.x = start_x;
			pos.y += (tex_coords[0][3] - tex_coords[0][1]) * tex_height;
		}

		if (c < ' ')
			continue;

		float tx1 = tex_coords[c - 32][0];
		float ty1 = tex_coords[c - 32][1];
		float tx2 = tex_coords[c - 32][2];
		float ty2 = tex_coords[c - 32][3];

		float w = (tx2 - tx1) * tex_width / text_scale;
		float h = (ty2 - ty1) * tex_height / text_scale;

		if (c != ' ')
		{
			Vertex v[] =
			{
				{ Vec4{ pos.x - 0.5f,     pos.y - 0.5f + h, 0.9f, 1.f }, color, Vec2{ tx1, ty2 } },
				{ Vec4{ pos.x - 0.5f,     pos.y - 0.5f,     0.9f, 1.f }, color, Vec2{ tx1, ty1 } },
				{ Vec4{ pos.x - 0.5f + w, pos.y - 0.5f + h, 0.9f, 1.f }, color, Vec2{ tx2, ty2 } },

				{ Vec4{ pos.x - 0.5f + w, pos.y - 0.5f,     0.9f, 1.f }, color, Vec2{ tx2, ty1 } },
				{ Vec4{ pos.x - 0.5f + w, pos.y - 0.5f + h, 0.9f, 1.f }, color, Vec2{ tx2, ty2 } },
				{ Vec4{ pos.x - 0.5f,     pos.y - 0.5f,     0.9f, 1.f }, color, Vec2{ tx1, ty1 } }
			};

			if (flags & TEXT_SHADOW)
			{
				Color shadow_color = D3DCOLOR_ARGB((color >> 24) & 0xff, 0x00, 0x00, 0x00);

				for (auto &vtx : v) { vtx.color = shadow_color; vtx.position.x += 1.f; }
				renderer->add_vertices(render_list, v, D3DPT_TRIANGLELIST, texture);

				for (auto &vtx : v) { vtx.position.x -= 2.f; }
				renderer->add_vertices(render_list, v, D3DPT_TRIANGLELIST, texture);

				for (auto &vtx : v) { vtx.position.x += 1.f; vtx.position.y += 1.f; }
				renderer->add_vertices(render_list, v, D3DPT_TRIANGLELIST, texture);

				for (auto &vtx : v) { vtx.position.y -= 2.f; }
				renderer->add_vertices(render_list, v, D3DPT_TRIANGLELIST, texture);
		
				for (auto &vtx : v) { vtx.color = color; vtx.position.y -= 1.f; }
			}

			renderer->add_vertices(render_list, v, D3DPT_TRIANGLELIST, texture);
		}

		pos.x += w - (2.f * spacing);
	}
}

std::shared_ptr<Font> Font::make_ptr()
{
	return shared_from_this();
}
