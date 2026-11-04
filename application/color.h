#pragma once

#include <cstddef>
#include <cstdint>

namespace color
{
    // ARGB8888。内存小端字节序是 B,G,R,A，full 为 0xAARRGGBB。
    //
    // 用法:
    //   Color c{255, 128, 0};                   // RGB，alpha 默认 255
    //   Color p = Color::fromRgb(0xFF8000);     // 0xRRGGBB
    //   uint32_t px = c.full();                 // 写入像素缓冲
    //   Color mid = lerp(named::blue, named::red, 0.5f);
    //   Color glow = c + named::white * 0.25f; // 加色，通道封顶 255
    //   Color layer = over(dst, src.withAlpha(128));
    //   Color heat = chart(Chart::Inferno, t); // t 取 [0, 1]
    //   Color band = spectrum(550.f);          // 波长，单位 nm
    //   Color ansi = fromIndex(Ansi::Cyan);
    union ColorARGB888
    {
        struct
        {
            uint8_t blue;
            uint8_t green;
            uint8_t red;
            uint8_t alpha;
        } ch;

        uint32_t full;
    };

    struct Hsv
    {
        float h = 0.0f; // 色相，角度 [0, 360)
        float s = 0.0f; // 饱和度 [0, 1]
        float v = 0.0f; // 明度 [0, 1]
    };

    struct Hsl
    {
        float h = 0.0f; // 色相，角度 [0, 360)
        float s = 0.0f; // 饱和度 [0, 1]
        float l = 0.0f; // 亮度 [0, 1]
    };

    class Color
    {
    public:
        // 透明黑 (0,0,0,0)。
        constexpr Color() noexcept = default;

        // 通道取值 0..255。不写 alpha 时为不透明。Color{255, 0, 0}
        constexpr Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) noexcept
            : value_((uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b))
        {
        }

        // 直接装入 0xAARRGGBB。Color(0xFF0000) 的 alpha 是 0，不透明色用 fromRgb。
        explicit constexpr Color(uint32_t packed) noexcept : value_(packed)
        {
        }

        // Color::fromArgb(0x80FF0000)，高 8 位是 alpha。
        static constexpr Color fromArgb(uint32_t packed) noexcept
        {
            return Color(packed);
        }

        // Color::fromRgb(0xFF8000) 或 fromRgb(0xFF8000, 128)。
        static constexpr Color fromRgb(uint32_t rrggbb, uint8_t alpha = 255) noexcept
        {
            return Color(static_cast<uint8_t>((rrggbb >> 16) & 0xFF),
                         static_cast<uint8_t>((rrggbb >> 8) & 0xFF),
                         static_cast<uint8_t>(rrggbb & 0xFF),
                         alpha);
        }

        constexpr uint8_t r() const noexcept { return static_cast<uint8_t>(value_ >> 16); }
        constexpr uint8_t g() const noexcept { return static_cast<uint8_t>(value_ >> 8); }
        constexpr uint8_t b() const noexcept { return static_cast<uint8_t>(value_); }
        constexpr uint8_t a() const noexcept { return static_cast<uint8_t>(value_ >> 24); }
        // 0xAARRGGBB，可直接交给 SDL 或像素缓冲。
        constexpr uint32_t full() const noexcept { return value_; }

        // 只改透明度。c.withAlpha(128)
        constexpr Color withAlpha(uint8_t alpha) const noexcept
        {
            return Color(r(), g(), b(), alpha);
        }

        // RGB 取反，alpha 不变。
        constexpr Color inverted() const noexcept
        {
            return Color(static_cast<uint8_t>(255 - r()),
                         static_cast<uint8_t>(255 - g()),
                         static_cast<uint8_t>(255 - b()),
                         a());
        }

        // 缩放 RGB 并饱和。c.scaled(0.5f) 减半，c.scaled(2.f) 加亮到封顶。负数当作 0，alpha 不变。
        constexpr Color scaled(float factor) const noexcept
        {
            Color copy = *this;
            copy *= factor;
            return copy;
        }

        // 缩放 alpha。c.scaleAlpha(0.5f) 变为半透明，0 为全透明。
        constexpr Color scaleAlpha(float factor) const noexcept
        {
            if (factor < 0.0f)
                factor = 0.0f;
            const float v = static_cast<float>(a()) * factor;
            const auto alpha = static_cast<uint8_t>(v >= 255.0f ? 255 : v + 0.5f);
            return Color(r(), g(), b(), alpha);
        }

