#include "SirenWaveColumn.h"

#if COMPOSESIREN_SIREN_WAVES

#include "BinaryData.h"
#include "SirenWaveRaster.h"
#include "../../lib/definitions/generated/SirenMetadata.h"

namespace cs::waves {

namespace meta = mecaviv::metadata;

constexpr float twoPi = juce::MathConstants<float>::twoPi;
// sin(x + ph) and sin(1.9 x - 1.4 ph) both repeat after 10 pi
constexpr double phaseWrap = 5.0 * juce::MathConstants<double>::twoPi;
// the ripples' density is given per 55 logical px, the height of a cell
constexpr float densityLength = 55.0f;

Tuning Tuning::from(const Settings& s)
{
    using Id = Settings::Id;
    Tuning t;
    t.enabled = s.getBool(Id::wavesEnabled);
    t.renderer = static_cast<Renderer>(s.getInt(Id::wavesRenderer));
    t.frameRate = s.getInt(Id::wavesFrameRate);
    t.reach = s.getFloat(Id::wavesReach);
    t.lowTide = s.getFloat(Id::wavesLowTide);
    t.overflow = s.getBool(Id::wavesOverflow);
    t.overflowReach = static_cast<float>(s.getInt(Id::wavesOverflowReach));
    t.seamBlend = s.getBool(Id::wavesSeamBlend);
    t.seamZone = static_cast<float>(s.getInt(Id::wavesSeamZone));
    t.floorDb = s.getFloat(Id::wavesLevelFloorDb);
    t.fullDb = s.getFloat(Id::wavesLevelFullDb);
    t.autoLevel = s.getBool(Id::wavesAutoLevel);
    t.attackMs = static_cast<float>(s.getInt(Id::wavesAttackMs));
    t.releaseMs = static_cast<float>(s.getInt(Id::wavesReleaseMs));
    t.levelToReach = s.getFloat(Id::wavesLevelToReach);
    t.pitchToReach = s.getFloat(Id::wavesPitchToReach);
    t.levelToSwell = s.getFloat(Id::wavesLevelToSwell);
    t.levelToDepth = s.getFloat(Id::wavesLevelToDepth);
    t.pitchToDensity = s.getFloat(Id::wavesPitchToDensity);
    t.pitchToSpeed = s.getFloat(Id::wavesPitchToSpeed);
    t.travelSpeed = s.getFloat(Id::wavesTravelSpeed);
    t.flatness = s.getFloat(Id::wavesFlatness);
    t.parallaxLayers = s.getInt(Id::wavesParallaxLayers);
    t.parallaxDepth = s.getFloat(Id::wavesParallaxDepth);
    t.opacity = s.getFloat(Id::wavesOpacity);
    t.maskSteps = s.getInt(Id::wavesMaskSteps);
    t.strokeWidth = s.getFloat(Id::wavesStrokeWidth);
    t.labelFlip = s.getBool(Id::wavesLabelFlip);
    t.clouds = s.getFloat(Id::wavesClouds);
    return t;
}

#define CS_WAVES_STRING2(x) #x
#define CS_WAVES_STRING(x) CS_WAVES_STRING2(x)

SirenWaveColumn::SirenWaveColumn() :
    shaderCode(juce::String::createStringFromData(BinaryData::SirenWave_frag,
                                                  BinaryData::SirenWave_fragSize))
{
    shader = std::make_unique<juce::OpenGLGraphicsContextCustomShader>(shaderCode);
#if COMPOSESIREN_DEV_BUILD && defined(COMPOSESIREN_WAVES_SHADER_DIR)
    shaderFile = juce::File(CS_WAVES_STRING(COMPOSESIREN_WAVES_SHADER_DIR)).getChildFile("SirenWave.frag");
#endif
    // accent colour and range of each siren, from the metadata
    for (const auto& [id, strId] : sirenStrIdById) {
        Motion m;
        for (const auto& s : meta::sirens) {
            if (s.id != strId) continue;
            m.accentArgb = s.colourArgb;
            for (const auto& c : meta::sirenCategories) {
                if (c.name != s.category) continue;
                m.noteMin = static_cast<float>(c.noteMin);
                m.noteMax = static_cast<float>(c.noteMax);
            }
        }
        motion.emplace(id, m);
    }
}

void SirenWaveColumn::setCells(std::vector<Cell> topToBottom)
{
    cells = std::move(topToBottom);
    column = {};
    for (const auto& c : cells)
        column = column.isEmpty() ? c.bounds : column.getUnion(c.bounds);
    backgrounds.assign(cells.size(), {});
    layoutTitles();
}

void SirenWaveColumn::setTitleFont(juce::Font f)
{
    titleFont = f;
    layoutTitles();
}

// As the track's label lays it out: its border, centred, as many lines as fit.
void SirenWaveColumn::layoutTitles()
{
    titleLayouts.assign(cells.size(), {});
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const auto area = juce::BorderSize<int>(1, 5, 1, 5).subtractedFrom(cells[i].bounds.toNearestInt()).toFloat();
        const int lines = juce::jmax(1, static_cast<int>(area.getHeight() / titleFont.getHeight()));
        auto& layout = titleLayouts[i];
        layout.glyphs.addFittedText(titleFont, cells[i].title, area.getX(), area.getY(), area.getWidth(),
                                    area.getHeight(), juce::Justification::centred, lines, 0.0f);
        layout.bounds = layout.glyphs.getBoundingBox(0, -1, true);
    }
}

