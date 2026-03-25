/*
  ==============================================================================

    sirenProperties.h
    Created: 19 Nov 2021 9:31:39am
    Author:  Joseph Larralde

  ==============================================================================
*/

#pragma once

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <cassert>
#include "palette.h"

// to avoid the troubles of 'int' representing both 1 based midi channel and 0 based:

struct OneBasedMidiChannel;
struct ZeroBasedMidiChannel {
    int zeroBased;
    bool operator< (const ZeroBasedMidiChannel& other) const;
    bool operator== (const ZeroBasedMidiChannel& other) const;
    explicit operator OneBasedMidiChannel() const;
};
struct OneBasedMidiChannel {
    int oneBased;
    bool operator< (const OneBasedMidiChannel& other) const;
    bool operator== (const OneBasedMidiChannel& other) const;
    explicit operator ZeroBasedMidiChannel() const;
};

struct AnyMidiChannel {};
using AnyOrOneBasedMidiChannel = std::variant<OneBasedMidiChannel, AnyMidiChannel>;

// for (approximate) reference :
// sources :
// https://agi-son.org/nl/parution-du-decret-niveaux-sonores-abaisses-17
// https://www.hse.gov.uk/event-safety/noise.htm
// https://le-cdn.website-editor.net/bd9826fbd0e746469b23ba164e960d33/dms3rep/multi/opt/Echelle+de+bruit+2-1920w.png

// const float dbThresholds[] = {
//   0,    // perception threshold
//   10,   // recording studio
//   35,   // library
//   60,   // washing machine
//   80,   // school playground (max safe level)
//   90,   // sounds become nocive above this level
//   95,   // drill
//   102,  // chain saw / max average db level authorized in events in fr
//   107,  // max average db level authorized in events in uk
//   110,  // jackhammer
//   115,  // must NOT go above ! irreversible damage to the ears !
//   120,  // gun shot
//   130,  // plane taking off
//   138,  // explosion
//   140,  // max peak db level authorized in events in uk
// };

// or for a more precise reference taking time of exposure into account,
// according to the WHO (see table at the end of the document) :
// https://www.who.int/docstore/peh/noise/Comnoise-4.pdf
// we could take :
// - 85 (highest value before starting damage after 1 hour, everyday)
// - 100 (highest value before starting damage after 4 hours, on a 5 times/year basis)

// let's follow WHO recommmendations :

inline const float dbThresholds[] = {
    85,   // approx. safe below
    100,  // approx. nocive above
};

enum sirenCategory {
    Alto,
    Bass,
    Tenor,
    Soprano,
    Piccolo
};

enum sirenId {
    S1,
    S2,
    S3,
    S4,
    S5,
    S6,
    S7
};

const int MAX_FILE_NAME_LENGTH = 32;

struct sirenCategorySynthConstants {
    int noteMin;
    int noteMidiCentMax;
    int pourcentClapetOff;
    int coeffPicolo;
    float inertiaFactorTweak;
    float engineSpeedFrequencyRatio;
};

struct sirenResourceFileNames {
    // resource file names
    std::string amp;
    std::string freq;
    std::string dureTabs;
    std::string vectorInterval;
};

struct sirenCategoryData {
    // int id;
    std::string name;
    uint32_t colour;
    std::tuple<int, int> minMaxNote = std::tuple(0, 0);
    // we get dbPerNote values from measures of the corresponding notes at maximum
    // velocity (127), all shutters open.
    // found in "MESURE ACOUSTIQUE SIRENES.xlsx" file
    std::vector<double> dbPerNote = { 0 }; // must be of length (maxNote - minNote) + 1
    sirenCategorySynthConstants synthConstants = { 0 };
    // sirenCategoryResourceFileNames resourceFileNames = { "" };
};

