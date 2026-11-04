#pragma once

#include <SDL.h>

#include "GridCanvas.h"
#include "color.h"

namespace sdl_graphics
{
    using grid_canvas::Point2I;
    using grid_canvas::Point2D;
    using namespace color;

    // ARGB8888, matches SDL_PIXELFORMAT_ARGB8888
    class Pixel
    {
        Color c{};
        Point2D pos;
    public:
        Pixel(const int x, const int y, const Color _color)
        {
            this->pos = {x, y};
            this->c = _color;
        }
        Pixel(const int x, const int y)
        {
            this->pos = {x, y};
            this->c = fromIndex(Ansi::White);
        }
        Pixel(Point2D pos, const Color _color)
        {
            this->pos = pos;
            this->c = _color;
        }
        double x() const
        {
            return this->pos.x();
        }
        double y() const
        {
            return this->pos.y();
        }

        [[nodiscard]] Color color() const
        {
            return this->c;
        }
    };
    class SDL_GraphicsClass
    {
        int w{}, h{};
        uint32_t bufferSize; //Draw buffer byte count
        Color color{};
        const char* title;
        uint32_t* gram{};
        uint32_t* drawBuffer{};
        SDL_Window* window{};
        SDL_Renderer* renderer{};
        SDL_Texture* texture{};
        bool sdlStarted{};

        [[nodiscard]] bool contains(int x, int y) const;
        void hline(int x0, int x1, int y, Color color);
        void vline(int x, int y0, int y1, Color color);

    public:
        SDL_GraphicsClass(int width, int height, const char* title);

        ~SDL_GraphicsClass();

        void init(); ///SDL Window size
        void uninit();
        void flush(Color _color);

        void update();
        void present();
        void clean();

        [[nodiscard]] SDL_Window* sdl_window() const { return this->window; }
        [[nodiscard]] SDL_Renderer* sdl_renderer() const { return this->renderer; }

        [[nodiscard]] int get_width() const;
        [[nodiscard]] int get_height() const;

        Color get_pixel(int x, int y);
        void set_pixel(int x, int y);
        void set_pixel(int x, int y, Color color);
        void set_pixel(Pixel p);

        [[nodiscard]] Color get_color() const;
        void set_color(Color _color);

        void draw_line(int x0, int y0, int x1, int y1);
        void draw_line(int x0, int y0, int x1, int y1, Color _color);

        void draw_rect(int x, int y, int width, int height);
        void draw_rect(int x, int y, int width, int height, Color _color);
        void fill_rect(int x, int y, int width, int height);
        void fill_rect(int x, int y, int width, int height, Color _color);

        /// Scale a row-major ARGB image into the destination rect. Clipped to the window.
        void blit_nearest(const uint32_t* src, int srcW, int srcH, int dstX, int dstY, int dstW, int dstH);
        void blit_bilinear(const uint32_t* src, int srcW, int srcH, int dstX, int dstY, int dstW, int dstH);

        void draw_circle(int cx, int cy, int radius);
        void draw_circle(int cx, int cy, int radius, Color _color);
        void fill_circle(int cx, int cy, int radius);
        void fill_circle(int cx, int cy, int radius, Color _color);
    };
};