#if COMPOSESIREN_DEV_BUILD && defined(COMPOSESIREN_WAVES_SHADER_DIR)
void SirenWaveColumn::reloadShaderIfChanged(double seconds)
{
    untilShaderCheck -= seconds;
    if (untilShaderCheck > 0.0) return;
    untilShaderCheck = 0.5;
    const auto modified = shaderFile.getLastModificationTime();
    if (modified == shaderTime || !shaderFile.existsAsFile()) return;
    const bool first = shaderTime == juce::Time();
    shaderTime = modified;
    const auto code = shaderFile.loadFileAsString();
    if (first && code == shaderCode) return;
    DBG("SirenWaveColumn: reloading " << shaderFile.getFullPathName());
    shader = std::make_unique<juce::OpenGLGraphicsContextCustomShader>(code);
    shaderErrorLogged = false;
    shaderReloaded = true;
}
#endif

void SirenWaveColumn::setSirenState(sirenId id, float levelLinear, float pitchNote, bool noteOn)
{
    auto& m = motion.at(id);
    // kept in dB until advance(), where the silence threshold is known
    m.target = levelLinear > 1.0e-7f ? 20.0f * std::log10(levelLinear) : -200.0f;
    if ((noteOn || levelLinear > 1.0e-5f) && pitchNote > 0.0f) {
        if (m.targetNote < 0.0f) m.note = pitchNote;
        m.targetNote = pitchNote;
    }
}

static float pitchNormOf(float note, float noteMin, float noteMax)
{
    return juce::jlimit(0.0f, 1.0f, (note - noteMin) / juce::jmax(1.0f, noteMax - noteMin));
}

// Ripple wavenumber (radians per logical px): pitchToDensity 1 doubles the
// number of crests per octave, like an audio wave.
static float wavenumberOf(float note, float noteMin, const Tuning& t)
{
    const float octaves = juce::jmax(0.0f, (note - noteMin) / 12.0f);
    const float cycles = juce::jmin(14.0f, 0.6f * std::exp2(octaves * t.pitchToDensity));
    return twoPi * cycles / densityLength;
}