        // 四通道饱和相加，结果不超过 255。c += named::red
        constexpr Color& operator+=(const Color& rhs) noexcept
        {
            *this = Color(sat8(int(r()) + rhs.r()),
                          sat8(int(g()) + rhs.g()),
                          sat8(int(b()) + rhs.b()),
                          sat8(int(a()) + rhs.a()));
            return *this;
        }

        // 四通道饱和相减，结果不低于 0。
        constexpr Color& operator-=(const Color& rhs) noexcept
        {
            *this = Color(sat8(int(r()) - rhs.r()),
                          sat8(int(g()) - rhs.g()),
                          sat8(int(b()) - rhs.b()),
                          sat8(int(a()) - rhs.a()));
            return *this;
        }

        // 缩放 RGB 并饱和，alpha 不变。c *= 1.5f
        constexpr Color& operator*=(float scale) noexcept
        {
            if (scale < 0.0f)
                scale = 0.0f;
            *this = Color(scale8(r(), scale), scale8(g(), scale), scale8(b(), scale), a());
            return *this;
        }

        // 逐通道相乘，含 alpha，相当于滤色。c *= named::gray
        constexpr Color& operator*=(const Color& rhs) noexcept
        {
            *this = Color(mul8(r(), rhs.r()), mul8(g(), rhs.g()), mul8(b(), rhs.b()), mul8(a(), rhs.a()));
            return *this;
        }

        constexpr bool operator==(const Color& rhs) const noexcept { return value_ == rhs.value_; }
        constexpr bool operator!=(const Color& rhs) const noexcept { return value_ != rhs.value_; }

    private:
        uint32_t value_ = 0;

        static constexpr uint8_t sat8(int v) noexcept
        {
            return static_cast<uint8_t>(v < 0 ? 0 : (v > 255 ? 255 : v));
        }

        static constexpr uint8_t scale8(uint8_t channel, float scale) noexcept
        {
            const float v = static_cast<float>(channel) * scale;
            if (v <= 0.0f)
                return 0;
            if (v >= 255.0f)
                return 255;
            return static_cast<uint8_t>(v + 0.5f);
        }

