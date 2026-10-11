// GENERATED from the shared metadata (sirens.csv and siren-categories.csv). Do not edit: change the metadata and regenerate.
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace mecaviv::metadata {

struct SirenCategory {
    std::string_view name;
    int noteMin;
    int noteMax;
    int noteMinClipped;
    int noteMaxClipped;
};

struct Siren {
    std::string_view id;
    std::string_view title;
    std::string_view category;
    int oneBasedMidiChannel;
    int pitchOrder;           // 0 = lowest siren
    std::uint32_t colourArgb; // 0xAARRGGBB
    std::string_view dataSet;
    std::string_view vectorIntervalSet;
};

inline constexpr std::array<SirenCategory, 5> sirenCategories {{
    { "Bass", 12, 65, 12, 64 },
    { "Tenor", 12, 67, 12, 65 },
    { "Alto", 12, 74, 12, 72 },
    { "Soprano", 24, 82, 24, 78 },
    { "Piccolo", 36, 82, 36, 78 },
}};

inline constexpr std::array<Siren, 7> sirens {{
    { "S1", "Alto 1", "Alto", 1, 2, 0xfffbeb4fu, "S1", "S1" },
    { "S2", "Alto 2", "Alto", 2, 3, 0xff64d940u, "S1", "S1" },
    { "S3", "Bass", "Bass", 3, 0, 0xffeb4125u, "S3", "S3" },
    { "S4", "Tenor", "Tenor", 4, 1, 0xfff4b83fu, "S4", "S4" },
    { "S5", "Soprano 1", "Soprano", 5, 4, 0xff367e21u, "S5", "S5" },
    { "S6", "Soprano 2", "Soprano", 6, 5, 0xff59aef9u, "S5", "S5" },
    { "S7", "Piccolo", "Piccolo", 7, 6, 0xff3b5df6u, "S7", "S5" },
}};

} // namespace mecaviv::metadata
