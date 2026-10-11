//
// The waves in the siren title cells (COMPOSESIREN_SIREN_WAVES): the maths
// shared by the OpenGL shader (Assets/shaders/SirenWave.frag) and the
// software renderer (SirenWaveRaster.h). No JUCE here, so the software
// renderer can be benchmarked on its own.
//
// A cell is filled from its right edge. `d` is the distance from that edge,
// `a` the position along the cell, from its top. The wave's edge sits at
// `surface(a)`: everything with d < surface(a) is wave, the rest keeps the
// cell's colour, lightened by clouds toward the top of the column. A black
// pencil line follows the edge. Keep this file and the shader in step.
//

#ifndef COMPOSESIREN_SIRENWAVEMATH_H
#define COMPOSESIREN_SIRENWAVEMATH_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace cs::waves {

constexpr int maxLayers = 4; // the main wave and up to 3 parallax layers

// Everything a renderer needs to draw one cell, in logical pixels.
struct CellParams {
    float width = 70.0f;   // the cell
    float height = 55.0f;
    float fill = 0.0f;     // edge distance from the right side, mid-cell
    float fillUp = 0.0f;   // edge distance at the top seam
    float fillDown = 0.0f; // edge distance at the bottom seam
    float seamZone = 0.0f; // blend length near the seams, 0: no blend
    float amp = 0.0f;      // ripple amplitude
    float k = 0.1f;        // ripple wavenumber, radians per logical pixel
    float shape = 0.0f;    // 0: ocean swell, 1: pure sine (audio-like)
    std::array<float, maxLayers> phase {};
    int layers = 0;        // parallax layers behind the main wave
    float parallax = 0.5f;
    float level = 0.0f;    // 0..1, shapes the wave's strength
    float depth = 0.0f;    // darkening toward the edge, 0..1
    float flatness = 0.0f; // 1: one flat colour
    float opacity = 1.0f;
    int steps = 16;        // shades in the antialiased edge
    float strokeWidth = 0.0f;   // the pencil line along the edge, 0: a light crest instead
    float clouds = 0.0f;        // 0..1
    float columnTop = 0.0f;     // the column of cells, in the editor (clouds fade downward)
    float columnHeight = 1.0f;
    float originX = 0.0f;       // the cell's top-left, in the editor (clouds are continuous)
    float originY = 0.0f;
    std::uint32_t baseArgb = 0xff000000u;
    std::uint32_t accentArgb = 0xffffffffu;
};

inline float smoothstep(float e0, float e1, float x)
{
    const float t = std::clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float mixf(float a, float b, float t) { return a + (b - a) * t; }

// Ripple profile in [-1, 1].
inline float ripple(float x, float ph, float shape)
{
    const float s1 = std::sin(x + ph);
    const float swell = (s1 + 0.35f * std::sin(1.9f * x - 1.4f * ph + 1.3f)) / 1.35f;
    return mixf(swell, s1, shape);
}

// Layer i (0 = main wave): how much further in it sits, and its ripple.
inline float layerScale(int i, float parallax) { return 1.0f + float(i) * (0.06f + 0.1f * parallax); }
inline float layerK(int i, float parallax) { return 1.0f - 0.18f * parallax * float(i); }
inline float layerAmp(int i) { return 1.0f - 0.15f * float(i); }
inline float layerAlpha(int i, float level) { return 0.35f * (1.0f - 0.3f * float(i - 1)) * (0.5f + 0.5f * level); }

// The edge of layer i at position a (logical px from the top of the cell).
inline float surface(const CellParams& p, int i, float a)
{
    const float st = p.seamZone > 0.0f ? smoothstep(0.0f, p.seamZone, a) : 1.0f;
    const float sb = p.seamZone > 0.0f ? smoothstep(0.0f, p.seamZone, p.height - a) : 1.0f;
    const float s = layerScale(i, p.parallax);
    const float base = s * (p.fill + (p.fillUp - p.fill) * (1.0f - st) + (p.fillDown - p.fill) * (1.0f - sb));
    const float k = p.k * layerK(i, p.parallax);
    return std::max(0.0f, base + p.amp * layerAmp(i) * st * sb * ripple(a * k, -p.phase[size_t(i)], p.shape));
}

// Slope of the main wave's edge, d(surface)/da.
inline float surfaceSlope(const CellParams& p, float a)
{
    return (surface(p, 0, a + 0.5f) - surface(p, 0, a - 0.5f));
}

// Coverage of the pencil line at a horizontal distance `dx` from the edge,
// all in device pixels: the distance is made perpendicular with the slope.
inline float strokeCoverage(float dx, float slope, float halfWidth)
{
    if (halfWidth <= 0.0f) return 0.0f;
    const float dist = std::abs(dx) / std::sqrt(1.0f + slope * slope);
    return 1.0f - smoothstep(halfWidth - 0.75f, halfWidth + 0.75f, dist);
}

inline float fract(float x) { return x - std::floor(x); }

// "Hash without sine" (Dave Hoskins): the same in float on the CPU and in GLSL.
inline float hash12(float x, float y)
{
    float a = fract(x * 0.1031f), b = fract(y * 0.1031f), c = fract(x * 0.1031f);
    const float d = a * (b + 33.33f) + b * (c + 33.33f) + c * (a + 33.33f);
    a += d; b += d; c += d;
    return fract((a + b) * c);
}

inline float valueNoise(float x, float y)
{
    const float ix = std::floor(x), iy = std::floor(y);
    const float fx = x - ix, fy = y - iy;
    const float ux = fx * fx * (3.0f - 2.0f * fx), uy = fy * fy * (3.0f - 2.0f * fy);
    return mixf(mixf(hash12(ix, iy), hash12(ix + 1.0f, iy), ux),
                mixf(hash12(ix, iy + 1.0f), hash12(ix + 1.0f, iy + 1.0f), ux), uy);
}

// How much white the clouds add at (x, y), logical px in the editor: patches
// of light, plus a haze, both strongest at the top of the column.
inline float cloudLight(float x, float y, float columnTop, float columnHeight, float amount)
{
    if (amount <= 0.0f) return 0.0f;
    const float t = std::clamp((y - columnTop) / std::max(columnHeight, 1.0f), 0.0f, 1.0f);
    const float weight = std::pow(1.0f - t, 1.6f);
    float n = 0.0f, scale = 0.5f, px = x / 38.0f, py = y / 38.0f;
    for (int octave = 0; octave < 4; ++octave) {
        n += scale * valueNoise(px, py);
        px = px * 2.03f + 17.1f;
        py = py * 2.03f + 9.2f;
        scale *= 0.5f;
    }
    return amount * weight * (0.14f + 0.42f * smoothstep(0.38f, 0.8f, n));
}

// Antialiased, quantised coverage of a point `dist` inside an edge (`edge`: px per logical px).
inline float coverage(float dist, float edge, int steps)
{
    const float m = std::clamp(dist / (2.0f * edge) + 0.5f, 0.0f, 1.0f);
    return std::floor(m * float(steps) + 0.0001f) / float(steps);
}

} // namespace cs::waves

#endif //COMPOSESIREN_SIRENWAVEMATH_H