inline const std::map<sirenCategory, sirenCategoryData> sirenCategoriesData = {
    {
        Bass,
        {
            "Bass",
            mecaviv::Colours::SirenPalette::darkBlue,
            std::tuple(12, 65), // C0, F4
            {
                70.90,72.40,72.90,74.50,75.80,77.20,78.00,80.20,80.50,82.50,82.70,84.70,
                86.80,86.90,88.20,90.30,91.00,91.40,93.50,96.60,96.20,96.00,96.80,100.20,
                100.90,100.50,101.20,102.90,105.50,106.40,108.30,111.00,109.20,109.00,110.00,113.00,
                113.40,112.10,114.00,115.80,115.20,118.00,119.20,122.30,119.40,117.80,119.60,124.10,
                125.10,122.70,122.90,123.40,125.30,127.80
            },
            { 24, 6400, 7, 1, 12, 7.5 }
        }
    },
    {
        Tenor,
        {
            "Tenor",
            mecaviv::Colours::SirenPalette::lightBlue,
            std::tuple(12, 67), // C0, G4
            {
                77.50,79.10,80.00,81.50,82.40,83.30,85.40,85.90,87.90,88.80,89.00,91.50,
                91.20,92.70,94.50,94.90,95.20,97.00,99.00,98.00,99.80,101.30,100.10,103.70,
                103.40,104.90,105.90,104.20,105.40,108.10,111.20,111.30,113.90,114.30,111.40,110.90,
                112.20,112.50,114.50,116.30,117.80,120.80,123.20,123.00,125.40,124.90,121.30,120.90,
                122.40,123.70,124.20,126.40,126.50,128.10,129.80,128.00
            },
            { 24, 6500, 15, 1, 12, 20./3. }
        }
    },
    {
        Alto,
        {
            "Alto",
            mecaviv::Colours::SirenPalette::lightGreen,
            // here we only use measures from S1, as we don't have any measures from S2
            // we consider S1 and S2 are identical
            // (unlike S5 and S6, apparently. see below)
            std::tuple(12, 74), // C0, D5
            {
                64.50,65.40,65.90,67.10,67.70,69.00,70.00,70.80,72.50,73.10,74.70,76.00,
                77.10,77.80,78.00,78.80,80.60,82.60,83.40,85.20,86.40,86.20,87.00,88.20,
                89.10,92.00,93.00,94.40,93.80,96.60,97.90,101.80,102.50,102.30,102.90,101.30,
                102.50,105.20,108.00,108.40,108.90,111.00,108.90,111.00,114.40,114.40,116.30,116.50,
                119.40,119.20,118.40,119.20,117.60,121.20,119.70,124.50,127.00,127.30,128.80,129.40,
                130.80,129.70,130.10
            },
            { 24, 7200, 7, 1, 24, 5 }
        }
    },
    {
        Soprano,
        {
            "Soprano",
            mecaviv::Colours::SirenPalette::sunnyYellow,
            // looks like both soprano sirens (S5 and S6) don't have the exact same
            // ranges and levels. Are they different versions ?
            // if so we should probably create Soprano1 and Soprano2 categories ...
            // or move some info from categoryData to sirenData
            // for now we will use data measured from S5
            std::tuple(24, 82), // C1, A#5
            {
                53.80,54.70,55.00,55.40,57.70,59.90,60.00,61.80,62.90,65.10,66.70,66.50,
                67.70,68.00,69.70,71.80,75.20,77.00,77.10,78.80,79.00,79.70,80.40,81.20,
                84.30,84.20,85.80,88.10,92.20,92.80,90.90,93.20,93.20,94.40,94.70,96.40,
                98.80,97.40,98.80,100.70,104.40,104.40,98.60,106.60,107.50,109.60,110.10,112.10,
                114.00,110.20,112.80,115.10,118.30,118.20,114.70,117.30,121.90,118.00,119.60
            },
            { 36, 7900, 7, 1, 48, 7.5 }
        }
    },
    {
        Piccolo,
        {
            "Piccolo",
            mecaviv::Colours::SirenPalette::darkOrange,
            std::tuple(36, 82), // C2, A#5
            {
                66.70,68.40,69.60,72.60,75.70,78.90,80.00,77.10,72.90,77.60,82.10,83.60,
                83.30,80.90,78.80,76.00,82.30,86.80,90.90,90.70,89.00,92.90,97.10,98.60,
                98.60,96.50,92.30,94.80,97.60,102.30,106.50,106.00,106.40,104.40,100.00,95.60,
                103.90,103.70,104.50,112.20,115.00,113.00,114.20,112.20,116.80,114.10,112.00
            },
            { 36, 7900, 7, 2, 24, 7.5 }
        }
    }
};

/**
 makeForCategory iterates over the dbPerNote vector of a sirenCategoryData struct and looks for the
 contiguous intervals that contain only notes below each threshold defined in the dbThresholds vector, or
 above the last threshold.

 dbThresholds could be of any length > 0, the number of velocity ranges will always be its length + 1
 This velocity range index is used as a key in an unordered_map where the bounds of these intervals are
 stored (after being converted into their corresponding MIDI notes) for use in the
 CMSMidiKeyboardComponent.

 Rationale : as soon as a note is found to be above a certain threshold, this note and all the following ones
 are considered to be in the corresponding velocity range (even if they go back below the threshold
 afterwards) until a new note is found to be above the next threshold.
 If a certain velocity range doesn't match any interval in the dbPerNote vector, it will simply not be added to
 the returned unordered_map
 */
