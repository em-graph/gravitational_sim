#include "sdl_graphics.h"
#include "SDL_pixels.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace sdl_graphics;

constexpr int kGlyphW = 5;
constexpr int kGlyphH = 7;
constexpr int kAdvance = 6;
constexpr int kLineGap = 2;

/// Rows of 5 pixels, bit 4 is the leftmost column. ASCII 32..126.
constexpr uint8_t kFont[95][kGlyphH] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}, // !
    {0x0A, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00}, // "
    {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}, // #
    {0x04, 0x0F, 0x14, 0x0E, 0x05, 0x1E, 0x04}, // $
    {0x18, 0x19, 0x02, 0x04, 0x08, 0x13, 0x03}, // %
    {0x08, 0x14, 0x14, 0x08, 0x15, 0x12, 0x0D}, // &
    {0x04, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00}, // '
    {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02}, // (
    {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08}, // )
    {0x00, 0x0A, 0x04, 0x1F, 0x04, 0x0A, 0x00}, // *
    {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08}, // ,
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}, // .
    {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}, // /
    {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}, // 0
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, // 1
    {0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F}, // 2
    {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E}, // 3
    {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}, // 4
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, // 5
    {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E}, // 6
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
    {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}, // 8
    {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}, // 9
    {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}, // :
    {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x04, 0x08}, // ;
    {0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02}, // <
    {0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00}, // =
    {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08}, // >
    {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, // ?
    {0x0E, 0x11, 0x17, 0x15, 0x17, 0x10, 0x0E}, // @
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // A
    {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}, // B
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, // C
    {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}, // D
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, // E
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}, // F
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, // G
    {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, // H
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // I
    {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C}, // J
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
    {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}, // L
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}, // N
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // O
    {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}, // P
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, // Q
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}, // R
    {0x0E, 0x11, 0x10, 0x0E, 0x01, 0x11, 0x0E}, // S
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, // U
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}, // V
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, // W
    {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}, // X
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, // Y
    {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}, // Z
    {0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E}, // [
    {0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x00}, // backslash
    {0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E}, // ]
    {0x04, 0x0A, 0x11, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F}, // _
    {0x08, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00}, // `
    {0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F}, // a
    {0x10, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E}, // b
    {0x00, 0x00, 0x0E, 0x10, 0x10, 0x11, 0x0E}, // c
    {0x01, 0x01, 0x0D, 0x13, 0x11, 0x11, 0x0F}, // d
    {0x00, 0x00, 0x0E, 0x11, 0x1F, 0x10, 0x0E}, // e
    {0x06, 0x09, 0x08, 0x1C, 0x08, 0x08, 0x08}, // f
    {0x00, 0x00, 0x0F, 0x11, 0x0F, 0x01, 0x0E}, // g
    {0x10, 0x10, 0x16, 0x19, 0x11, 0x11, 0x11}, // h
    {0x04, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E}, // i
    {0x02, 0x00, 0x06, 0x02, 0x02, 0x12, 0x0C}, // j
    {0x10, 0x10, 0x12, 0x14, 0x18, 0x14, 0x12}, // k
    {0x0C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, // l
    {0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11}, // m
    {0x00, 0x00, 0x16, 0x19, 0x11, 0x11, 0x11}, // n
    {0x00, 0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E}, // o
    {0x00, 0x00, 0x1E, 0x11, 0x1E, 0x10, 0x10}, // p
    {0x00, 0x00, 0x0D, 0x13, 0x0F, 0x01, 0x01}, // q
    {0x00, 0x00, 0x16, 0x19, 0x10, 0x10, 0x10}, // r
    {0x00, 0x00, 0x0F, 0x10, 0x0E, 0x01, 0x1E}, // s
    {0x08, 0x08, 0x1C, 0x08, 0x08, 0x09, 0x06}, // t
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x13, 0x0D}, // u
    {0x00, 0x00, 0x11, 0x11, 0x11, 0x0A, 0x04}, // v
    {0x00, 0x00, 0x11, 0x11, 0x15, 0x15, 0x0A}, // w
    {0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11}, // x
    {0x00, 0x00, 0x11, 0x11, 0x0F, 0x01, 0x0E}, // y
    {0x00, 0x00, 0x1F, 0x02, 0x04, 0x08, 0x1F}, // z
    {0x02, 0x04, 0x04, 0x08, 0x04, 0x04, 0x02}, // {
    {0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // |
    {0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08}, // }
    {0x00, 0x00, 0x08, 0x15, 0x02, 0x00, 0x00}, // ~
};

