// The waves in the siren title cells (COMPOSESIREN_SIREN_WAVES), drawn with
// juce::OpenGLGraphicsContextCustomShader, which declares pixelPos (device
// pixels from the top-left of the OpenGL target) and pixelAlpha (clip
// antialiasing). Same maths as Source/ComposeSirenCore/Components/waves/
// SirenWaveMath.h and SirenWaveRaster.h: keep them in step.
//
// The cell is filled from its right edge: d is the distance from that edge,
// a the position along the cell from its top, in logical pixels. Clouds
// lighten the cell's colour toward the top of the column, a black pencil
// line follows the wave's edge.

uniform vec4 uRect;    // the cell: x, y, width, height (device pixels)
uniform float uEdgeX;  // device x of the cell's right edge
uniform float uScale;  // device pixels per logical pixel
uniform vec4 uBase;    // the cell's colour, premultiplied (alpha 0 for the overflow)
uniform vec3 uAccent;
uniform vec4 uFills;   // fill, fill at the top seam, fill at the bottom seam, seam zone (logical px)
uniform vec4 uRipple;  // amplitude (logical px), wavenumber (rad / logical px), shape, parallax
uniform vec4 uPhase;   // main wave, parallax layers 1 to 3
uniform vec4 uLook;    // level, depth, flatness, opacity
uniform vec4 uLayers;  // parallax layers, shades in the edge, corner radius (logical px), unused
uniform vec4 uExtra;   // pencil line width (logical px), clouds, column top, column height (logical px)

float ripple(float x, float ph, float shape)
{
    float s1 = sin(x + ph);
    float swell = (s1 + 0.35 * sin(1.9 * x - 1.4 * ph + 1.3)) / 1.35;
    return mix(swell, s1, shape);
}

// Edge of layer i at a, in device pixels.
float surfaceOf(float i, float a, float ph)
{
    float zone = uFills.w;
    float st = zone > 0.0 ? smoothstep(0.0, zone, a) : 1.0;
    float sb = zone > 0.0 ? smoothstep(0.0, zone, uRect.w / uScale - a) : 1.0;
    float par = uRipple.w;
    float s = 1.0 + i * (0.06 + 0.1 * par);
    float base = s * (uFills.x + (uFills.y - uFills.x) * (1.0 - st) + (uFills.z - uFills.x) * (1.0 - sb));
    float k = uRipple.y * (1.0 - 0.18 * par * i);
    return max(0.0, base + uRipple.x * (1.0 - 0.15 * i) * st * sb * ripple(a * k, -ph, uRipple.z)) * uScale;
}

// "Hash without sine" (Dave Hoskins), as in SirenWaveMath.h.
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = p - i;
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash12(i), hash12(i + vec2(1.0, 0.0)), u.x),
               mix(hash12(i + vec2(0.0, 1.0)), hash12(i + vec2(1.0, 1.0)), u.x), u.y);
}

// White added by the clouds at p (logical px in the editor).
float cloudLight(vec2 p)
{
    if (uExtra.y <= 0.0)
        return 0.0;
    float t = clamp((p.y - uExtra.z) / max(uExtra.w, 1.0), 0.0, 1.0);
    float weight = pow(1.0 - t, 1.6);
    float n = 0.0;
    float scale = 0.5;
    vec2 q = p / 38.0;
    for (int octave = 0; octave < 4; ++octave) {
        n += scale * valueNoise(q);
        q = q * 2.03 + vec2(17.1, 9.2);
        scale *= 0.5;
    }
    return uExtra.y * weight * (0.14 + 0.42 * smoothstep(0.38, 0.8, n));
}

float coverage(float dist)
{
    float m = clamp(dist / (2.0 * uScale) + 0.5, 0.0, 1.0);
    return floor(m * uLayers.y + 0.0001) / uLayers.y;
}

vec4 over(vec4 c, vec3 top, float t)
{
    return c * (1.0 - t) + vec4(top, 1.0) * t;
}

vec4 backLayer(vec4 c, float i, float ph, float a, float d)
{
    if (i > uLayers.x)
        return c;
    float s = surfaceOf(i, a, ph);
    float m = coverage(s - d) * smoothstep(0.0, uScale, s);
    float alpha = 0.35 * (1.0 - 0.3 * (i - 1.0)) * (0.5 + 0.5 * uLook.x) * uLook.w;
    return over(c, uAccent, m * alpha);
}

void main()
{
    float a = (pixelPos.y - uRect.y) / uScale;
    float d = uEdgeX - pixelPos.x;

    vec4 c = uBase;
    c.rgb += (vec3(c.a) - c.rgb) * cloudLight(pixelPos / uScale);
    c = backLayer(c, 3.0, uPhase.w, a, d);
    c = backLayer(c, 2.0, uPhase.z, a, d);
    c = backLayer(c, 1.0, uPhase.y, a, d);

    float sf = surfaceOf(0.0, a, uPhase.x);
    float gf = smoothstep(0.0, uScale, sf);
    float mf = coverage(sf - d) * gf;
    float t = clamp((sf - d) / max(sf, 1.0), 0.0, 1.0) * uLook.y;
    c = over(c, uAccent * (1.0 - 0.55 * t), mf * mix(0.6, 0.95, uLook.x) * uLook.w);

    float crest = (1.0 - smoothstep(0.0, 1.6 * uScale, abs(sf - d))) * gf;
    float crestAlpha = uExtra.x > 0.0 ? 0.0 : 0.7 * uLook.w * (1.0 - 0.6 * uLook.z);
    c = over(c, mix(uAccent, vec3(1.0), 0.45), crest * crestAlpha);

    // the pencil line, as thick across a steep ripple as across a flat one
    float halfStroke = 0.5 * uExtra.x * uScale;
    if (halfStroke > 0.0) {
        float slope = (surfaceOf(0.0, a + 0.5, uPhase.x) - surfaceOf(0.0, a - 0.5, uPhase.x)) / uScale;
        float dist = abs(sf - d) / sqrt(1.0 + slope * slope);
        c = over(c, vec3(0.0), (1.0 - smoothstep(halfStroke - 0.75, halfStroke + 0.75, dist)) * gf);
    }

    // the cell's rounded corners
    vec2 q = abs(pixelPos - uRect.xy - 0.5 * uRect.zw) - 0.5 * uRect.zw + uLayers.z * uScale;
    float corner = length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - uLayers.z * uScale;
    gl_FragColor = c * pixelAlpha * (1.0 - smoothstep(-0.5, 0.5, corner));
}
