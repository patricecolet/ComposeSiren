//
// Software renderer of a wave cell (COMPOSESIREN_SIREN_WAVES): the same
// picture as Assets/shaders/SirenWave.frag, computed on the CPU. The edges
// depend only on the row, so they are computed once per row, and the pixels
// beyond every edge are a plain copy of the cell's colour.
//

#ifndef COMPOSESIREN_SIRENWAVERASTER_H
#define COMPOSESIREN_SIRENWAVERASTER_H

#include "SirenWaveMath.h"

namespace cs::waves {

struct Rgba { float r, g, b, a; }; // premultiplied

inline Rgba unpack(std::uint32_t argb, float alpha)
{
    return { float((argb >> 16) & 0xff) / 255.0f * alpha,
             float((argb >> 8) & 0xff) / 255.0f * alpha,
             float(argb & 0xff) / 255.0f * alpha,
             alpha };
}

inline Rgba unpackPremultiplied(std::uint32_t argb)
{
    return { float((argb >> 16) & 0xff) / 255.0f, float((argb >> 8) & 0xff) / 255.0f,
             float(argb & 0xff) / 255.0f, float(argb >> 24) / 255.0f };
}

inline std::uint32_t pack(Rgba c)
{
    auto q = [](float v) { return std::uint32_t(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return (q(c.a) << 24) | (q(c.r) << 16) | (q(c.g) << 8) | q(c.b);
}

// c, then an opaque colour on top with coverage t.
inline Rgba over(Rgba c, Rgba top, float t)
{
    const float u = 1.0f - t;
    return { c.r * u + top.r * t, c.g * u + top.g * t, c.b * u + top.b * t, c.a * u + t };
}

// The cell's colour with the clouds, premultiplied by baseAlpha, into `dst`.
// It only changes with the cell's size, colour and place, so it is computed
// once and passed to rasterCell.
inline void rasterBackground(const CellParams& p, std::uint32_t* dst, int w, int h, int stride,
                             float scale, float baseAlpha)
{
    const Rgba base = unpack(p.baseArgb, baseAlpha);
    for (int y = 0; y < h; ++y) {
        const float ey = p.originY + (float(y) + 0.5f) / scale;
        for (int x = 0; x < w; ++x) {
            const float ex = p.originX + (float(x) + 0.5f) / scale;
            const float k = cloudLight(ex, ey, p.columnTop, p.columnHeight, p.clouds);
            dst[y * stride + x] = pack({ base.r + (base.a - base.r) * k, base.g + (base.a - base.g) * k,
                                         base.b + (base.a - base.b) * k, base.a });
        }
    }
}

// Renders a cell into `dst` (premultiplied 0xAARRGGBB, the layout of JUCE's
// PixelARGB on little-endian machines). `scale`: device pixels per logical
// pixel. `edgeX`: the device x of the cell's right edge, relative to dst's
// left: the image's width for the cell itself, more for an image lying left
// of the cell (the overflow). `baseAlpha`: 1 paints the cell's colour, 0
// leaves everything but the wave transparent. `background` (same size, from
// rasterBackground) replaces the plain colour when given.
inline void rasterCell(const CellParams& p, std::uint32_t* dst, int w, int h, int stride,
                       float scale, float edgeX, float baseAlpha, const std::uint32_t* background = nullptr)
{
    const float edge = scale;
    const Rgba plain = unpack(p.baseArgb, baseAlpha);
    const std::uint32_t plainPx = pack(plain);
    const float halfStroke = 0.5f * p.strokeWidth * scale;
    const Rgba accent = unpack(p.accentArgb, 1.0f);
    const Rgba crestColour { mixf(accent.r, 1.0f, 0.45f), mixf(accent.g, 1.0f, 0.45f), mixf(accent.b, 1.0f, 0.45f), 1.0f };
    const float waveA = mixf(0.6f, 0.95f, p.level) * p.opacity;
    const float crestA = p.strokeWidth > 0.0f ? 0.0f : 0.7f * p.opacity * (1.0f - 0.6f * p.flatness);
    const Rgba black { 0.0f, 0.0f, 0.0f, 1.0f };
    const int layers = std::clamp(p.layers, 0, maxLayers - 1);

    std::array<float, maxLayers> surf {};
    std::array<float, maxLayers> gate {};
    for (int y = 0; y < h; ++y) {
        std::uint32_t* row = dst + y * stride;
        const float a = (float(y) + 0.5f) / scale;
        float reach = 0.0f;
        for (int i = 0; i <= layers; ++i) {
            surf[size_t(i)] = surface(p, i, a) * scale;
            gate[size_t(i)] = smoothstep(0.0f, edge, surf[size_t(i)]);
            reach = std::max(reach, surf[size_t(i)]);
        }
        const float sf = surf[0];
        const float slope = halfStroke > 0.0f ? surfaceSlope(p, a) : 0.0f;
        const std::uint32_t* back = background != nullptr ? background + y * w : nullptr;
        reach += halfStroke * std::sqrt(1.0f + slope * slope);
        for (int x = 0; x < w; ++x) {
            const float d = edgeX - (float(x) + 0.5f);
            if (d > reach + 2.0f * edge) {
                row[x] = back != nullptr ? back[x] : plainPx;
                continue;
            }
            Rgba c = back != nullptr ? unpackPremultiplied(back[x]) : plain;
            for (int i = layers; i >= 1; --i) {
                const float m = coverage(surf[size_t(i)] - d, edge, p.steps) * gate[size_t(i)];
                c = over(c, accent, m * layerAlpha(i, p.level) * p.opacity);
            }
            const float mf = coverage(sf - d, edge, p.steps) * gate[0];
            if (mf > 0.0f) {
                const float t = std::clamp((sf - d) / std::max(sf, 1.0f), 0.0f, 1.0f) * p.depth;
                const float dark = 1.0f - 0.55f * t;
                c = over(c, { accent.r * dark, accent.g * dark, accent.b * dark, 1.0f }, mf * waveA);
            }
            const float crest = (1.0f - smoothstep(0.0f, 1.6f * edge, std::abs(sf - d))) * gate[0];
            c = over(c, crestColour, crest * crestA);
            c = over(c, black, strokeCoverage(sf - d, slope, halfStroke) * gate[0]);
            row[x] = pack(c);
        }
    }
}

} // namespace cs::waves

#endif //COMPOSESIREN_SIRENWAVERASTER_H
