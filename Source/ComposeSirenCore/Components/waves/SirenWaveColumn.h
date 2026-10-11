//
// The waves in SirenOrchestra's siren title cells (COMPOSESIREN_SIREN_WAVES).
//
// The column follows each siren's output level and pitch (SirenStateMonitor,
// 30 times a second), animates them at the display's rate, and paints the
// title cells: with OpenGL (Assets/shaders/SirenWave.frag) when the
// Graphics it is given renders through an attached juce::OpenGLContext,
// in software (SirenWaveRaster.h) otherwise. Message thread only.
//

#ifndef COMPOSESIREN_SIRENWAVECOLUMN_H
#define COMPOSESIREN_SIRENWAVECOLUMN_H

#if COMPOSESIREN_SIREN_WAVES

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>
#include <map>
#include <vector>
#include "SirenWaveMath.h"
#include "../../lib/definitions/sirenProperties.h"
#include "../../lib/settings/Settings.h"

namespace cs::waves {

// The wave settings, read once per frame.
struct Tuning {
    enum class Renderer { automatic, gpu, software };

    bool enabled = true;
    Renderer renderer = Renderer::automatic;
    int frameRate = 60;
    float reach = 0.96f;
    float lowTide = 0.15f;
    bool overflow = false;
    float overflowReach = 120.0f;
    bool seamBlend = true;
    float seamZone = 16.0f;
    float floorDb = -66.0f;
    float fullDb = -20.0f;
    bool autoLevel = true;
    float attackMs = 80.0f;
    float releaseMs = 450.0f;
    float levelToReach = 1.0f;
    float pitchToReach = 0.5f;
    float levelToSwell = 1.0f;
    float levelToDepth = 1.0f;
    float pitchToDensity = 0.6f;
    float pitchToSpeed = 0.7f;
    float travelSpeed = 60.0f;
    float flatness = 0.3f;
    int parallaxLayers = 1;
    float parallaxDepth = 0.5f;
    float opacity = 0.9f;
    int maskSteps = 16;
    float strokeWidth = 2.5f;
    bool labelFlip = true;
    float clouds = 0.5f;

    static Tuning from(const Settings& s);
};

class SirenWaveColumn
{
public:
    struct Cell {
        sirenId id;
        juce::Rectangle<float> bounds; // in the editor
        juce::Colour colour;           // the cell's colour when silent
        juce::String title;            // the siren's name, drawn over the wave
    };

    SirenWaveColumn();

    // top to bottom, in the editor's coordinates
    void setCells(std::vector<Cell> topToBottom);
    void setTitleFont(juce::Font f);
    const std::vector<Cell>& getCells() const { return cells; }

    // the latest state of a siren: output level (linear RMS), pitch (MIDI note)
    void setSirenState(sirenId id, float levelLinear, float pitchNote, bool noteOn);

    // advances the animation; true while anything moves or has yet to settle
    bool advance(double seconds, const Tuning& t);

    // the cells, with rounded corners, in g = the editor's Graphics
    void paintCells(juce::Graphics& g, const Tuning& t, float cornerSize);

    // what lies left of the cells when overflow is on, in the Graphics of a
    // component placed at `origin` in the editor
    void paintOverflow(juce::Graphics& g, const Tuning& t, juce::Point<float> origin);

    // how far left of its cell's left edge the furthest wave goes (logical px)
    float overflowExtent(const Tuning& t) const;

    // whether the last paint went through OpenGL
    bool lastPaintUsedOpenGL() const { return usedOpenGL; }

private:
    struct Motion {
        float target = 0.0f;      // the level in dB (mapped to 0..1 in advance())
        float peakDb = -200.0f;   // the loudest lately, for the auto level
        float level = 0.0f;       // target, smoothed
        float targetNote = -1.0f; // the note the siren plays, -1 before any
        float note = 0.0f;        // smoothed
        float noteMin = 12.0f;    // the siren's range (metadata)
        float noteMax = 74.0f;
        std::array<double, maxLayers> phase {};
        std::uint32_t accentArgb = 0xffffffffu;
    };

    CellParams paramsFor(std::size_t index, const Tuning& t) const;
    float fillOf(const Motion& m, const Tuning& t, float width) const;
    void paintCell(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                   juce::Point<float> origin, float edgeX, float baseAlpha, bool allowGpu,
                   std::size_t index, float cornerSize);
    bool paintWithShader(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                         juce::Point<float> origin, float edgeX, float baseAlpha, float cornerSize);
    void paintInSoftware(juce::Graphics& g, const CellParams& p, juce::Rectangle<float> area,
                         float edgeX, float baseAlpha, std::size_t index);
    void paintTitle(juce::Graphics& g, const CellParams& p, std::size_t index, const Tuning& t) const;

    // the software renderer's cell colour with clouds, per cell
    struct Background {
        int w = 0, h = 0;
        std::uint32_t base = 0;
        float clouds = -1.0f;
        juce::Rectangle<float> area;
        std::vector<std::uint32_t> pixels;
    };
    std::vector<Background> backgrounds;
    juce::Rectangle<float> column;
    juce::Font titleFont { juce::FontOptions {} };
    // the titles, laid out once, and drawn once into images at the display's
    // scale: drawing glyphs at every frame is slow, through OpenGL above all
    struct TitleLayout {
        juce::GlyphArrangement glyphs;
        juce::Rectangle<float> bounds;
        mutable juce::Image white;          // the name as the label draws it
        mutable juce::Image flipped;        // the name with the wave's negative, redone per frame
        mutable std::vector<std::uint8_t> mask; // the name's coverage, at the display's scale
        mutable juce::Rectangle<int> maskArea;  // where the name is, in the images' pixels
        mutable float scale = 0.0f;
    };
    std::vector<TitleLayout> titleLayouts;
    void layoutTitles();

    std::vector<Cell> cells;
    std::map<sirenId, Motion> motion;
    std::unique_ptr<juce::OpenGLGraphicsContextCustomShader> shader;
    juce::String shaderCode;          // the code that last compiled
#if COMPOSESIREN_DEV_BUILD && defined(COMPOSESIREN_WAVES_SHADER_DIR)
    // Debug builds read the shader from the source tree, and reload it when it changes
    void reloadShaderIfChanged(double seconds);
    juce::File shaderFile;
    juce::Time shaderTime;
    double untilShaderCheck = 0.0;
    bool shaderReloaded = false;
#endif
    bool shaderErrorLogged = false;
    bool usedOpenGL = false;
    bool reported = false;
};

} // namespace cs::waves

#endif // COMPOSESIREN_SIREN_WAVES

#endif //COMPOSESIREN_SIRENWAVECOLUMN_H