int glyph_advance(unsigned char glyph, int scale)
{
    if (glyph == '\t')
        return kAdvance * 4 * scale;
    return kAdvance * scale;
}

SDL_GraphicsClass::SDL_GraphicsClass(int _width, int _height, const char* _title)
{
    this->w = _width;
    this->h = _height;
    this->title = _title;
    this->bufferSize = static_cast<uint32_t>(_width * _height) * sizeof(uint32_t);
}

SDL_GraphicsClass::~SDL_GraphicsClass()
{
    this->uninit();
}

void SDL_GraphicsClass::init()
{
    this->uninit();

    this->gram = new uint32_t[this->w * this->h];
    this->drawBuffer = new uint32_t[this->w * this->h];

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
        uninit();
        return;
    }
    this->sdlStarted = true;

    this->window = SDL_CreateWindow(this->title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, this->w, this->h,
                                    SDL_WINDOW_SHOWN);
    if (this->window == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
        uninit();
        return;
    }

    this->renderer = SDL_CreateRenderer(this->window, -1, 0);
    if (this->renderer == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s", SDL_GetError());
        uninit();
        return;
    }

    this->texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (this->texture == nullptr)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateTexture failed: %s", SDL_GetError());
        uninit();
        return;
    }
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    std::cout << "Graphics construction successful." << std::endl;
}

void SDL_GraphicsClass::uninit()
{
    if (this->texture != nullptr)
    {
        SDL_DestroyTexture(this->texture);
        this->texture = nullptr;
    }
    if (this->renderer != nullptr)
    {
        SDL_DestroyRenderer(this->renderer);
        this->renderer = nullptr;
    }
    if (this->window != nullptr)
    {
        SDL_DestroyWindow(this->window);
        this->window = nullptr;
    }
    if (this->sdlStarted)
    {
        SDL_Quit();
        this->sdlStarted = false;
    }

    delete[] this->gram;
    this->gram = nullptr;
    delete[] this->drawBuffer;
    this->drawBuffer = nullptr;
}

void SDL_GraphicsClass::flush(Color _color)
{
    if (this->drawBuffer == nullptr)
        return;
    std::fill(this->drawBuffer, this->drawBuffer + this->w * this->h, _color.full);
}

void SDL_GraphicsClass::update()
{
    if (this->drawBuffer == nullptr || this->texture == nullptr || this->renderer == nullptr)
        return;
    memcpy(this->gram, this->drawBuffer, this->bufferSize);
    SDL_UpdateTexture(this->texture, nullptr, this->gram, this->w * sizeof(Uint32));
    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
    SDL_RenderPresent(this->renderer);
}

void SDL_GraphicsClass::clean()
{
    if (this->drawBuffer == nullptr)
        return;
    memset(this->drawBuffer, 0x00000000, this->bufferSize);
}

int SDL_GraphicsClass::get_width() const
{
    return this->w;
}

int SDL_GraphicsClass::get_height() const
{
    return this->h;
}

bool SDL_GraphicsClass::contains(int x, int y) const
{
    return x >= 0 && y >= 0 && x < this->w && y < this->h;
}

Color SDL_GraphicsClass::get_color() const
{
    return this->color;
}

void SDL_GraphicsClass::set_color(Color _color)
{
    this->color = _color;
}