struct VelocityRanges {
    static std::map<int, std::tuple<int, int>> makeForCategory(const sirenCategory& c);
};

struct sirenData {
    sirenId id = S1;
    sirenCategory category = Alto;
    sirenResourceFileNames resourceFileNames = {
        "dataAmpS1",
        "dataFreqS1",
        "datadureTabsS1",
        "dataVectorIntervalS1"
    };
    sirenCategorySynthConstants synthConstants = sirenCategoriesData.at(category).synthConstants;
    std::map<int, std::tuple<int, int>> velocityRanges = VelocityRanges::makeForCategory(category);
    OneBasedMidiChannel oneBasedMidiChannel = {.oneBased=1};
};

inline const std::vector<sirenData> sirenProperties = {
    {
        .id = S1
      , .category = Alto
      , .resourceFileNames = {
          "dataAmpS1",
          "dataFreqS1",
          "datadureTabsS1",
          "dataVectorIntervalS1"
        }
      , .oneBasedMidiChannel = {.oneBased=1}
    },
    {
        .id = S2
      , .category = Alto
      , .resourceFileNames = {
          "dataAmpS1",
          "dataFreqS1",
          "datadureTabsS1",
          // there is a dataVectorIntervalS2 file but we use the S1 version
          // "dataVectorIntervalS2"
          "dataVectorIntervalS1"
        }
      , .oneBasedMidiChannel = {.oneBased=2}
    },
    {
        .id = S3
      , .category = Bass
      , .resourceFileNames = {
          "dataAmpS3",
          "dataFreqS3",
          "datadureTabsS3",
          "dataVectorIntervalS3"
        }
      , .oneBasedMidiChannel = {.oneBased=3}
    },
    {
        .id = S4
      , .category = Tenor
      , .resourceFileNames = {
          "dataAmpS4",
          "dataFreqS4",
          "datadureTabsS4",
          "dataVectorIntervalS4"
        }
      , .oneBasedMidiChannel = {.oneBased=4}
    },
    {
        .id = S5
      , .category = Soprano
      , .resourceFileNames = {
          "dataAmpS5",
          "dataFreqS5",
          "datadureTabsS5",
          "dataVectorIntervalS5"
        }
      , .oneBasedMidiChannel = {.oneBased=5}
    },
    {
        .id = S6
      , .category = Soprano
      , .resourceFileNames = {
          "dataAmpS5",
          "dataFreqS5",
          "datadureTabsS5",
          "dataVectorIntervalS5"
        }
      , .oneBasedMidiChannel = {.oneBased=6}
    },
    {
        .id = S7
      , .category = Piccolo
      , .resourceFileNames = {
          "dataAmpS7",
          "dataFreqS7",
          "datadureTabsS7",
          // S7 uses vectorInterval data from S5
          "dataVectorIntervalS5"
        }
      , .oneBasedMidiChannel = {.oneBased=7}
    },
  };

inline const std::map<sirenId, std::shared_ptr<sirenData>>
sirenPropertiesById = []() {
    std::map<sirenId, std::shared_ptr<sirenData>> res;
    for (auto& p : sirenProperties) {
        res[p.id] = std::make_shared<sirenData>(p);
    }
    return res;
}();

inline const std::map<sirenCategory, std::shared_ptr<sirenData>>
sirenPropertiesByCategory = []() {
    std::map<sirenCategory, std::shared_ptr<sirenData>> res;
    for (auto& p : sirenProperties) {
        res[p.category] = std::make_shared<sirenData>(p);
    }
    return res;
}();

inline const std::map<OneBasedMidiChannel, std::shared_ptr<sirenData>>
sirenPropertiesByChannel = []() {
    std::map<OneBasedMidiChannel, std::shared_ptr<sirenData>> res;
    for (auto& p : sirenProperties) {
        res[p.oneBasedMidiChannel] = std::make_shared<sirenData>(p);
    }
    return res;
}();

inline const std::map<sirenCategory, sirenId>
defaultSirenIdByCategory = {
    { Alto, S1 },
    { Bass, S3 },
    { Tenor, S4 },
    { Soprano, S5 },
    { Piccolo, S7 },
};