bool SirenWaveColumn::advance(double seconds, const Tuning& t)
{
    const auto dt = static_cast<float>(seconds);
    bool moving = false;
#if COMPOSESIREN_DEV_BUILD && defined(COMPOSESIREN_WAVES_SHADER_DIR)
    reloadShaderIfChanged(seconds);
    moving = std::exchange(shaderReloaded, false);
#endif
    for (auto& [id, m] : motion) {
        // auto level: full at the loudest this siren played lately (decaying 0.1 dB/s),
        // within 15 dB under the full level
        m.peakDb = juce::jmax(m.peakDb - 0.1f * dt, m.target);
        const float full = t.autoLevel ? juce::jlimit(t.fullDb - 15.0f, t.fullDb, m.peakDb) : t.fullDb;
        const float target = juce::jlimit(0.0f, 1.0f, (m.target - t.floorDb) / juce::jmax(1.0f, full - t.floorDb));
        const float tau = (target > m.level ? t.attackMs : t.releaseMs) / 1000.0f;
        m.level = tau <= 0.0f ? target : m.level + (target - m.level) * (1.0f - std::exp(-dt / tau));
        if (std::abs(m.level - target) < 1.0e-4f) m.level = target;
        if (m.targetNote >= 0.0f) m.note += (m.targetNote - m.note) * (1.0f - std::exp(-dt / 0.05f));

        if (m.level <= 0.0f) continue;
        moving = true;

        const float pitchNorm = pitchNormOf(m.note, m.noteMin, m.noteMax);
        const float k = wavenumberOf(m.note, m.noteMin, t);
        const float speed = t.travelSpeed * (0.5f + 0.5f * m.level) * (1.0f + 3.0f * t.pitchToSpeed * pitchNorm);
        for (int i = 0; i < maxLayers; ++i) {
            const float layerSpeed = speed * (1.0f - 0.35f * t.parallaxDepth * float(i));
            auto& ph = m.phase[size_t(i)];
            ph = std::fmod(ph + double(dt * layerSpeed * k * layerK(i, t.parallaxDepth)), phaseWrap);
        }
    }
    return moving;
}

// Low tide at the slightest sound; the reach at full loudness, the highest
// notes going furthest (pitchToReach: how much a low note holds the wave back).
float SirenWaveColumn::fillOf(const Motion& m, const Tuning& t, float width) const
{
    const float on = smoothstep(0.0f, 0.004f, m.level);
    const float pitch = pitchNormOf(m.note, m.noteMin, m.noteMax);
    const float drive = m.level * t.levelToReach * (1.0f - 0.5f * t.pitchToReach * (1.0f - pitch));
    const float least = juce::jmin(t.lowTide, t.reach) * width;
    const float most = t.reach * width + (t.overflow ? t.overflowReach : 0.0f);
    return on * (least + (most - least) * drive);
}

CellParams SirenWaveColumn::paramsFor(std::size_t index, const Tuning& t) const
{
    const auto& cell = cells[index];
    const auto& m = motion.at(cell.id);
    const float width = cell.bounds.getWidth();
    const float on = smoothstep(0.0f, 0.004f, m.level);
    const float pitchNorm = pitchNormOf(m.note, m.noteMin, m.noteMax);

    auto neighbourFill = [&](std::size_t j) {
        return fillOf(motion.at(cells[j].id), t, cells[j].bounds.getWidth());
    };

    CellParams p;
    p.width = width;
    p.height = cell.bounds.getHeight();
    p.fill = fillOf(m, t, width);
    p.fillUp = t.seamBlend && index > 0 ? juce::jmin(p.fill, neighbourFill(index - 1)) : p.fill;
    p.fillDown = t.seamBlend && index + 1 < cells.size() ? juce::jmin(p.fill, neighbourFill(index + 1)) : p.fill;
    p.seamZone = t.seamBlend ? juce::jmin(t.seamZone, 0.45f * p.height) : 0.0f;
    p.amp = width * (0.02f + 0.09f * m.level * t.levelToSwell) * on;
    p.k = wavenumberOf(m.note, m.noteMin, t);
    p.shape = smoothstep(0.2f, 0.9f, pitchNorm);
    for (size_t i = 0; i < p.phase.size(); ++i) p.phase[i] = static_cast<float>(m.phase[i]);
    p.layers = juce::jlimit(0, maxLayers - 1, t.parallaxLayers);
    p.parallax = t.parallaxDepth;
    p.level = m.level;
    p.depth = t.levelToDepth * (0.35f + 0.55f * m.level) * (1.0f - t.flatness);
    p.flatness = t.flatness;
    p.opacity = t.opacity;
    p.steps = juce::jlimit(1, 16, t.maskSteps);
    p.strokeWidth = t.strokeWidth;
    p.clouds = t.clouds;
    p.columnTop = column.getY();
    p.columnHeight = column.getHeight();
    p.originX = cell.bounds.getX();
    p.originY = cell.bounds.getY();
    p.baseArgb = cell.colour.getARGB();
    p.accentArgb = m.accentArgb;
    return p;
}

