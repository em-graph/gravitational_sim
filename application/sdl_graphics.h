#pragma once

#include <SDL.h>

namespace sdl_graphics
{
    typedef union
    {
        struct
        {
            uint8_t blue;
            uint8_t green;
            uint8_t red;
            uint8_t alpha;
        } ch;

        uint32_t full;
    } ColorARGB888;

    typedef enum {
        COLOR_BLACK,
        COLOR_MAROON,
        COLOR_GREEN,
        COLOR_OLIVE,
        COLOR_NAVY,
        COLOR_PURPLE,
        COLOR_TEAL,
        COLOR_SILVER,
        COLOR_GRAY,
        COLOR_RED,
        COLOR_LIME,
        COLOR_YELLOW,
        COLOR_BLUE,
        COLOR_MAGENTA,
        COLOR_CYAN,
        COLOR_WHITE,

        //  R0 Gx B0      R0 Gx B1      R0 Gx B2      R0 Gx B3      R0 Gx B4      R0 Gx B5
        COLOR_RGB000, COLOR_RGB001, COLOR_RGB002, COLOR_RGB003, COLOR_RGB004, COLOR_RGB005,
        COLOR_RGB010, COLOR_RGB011, COLOR_RGB012, COLOR_RGB013, COLOR_RGB014, COLOR_RGB015,
        COLOR_RGB020, COLOR_RGB021, COLOR_RGB022, COLOR_RGB023, COLOR_RGB024, COLOR_RGB025,
        COLOR_RGB030, COLOR_RGB031, COLOR_RGB032, COLOR_RGB033, COLOR_RGB034, COLOR_RGB035,
        COLOR_RGB040, COLOR_RGB041, COLOR_RGB042, COLOR_RGB043, COLOR_RGB044, COLOR_RGB045,
        COLOR_RGB050, COLOR_RGB051, COLOR_RGB052, COLOR_RGB053, COLOR_RGB054, COLOR_RGB055,
        //  R1 Gx B0      R1 Gx B1      R1 Gx B2      R1 Gx B3      R1 Gx B4      R1 Gx B5
        COLOR_RGB100, COLOR_RGB101, COLOR_RGB102, COLOR_RGB103, COLOR_RGB104, COLOR_RGB105,
        COLOR_RGB110, COLOR_RGB111, COLOR_RGB112, COLOR_RGB113, COLOR_RGB114, COLOR_RGB115,
        COLOR_RGB120, COLOR_RGB121, COLOR_RGB122, COLOR_RGB123, COLOR_RGB124, COLOR_RGB125,
        COLOR_RGB130, COLOR_RGB131, COLOR_RGB132, COLOR_RGB133, COLOR_RGB134, COLOR_RGB135,
        COLOR_RGB140, COLOR_RGB141, COLOR_RGB142, COLOR_RGB143, COLOR_RGB144, COLOR_RGB145,
        COLOR_RGB150, COLOR_RGB151, COLOR_RGB152, COLOR_RGB153, COLOR_RGB154, COLOR_RGB155,
        //  R2 Gx B0      R2 Gx B1      R2 Gx B2      R2 Gx B3      R2 Gx B4      R2 Gx B5
        COLOR_RGB200, COLOR_RGB201, COLOR_RGB202, COLOR_RGB203, COLOR_RGB204, COLOR_RGB205,
        COLOR_RGB210, COLOR_RGB211, COLOR_RGB212, COLOR_RGB213, COLOR_RGB214, COLOR_RGB215,
        COLOR_RGB220, COLOR_RGB221, COLOR_RGB222, COLOR_RGB223, COLOR_RGB224, COLOR_RGB225,
        COLOR_RGB230, COLOR_RGB231, COLOR_RGB232, COLOR_RGB233, COLOR_RGB234, COLOR_RGB235,
        COLOR_RGB240, COLOR_RGB241, COLOR_RGB242, COLOR_RGB243, COLOR_RGB244, COLOR_RGB245,
        COLOR_RGB250, COLOR_RGB251, COLOR_RGB252, COLOR_RGB253, COLOR_RGB254, COLOR_RGB255,
        //  R3 Gx B0      R3 Gx B1      R3 Gx B2      R3 Gx B3      R3 Gx B4      R3 Gx B5
        COLOR_RGB300, COLOR_RGB301, COLOR_RGB302, COLOR_RGB303, COLOR_RGB304, COLOR_RGB305,
        COLOR_RGB310, COLOR_RGB311, COLOR_RGB312, COLOR_RGB313, COLOR_RGB314, COLOR_RGB315,
        COLOR_RGB320, COLOR_RGB321, COLOR_RGB322, COLOR_RGB323, COLOR_RGB324, COLOR_RGB325,
        COLOR_RGB330, COLOR_RGB331, COLOR_RGB332, COLOR_RGB333, COLOR_RGB334, COLOR_RGB335,
        COLOR_RGB340, COLOR_RGB341, COLOR_RGB342, COLOR_RGB343, COLOR_RGB344, COLOR_RGB345,
        COLOR_RGB350, COLOR_RGB351, COLOR_RGB352, COLOR_RGB353, COLOR_RGB354, COLOR_RGB355,
        //  R4 Gx B0      R4 Gx B1      R4 Gx B2      R4 Gx B3      R4 Gx B4      R4 Gx B5
        COLOR_RGB400, COLOR_RGB401, COLOR_RGB402, COLOR_RGB403, COLOR_RGB404, COLOR_RGB405,
        COLOR_RGB410, COLOR_RGB411, COLOR_RGB412, COLOR_RGB413, COLOR_RGB414, COLOR_RGB415,
        COLOR_RGB420, COLOR_RGB421, COLOR_RGB422, COLOR_RGB423, COLOR_RGB424, COLOR_RGB425,
        COLOR_RGB430, COLOR_RGB431, COLOR_RGB432, COLOR_RGB433, COLOR_RGB434, COLOR_RGB435,
        COLOR_RGB440, COLOR_RGB441, COLOR_RGB442, COLOR_RGB443, COLOR_RGB444, COLOR_RGB445,
        COLOR_RGB450, COLOR_RGB451, COLOR_RGB452, COLOR_RGB453, COLOR_RGB454, COLOR_RGB455,
        //  R5 Gx B0      R5 Gx B1      R5 Gx B2      R5 Gx B3      R5 Gx B4      R5 Gx B5
        COLOR_RGB500, COLOR_RGB501, COLOR_RGB502, COLOR_RGB503, COLOR_RGB504, COLOR_RGB505,
        COLOR_RGB510, COLOR_RGB511, COLOR_RGB512, COLOR_RGB513, COLOR_RGB514, COLOR_RGB515,
        COLOR_RGB520, COLOR_RGB521, COLOR_RGB522, COLOR_RGB523, COLOR_RGB524, COLOR_RGB525,
        COLOR_RGB530, COLOR_RGB531, COLOR_RGB532, COLOR_RGB533, COLOR_RGB534, COLOR_RGB535,
        COLOR_RGB540, COLOR_RGB541, COLOR_RGB542, COLOR_RGB543, COLOR_RGB544, COLOR_RGB545,
        COLOR_RGB550, COLOR_RGB551, COLOR_RGB552, COLOR_RGB553, COLOR_RGB554, COLOR_RGB555,

        // Gray ramp 232 ~ 255
        COLOR_GRAY_08, COLOR_GRAY_12, COLOR_GRAY_1C, COLOR_GRAY_26, COLOR_GRAY_30, COLOR_GRAY_3A,
        COLOR_GRAY_44, COLOR_GRAY_4E, COLOR_GRAY_58, COLOR_GRAY_62, COLOR_GRAY_6C, COLOR_GRAY_76,
        COLOR_GRAY_80, COLOR_GRAY_8A, COLOR_GRAY_94, COLOR_GRAY_9E, COLOR_GRAY_A8, COLOR_GRAY_B2,
        COLOR_GRAY_BC, COLOR_GRAY_C6, COLOR_GRAY_D0, COLOR_GRAY_DA, COLOR_GRAY_E4, COLOR_GRAY_EE,
    } ColorIndex;