Color SDL_GraphicsClass::get_pixel(int x, int y)
{
    Color pixelColor = {0};
    if (this->drawBuffer == nullptr || !contains(x, y))
        return pixelColor;
    pixelColor.full = this->drawBuffer[y * this->w + x];
    return pixelColor;
}

void SDL_GraphicsClass::set_pixel(int x, int y)
{
    set_pixel(x, y, this->color);
}

void SDL_GraphicsClass::set_pixel(int x, int y, Color _color)
{
    if (this->drawBuffer == nullptr || !contains(x, y))
        return;
    this->drawBuffer[y * this->w + x] = _color.full;
}

void SDL_GraphicsClass::hline(int x0, int x1, int y, Color _color)
{
    if (this->drawBuffer == nullptr || y < 0 || y >= this->h)
        return;
    if (x0 > x1)
        std::swap(x0, x1);
    x0 = std::max(x0, 0);
    x1 = std::min(x1, this->w - 1);
    if (x0 > x1)
        return;
    std::fill(this->drawBuffer + y * this->w + x0,
              this->drawBuffer + y * this->w + x1 + 1,
              _color.full);
}

void SDL_GraphicsClass::vline(int x, int y0, int y1, Color _color)
{
    if (this->drawBuffer == nullptr || x < 0 || x >= this->w)
        return;
    if (y0 > y1)
        std::swap(y0, y1);
    y0 = std::max(y0, 0);
    y1 = std::min(y1, this->h - 1);
    if (y0 > y1)
        return;
    for (int y = y0; y <= y1; ++y)
        this->drawBuffer[y * this->w + x] = _color.full;
}

void SDL_GraphicsClass::draw_line(int x0, int y0, int x1, int y1)
{
    draw_line(x0, y0, x1, y1, this->color);
}