float SirenWaveColumn::overflowExtent(const Tuning& t) const
{
    if (!t.overflow) return 0.0f;
    float extent = 0.0f;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const auto p = paramsFor(i, t);
        const float furthest = p.fill * layerScale(p.layers, p.parallax) + p.amp + 0.5f * p.strokeWidth;
        extent = juce::jmax(extent, furthest - p.width);
    }
    return extent > 0.0f ? extent + 2.0f : 0.0f;
}

void SirenWaveColumn::paintCells(juce::Graphics& g, const Tuning& t, float cornerSize)
{
    const bool allowGpu = t.renderer != Tuning::Renderer::software;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const auto area = cells[i].bounds;
        const auto p = paramsFor(i, t);
        paintCell(g, p, area, {}, area.getRight(), 1.0f, allowGpu, i, cornerSize);
        paintTitle(g, p, i, t);
    }
}

// The name, as the track's label would draw it (it is empty while the waves
// are on): white, black where the wave covers it, white again on the pencil
// line. The negative is composited here, row by row with the wave's own edge,
// and drawn as one image: clip paths are slow through OpenGL.
void SirenWaveColumn::paintTitle(juce::Graphics& g, const CellParams& p, std::size_t index, const Tuning& t) const
{
    const auto& cell = cells[index];
    const auto& layout = titleLayouts[index];
    if (cell.title.isEmpty()) return;
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int w = juce::jmax(1, juce::roundToInt(cell.bounds.getWidth() * scale));
    const int h = juce::jmax(1, juce::roundToInt(cell.bounds.getHeight() * scale));
    if (layout.scale != scale) {
        layout.white = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
        {
            juce::Graphics ig(layout.white);
            ig.addTransform(juce::AffineTransform::translation(-cell.bounds.getX(), -cell.bounds.getY()).scaled(scale));
            ig.setColour(juce::Colours::white);
            layout.glyphs.draw(ig);
        }
        layout.mask.assign(size_t(w) * size_t(h), 0);
        juce::Image::BitmapData bits(layout.white, juce::Image::BitmapData::readOnly);
        juce::Rectangle<int> area;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (const auto a = bits.getPixelColour(x, y).getAlpha(); a > 0) {
                    layout.mask[size_t(y * w + x)] = a;
                    area = area.isEmpty() ? juce::Rectangle<int>(x, y, 1, 1) : area.getUnion({ x, y, 1, 1 });
                }
        layout.maskArea = area;
        layout.flipped = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType());
        layout.scale = scale;
    }

    // the furthest the wave and its line can go: the name, left of that, keeps its colour
    const float furthest = p.fill * layerScale(0, p.parallax) + p.amp + p.strokeWidth;
    if (!t.labelFlip || p.level <= 0.0f || cell.bounds.getRight() - furthest > layout.bounds.getRight()) {
        g.drawImage(layout.white, cell.bounds, juce::RectanglePlacement::stretchToFit);
        return;
    }

    {
        juce::Image::BitmapData bits(layout.flipped, juce::Image::BitmapData::writeOnly);
        const float halfStroke = 0.5f * p.strokeWidth * scale;
        const auto& area = layout.maskArea;
        for (int y = area.getY(); y < area.getBottom(); ++y) {
            const float a = (float(y) + 0.5f) / scale;
            const float sf = surface(p, 0, a) * scale;
            const float slope = halfStroke > 0.0f ? surfaceSlope(p, a) : 0.0f;
            auto* row = reinterpret_cast<std::uint32_t*>(bits.getLinePointer(y));
            for (int x = area.getX(); x < area.getRight(); ++x) {
                const auto m = layout.mask[size_t(y * w + x)];
                if (m == 0) { row[x] = 0; continue; }
                const float dx = sf - (float(w) - (float(x) + 0.5f));
                const float inside = std::clamp(dx / scale + 0.5f, 0.0f, 1.0f) * smoothstep(0.0f, scale, sf);
                float lum = 1.0f - inside;
                lum += (1.0f - lum) * strokeCoverage(dx, slope, halfStroke) * smoothstep(0.0f, scale, sf);
                const auto alpha = std::uint32_t(m);
                const auto v = std::uint32_t(lum * float(m) + 0.5f);
                row[x] = (alpha << 24) | (v << 16) | (v << 8) | v;
            }
        }
    }
    g.drawImage(layout.flipped, cell.bounds, juce::RectanglePlacement::stretchToFit);
}

