#ifndef SDL_GRAPHICS_H
#define SDL_GRAPHICS_H

#include <SDL.h>
#include "SDL_pixels.h"
#include <cstdint>
#include <iostream>

using namespace std;

#define constrain(x, min, max)		(x < min ? min : x>max ? max : x)

class SDL_GraphicsClass
{
	int w{}, h{};
	uint32_t bufferSize;		//Draw buffer byte count
	uint32_t color{};
	const char* title;
	uint32_t *gram{};
	uint32_t *drawBuffer{};
	SDL_Window *window{};
	SDL_Renderer *renderer{};
	SDL_Texture* texture{};
	bool sdlStarted{};

	bool contains(int x, int y) const;
	void hline(int x0, int x1, int y, uint32_t color);
	void vline(int x, int y0, int y1, uint32_t color);
	void draw_glyph(int x, int y, unsigned char glyph, uint32_t color, int scale);

public:
	SDL_GraphicsClass(int width, int height, const char* title);

	~SDL_GraphicsClass();

	void init();	///SDL Window size
	void uninit();
	void flush(uint32_t color);
	void update();
	void clean();

	[[nodiscard]] int get_width() const;
	[[nodiscard]] int get_height() const;

	uint32_t get_pixel(int x, int y);
	void set_pixel(int x, int y);
	void set_pixel(int x, int y, uint32_t color);

	[[nodiscard]] uint32_t get_color() const;
	void set_color(uint32_t color);

	/// ARGB8888, matches SDL_PIXELFORMAT_ARGB8888
	static uint32_t rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF);

	void draw_line(int x0, int y0, int x1, int y1);
	void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

	void draw_rect(int x, int y, int width, int height);
	void draw_rect(int x, int y, int width, int height, uint32_t color);
	void fill_rect(int x, int y, int width, int height);
	void fill_rect(int x, int y, int width, int height, uint32_t color);

	void draw_circle(int cx, int cy, int radius);
	void draw_circle(int cx, int cy, int radius, uint32_t color);
	void fill_circle(int cx, int cy, int radius);
	void fill_circle(int cx, int cy, int radius, uint32_t color);

	/// ASCII 32-126, 5x7 bitmap. '\\n' starts a new line. scale >= 1.
	void draw_text(int x, int y, const char* text);
	void draw_text(int x, int y, const char* text, uint32_t color);
	void draw_text(int x, int y, const char* text, uint32_t color, int scale);

	[[nodiscard]] int text_width(const char* text, int scale = 1) const;
	[[nodiscard]] int text_height(int scale = 1) const;
	[[nodiscard]] int text_height(const char* text, int scale = 1) const;
};

#endif //SDL_GRAPHICS_H