        static constexpr uint8_t mul8(uint8_t x, uint8_t y) noexcept
        {
            return static_cast<uint8_t>((static_cast<unsigned>(x) * y + 127u) / 255u);
        }
    };

    static_assert(sizeof(Color) == 4);

    // c + d，四通道饱和相加。
    constexpr Color operator+(Color lhs, const Color& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    // c - d，四通道饱和相减。
    constexpr Color operator-(Color lhs, const Color& rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    // c * 0.5f，只缩放 RGB。
    constexpr Color operator*(Color lhs, float scale) noexcept
    {
        lhs *= scale;
        return lhs;
    }

    // 0.5f * c，与 c * 0.5f 相同。
    constexpr Color operator*(float scale, Color rhs) noexcept
    {
        rhs *= scale;
        return rhs;
    }

    // c * d，逐通道相乘。
    constexpr Color operator*(Color lhs, const Color& rhs) noexcept
    {
        lhs *= rhs;
        return lhs;
    }

    // 加色。add(base, glow)，与 + 相同。
    constexpr Color add(Color lhs, const Color& rhs) noexcept
    {
        lhs += rhs;
        return lhs;
    }

    // 减色。sub(c, named::red)，与 - 相同。
    constexpr Color sub(Color lhs, const Color& rhs) noexcept
    {
        lhs -= rhs;
        return lhs;
    }

    // 正片叠底。multiply(c, named::red) 只留下红色分量。
    constexpr Color multiply(Color lhs, const Color& rhs) noexcept
    {
        lhs *= rhs;
        return lhs;
    }

    // 亮度。brightness(c, 1.f) 不变，0 变黑，大于 1 加亮并封顶。
    constexpr Color brightness(Color c, float factor) noexcept
    {
        c *= factor;
        return c;
    }

    // 插值。lerp(a, b, 0.f) 得到 a，lerp(a, b, 1.f) 得到 b，越界 t 会夹取。按 8 位通道混合，不是光学混合。
    constexpr Color lerp(const Color& from, const Color& to, float t) noexcept
    {
        if (!(t > 0.0f))
            return from;
        if (!(t < 1.0f))
            return to;

        auto channel = [t](uint8_t a, uint8_t b) constexpr -> uint8_t {
            const float v = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t;
            if (v <= 0.0f)
                return 0;
            if (v >= 255.0f)
                return 255;
            return static_cast<uint8_t>(v + 0.5f);
        };
        return Color(channel(from.r(), to.r()),
                     channel(from.g(), to.g()),
                     channel(from.b(), to.b()),
                     channel(from.a(), to.a()));
    }

    // 混色，与 lerp 相同。mix(named::red, named::blue, 0.5f)
    constexpr Color mix(const Color& from, const Color& to, float t) noexcept
    {
        return lerp(from, to, t);
    }

    static_assert(Color(255, 0, 0).full() == 0xFFFF0000u);
    static_assert(Color(0, 255, 0).full() == 0xFF00FF00u);
    static_assert(Color(0, 0, 255).full() == 0xFF0000FFu);
    static_assert(Color(255, 128, 64, 32).full() == 0x20FF8040u);
    static_assert((Color(255, 0, 0) + Color(0, 0, 255)) == Color(255, 0, 255));
    static_assert((Color(10, 20, 30) - Color(3, 40, 1)) == Color(7, 0, 29, 0));
    static_assert((Color(200, 10, 0) * 2.0f).r() == 255);
    static_assert((Color(10, 0, 0) * 2.0f).r() == 20);
    static_assert((Color(255, 255, 255) * 0.5f).r() == 128);
    static_assert((Color(255, 128, 0) * Color(128, 255, 0)) == Color(128, 128, 0));
    static_assert(lerp(Color(255, 0, 0), Color(0, 0, 100), 0.0f) == Color(255, 0, 0));
    static_assert(lerp(Color(255, 0, 0), Color(0, 0, 100), 1.0f) == Color(0, 0, 100));
    static_assert(lerp(Color(0, 0, 0), Color(255, 0, 0), 0.5f).r() == 128);
    static_assert(lerp(Color(0, 0, 0, 0), Color(0, 0, 0, 255), 0.5f).a() == 128);

    // 线性光下混合 RGB，渐变亮度更均匀。alpha 仍按覆盖率插值。mixLinear(a, b, 0.5f)
    Color mixLinear(const Color& from, const Color& to, float t) noexcept;
    // HSV 插值，色相走较短的一弧。一端是灰色时沿用另一端的色相。
    Color lerpHsv(const Color& from, const Color& to, float t) noexcept;

    // 以下混合只改 RGB，alpha 取第一个参数。
    // 滤色，结果不暗于两个输入。screen(base, blend)
    Color screen(const Color& base, const Color& blend) noexcept;
    // 叠加：暗部相乘，亮部滤色。overlay(base, blend)
    Color overlay(const Color& base, const Color& blend) noexcept;
    // 通道差的绝对值。difference(a, b)
    Color difference(const Color& base, const Color& blend) noexcept;
    // 逐通道取较暗者。darken(a, b)
    Color darken(const Color& base, const Color& blend) noexcept;
    // 逐通道取较亮者。lighten(a, b)
    Color lighten(const Color& base, const Color& blend) noexcept;

    // 源覆盖目标。over(dst, src)：src 不透明时得到 src，src 全透明时得到 dst。
    Color over(const Color& dst, const Color& src) noexcept;
    // RGB 乘上 alpha/255。之后可以把颜色直接相加。
    Color premultiply(const Color& c) noexcept;
    // 把预乘颜色还原。alpha 为 0 时得到透明黑。
    Color unpremultiply(const Color& c) noexcept;

    // toHsv(c)，h 为角度，s 和 v 取 [0, 1]。
    Hsv toHsv(const Color& c) noexcept;
    // toHsl(c)，l 是亮度。
    Hsl toHsl(const Color& c) noexcept;
    // fromHsv(120.f, 1.f, 1.f) 得到亮绿。h 可以超出一圈，会自动折回。
    Color fromHsv(float h, float s, float v, uint8_t alpha = 255) noexcept;
    // fromHsl(0.f, 1.f, 0.5f) 得到纯红。
    Color fromHsl(float h, float s, float l, uint8_t alpha = 255) noexcept;

    // fromHsv(toHsv(c))，可另给 alpha。
    inline Color fromHsv(const Hsv& hsv, uint8_t alpha = 255) noexcept
    {
        return fromHsv(hsv.h, hsv.s, hsv.v, alpha);
    }

    // fromHsl(toHsl(c))，可另给 alpha。
    inline Color fromHsl(const Hsl& hsl, uint8_t alpha = 255) noexcept
    {
        return fromHsl(hsl.h, hsl.s, hsl.l, alpha);
    }

    // 色相偏移，单位度。hueShift(named::red, 120.f) 转到绿色。
    Color hueShift(const Color& c, float degrees) noexcept;
    // 饱和度乘系数。saturate(c, 0.f) 变灰，1 不变，大于 1 更艳并封顶。
    Color saturate(const Color& c, float factor) noexcept;
    // 对比度，支点是 128。contrast(c, 1.f) 不变，0 变成中灰，大于 1 拉开。
    Color contrast(const Color& c, float factor) noexcept;
    // 编码值的幂曲线。gamma(c, 1.f) 不变，大于 1 压暗中间调。
    Color gamma(const Color& c, float gamma) noexcept;
    // 曝光档。expose(c, 1.f) 线性光加倍，expose(c, -1.f) 减半。
    Color expose(const Color& c, float stops) noexcept;
    // 按相对亮度变成灰色，alpha 保留。
    Color grayscale(const Color& c) noexcept;

    // 相对亮度，线性光，范围 [0, 1]。白约为 1，黑为 0。
    float luminance(const Color& c) noexcept;
    // RGB 欧氏距离，忽略 alpha。相同为 0，黑到白约为 441.7。
    float distance(const Color& a, const Color& b) noexcept;

    // 单通道 sRGB 编码值转线性光。channelToLinear(255) 约为 1。
    float channelToLinear(uint8_t channel) noexcept;
    // 线性光转回 0..255。channelFromLinear(1.f) 为 255，越界会夹取。
    uint8_t channelFromLinear(float linear) noexcept;

    // 256 色索引：0..15 为 ANSI，16..231 为 6×6×6 色立方，232..255 为 24 级灰。
    // fromIndex(Ansi::Red)、fromIndex(cubeIndex(5, 0, 0))、fromIndex(grayIndex(12))
    enum class Ansi : uint8_t
    {
        Black = 0,
        Maroon,
        Green,
        Olive,
        Navy,
        Purple,
        Teal,
        Silver,
        Gray,
        Red,
        Lime,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

    // 色立方一档的分量。cubeLevel(0..5) 对应 0, 95, 135, 175, 215, 255。
    constexpr uint8_t cubeLevel(int step) noexcept
    {
        const int i = step < 0 ? 0 : (step > 5 ? 5 : step);
        return i == 0 ? 0 : static_cast<uint8_t>(55 + i * 40);
    }

    // 色立方下标，r/g/b 各取 0..5，结果在 16..231。越界会夹取。
    constexpr uint8_t cubeIndex(int r, int g, int b) noexcept
    {
        const int rr = r < 0 ? 0 : (r > 5 ? 5 : r);
        const int gg = g < 0 ? 0 : (g > 5 ? 5 : g);
        const int bb = b < 0 ? 0 : (b > 5 ? 5 : b);
        return static_cast<uint8_t>(16 + 36 * rr + 6 * gg + bb);
    }

    // 灰阶下标，step 取 0..23，结果在 232..255。
    constexpr uint8_t grayIndex(int step) noexcept
    {
        const int i = step < 0 ? 0 : (step > 23 ? 23 : step);
        return static_cast<uint8_t>(232 + i);
    }

    // 按下标取不透明色。fromIndex(uint8_t{17})
    constexpr Color fromIndex(uint8_t index) noexcept
    {
        if (index < 16)
        {
            constexpr uint8_t kAnsi[16][3] = {
                {0, 0, 0},       {128, 0, 0},     {0, 128, 0},     {128, 128, 0},
                {0, 0, 128},     {128, 0, 128},   {0, 128, 128},   {192, 192, 192},
                {128, 128, 128}, {255, 0, 0},     {0, 255, 0},     {255, 255, 0},
                {0, 0, 255},     {255, 0, 255},   {0, 255, 255},   {255, 255, 255},
            };
            return Color(kAnsi[index][0], kAnsi[index][1], kAnsi[index][2]);
        }
        if (index >= 232)
        {
            const auto y = static_cast<uint8_t>(8 + (index - 232) * 10);
            return Color(y, y, y);
        }
        const int n = index - 16;
        return Color(cubeLevel(n / 36), cubeLevel((n / 6) % 6), cubeLevel(n % 6));
    }

    // fromIndex(Ansi::White)
    constexpr Color fromIndex(Ansi index) noexcept
    {
        return fromIndex(static_cast<uint8_t>(index));
    }

    static_assert(fromIndex(Ansi::Maroon).full() == 0xFF800000u);
    static_assert(fromIndex(Ansi::Red).full() == 0xFFFF0000u);
    static_assert(fromIndex(Ansi::White).full() == 0xFFFFFFFFu);
    static_assert(fromIndex(uint8_t{16}).full() == 0xFF000000u);
    static_assert(fromIndex(uint8_t{17}).full() == 0xFF00005Fu);
    static_assert(fromIndex(cubeIndex(1, 0, 0)).full() == 0xFF5F0000u);
    static_assert(fromIndex(uint8_t{231}).full() == 0xFFFFFFFFu);
    static_assert(fromIndex(grayIndex(0)).full() == 0xFF080808u);
    static_assert(fromIndex(grayIndex(23)).full() == 0xFFEEEEEE);

    // 不透明常用色。绿色是 (0,128,0)，亮绿用 lime。透明色是 named::transparent。
    namespace named
    {
        inline constexpr Color transparent{0, 0, 0, 0};
        inline constexpr Color black{0, 0, 0};
        inline constexpr Color white{255, 255, 255};
        inline constexpr Color red{255, 0, 0};
        inline constexpr Color green{0, 128, 0};
        inline constexpr Color blue{0, 0, 255};
        inline constexpr Color lime{0, 255, 0};
        inline constexpr Color yellow{255, 255, 0};
        inline constexpr Color cyan{0, 255, 255};
        inline constexpr Color magenta{255, 0, 255};
        inline constexpr Color maroon{128, 0, 0};
        inline constexpr Color olive{128, 128, 0};
        inline constexpr Color navy{0, 0, 128};
        inline constexpr Color purple{128, 0, 128};
        inline constexpr Color teal{0, 128, 128};
        inline constexpr Color silver{192, 192, 192};
        inline constexpr Color gray{128, 128, 128};
        inline constexpr Color orange{255, 165, 0};
        inline constexpr Color gold{255, 215, 0};
        inline constexpr Color pink{255, 192, 203};
        inline constexpr Color brown{165, 42, 42};
        inline constexpr Color violet{238, 130, 238};
        inline constexpr Color indigo{75, 0, 130};
        inline constexpr Color coral{255, 127, 80};
        inline constexpr Color salmon{250, 128, 114};
        inline constexpr Color tomato{255, 99, 71};
        inline constexpr Color crimson{220, 20, 60};
        inline constexpr Color chocolate{210, 105, 30};
        inline constexpr Color khaki{240, 230, 140};
        inline constexpr Color beige{245, 245, 220};
        inline constexpr Color ivory{255, 255, 240};
        inline constexpr Color wheat{245, 222, 179};
        inline constexpr Color sky{135, 206, 235};
        inline constexpr Color azure{240, 255, 255};
        inline constexpr Color turquoise{64, 224, 208};
        inline constexpr Color steel{70, 130, 180};
        inline constexpr Color slate{112, 128, 144};
        inline constexpr Color orchid{218, 112, 214};
        inline constexpr Color forest{34, 139, 34};
        inline constexpr Color midnight{25, 25, 112};
    }

    // 光谱表：380..780 nm，步长 5，共 81 档。kSpectrumRgb[i] 为 0x00RRGGBB，波长 = 380 + i * 5。
    inline constexpr int kSpectrumMinNm = 380;
    inline constexpr int kSpectrumMaxNm = 780;
    inline constexpr int kSpectrumStepNm = 5;
    inline constexpr int kSpectrumCount = (kSpectrumMaxNm - kSpectrumMinNm) / kSpectrumStepNm + 1;
    extern const uint32_t kSpectrumRgb[kSpectrumCount];

    // 取最近的 5 nm 档。spectrumAt(550)，超出 380..780 会夹到两端。
    Color spectrumAt(int wavelengthNm) noexcept;
    // 在相邻档之间插值。spectrum(552.5f)
    Color spectrum(float wavelengthNm) noexcept;
    // 可见光近似，红端停在红色。wavelength(650.f)。原表 spectrum() 的红端偏品红。
    Color wavelength(float wavelengthNm) noexcept;

    // 色谱。chart(Chart::Hot, 0.f) 是低端，chart(Chart::Hot, 1.f) 是高端。
    enum class Chart
    {
        Gray,
        Hot,
        Cool,
        Jet,
        Rainbow,
        Spectrum,
        CoolWarm,
        Viridis,
        Inferno,
        Magma,
        Plasma
    };

    // 等间距色标采样。gradient(stops, count, t)，t 夹到 [0, 1]；count 为 0 时返回透明黑。
    Color gradient(const Color* stops, std::size_t count, float t) noexcept;
    // 按色谱取样。Viridis、Inferno、Magma、Plasma 是对应色谱的分段线性采样。
    Color chart(Chart which, float t) noexcept;
}
