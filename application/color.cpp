#include "color.h"

#include <algorithm>
#include <cmath>

namespace color
{
    namespace
    {
        float clamp01(float t) noexcept
        {
            if (!(t > 0.0f))
                return 0.0f;
            if (t > 1.0f)
                return 1.0f;
            return t;
        }

        uint8_t encode01(float unit) noexcept
        {
            const float c = std::clamp(unit, 0.0f, 1.0f);
            return static_cast<uint8_t>(std::lround(c * 255.0f));
        }

        uint8_t lerp8(uint8_t a, uint8_t b, float t) noexcept
        {
            const float v = static_cast<float>(a) + (static_cast<float>(b) - static_cast<float>(a)) * t;
            if (v <= 0.0f)
                return 0;
            if (v >= 255.0f)
                return 255;
            return static_cast<uint8_t>(std::lround(v));
        }

        uint8_t mulDiv255(unsigned x, unsigned y) noexcept
        {
            return static_cast<uint8_t>((x * y + 127u) / 255u);
        }

        uint8_t screen8(uint8_t a, uint8_t b) noexcept
        {
            return static_cast<uint8_t>(255u - ((255u - a) * (255u - b) + 127u) / 255u);
        }

        uint8_t overlay8(uint8_t base, uint8_t blend) noexcept
        {
            if (base < 128)
                return mulDiv255(static_cast<unsigned>(base) * 2u, blend);
            const unsigned inv = (255u - base) * (255u - blend);
            return static_cast<uint8_t>(255u - (2u * inv + 127u) / 255u);
        }

        Color rgbOf(const Color& alphaSource, uint8_t r, uint8_t g, uint8_t b) noexcept
        {
            return Color(r, g, b, alphaSource.a());
        }

        float hueDegrees(float r, float g, float b, float maxChannel, float delta) noexcept
        {
            if (delta <= 0.0f)
                return 0.0f;

            float h;
            if (maxChannel == r)
                h = std::fmod((g - b) / delta, 6.0f);
            else if (maxChannel == g)
                h = (b - r) / delta + 2.0f;
            else
                h = (r - g) / delta + 4.0f;
            h *= 60.0f;
            if (h < 0.0f)
                h += 360.0f;
            return h;
        }

        Color fromChroma(float h, float chroma, float match, uint8_t alpha) noexcept
        {
            h = std::fmod(h, 360.0f);
            if (h < 0.0f)
                h += 360.0f;
            if (!(h >= 0.0f))
                h = 0.0f;

            const float x = chroma * (1.0f - std::fabs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            switch (static_cast<int>(h / 60.0f))
            {
            case 0: r = chroma; g = x; break;
            case 1: r = x; g = chroma; break;
            case 2: g = chroma; b = x; break;
            case 3: g = x; b = chroma; break;
            case 4: r = x; b = chroma; break;
            default: r = chroma; b = x; break;
            }
            return Color(encode01(r + match), encode01(g + match), encode01(b + match), alpha);
        }
    }

