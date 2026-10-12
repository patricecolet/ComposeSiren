// GENERATED from the shared metadata (composesiren-settings.csv). Do not edit: change the metadata and regenerate.
#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace mecaviv::metadata {

enum class SettingType { Bool, Int, Float, Choice, String };

struct Setting {
    std::string_view id;
    std::string_view group;
    std::string_view label;
    SettingType type;
    double defaultValue;            // bool: 0 or 1, choice: index into choices
    double minimum;                 // bool and choice: 0
    double maximum;                 // bool: 1, choice: number of choices - 1
    std::string_view choices;       // '|'-separated, empty unless Choice
    std::string_view unit;
    std::string_view requiredOption; // CMake option the setting needs, or empty
    std::string_view scope;          // "global" or "instance"
    std::string_view description;
};

enum class SettingId : std::size_t {
    wavesEnabled,
    wavesRenderer,
    wavesFrameRate,
    wavesReach,
    wavesLowTide,
    wavesOverflow,
    wavesOverflowReach,
    wavesSeamBlend,
    wavesSeamZone,
    wavesLevelFloorDb,
    wavesLevelFullDb,
    wavesAutoLevel,
    wavesAttackMs,
    wavesReleaseMs,
    wavesLevelToReach,
    wavesPitchToReach,
    wavesLevelToSwell,
    wavesLevelToDepth,
    wavesPitchToDensity,
    wavesPitchToSpeed,
    wavesTravelSpeed,
    wavesFlatness,
    wavesParallaxLayers,
    wavesParallaxDepth,
    wavesOpacity,
    wavesStrokeWidth,
    wavesLabelFlip,
    wavesClouds,
    wavesMaskSteps,
    clicOutputDevice,
    dspVariant,
    dspDataset,
};

inline constexpr std::size_t settingCount = 32;