    const uint32_t color_lut_table[256] = {
        0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00C0C0C0,
        0x00808080, 0x00FF0000, 0x0000FF00, 0x00FFFF00, 0x000000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF,
        0x00000000, 0x0000005F, 0x00000087, 0x000000AF, 0x000000D7, 0x000000FF, 0x00005F00, 0x00005F5F,
        0x00005F87, 0x00005FAF, 0x00005FD7, 0x00005FFF, 0x00008700, 0x0000875F, 0x00008787, 0x000087AF,
        0x000087D7, 0x000087FF, 0x0000AF00, 0x0000AF5F, 0x0000AF87, 0x0000AFAF, 0x0000AFD7, 0x0000AFFF,
        0x0000D700, 0x0000D75F, 0x0000D787, 0x0000D7AF, 0x0000D7D7, 0x0000D7FF, 0x0000FF00, 0x0000FF5F,
        0x0000FF87, 0x0000FFAF, 0x0000FFD7, 0x0000FFFF, 0x005F0000, 0x005F005F, 0x005F0087, 0x005F00AF,
        0x005F00D7, 0x005F00FF, 0x005F5F00, 0x005F5F5F, 0x005F5F87, 0x005F5FAF, 0x005F5FD7, 0x005F5FFF,
        0x005F8700, 0x005F875F, 0x005F8787, 0x005F87AF, 0x005F87D7, 0x005F87FF, 0x005FAF00, 0x005FAF5F,
        0x005FAF87, 0x005FAFAF, 0x005FAFD7, 0x005FAFFF, 0x005FD700, 0x005FD75F, 0x005FD787, 0x005FD7AF,
        0x005FD7D7, 0x005FD7FF, 0x005FFF00, 0x005FFF5F, 0x005FFF87, 0x005FFFAF, 0x005FFFD7, 0x005FFFFF,
        0x00870000, 0x0087005F, 0x00870087, 0x008700AF, 0x008700D7, 0x008700FF, 0x00875F00, 0x00875F5F,
        0x00875F87, 0x00875FAF, 0x00875FD7, 0x00875FFF, 0x00878700, 0x0087875F, 0x00878787, 0x008787AF,
        0x008787D7, 0x008787FF, 0x0087AF00, 0x0087AF5F, 0x0087AF87, 0x0087AFAF, 0x0087AFD7, 0x0087AFFF,
        0x0087D700, 0x0087D75F, 0x0087D787, 0x0087D7AF, 0x0087D7D7, 0x0087D7FF, 0x0087FF00, 0x0087FF5F,
        0x0087FF87, 0x0087FFAF, 0x0087FFD7, 0x0087FFFF, 0x00AF0000, 0x00AF005F, 0x00AF0087, 0x00AF00AF,
        0x00AF00D7, 0x00AF00FF, 0x00AF5F00, 0x00AF5F5F, 0x00AF5F87, 0x00AF5FAF, 0x00AF5FD7, 0x00AF5FFF,
        0x00AF8700, 0x00AF875F, 0x00AF8787, 0x00AF87AF, 0x00AF87D7, 0x00AF87FF, 0x00AFAF00, 0x00AFAF5F,
        0x00AFAF87, 0x00AFAFAF, 0x00AFAFD7, 0x00AFAFFF, 0x00AFD700, 0x00AFD75F, 0x00AFD787, 0x00AFD7AF,
        0x00AFD7D7, 0x00AFD7FF, 0x00AFFF00, 0x00AFFF5F, 0x00AFFF87, 0x00AFFFAF, 0x00AFFFD7, 0x00AFFFFF,
        0x00D70000, 0x00D7005F, 0x00D70087, 0x00D700AF, 0x00D700D7, 0x00D700FF, 0x00D75F00, 0x00D75F5F,
        0x00D75F87, 0x00D75FAF, 0x00D75FD7, 0x00D75FFF, 0x00D78700, 0x00D7875F, 0x00D78787, 0x00D787AF,
        0x00D787D7, 0x00D787FF, 0x00D7AF00, 0x00D7AF5F, 0x00D7AF87, 0x00D7AFAF, 0x00D7AFD7, 0x00D7AFFF,
        0x00D7D700, 0x00D7D75F, 0x00D7D787, 0x00D7D7AF, 0x00D7D7D7, 0x00D7D7FF, 0x00D7FF00, 0x00D7FF5F,
        0x00D7FF87, 0x00D7FFAF, 0x00D7FFD7, 0x00D7FFFF, 0x00FF0000, 0x00FF005F, 0x00FF0087, 0x00FF00AF,
        0x00FF00D7, 0x00FF00FF, 0x00FF5F00, 0x00FF5F5F, 0x00FF5F87, 0x00FF5FAF, 0x00FF5FD7, 0x00FF5FFF,
        0x00FF8700, 0x00FF875F, 0x00FF8787, 0x00FF87AF, 0x00FF87D7, 0x00FF87FF, 0x00FFAF00, 0x00FFAF5F,
        0x00FFAF87, 0x00FFAFAF, 0x00FFAFD7, 0x00FFAFFF, 0x00FFD700, 0x00FFD75F, 0x00FFD787, 0x00FFD7AF,
        0x00FFD7D7, 0x00FFD7FF, 0x00FFFF00, 0x00FFFF5F, 0x00FFFF87, 0x00FFFFAF, 0x00FFFFD7, 0x00FFFFFF,
        0x00080808, 0x00121212, 0x001C1C1C, 0x00262626, 0x00303030, 0x003A3A3A, 0x00444444, 0x004E4E4E,
        0x00585858, 0x00626262, 0x006C6C6C, 0x00767676, 0x00808080, 0x008A8A8A, 0x00949494, 0x009E9E9E,
        0x00A8A8A8, 0x00B2B2B2, 0x00BCBCBC, 0x00C6C6C6, 0x00D0D0D0, 0x00DADADA, 0x00E4E4E4, 0x00EEEEEE
    };
    const uint32_t color_spectrum_table[81] = {
        0x00420068, 0x00480072, 0x004F007E, 0x0056008B, 0x005F009A, 0x006800A9, 0x007300BA, 0x007E00CB,
        0x008900DC, 0x009400EB, 0x009F00F8, 0x00A900FF, 0x00B200FF, 0x00BB00FF, 0x00C300FF, 0x00CA00FF,
        0x00D000FF, 0x00D500FF, 0x00DA00FF, 0x00DE00FF, 0x00E100FF, 0x00E400FF, 0x00E600FF, 0x00E700FF,
        0x00E800FF, 0x00E80CFF, 0x00E81EFF, 0x00E730FF, 0x00E642FF, 0x00E453FF, 0x00E163FF, 0x00DD73FF,
        0x00D982FF, 0x00D490FF, 0x00CE9EFF, 0x00C8ABFF, 0x00C1B7FF, 0x00B9C2FF, 0x00B1CDFF, 0x00A8D6FF,
        0x009EDFFF, 0x0094E7FF, 0x0089EEFF, 0x007DF4FF, 0x0071F9FF, 0x0064FDFF, 0x0057FFFF, 0x0049FFFF,
        0x003BFFFF, 0x002CFFFF, 0x001DFFFF, 0x000CFFFF, 0x0000FFFF, 0x0000F7FF, 0x0000EEFF, 0x0000E4FF,
        0x0000D9FF, 0x0000CEFF, 0x0000C2FF, 0x0000B5FF, 0x0000A8FF, 0x00009AFF, 0x00008BFF, 0x00007CFF,
        0x00006CFF, 0x00005CFF, 0x00004BFF, 0x00003AFF, 0x000028FF, 0x000016FF, 0x000003FF, 0x000C00FF,
        0x001C00FF, 0x002D00FF, 0x003E00FF, 0x005000FF, 0x006200FF, 0x007500FF, 0x008900FF, 0x009D00FF, 0x00B200FF
    };
    using Color = ColorARGB888;

