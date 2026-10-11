// GENERATED from the shared metadata (composesiren-parameters.csv, composesiren-ui-sections.csv, composesiren-ui-sirens.csv, composesiren-ui-theme.csv, composesiren-siren-waves.csv and sirens.csv). Do not edit: change the metadata and regenerate.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace mecaviv::metadata::ui {

enum class ParameterClass { Siren, Reverb, Track, Master, Clic };
enum class Midi { None, Cc, PitchWheel };
enum class Widget { None, Knob, CentredKnob, Spin, Toggle, Button };

struct Parameter {
    std::string_view codeName;   // the JUCE parameter id is "<group> | <codeName>"
    ParameterClass parameterClass;
    std::string_view label;      // what hosts show
    std::string_view stripLabel; // the caption on the strip, empty when on no section
    std::string_view unit;
    float minValue;
    float maxValue;
    float step;                  // 0: continuous
    float defaultValue;
    Midi midi;
    int cc;                      // -1 unless midi is Midi::Cc
    Widget widget;
    int section;                 // index in sections, -1: on no section
    int position;                // place in the section, -1: on no section
    std::string_view requiredOption; // CMake option it needs, empty: always built
};

struct Section {
    std::string_view id;
    ParameterClass parameterClass;
    std::string_view title; // may be empty
};

struct SirenUi {
    std::string_view id;
    std::uint32_t stripColourArgb; // 0xAARRGGBB
    bool categoryDefault;          // the siren the "Siren type" menu shows for its category
};

// A look of the SirenWaves shader (SirenWave.frag): the uniform components that stay put
// while the sound changes. sirenWaves[defaultSirenWave[i]] is the default look of sirens[i].
struct SirenWave {
    std::string_view label;
    std::string_view siren;
    std::uint32_t accentArgb;     // uAccent: override_color, or the siren's colour
    bool accentOverridden;
    int seamZone; // uFills.w, px
    float parallax; // uRipple.w
    float flatness; // uLook.z
    float opacity; // uLook.w
    int layers; // uLayers.x
    int steps; // uLayers.y
    float strokeWidth; // uExtra.x, px
    float clouds; // uExtra.y
};