inline constexpr std::array<Setting, settingCount> settings {{
    { "waves.enabled", "Waves", "Show waves", SettingType::Bool, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Animated waves in the siren title cells, following each siren's loudness and pitch." },
    { "waves.renderer", "Waves", "Renderer", SettingType::Choice, 0.0, 0.0, 2.0, "auto|gpu|software", "", "COMPOSESIREN_SIREN_WAVES", "global", "auto: OpenGL when the editor can use it, software otherwise. software never attaches an OpenGL context." },
    { "waves.frame_rate", "Waves", "Frame rate cap", SettingType::Int, 60.0, 15.0, 60.0, "", "fps", "COMPOSESIREN_SIREN_WAVES", "global", "Highest animation rate. Lower it to save CPU, mostly with the software renderer." },
    { "waves.reach", "Waves", "Reach", SettingType::Float, 0.96, 0.1, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How far into the title cell the wave goes at full loudness on the highest notes, as a fraction of the cell width." },
    { "waves.low_tide", "Waves", "Low tide", SettingType::Float, 0.15, 0.0, 0.5, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How far into the title cell the wave goes at the slightest sound, as a fraction of the cell width." },
    { "waves.overflow", "Waves", "Extend past the title cells", SettingType::Bool, 0.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Let loud waves continue to the left of the title cells, over the end of the strip." },
    { "waves.overflow_reach", "Waves", "Overflow distance", SettingType::Int, 120.0, 0.0, 400.0, "", "px", "COMPOSESIREN_SIREN_WAVES", "global", "How far past the title cell a wave at full loudness goes when overflow is on." },
    { "waves.seam_blend", "Waves", "Seam blend", SettingType::Bool, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Join each wave to its neighbours' at the gaps between cells, so the column reads as one continuous edge." },
    { "waves.seam_zone", "Waves", "Seam zone", SettingType::Int, 16.0, 4.0, 40.0, "", "px", "COMPOSESIREN_SIREN_WAVES", "global", "Length of the blend near each gap." },
    { "waves.level_floor_db", "Waves", "Silence threshold", SettingType::Float, -66.0, -96.0, -24.0, "", "dB", "COMPOSESIREN_SIREN_WAVES", "global", "Output level shown as the low tide. Anything quieter is silence." },
    { "waves.level_full_db", "Waves", "Full level", SettingType::Float, -20.0, -48.0, 0.0, "", "dB", "COMPOSESIREN_SIREN_WAVES", "global", "Output level that takes the wave to its full reach." },
    { "waves.auto_level", "Waves", "Auto level", SettingType::Bool, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Each siren's wave reaches full at the loudest that siren played lately (at most 15 dB under the full level), so quieter sirens fill their cells too." },
    { "waves.attack_ms", "Waves", "Rise time", SettingType::Int, 80.0, 0.0, 500.0, "", "ms", "COMPOSESIREN_SIREN_WAVES", "global", "How fast the wave follows a louder level." },
    { "waves.release_ms", "Waves", "Fall time", SettingType::Int, 450.0, 0.0, 2000.0, "", "ms", "COMPOSESIREN_SIREN_WAVES", "global", "How fast the wave follows a quieter level." },
    { "waves.level_to_reach", "Waves", "Loudness to reach", SettingType::Float, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How much loudness pushes the wave into the cell. 0: the wave stays at its smallest size." },
    { "waves.pitch_to_reach", "Waves", "Pitch to reach", SettingType::Float, 0.5, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How much the highest notes push the wave further than the lowest, at the same loudness. 0: loudness alone." },
    { "waves.level_to_swell", "Waves", "Loudness to swell", SettingType::Float, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How much loudness raises the height of the ripples." },
    { "waves.level_to_depth", "Waves", "Loudness to depth", SettingType::Float, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How much loudness darkens the wave toward the cell edge." },
    { "waves.pitch_to_density", "Waves", "Pitch to density", SettingType::Float, 0.6, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How strongly pitch packs the ripples. 1: each octave doubles the number of crests, like an audio wave." },
    { "waves.pitch_to_speed", "Waves", "Pitch to speed", SettingType::Float, 0.7, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How much pitch speeds up the travelling ripples." },
    { "waves.travel_speed", "Waves", "Travel speed", SettingType::Float, 60.0, 0.0, 300.0, "", "px/s", "COMPOSESIREN_SIREN_WAVES", "global", "Base speed of the ripples travelling along the cell." },
    { "waves.flatness", "Waves", "Flatness", SettingType::Float, 0.3, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "1: one flat colour. 0: the full shading from the crest to the cell edge." },
    { "waves.parallax_layers", "Waves", "Parallax layers", SettingType::Int, 1.0, 0.0, 3.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Fainter waves behind the main one, moving at their own speed." },
    { "waves.parallax_depth", "Waves", "Parallax depth", SettingType::Float, 0.5, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "How far behind, and how much slower, each parallax layer is." },
    { "waves.opacity", "Waves", "Opacity", SettingType::Float, 0.9, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Opacity of the wave over the cell colour." },
    { "waves.stroke_width", "Waves", "Pencil line", SettingType::Float, 2.5, 0.0, 5.0, "", "px", "COMPOSESIREN_SIREN_WAVES", "global", "Bold black line along the wave's edge. 0: no line, a light crest instead." },
    { "waves.label_flip", "Waves", "Negative label", SettingType::Bool, 1.0, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "The siren's name turns negative where the wave covers it, and back again on the pencil line." },
    { "waves.clouds", "Waves", "Clouds", SettingType::Float, 0.5, 0.0, 1.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Light clouds over the title cells, strongest on the top rows, so the waves stand out from the blue." },
    { "waves.mask_steps", "Waves", "Edge steps", SettingType::Int, 16.0, 1.0, 16.0, "", "", "COMPOSESIREN_SIREN_WAVES", "global", "Number of shades in the antialiased wave edge (1: hard edge)." },
    { "clic.output_device", "Clic", "Audio output", SettingType::String, 0.0, 0.0, 0.0, "", "", "COMPOSESIREN_CLIC", "global", "Where the click plays. Main output: the Clic bus (the host routes it). Any other entry: that output device, so the click can go to a separate amp or headphones." },
    { "dsp.variant", "Synthesis", "DSP", SettingType::Choice, 0.0, 0.0, 9.0, "cpp|rust-exact|rust-time|frames|trio|trio-linear|trio-nearest|trio-fast|trio-linear-fast|trio-nearest-fast", "", "COMPOSESIREN_RUST_DSP", "global", "Which synthesis plays the sirens: the plugin's C++ DSP, its Rust port, or one of the Rust variants. App-wide, not a plugin parameter." },
    { "dsp.dataset", "Synthesis", "Data set", SettingType::Choice, 0.0, 0.0, 1.0, "fixed|raw", "", "COMPOSESIREN_RUST_DSP", "global", "Which siren data the Rust synthesis plays: with the crossfade fix (as shipped by default) or as measured. App-wide, not a plugin parameter." },
}};

} // namespace mecaviv::metadata