    extern const uint32_t kSpectrumRgb[kSpectrumCount] = {
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

    float channelToLinear(uint8_t channel) noexcept
    {
        const float c = static_cast<float>(channel) / 255.0f;
        if (c <= 0.04045f)
            return c / 12.92f;
        return std::pow((c + 0.055f) / 1.055f, 2.4f);
    }

    uint8_t channelFromLinear(float linear) noexcept
    {
        const float c = std::clamp(linear, 0.0f, 1.0f);
        const float encoded = c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
        return encode01(encoded);
    }

    Color mixLinear(const Color& from, const Color& to, float t) noexcept
    {
        t = clamp01(t);
        auto channel = [t](uint8_t a, uint8_t b) {
            const float mixed = channelToLinear(a) + (channelToLinear(b) - channelToLinear(a)) * t;
            return channelFromLinear(mixed);
        };
        return Color(channel(from.r(), to.r()),
                     channel(from.g(), to.g()),
                     channel(from.b(), to.b()),
                     lerp8(from.a(), to.a(), t));
    }

    Color lerpHsv(const Color& from, const Color& to, float t) noexcept
    {
        t = clamp01(t);
        const Hsv a = toHsv(from);
        const Hsv b = toHsv(to);

        float h;
        if (a.s <= 0.0f)
            h = b.h;
        else if (b.s <= 0.0f)
            h = a.h;
        else
        {
            float dh = b.h - a.h;
            if (dh > 180.0f)
                dh -= 360.0f;
            else if (dh < -180.0f)
                dh += 360.0f;
            h = a.h + dh * t;
        }

        const float s = a.s + (b.s - a.s) * t;
        const float v = a.v + (b.v - a.v) * t;
        return fromHsv(h, s, v, lerp8(from.a(), to.a(), t));
    }

    Color screen(const Color& base, const Color& blend) noexcept
    {
        return rgbOf(base, screen8(base.r(), blend.r()), screen8(base.g(), blend.g()), screen8(base.b(), blend.b()));
    }

    Color overlay(const Color& base, const Color& blend) noexcept
    {
        return rgbOf(base,
                     overlay8(base.r(), blend.r()),
                     overlay8(base.g(), blend.g()),
                     overlay8(base.b(), blend.b()));
    }

    Color difference(const Color& base, const Color& blend) noexcept
    {
        auto delta = [](uint8_t a, uint8_t b) {
            return static_cast<uint8_t>(a > b ? a - b : b - a);
        };
        return rgbOf(base, delta(base.r(), blend.r()), delta(base.g(), blend.g()), delta(base.b(), blend.b()));
    }

    Color darken(const Color& base, const Color& blend) noexcept
    {
        return rgbOf(base,
                     std::min(base.r(), blend.r()),
                     std::min(base.g(), blend.g()),
                     std::min(base.b(), blend.b()));
    }

    Color lighten(const Color& base, const Color& blend) noexcept
    {
        return rgbOf(base,
                     std::max(base.r(), blend.r()),
                     std::max(base.g(), blend.g()),
                     std::max(base.b(), blend.b()));
    }

    Color over(const Color& dst, const Color& src) noexcept
    {
        const float srcA = static_cast<float>(src.a()) / 255.0f;
        const float dstA = static_cast<float>(dst.a()) / 255.0f;
        const float outA = srcA + dstA * (1.0f - srcA);
        if (outA <= 0.0f)
            return Color(0, 0, 0, 0);

        auto channel = [&](uint8_t s, uint8_t d) {
            const float v = (static_cast<float>(s) * srcA + static_cast<float>(d) * dstA * (1.0f - srcA)) / outA;
            return static_cast<uint8_t>(std::lround(std::clamp(v, 0.0f, 255.0f)));
        };
        const auto alpha = static_cast<uint8_t>(std::lround(std::clamp(outA * 255.0f, 0.0f, 255.0f)));
        return Color(channel(src.r(), dst.r()), channel(src.g(), dst.g()), channel(src.b(), dst.b()), alpha);
    }

    Color premultiply(const Color& c) noexcept
    {
        return Color(mulDiv255(c.r(), c.a()), mulDiv255(c.g(), c.a()), mulDiv255(c.b(), c.a()), c.a());
    }

    Color unpremultiply(const Color& c) noexcept
    {
        if (c.a() == 0)
            return Color(0, 0, 0, 0);
        auto restore = [a = c.a()](uint8_t channel) {
            const unsigned v = (static_cast<unsigned>(channel) * 255u + a / 2u) / a;
            return static_cast<uint8_t>(v > 255u ? 255u : v);
        };
        return Color(restore(c.r()), restore(c.g()), restore(c.b()), c.a());
    }

    Hsv toHsv(const Color& c) noexcept
    {
        const float r = static_cast<float>(c.r()) / 255.0f;
        const float g = static_cast<float>(c.g()) / 255.0f;
        const float b = static_cast<float>(c.b()) / 255.0f;
        const float maxChannel = std::max({r, g, b});
        const float minChannel = std::min({r, g, b});
        const float delta = maxChannel - minChannel;

        Hsv out;
        out.v = maxChannel;
        out.s = maxChannel <= 0.0f ? 0.0f : delta / maxChannel;
        out.h = hueDegrees(r, g, b, maxChannel, delta);
        return out;
    }

    Hsl toHsl(const Color& c) noexcept
    {
        const float r = static_cast<float>(c.r()) / 255.0f;
        const float g = static_cast<float>(c.g()) / 255.0f;
        const float b = static_cast<float>(c.b()) / 255.0f;
        const float maxChannel = std::max({r, g, b});
        const float minChannel = std::min({r, g, b});
        const float delta = maxChannel - minChannel;

        Hsl out;
        out.l = (maxChannel + minChannel) * 0.5f;
        if (delta <= 0.0f)
            return out;
        out.s = delta / (1.0f - std::fabs(2.0f * out.l - 1.0f));
        out.h = hueDegrees(r, g, b, maxChannel, delta);
        return out;
    }

    Color fromHsv(float h, float s, float v, uint8_t alpha) noexcept
    {
        s = clamp01(s);
        v = clamp01(v);
        const float chroma = v * s;
        return fromChroma(h, chroma, v - chroma, alpha);
    }

    Color fromHsl(float h, float s, float l, uint8_t alpha) noexcept
    {
        s = clamp01(s);
        l = clamp01(l);
        const float chroma = (1.0f - std::fabs(2.0f * l - 1.0f)) * s;
        return fromChroma(h, chroma, l - chroma * 0.5f, alpha);
    }

    Color hueShift(const Color& c, float degrees) noexcept
    {
        const Hsv hsv = toHsv(c);
        return fromHsv(hsv.h + degrees, hsv.s, hsv.v, c.a());
    }

    Color saturate(const Color& c, float factor) noexcept
    {
        const Hsv hsv = toHsv(c);
        const float s = factor <= 0.0f ? 0.0f : hsv.s * factor;
        return fromHsv(hsv.h, s, hsv.v, c.a());
    }

    Color contrast(const Color& c, float factor) noexcept
    {
        auto adjust = [factor](uint8_t channel) {
            const float v = (static_cast<float>(channel) - 128.0f) * factor + 128.0f;
            if (v <= 0.0f)
                return uint8_t{0};
            if (v >= 255.0f)
                return uint8_t{255};
            return static_cast<uint8_t>(std::lround(v));
        };
        return Color(adjust(c.r()), adjust(c.g()), adjust(c.b()), c.a());
    }

    Color gamma(const Color& c, float power) noexcept
    {
        if (!(power > 0.0f))
            return Color(0, 0, 0, c.a());
        auto adjust = [power](uint8_t channel) {
            const float v = std::pow(static_cast<float>(channel) / 255.0f, power);
            return encode01(v);
        };
        return Color(adjust(c.r()), adjust(c.g()), adjust(c.b()), c.a());
    }

    Color expose(const Color& c, float stops) noexcept
    {
        const float gain = std::exp2(stops);
        auto adjust = [gain](uint8_t channel) {
            return channelFromLinear(channelToLinear(channel) * gain);
        };
        return Color(adjust(c.r()), adjust(c.g()), adjust(c.b()), c.a());
    }

    float luminance(const Color& c) noexcept
    {
        return 0.2126f * channelToLinear(c.r()) + 0.7152f * channelToLinear(c.g()) + 0.0722f * channelToLinear(c.b());
    }

    Color grayscale(const Color& c) noexcept
    {
        const auto y = channelFromLinear(luminance(c));
        return Color(y, y, y, c.a());
    }

    float distance(const Color& a, const Color& b) noexcept
    {
        const float dr = static_cast<float>(a.r()) - static_cast<float>(b.r());
        const float dg = static_cast<float>(a.g()) - static_cast<float>(b.g());
        const float db = static_cast<float>(a.b()) - static_cast<float>(b.b());
        return std::sqrt(dr * dr + dg * dg + db * db);
    }

    Color spectrumAt(int wavelengthNm) noexcept
    {
        const int nm = std::clamp(wavelengthNm, kSpectrumMinNm, kSpectrumMaxNm);
        const int index = (nm - kSpectrumMinNm) / kSpectrumStepNm;
        return Color(kSpectrumRgb[index] | 0xFF000000u);
    }

    Color spectrum(float wavelengthNm) noexcept
    {
        if (!(wavelengthNm > static_cast<float>(kSpectrumMinNm)))
            return spectrumAt(kSpectrumMinNm);
        if (!(wavelengthNm < static_cast<float>(kSpectrumMaxNm)))
            return spectrumAt(kSpectrumMaxNm);

        const float sample = (wavelengthNm - static_cast<float>(kSpectrumMinNm)) / static_cast<float>(kSpectrumStepNm);
        const int index = static_cast<int>(sample);
        const float t = sample - static_cast<float>(index);
        const Color lo(kSpectrumRgb[index] | 0xFF000000u);
        const Color hi(kSpectrumRgb[index + 1] | 0xFF000000u);
        return lerp(lo, hi, t);
    }

    Color wavelength(float nm) noexcept
    {
        nm = std::clamp(nm, 380.0f, 780.0f);
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
        if (nm < 440.0f)
        {
            r = (440.0f - nm) / 60.0f;
            b = 1.0f;
        }
        else if (nm < 490.0f)
        {
            g = (nm - 440.0f) / 50.0f;
            b = 1.0f;
        }
        else if (nm < 510.0f)
        {
            g = 1.0f;
            b = (510.0f - nm) / 20.0f;
        }
        else if (nm < 580.0f)
        {
            r = (nm - 510.0f) / 70.0f;
            g = 1.0f;
        }
        else if (nm < 645.0f)
        {
            r = 1.0f;
            g = (645.0f - nm) / 65.0f;
        }
        else
        {
            r = 1.0f;
        }

        float falloff = 1.0f;
        if (nm < 420.0f)
            falloff = 0.3f + 0.7f * (nm - 380.0f) / 40.0f;
        else if (nm > 700.0f)
            falloff = 0.3f + 0.7f * (780.0f - nm) / 80.0f;

        auto curve = [falloff](float channel) {
            const float shaped = std::pow(std::clamp(channel * falloff, 0.0f, 1.0f), 0.8f);
            return encode01(shaped);
        };
        return Color(curve(r), curve(g), curve(b));
    }

    Color gradient(const Color* stops, std::size_t count, float t) noexcept
    {
        if (stops == nullptr || count == 0)
            return {};
        if (count == 1 || !(t > 0.0f))
            return stops[0];
        if (!(t < 1.0f))
            return stops[count - 1];

        const float scaled = t * static_cast<float>(count - 1);
        const auto index = static_cast<std::size_t>(scaled);
        return lerp(stops[index], stops[index + 1], scaled - static_cast<float>(index));
    }

    Color chart(Chart which, float t) noexcept
    {
        t = clamp01(t);
        switch (which)
        {
        case Chart::Gray:
        {
            const auto y = encode01(t);
            return Color(y, y, y);
        }
        case Chart::Hot:
            return Color(encode01(t * 3.0f), encode01(t * 3.0f - 1.0f), encode01(t * 3.0f - 2.0f));
        case Chart::Cool:
            return Color(encode01(t), encode01(1.0f - t), 255);
        case Chart::Jet:
        {
            auto band = [](float x) {
                return std::clamp(1.5f - std::fabs(x), 0.0f, 1.0f);
            };
            return Color(encode01(band(4.0f * t - 3.0f)),
                         encode01(band(4.0f * t - 2.0f)),
                         encode01(band(4.0f * t - 1.0f)));
        }
        case Chart::Rainbow:
            return fromHsv(t * 300.0f, 1.0f, 1.0f);
        case Chart::Spectrum:
            return spectrum(static_cast<float>(kSpectrumMinNm) + t * static_cast<float>(kSpectrumMaxNm - kSpectrumMinNm));
        case Chart::CoolWarm:
        {
            static constexpr Color kStops[] = {
                Color::fromRgb(0x3B4CC0),
                Color::fromRgb(0xDDDDDD),
                Color::fromRgb(0xB40426),
            };
            return gradient(kStops, std::size_t(kStops), t);
        }
        case Chart::Viridis:
        {
            static constexpr Color kStops[] = {
                Color::fromRgb(0x440154), Color::fromRgb(0x482878), Color::fromRgb(0x3E4989),
                Color::fromRgb(0x31688E), Color::fromRgb(0x26828E), Color::fromRgb(0x1F9E89),
                Color::fromRgb(0x35B779), Color::fromRgb(0x6ECE58), Color::fromRgb(0xB5DE2B),
                Color::fromRgb(0xFDE725),
            };
            return gradient(kStops, std::size_t(kStops), t);
        }
        case Chart::Inferno:
        {
            static constexpr Color kStops[] = {
                Color::fromRgb(0x000004), Color::fromRgb(0x1B0C41), Color::fromRgb(0x4A0C6B),
                Color::fromRgb(0x781C6D), Color::fromRgb(0xA52C60), Color::fromRgb(0xCF4446),
                Color::fromRgb(0xED6925), Color::fromRgb(0xFB9B06), Color::fromRgb(0xF7D13D),
                Color::fromRgb(0xFCFFA4),
            };
            return gradient(kStops, std::size_t(kStops), t);
        }
        case Chart::Magma:
        {
            static constexpr Color kStops[] = {
                Color::fromRgb(0x000004), Color::fromRgb(0x140E36), Color::fromRgb(0x3B0F70),
                Color::fromRgb(0x641A80), Color::fromRgb(0x8C2981), Color::fromRgb(0xB73779),
                Color::fromRgb(0xDE4968), Color::fromRgb(0xF7705C), Color::fromRgb(0xFE9F6D),
                Color::fromRgb(0xFCFDBF),
            };
            return gradient(kStops, std::size_t(kStops), t);
        }
        case Chart::Plasma:
        {
            static constexpr Color kStops[] = {
                Color::fromRgb(0x0D0887), Color::fromRgb(0x46039F), Color::fromRgb(0x7201A8),
                Color::fromRgb(0x9C179E), Color::fromRgb(0xBD3786), Color::fromRgb(0xD8576B),
                Color::fromRgb(0xED7953), Color::fromRgb(0xFB9F3A), Color::fromRgb(0xFDCA26),
                Color::fromRgb(0xF0F921),
            };
            return gradient(kStops, std::size_t(kStops), t);
        }
        }
        return {};
    }
}