void SirenWaveColumn::paintOverflow(juce::Graphics& g, const Tuning& t, juce::Point<float> origin)
{
    const float extent = overflowExtent(t);
    if (extent <= 0.0f) return;
    const bool allowGpu = t.renderer != Tuning::Renderer::software;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        const auto p = paramsFor(i, t);
        if (p.fill * layerScale(p.layers, p.parallax) + p.amp <= p.width) continue;
        const auto cell = cells[i].bounds;
        const auto area = juce::Rectangle<float>(cell.getX() - extent, cell.getY(), extent, cell.getHeight()) - origin;
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(area.toNearestInt());
        paintCell(g, p, area, origin, cell.getRight() - origin.x, 0.0f, allowGpu, cells.size(), 0.0f);
    }
}

void SirenWaveColumn::paintCell(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                                juce::Point<float> origin, float edgeX, float baseAlpha, bool allowGpu,
                                std::size_t index, float cornerSize)
{
    // the shader rounds the corners itself: a clip path would split its quad
    // into one per line, which JUCE's OpenGL renderer queues slowly
    const bool gpu = allowGpu && paintWithShader(g, p, area, origin, edgeX, baseAlpha, cornerSize);
    if (!gpu) {
        juce::Graphics::ScopedSaveState state(g);
        if (cornerSize > 0.0f) {
            juce::Path clip;
            clip.addRoundedRectangle(area, cornerSize);
            g.reduceClipRegion(clip);
        }
        paintInSoftware(g, p, area, edgeX, baseAlpha, index);
    }
    if (gpu != usedOpenGL || !reported) {
        DBG("SirenWaveColumn: drawing " << (gpu ? "with OpenGL" : "in software"));
        reported = true;
    }
    usedOpenGL = gpu;
}