    inline Color color_lux(const uint8_t index)
    {
        return {.full = color_lut_table[index] | 0xFF000000};
    }

    inline Color color_rgb(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
    {
        return {.ch = {b, g, r, a}};
    }

    inline Color color_rgb(const uint8_t r, const uint8_t g, const uint8_t b)
    {
        return {.ch = {b, g, r, 0xFF}};
    }

    inline Color color_spectrum(uint16_t wl)
    {
        uint16_t index = (wl - 380U) / 5U;
        return {.full = color_spectrum_table[index] | 0xFF000000};
    }

    inline Color color_full(const uint32_t color)
    {
        return {.full = color};
    }

    // ARGB8888, matches SDL_PIXELFORMAT_ARGB8888

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
        void draw_glyph(int x, int y, unsigned char glyph, Color color, int scale);

    public:
        SDL_GraphicsClass(int width, int height, const char* title);

        ~SDL_GraphicsClass();

        void init(); ///SDL Window size
        void uninit();
        void flush(Color _color);

        void update();
        void clean();

        [[nodiscard]] int get_width() const;
        [[nodiscard]] int get_height() const;

        Color get_pixel(int x, int y);
        void set_pixel(int x, int y);
        void set_pixel(int x, int y, Color color);

        [[nodiscard]] Color get_color() const;
        void set_color(Color _color);

        void draw_line(int x0, int y0, int x1, int y1);
        void draw_line(int x0, int y0, int x1, int y1, Color _color);

        void draw_rect(int x, int y, int width, int height);
        void draw_rect(int x, int y, int width, int height, Color _color);
        void fill_rect(int x, int y, int width, int height);
        void fill_rect(int x, int y, int width, int height, Color _color);

        void draw_circle(int cx, int cy, int radius);
        void draw_circle(int cx, int cy, int radius, Color _color);
        void fill_circle(int cx, int cy, int radius);
        void fill_circle(int cx, int cy, int radius, Color _color);

        /// ASCII 32-126, 5x7 bitmap. '\\n' starts a new line. scale >= 1.
        void draw_text(int x, int y, const char* text);
        void draw_text(int x, int y, const char* text, Color _color);
        void draw_text(int x, int y, const char* text, Color _color, int scale);

        [[nodiscard]] int text_width(const char* text, int scale = 1) const;
        [[nodiscard]] int text_height(int scale = 1) const;
        [[nodiscard]] int text_height(const char* text, int scale = 1) const;
    };
};