inline constexpr std::array<Parameter, 32> parameters {{
    { "Transpose", ParameterClass::Siren, "Transpose", "Transpose", "semitones", -24.0f, 24.0f, 1.0f, 0.0f, Midi::None, -1, Widget::Spin, 0, 2, "" },
    { "AllSoundOff", ParameterClass::Siren, "All Sound Off", "", "", 0.0f, 1.0f, 1.0f, 0.0f, Midi::Cc, 120, Widget::Button, -1, -1, "" },
    { "ResetAllController", ParameterClass::Siren, "Reset All Controller", "", "", 0.0f, 1.0f, 1.0f, 0.0f, Midi::Cc, 121, Widget::Button, -1, -1, "" },
    { "AllNoteOff", ParameterClass::Siren, "All Note Off", "", "", 0.0f, 1.0f, 1.0f, 0.0f, Midi::Cc, 123, Widget::Button, -1, -1, "" },
    { "Volume", ParameterClass::Siren, "Volume", "Volume", "", 0.0f, 127.0f, 1.0f, 127.0f, Midi::Cc, 7, Widget::Knob, 4, 2, "" },
    { "PitchBend", ParameterClass::Siren, "Pitch Bend", "Bend", "", -1.0f, 1.0f, 0.0f, 0.0f, Midi::PitchWheel, -1, Widget::CentredKnob, 0, 0, "" },
    { "PitchBendRange", ParameterClass::Siren, "Pitch Bend Range", "Bend Range", "semitones", 1.0f, 36.0f, 1.0f, 1.0f, Midi::Cc, 16, Widget::Spin, 0, 1, "" },
    { "VibratoFrequency", ParameterClass::Siren, "Vibrato Frequency", "Speed", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 9, Widget::Knob, 1, 0, "" },
    { "VibratoAmplitude", ParameterClass::Siren, "Vibrato Amplitude", "Depth", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 1, Widget::Knob, 1, 1, "" },
    { "VibratoAcceleration", ParameterClass::Siren, "Vibrato Acceleration", "Evolve", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 11, Widget::Knob, 1, 2, "" },
    { "Portamento", ParameterClass::Siren, "Portamento", "Porta", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 5, Widget::Knob, 0, 3, "" },
    { "TremoloFrequency", ParameterClass::Siren, "Tremolo Frequency", "Speed", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 15, Widget::Knob, 2, 0, "" },
    { "TremoloAmplitude", ParameterClass::Siren, "Tremolo Amplitude", "Depth", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 92, Widget::Knob, 2, 1, "" },
    { "AttackDuration", ParameterClass::Siren, "Attack Duration", "Attack", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 73, Widget::Knob, 3, 0, "" },
    { "ReleaseDuration", ParameterClass::Siren, "Release Duration", "Release", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 72, Widget::Knob, 3, 1, "" },
    { "Timbre", ParameterClass::Siren, "Timbre", "Timbre", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 13, Widget::Knob, 4, 0, "" },
    { "Mute", ParameterClass::Siren, "Mute", "Mute", "", 0.0f, 127.0f, 1.0f, 0.0f, Midi::Cc, 12, Widget::Knob, 4, 1, "" },
    { "ReverbEnable", ParameterClass::Reverb, "Enable Reverb", "Enable Reverb", "", 0.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 64, Widget::Toggle, 5, 0, "" },
    { "ReverbDryWet", ParameterClass::Reverb, "Reverb Dry/Wet", "DryWet", "", 0.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 66, Widget::Knob, 6, 0, "" },
    { "ReverbDamping", ParameterClass::Reverb, "Reverb Damping", "Damp", "", 0.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 67, Widget::Knob, 6, 1, "" },
    { "ReverbRoomSize", ParameterClass::Reverb, "Reverb Room Size", "Size", "", 0.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 65, Widget::Knob, 6, 2, "" },
    { "ReverbWidth", ParameterClass::Reverb, "Reverb Width", "Width", "", 0.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 70, Widget::Knob, 6, 3, "" },
    { "ReverbLowCut", ParameterClass::Reverb, "Reverb Low Cut", "LowCut", "Hz", 20.0f, 1000.0f, 0.0f, 20.0f, Midi::Cc, 68, Widget::Knob, 7, 0, "" },
    { "ReverbHighCut", ParameterClass::Reverb, "Reverb High Cut", "HighCut", "Hz", 5000.0f, 20000.0f, 0.0f, 20000.0f, Midi::Cc, 69, Widget::Knob, 7, 1, "" },
    { "TrackPanning", ParameterClass::Track, "Track Panning", "", "", -1.0f, 1.0f, 0.0f, 0.0f, Midi::Cc, 10, Widget::CentredKnob, -1, -1, "" },
    { "TrackOutputGain", ParameterClass::Track, "Track Output Gain", "", "dB", 0.0f, 1.0f, 0.0f, 1.0f, Midi::Cc, 70, Widget::Knob, -1, -1, "" },
    { "MasterVolume", ParameterClass::Master, "Master Volume", "", "dB", 0.0f, 1.0f, 0.0f, 1.0f, Midi::Cc, 7, Widget::Knob, -1, -1, "" },
    { "ClicEnable", ParameterClass::Clic, "Enable Clic", "", "", 0.0f, 1.0f, 1.0f, 1.0f, Midi::None, -1, Widget::Toggle, -1, -1, "COMPOSESIREN_CLIC" },
    { "ClicVolume", ParameterClass::Clic, "Clic Volume", "", "", 0.0f, 1.0f, 0.0f, 1.0f, Midi::None, -1, Widget::Knob, -1, -1, "COMPOSESIREN_CLIC" },
    { "ClicSpread", ParameterClass::Clic, "Clic Spread", "", "", -1.0f, 1.0f, 0.0f, 0.0f, Midi::None, -1, Widget::CentredKnob, -1, -1, "COMPOSESIREN_CLIC" },
    { "ClicBias", ParameterClass::Clic, "Clic Bias", "", "", -1.0f, 1.0f, 0.0f, 0.0f, Midi::None, -1, Widget::CentredKnob, -1, -1, "COMPOSESIREN_CLIC" },
    { "ClicDecay", ParameterClass::Clic, "Clic Decay", "", "", 0.0f, 1.0f, 0.0f, 1.0f, Midi::None, -1, Widget::Knob, -1, -1, "COMPOSESIREN_CLIC" },
}};

inline constexpr std::array<Section, 8> sections {{
    { "pitch", ParameterClass::Siren, "Pitch" },
    { "vibrato", ParameterClass::Siren, "Vibrato" },
    { "tremolo", ParameterClass::Siren, "Tremolo" },
    { "envelope", ParameterClass::Siren, "Envelope" },
    { "output", ParameterClass::Siren, "" },
    { "reverb_enable", ParameterClass::Reverb, "Enable" },
    { "reverb", ParameterClass::Reverb, "Reverb" },
    { "reverb_filter", ParameterClass::Reverb, "Filter" },
}};