void SDL_GraphicsClass::draw_line(int x0, int y0, int x1, int y1, Color _color)
{
    int dx = std::abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    for (;;)
    {
        set_pixel(x0, y0, _color);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void SDL_GraphicsClass::draw_rect(int x, int y, int width, int height)
{
    draw_rect(x, y, width, height, this->color);
}

void SDL_GraphicsClass::draw_rect(int x, int y, int width, int height, Color _color)
{
    if (width <= 0 || height <= 0)
        return;
    const int x1 = x + width - 1;
    const int y1 = y + height - 1;
    hline(x, x1, y, _color);
    hline(x, x1, y1, _color);
    vline(x, y, y1, _color);
    vline(x1, y, y1, _color);
}

void SDL_GraphicsClass::fill_rect(int x, int y, int width, int height)
{
    fill_rect(x, y, width, height, this->color);
}

void SDL_GraphicsClass::fill_rect(int x, int y, int width, int height, Color _color)
{
    if (this->drawBuffer == nullptr || width <= 0 || height <= 0)
        return;
    const int x0 = std::max(x, 0);
    const int y0 = std::max(y, 0);
    const int x1 = std::min(x + width, this->w);
    const int y1 = std::min(y + height, this->h);
    if (x0 >= x1 || y0 >= y1)
        return;
    for (int row = y0; row < y1; ++row)
    {
        std::fill(this->drawBuffer + row * this->w + x0,
                  this->drawBuffer + row * this->w + x1,
                  _color.full);
    }
}

void SDL_GraphicsClass::draw_circle(int cx, int cy, int radius)
{
    draw_circle(cx, cy, radius, this->color);
}

void SDL_GraphicsClass::draw_circle(int cx, int cy, int radius, Color _color)
{
    if (radius < 0)
        return;
    if (radius == 0)
    {
        set_pixel(cx, cy, _color);
        return;
    }

    int x = radius;
    int y = 0;
    int err = 1 - radius;
    while (x >= y)
    {
        set_pixel(cx + x, cy + y, _color);
        set_pixel(cx + y, cy + x, _color);
        set_pixel(cx - y, cy + x, _color);
        set_pixel(cx - x, cy + y, _color);
        set_pixel(cx - x, cy - y, _color);
        set_pixel(cx - y, cy - x, _color);
        set_pixel(cx + y, cy - x, _color);
        set_pixel(cx + x, cy - y, _color);
        ++y;
        if (err < 0)
        {
            err += 2 * y + 1;
        }
        else
        {
            --x;
            err += 2 * (y - x) + 1;
        }
    }
}

void SDL_GraphicsClass::fill_circle(int cx, int cy, int radius)
{
    fill_circle(cx, cy, radius, this->color);
}

void SDL_GraphicsClass::fill_circle(int cx, int cy, int radius, Color _color)
{
    if (radius < 0)
        return;
    if (radius == 0)
    {
        set_pixel(cx, cy, _color);
        return;
    }

    for (int dy = -radius; dy <= radius; ++dy)
    {
        const long long remain = static_cast<long long>(radius) * radius - static_cast<long long>(dy) * dy;
        const int dx = static_cast<int>(std::sqrt(static_cast<double>(remain)));
        hline(cx - dx, cx + dx, cy + dy, _color);
    }
}

void SDL_GraphicsClass::draw_glyph(int x, int y, unsigned char glyph, Color _color, int scale)
{
    const uint8_t* rows = nullptr;
    uint8_t unknown[kGlyphH] = {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F};
    if (glyph >= 32 && glyph <= 126)
        rows = kFont[glyph - 32];
    else
        rows = unknown;

    for (int row = 0; row < kGlyphH; ++row)
    {
        for (int col = 0; col < kGlyphW; ++col)
        {
            if ((rows[row] & (1u << (kGlyphW - 1 - col))) == 0)
                continue;
            const int px = x + col * scale;
            const int py = y + row * scale;
            if (scale == 1)
            {
                set_pixel(px, py, _color);
                continue;
            }
            fill_rect(px, py, scale, scale, _color);
        }
    }
}

void SDL_GraphicsClass::draw_text(int x, int y, const char* text)
{
    draw_text(x, y, text, this->color, 1);
}

void SDL_GraphicsClass::draw_text(int x, int y, const char* text, Color _color)
{
    draw_text(x, y, text, _color, 1);
}

void SDL_GraphicsClass::draw_text(int x, int y, const char* text, Color _color, int scale)
{
    if (text == nullptr || this->drawBuffer == nullptr)
        return;
    if (scale < 1)
        scale = 1;

    const int originX = x;
    const int stepY = (kGlyphH + kLineGap) * scale;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != 0; ++p)
    {
        const unsigned char glyph = *p;
        if (glyph == '\n')
        {
            x = originX;
            y += stepY;
            continue;
        }
        if (glyph == '\r')
            continue;
        if (glyph != ' ' && glyph != '\t')
            draw_glyph(x, y, glyph, _color, scale);
        x += glyph_advance(glyph, scale);
    }
}

int SDL_GraphicsClass::text_width(const char* text, int scale) const
{
    if (text == nullptr)
        return 0;
    if (scale < 1)
        scale = 1;

    int line = 0;
    int maxLine = 0;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != 0; ++p)
    {
        if (*p == '\n')
        {
            maxLine = std::max(maxLine, line);
            line = 0;
            continue;
        }
        line += glyph_advance(*p, scale);
    }
    return std::max(maxLine, line);
}

int SDL_GraphicsClass::text_height(int scale) const
{
    if (scale < 1)
        scale = 1;
    return kGlyphH * scale;
}

int SDL_GraphicsClass::text_height(const char* text, int scale) const
{
    if (text == nullptr || text[0] == '\0')
        return 0;
    if (scale < 1)
        scale = 1;

    int lines = 1;
    for (const char* p = text; *p != '\0'; ++p)
    {
        if (*p == '\n')
            ++lines;
    }
    return lines * kGlyphH * scale + (lines - 1) * kLineGap * scale;
}