// `area` and `edgeX` are in g's coordinates, g being the Graphics of a
// component at `origin` in the editor, which the OpenGL context renders.
bool SirenWaveColumn::paintWithShader(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                                      juce::Point<float> origin, float edgeX, float baseAlpha, float cornerSize)
{
    auto& context = g.getInternalContext();
    const auto compiled = shader->checkCompilation(context);
    if (compiled.failed()) {
        // an empty message: not an OpenGL context, nothing to report
        if (compiled.getErrorMessage().isEmpty()) return false;
        if (!shaderErrorLogged) {
            juce::Logger::writeToLog("SirenWave.frag: " + compiled.getErrorMessage());
            shaderErrorLogged = true;
        }
        // an edited shader that doesn't compile: back to the last one that did
        if (shader->getFragmentShaderCode().endsWith(shaderCode)) return false;
        shader = std::make_unique<juce::OpenGLGraphicsContextCustomShader>(shaderCode);
        return false;
    }
    if (!shader->getFragmentShaderCode().endsWith(shaderCode))
        shaderCode = shader->getFragmentShaderCode().fromFirstOccurrenceOf("#define pixelAlpha frontColour.a\n", false, false);

    const float scale = context.getPhysicalPixelScaleFactor();
    const auto device = (area + origin) * scale;
    const float edgeDevice = (edgeX + origin.x) * scale;
    const juce::Colour base(p.baseArgb);
    const juce::Colour accent(p.accentArgb);

    shader->onShaderActivated = [=](juce::OpenGLShaderProgram& program) {
        program.setUniform("uRect", device.getX(), device.getY(), device.getWidth(), device.getHeight());
        program.setUniform("uEdgeX", edgeDevice);
        program.setUniform("uScale", scale);
        program.setUniform("uBase", base.getFloatRed() * baseAlpha, base.getFloatGreen() * baseAlpha,
                           base.getFloatBlue() * baseAlpha, baseAlpha);
        program.setUniform("uAccent", accent.getFloatRed(), accent.getFloatGreen(), accent.getFloatBlue());
        program.setUniform("uFills", p.fill, p.fillUp, p.fillDown, p.seamZone);
        program.setUniform("uRipple", p.amp, p.k, p.shape, p.parallax);
        program.setUniform("uPhase", p.phase[0], p.phase[1], p.phase[2], p.phase[3]);
        program.setUniform("uLook", p.level, p.depth, p.flatness, p.opacity);
        program.setUniform("uLayers", static_cast<GLfloat>(p.layers), static_cast<GLfloat>(p.steps),
                           cornerSize, 0.0f);
        program.setUniform("uExtra", p.strokeWidth, p.clouds, p.columnTop, p.columnHeight);
    };
    g.setColour(juce::Colours::white); // pixelAlpha: the clip's antialiasing only
    shader->fillRect(context, area.getSmallestIntegerContainer());
    return true;
}

void SirenWaveColumn::paintInSoftware(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                                      float edgeX, float baseAlpha, std::size_t index)
{
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    const int w = juce::jmax(1, juce::roundToInt(area.getWidth() * scale));
    const int h = juce::jmax(1, juce::roundToInt(area.getHeight() * scale));
    // a fresh image per cell: a renderer may still hold the previous one
    // the cell's colour with clouds (not for the overflow, which has none)
    const std::uint32_t* background = nullptr;
    if (index < backgrounds.size() && baseAlpha >= 1.0f) {
        auto& b = backgrounds[index];
        if (b.w != w || b.h != h || b.base != p.baseArgb || b.clouds != p.clouds || b.area != area) {
            b = { w, h, p.baseArgb, p.clouds, area, std::vector<std::uint32_t>(size_t(w) * size_t(h)) };
            rasterBackground(p, b.pixels.data(), w, h, w, scale, baseAlpha);
        }
        background = b.pixels.data();
    }
    juce::Image image(juce::Image::ARGB, w, h, false, juce::SoftwareImageType());
    {
        juce::Image::BitmapData bits(image, juce::Image::BitmapData::writeOnly);
        rasterCell(p, reinterpret_cast<std::uint32_t*>(bits.data), w, h, bits.lineStride / bits.pixelStride,
                   scale, (edgeX - area.getX()) * scale, baseAlpha, background);
    }
    g.drawImage(image, area, juce::RectanglePlacement::stretchToFit);
}

} // namespace cs::waves

#endif // COMPOSESIREN_SIREN_WAVES