inline constexpr std::array<SirenUi, 7> sirens {{
    { "S1", 0xff295dd1u, true },
    { "S2", 0xff1f6cd1u, false },
    { "S3", 0xff4650c8u, true },
    { "S4", 0xff3654ceu, true },
    { "S5", 0xff177ccfu, true },
    { "S6", 0xff0e8eceu, false },
    { "S7", 0xff07a0cbu, true },
}};

inline constexpr std::array<SirenWave, 7> sirenWaves {{
    { "s1-default", "S1", 0xfffbeb4fu, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s2-default", "S2", 0xff64d940u, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s3-default", "S3", 0xffeb4125u, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s4-default", "S4", 0xfff4b83fu, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s5-default", "S5", 0xff367e21u, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s6-default", "S6", 0xff59aef9u, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
    { "s7-default", "S7", 0xff3b5df6u, false, 16, 0.5f, 0.3f, 0.9f, 1, 16, 2.5f, 0.5f },
}};

// In siren order (S1 first): the index in sirenWaves of the siren's default look.
inline constexpr std::array<std::size_t, 7> defaultSirenWave { 0, 1, 2, 3, 4, 5, 6 };

namespace theme {
inline constexpr std::uint32_t colourOrangeMecanique = 0xffff9900u; // Accent: knob arcs, the held keyboard key, selections.
inline constexpr std::uint32_t colourDarkTransparentBackground = 0xf2283541u; // Background of the overlay panels.
inline constexpr std::uint32_t colourBackgroundStripGrey = 0xff314159u; // Background of a siren strip.
inline constexpr std::uint32_t colourSirenRampDarkBlue = 0xff4650c8u; // First colour of the strip ramp (the lowest siren).
inline constexpr std::uint32_t colourSirenRampLightBlue = 0xff00b4c8u; // Last colour of the strip ramp.
inline constexpr std::uint32_t colourSirenRampDarkGreen = 0xff3c8c28u; // Siren palette.
inline constexpr std::uint32_t colourSirenRampLightGreen = 0xff78b428u; // Siren palette.
inline constexpr std::uint32_t colourSirenRampSunnyYellow = 0xffd7b700u; // Siren palette.
inline constexpr std::uint32_t colourSirenRampLightOrange = 0xffff7f00u; // Siren palette.
inline constexpr std::uint32_t colourSirenRampDarkOrange = 0xffff4500u; // Siren palette.
inline constexpr std::uint32_t colourMidiKeyboardLowLevelRed = 0xff800000u; // Keyboard level meter: low.
inline constexpr std::uint32_t colourMidiKeyboardMidLevelRed = 0xffff0000u; // Keyboard level meter: middle.
inline constexpr std::uint32_t colourMidiKeyboardHighLevelRed = 0xffff6600u; // Keyboard level meter: high.
inline constexpr std::uint32_t colourMidiKeyboardDbRangeSeparatorBlue = 0xff0000ffu; // Keyboard level meter: the dB range separators.
inline constexpr float stripTitleAreaWidth = 70.0f; // Width of the siren title cell at the left of a strip.
inline constexpr float stripTitleFontSize = 13.0f; // Font size of the siren title.
inline constexpr float stripCornerSize = 10.0f; // Corner radius of a strip.
inline constexpr float stripSpacerSize = 2.0f; // Gap between the cells of a strip.
inline constexpr float stripGroupLabelHeight = 16.0f; // Height of a section title.
inline constexpr float stripGroupLabelFontSize = 12.0f; // Font size of a section title.
inline constexpr float stripSliderLabelHeight = 28.0f; // Height of a knob caption (two lines: the label, then the CC number).
inline constexpr float stripSliderLabelFontSize = 11.5f; // Font size of a knob caption.
inline constexpr float stripMinSliderHeight = 62.0f; // Smallest height of a knob.
inline constexpr float stripMinFullStripHeight = 100.0f; // Smallest height of a whole strip.
inline constexpr float stripMinKnobSliderWidth = 47.0f; // Smallest width of a knob cell.
inline constexpr float stripMinIncDecSliderWidth = 70.0f; // Smallest width of an up/down value cell.
inline constexpr float stripKnobIndicatorOffThickness = 2.0f; // Width of the knob track.
inline constexpr float stripKnobIndicatorOnThickness = 4.0f; // Width of the knob value arc.
inline constexpr float editorOneSirenWidth = 754.0f; // Initial width of the OneSiren editor.
inline constexpr float editorOneSirenHeight = 200.0f; // Initial height of the OneSiren editor.
} // namespace theme

} // namespace mecaviv::metadata::ui
