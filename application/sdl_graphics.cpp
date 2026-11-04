#include "sdl_graphics.h"
#include "SDL_pixels.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace sdl_graphics;

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
    std::fill(this->drawBuffer, this->drawBuffer + this->w * this->h, _color.full());
}

void SDL_GraphicsClass::update()
{
    if (this->drawBuffer == nullptr || this->texture == nullptr || this->renderer == nullptr)
        return;
    memcpy(this->gram, this->drawBuffer, this->bufferSize);
    SDL_UpdateTexture(this->texture, nullptr, this->gram, this->w * sizeof(Uint32));
    SDL_RenderClear(this->renderer);
    SDL_RenderCopy(this->renderer, this->texture, nullptr, nullptr);
}

void SDL_GraphicsClass::present()
{
    if (this->renderer == nullptr)
        return;
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
    if (this->drawBuffer == nullptr || !contains(x, y))
        return fromIndex(Ansi::Black);
    return Color(this->drawBuffer[y * this->w + x]);
}

void SDL_GraphicsClass::set_pixel(Pixel p)
{
    set_pixel(p.x(), p.y(), p.color());
}

void SDL_GraphicsClass::set_pixel(int x, int y)
{
    set_pixel(x, y, this->color);
}

void SDL_GraphicsClass::set_pixel(int x, int y, uint8_t alpha)
{
    set_pixel(x, y, this->color.withAlpha(alpha));
}

void SDL_GraphicsClass::set_pixel(int x, int y, Color _color)
{
    if (this->drawBuffer == nullptr || !contains(x, y))
        return;
    this->drawBuffer[y * this->w + x] = _color.full();
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
              _color.full());
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
        this->drawBuffer[y * this->w + x] = _color.full();
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
                  _color.full());
    }
}

void SDL_GraphicsClass::blit_nearest(const uint32_t* src, int srcW, int srcH, int dstX, int dstY, int dstW, int dstH)
{
    if (this->drawBuffer == nullptr || src == nullptr || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
        return;

    const int x0 = std::max(dstX, 0);
    const int y0 = std::max(dstY, 0);
    const int x1 = std::min(dstX + dstW, this->w);
    const int y1 = std::min(dstY + dstH, this->h);
    if (x0 >= x1 || y0 >= y1)
        return;

    const double stepX = static_cast<double>(srcW) / dstW;
    const double stepY = static_cast<double>(srcH) / dstH;
    double sy = static_cast<double>(y0 - dstY) * stepY;

    for (int y = y0; y < y1; ++y)
    {
        int isy = static_cast<int>(sy);
        if (isy >= srcH)
            isy = srcH - 1;
        if (isy < 0)
            isy = 0;
        const uint32_t* srcRow = src + static_cast<size_t>(isy) * static_cast<size_t>(srcW);
        uint32_t* dst = this->drawBuffer + static_cast<size_t>(y) * static_cast<size_t>(this->w) + x0;
        double sx = static_cast<double>(x0 - dstX) * stepX;
        for (int x = x0; x < x1; ++x)
        {
            int isx = static_cast<int>(sx);
            if (isx >= srcW)
                isx = srcW - 1;
            if (isx < 0)
                isx = 0;
            *dst++ = srcRow[isx];
            sx += stepX;
        }
        sy += stepY;
    }
}

uint32_t lerp_channel(uint32_t c0, uint32_t c1, int t256)
{
    uint32_t out = 0;
    for (int shift = 0; shift < 32; shift += 8)
    {
        const int a = static_cast<int>((c0 >> shift) & 0xFF);
        const int b = static_cast<int>((c1 >> shift) & 0xFF);
        const int v = a + (((b - a) * t256) >> 8);
        out |= static_cast<uint32_t>(v) << shift;
    }
    return out;
}

void SDL_GraphicsClass::blit_bilinear(const uint32_t* src, int srcW, int srcH, int dstX, int dstY, int dstW, int dstH)
{
    if (this->drawBuffer == nullptr || src == nullptr || srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
        return;

    const int x0 = std::max(dstX, 0);
    const int y0 = std::max(dstY, 0);
    const int x1 = std::min(dstX + dstW, this->w);
    const int y1 = std::min(dstY + dstH, this->h);
    if (x0 >= x1 || y0 >= y1)
        return;

    const double stepX = static_cast<double>(srcW) / dstW;
    const double stepY = static_cast<double>(srcH) / dstH;
    const double maxX = static_cast<double>(srcW - 1);
    const double maxY = static_cast<double>(srcH - 1);
    double sy = static_cast<double>(y0 - dstY) * stepY;

    for (int y = y0; y < y1; ++y)
    {
        const double clampedY = std::clamp(sy, 0.0, maxY);
        const int iy0 = static_cast<int>(clampedY);
        const int iy1 = std::min(iy0 + 1, srcH - 1);
        const int ty = std::clamp(static_cast<int>((clampedY - iy0) * 256.0), 0, 256);
        const uint32_t* row0 = src + static_cast<size_t>(iy0) * static_cast<size_t>(srcW);
        const uint32_t* row1 = src + static_cast<size_t>(iy1) * static_cast<size_t>(srcW);
        uint32_t* dst = this->drawBuffer + static_cast<size_t>(y) * static_cast<size_t>(this->w) + x0;
        double sx = static_cast<double>(x0 - dstX) * stepX;
        for (int x = x0; x < x1; ++x)
        {
            const double clampedX = std::clamp(sx, 0.0, maxX);
            const int ix0 = static_cast<int>(clampedX);
            const int ix1 = std::min(ix0 + 1, srcW - 1);
            const int tx = std::clamp(static_cast<int>((clampedX - ix0) * 256.0), 0, 256);
            const uint32_t top = lerp_channel(row0[ix0], row0[ix1], tx);
            const uint32_t bottom = lerp_channel(row1[ix0], row1[ix1], tx);
            *dst++ = lerp_channel(top, bottom, ty);
            sx += stepX;
        }
        sy += stepY;
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
