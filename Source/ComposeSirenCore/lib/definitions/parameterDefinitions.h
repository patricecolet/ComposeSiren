#pragma once
#include <string>
#include <iostream>
#include <set>
#include <map>
#include <vector>
#include <variant>
#include "stdint.h"
// #include <cstdint> // instead ?
#include "sirenProperties.h"

#if DEBUG
#endif
template<typename TBound> struct ParameterBounds {
  std::tuple<TBound,TBound> minMaxValue;
  TBound incValue;
  TBound defaultValue;
  TBound skewValue;
};

struct UIStateParam {
};

struct PitchBendParam {
};

struct CCParam {
  uint8_t midiCCNumber;
};

using PluginParam =
  std::variant< PitchBendParam
              , CCParam
              , UIStateParam
  >
;

// enum class PluginParamType{
//     PitchBend
//     , CC
//     , UIState
// };

enum class ParameterClass {
    Undefined
    , SirenControl
    , ReverbControl
    , TrackControl
    , MasterControl
    // , MatrixControl
};

enum class ParameterId {
    Undefined
    // Siren parameters ////////////////////////////////////
    , Transpose
    // , Tune
    , AllSoundOff
    , ResetAllController
    , AllNoteOff
    , Volume
    // , AutoVolume
    , PitchBend
    , PitchBendRange
    , VibratoFrequency
    , VibratoAmplitude
    , VibratoAcceleration
    // , EffectOrderSwitch
    , Portamento
    , TremoloFrequency
    , TremoloAmplitude
    // , EnvSpeed
    // , AttackReleaseLink
    , AttackDuration
    , ReleaseDuration
    , Timbre
    , Mute
    // Reverb parameters ///////////////////////////////////
    , ReverbEnable
    , ReverbDryWet
    , ReverbDamping
    , ReverbRoomSize
    , ReverbWidth
    , ReverbLowCut
    , ReverbHighCut
    // Track parameters ////////////////////////////////////
    , TrackPanning
    , TrackOutputGain
    // Mixer parameters ////////////////////////////////////
    , MasterVolume
    // Matrix parameters ///////////////////////////////////
    // , MatrixNoteIn
};

struct ParameterIdGet {
    static ParameterClass getClass(const ParameterId& parameter);
    static std::string codeName(const ParameterId& p);
    static ParameterId ofCodeName(const std::string& p);
    static bool tryOfCodeName(const std::string& p, ParameterId& fullId);
    static std::string toJuceParameterId(const std::string& groupId,
                                         const ParameterId& parameterId);
    // static bool tryOfJuceAudioProcessorParameterIdString(const std::string& audioProcessorParameterId,
    //                                                                       std::pair<ZeroBasedMidiChannel,
    //                                                                       ParameterId>& fullId);
    // static std::string toJuceParameterId(std::pair<ZeroBasedMidiChannel, ParameterId> fullId);
};

namespace sirenDataGet {
    static std::string name(const sirenData &siren) {
        switch (siren.oneBasedMidiChannel.oneBased) {
            case 1: return "Sirène 1 | Alto";
            case 2: return "Sirène 2 | Alto";
            case 3: return "Sirène 3 | Basse";
            case 4: return "Sirène 4 | Ténor";
            case 5: return "Sirène 5 | Soprano";
            case 6: return "Sirène 6 | Soprano";
            case 7: return "Sirène 7 | Piccolo";
            default:
                assert(false);
                return "unexpected siren getName";
        }
    }

    static std::string id(const sirenData &siren) {
        switch (siren.oneBasedMidiChannel.oneBased) {
            case 1: return "S1";
            case 2: return "S2";
            case 3: return "S3";
            case 4: return "S4";
            case 5: return "S5";
            case 6: return "S6";
            case 7: return "S7";
            default:
                assert(false);
                return "unexpected siren getId";
        }
    }

    static sirenData getForFullId(std::pair<ZeroBasedMidiChannel,ParameterId> fullId) {
        if (fullId.first.zeroBased < sirenProperties.size()) {
            return sirenProperties[fullId.first.zeroBased];
        }
        std::cout << "cpp:> todo shouldn't be called" << std::endl;
        assert(false);
    }
}


static const char* parameterGroupSeparator = " | ";

struct paramData {
  const ParameterId id = ParameterId::Undefined;
  const ParameterBounds<float> bounds;
  const PluginParam data;
};

/*
constexpr PluginParam CC(uint8_t cc) {
  return CCParam{cc};
}

constexpr PluginParam PB() {
  //return PB();
  return PitchBendParam{};
}

constexpr PluginParam UIState() {
  //return UIState();
  return UIStateParam{};
}
*/

constexpr float defaultSkew = 1.0;
constexpr paramData mkCCParam(const ParameterId id,
                              float minValue,
                              float maxValue,
                              float incValue,
                              float defValue,
                              uint8_t cc)
{
  ParameterBounds<float> bounds = {
    std::tuple(minValue, maxValue),
    incValue,
    defValue,
    defaultSkew
  };
  return paramData{id, bounds, CCParam{cc}};
}

constexpr paramData mkPBParam(const ParameterId id,
                              float minValue,
                              float maxValue,
                              float incValue,
                              float defValue)
{
  ParameterBounds<float> bounds = {
    std::tuple(minValue, maxValue),
    incValue,
    defValue,
    defaultSkew
  };
  return paramData{id, bounds, PitchBendParam()};
}
  
constexpr paramData mkUIStateParam(const ParameterId id,
                                   float minValue,
                                   float maxValue,
                                   float incValue,
                                   float defValue)
{
  ParameterBounds<float> bounds = {
    std::tuple(minValue, maxValue),
    incValue,
    defValue,
    defaultSkew
  };
  return paramData{id, bounds, UIStateParam()};
}
  
constexpr paramData mkUIStateParam(const ParameterId id,
                                   float minValue,
                                   float maxValue,
                                   float incValue,
                                   float defValue,
                                   float skewValue)
{
  ParameterBounds<float> bounds = {
    std::tuple(minValue, maxValue),
    incValue,
    defValue,
    skewValue
  };
  return paramData{id, bounds, UIStateParam()};
}

// todo: set it with Antoine.
// const auto ccMasterTune = 111;

inline const std::map<ParameterId, const char*> englishLabels =
  {
    {ParameterId::Transpose            , "Transpose"            },
    // {ParameterId::Tune                 , "Tune"                 },
    {ParameterId::AllSoundOff          , "All Sound Off"        },
    {ParameterId::ResetAllController   , "Reset All Controller" },
    {ParameterId::AllNoteOff           , "All Note Off"         },
    {ParameterId::Volume               , "Volume"               },
    // {ParameterId::AutoVolume           , "AutoVolume"           },
    {ParameterId::PitchBend            , "Pitch Bend"           },
    {ParameterId::PitchBendRange       , "Pitch Bend Range"     },
    {ParameterId::VibratoFrequency     , "Vibrato Frequency"    },
    {ParameterId::VibratoAmplitude     , "Vibrato Amplitude"    },
    {ParameterId::VibratoAcceleration  , "Vibrato Acceleration" },
    // {ParameterId::EffectOrderSwitch    , "Effect Order"         },
    {ParameterId::Portamento           , "Portamento"           },
    {ParameterId::TremoloFrequency     , "Tremolo Frequency"    },
    {ParameterId::TremoloAmplitude     , "Tremolo Amplitude"    },
    // {ParameterId::EnvSpeed             , "Envelope Speed"       },
    // {ParameterId::AttackReleaseLink    , "Attack/Release Link"  },
    {ParameterId::AttackDuration       , "Attack Duration"      },
    {ParameterId::ReleaseDuration      , "Release Duration"     },
    {ParameterId::Timbre               , "Timbre"               },
    {ParameterId::Mute                 , "Mute"                 },

    {ParameterId::ReverbEnable         , "Enable Reverb"        },
    {ParameterId::ReverbDryWet         , "Reverb Dry/Wet"       },
    {ParameterId::ReverbDamping        , "Reverb Damping"       },
    {ParameterId::ReverbRoomSize       , "Reverb Room Size"     },
    {ParameterId::ReverbWidth          , "Reverb Width"         },
    {ParameterId::ReverbLowCut         , "Reverb Low Cut"       },
    {ParameterId::ReverbHighCut        , "Reverb High Cut"      },

    {ParameterId::TrackPanning         , "Track Panning"        },
    {ParameterId::TrackOutputGain      , "Track Output Gain"    },
    {ParameterId::MasterVolume         , "Master Volume"        },

    // {ParameterId::MatrixNoteIn         , "Matrix Note In"       }
  };

inline const std::map<ParameterId, const char*> englishUnits =
  {
    {ParameterId::Transpose            , "semitones" },
    // {ParameterId::Tune                 , "Hz"        },
    {ParameterId::AllSoundOff          , ""          },
    {ParameterId::ResetAllController   , ""          },
    {ParameterId::AllNoteOff           , ""          },
    {ParameterId::Volume               , ""          },
    // {ParameterId::AutoVolume           , ""          },
    {ParameterId::PitchBend            , ""          },
    {ParameterId::PitchBendRange       , "semitones" },
    {ParameterId::VibratoFrequency     , ""          },
    {ParameterId::VibratoAmplitude     , ""          },
    {ParameterId::VibratoAcceleration  , ""          },
    // {ParameterId::EffectOrderSwitch    , ""          },
    {ParameterId::Portamento           , ""          },
    {ParameterId::TremoloFrequency     , ""          },
    {ParameterId::TremoloAmplitude     , ""          },
    // {ParameterId::EnvSpeed             , "ms"        },
    // {ParameterId::AttackReleaseLink    , ""          },
    {ParameterId::AttackDuration       , ""          },
    {ParameterId::ReleaseDuration      , ""          },
    {ParameterId::Timbre               , ""          },
    {ParameterId::Mute                 , ""          },

    {ParameterId::ReverbEnable         , ""          },
    {ParameterId::ReverbDryWet         , ""          },
    {ParameterId::ReverbDamping        , ""          },
    {ParameterId::ReverbRoomSize       , ""          },
    {ParameterId::ReverbWidth          , ""          },
    {ParameterId::ReverbLowCut         , "Hz"        },
    {ParameterId::ReverbHighCut        , "Hz"        },

    {ParameterId::TrackPanning         , ""          },
    {ParameterId::TrackOutputGain      , "dB"        },

    {ParameterId::MasterVolume         , "dB"        },

    // {ParameterId::MatrixNoteIn         , ""          }
  };

inline const std::vector<paramData> parameterDefinitions = {
  mkUIStateParam(ParameterId::Transpose          , -24.0f,  24.0f,  1.00f,   0.0f              ),
  // mkCCParam     (ParameterId::Tune               , 438.0f, 442.0f,  0.10f, 440.0f, ccMasterTune),
  mkCCParam     (ParameterId::AllSoundOff        ,   0.0f,   1.0f,  1.00f,   0.0f, 120         ),
  mkCCParam     (ParameterId::ResetAllController ,   0.0f,   1.0f,  1.00f,   0.0f, 121         ),
  mkCCParam     (ParameterId::AllNoteOff         ,   0.0f,   1.0f,  1.00f,   0.0f, 123         ),
  mkCCParam     (ParameterId::Volume             ,   0.0f, 127.0f,  1.00f, 127.0f,   7         ),
  // mkUIStateParam(ParameterId::AutoVolume         ,   0.0f,   1.0f,  1.00f,   0.0f              ),
  mkPBParam     (ParameterId::PitchBend          ,  -1.0f,   1.0f,  0.00f,   0.0f              ),
  mkCCParam     (ParameterId::PitchBendRange     ,   1.0f,  36.0f,  1.00f,   1.0f,  16         ),
  mkCCParam     (ParameterId::VibratoFrequency   ,   0.0f, 127.0f,  1.00f,   0.0f,   9         ),
  mkCCParam     (ParameterId::VibratoAmplitude   ,   0.0f, 127.0f,  1.00f,   0.0f,   1         ),
  mkCCParam     (ParameterId::VibratoAcceleration,   0.0f, 127.0f,  1.00f,   0.0f,  11         ),
  // mkCCParam     (ParameterId::EffectOrderSwitch  ,   0.0f,   1.0f,  1.00f,   0.0f,  42         ),
  mkCCParam     (ParameterId::Portamento         ,   0.0f, 127.0f,  1.00f,   0.0f,   5         ),
  mkCCParam     (ParameterId::TremoloFrequency   ,   0.0f, 127.0f,  1.00f,   0.0f,  15         ),
  mkCCParam     (ParameterId::TremoloAmplitude   ,   0.0f, 127.0f,  1.00f,   0.0f,  92         ),
  // mkUIStateParam(ParameterId::EnvSpeed           ,   0.0f,   5000,  0.01f,   0.0f, 0.33f       ),
  // mkUIStateParam(ParameterId::AttackReleaseLink  ,   0.0f,   1.0f,  1.00f,   0.0f              ),
  mkCCParam     (ParameterId::AttackDuration     ,   0.0f, 127.0f,  1.00f,   0.0f,   73        ),
  mkCCParam     (ParameterId::ReleaseDuration    ,   0.0f, 127.0f,  1.00f,   0.0f,   72        ),
  mkCCParam     (ParameterId::Timbre             ,   0.0f, 127.0f,  1.00f,   0.0f,   13        ),
  mkCCParam     (ParameterId::Mute               ,   0.0f, 127.0f,  1.00f,   0.0f,   12        ),

  mkCCParam     (ParameterId::ReverbEnable       ,   0.0f,   1.0f,  1.00f,   0.0f,   64        ),
  mkCCParam     (ParameterId::ReverbDryWet       ,   0.0f, 127.0f,  1.00f,   0.0f,   66        ),
  mkCCParam     (ParameterId::ReverbDamping      ,   0.0f, 127.0f,  1.00f,   0.0f,   67        ),
  mkCCParam     (ParameterId::ReverbRoomSize     ,   0.0f, 127.0f,  1.00f,   0.0f,   65        ),
  mkCCParam     (ParameterId::ReverbWidth        ,   0.0f, 127.0f,  1.00f,   0.0f,   70        ),
  mkCCParam     (ParameterId::ReverbLowCut       ,   0.0f, 127.0f,  1.00f,   0.0f,   68        ),
  mkCCParam     (ParameterId::ReverbHighCut      ,   0.0f, 127.0f,  1.00f,   0.0f,   69        ),

  mkCCParam     (ParameterId::TrackPanning       ,   -1.0f,  1.0f,  0.00f,   0.0f,   10        ),
  mkCCParam     (ParameterId::TrackOutputGain    ,   0.0f, 1.0f,    0.00f,   1.0f,   70        ),

  mkCCParam     (ParameterId::MasterVolume       ,   0.0f, 127.0f,  1.00f,   0.0f,   7         ),

  // mkCCParam     (ParameterId::MatrixNoteIn       ,   0.0f, 127.0f,  1.00f,   0.0f,   10        ),
};

// BUNCH OF UTILITIES FOR PARAMETER MANAGEMENT /////////////////////////////////

struct parameterIdAndCodeName {
    ParameterId id = ParameterId::Undefined;
    const std::string codeName;
};

inline const std::vector<parameterIdAndCodeName> parameterIdAndCodeNames = {
    { ParameterId::Transpose,               "Transpose" },
    { ParameterId::AllSoundOff,             "AllSoundOff" },
    { ParameterId::ResetAllController,      "ResetAllController" },
    { ParameterId::AllNoteOff,              "AllNoteOff" },
    { ParameterId::Volume,                  "Volume" },
    // {ParameterId::AutoVolume,               "AutoVolume" },
    { ParameterId::PitchBend,               "PitchBend" },
    { ParameterId::PitchBendRange,          "PitchBendRange" },
    { ParameterId::VibratoFrequency,        "VibratoFrequency" },
    { ParameterId::VibratoAmplitude,        "VibratoAmplitude" },
    { ParameterId::VibratoAcceleration,     "VibratoAcceleration" },
    // { ParameterId::EffectOrderSwitch,       "EffectOrderSwitch" },
    { ParameterId::Portamento,              "Portamento" },
    { ParameterId::TremoloFrequency,        "TremoloFrequency" },
    { ParameterId::TremoloAmplitude,        "TremoloAmplitude" },
    // { ParameterId::EnvSpeed,                "EnvSpeed" },
    // { ParameterId::AttackReleaseLink,       "AttackReleaseLink" },
    { ParameterId::AttackDuration,          "AttackDuration" },
    { ParameterId::ReleaseDuration,         "ReleaseDuration" },
    { ParameterId::Timbre,                  "Timbre" },
    { ParameterId::Mute,                    "Mute" },

    { ParameterId::ReverbEnable,            "ReverbEnable" },
    { ParameterId::ReverbDryWet,            "ReverbDryWet" },
    { ParameterId::ReverbDamping,           "ReverbDamping" },
    { ParameterId::ReverbRoomSize,          "ReverbRoomSize" },
    { ParameterId::ReverbWidth,             "ReverbWidth" },
    { ParameterId::ReverbLowCut,            "ReverbLowCut" },
    { ParameterId::ReverbHighCut,           "ReverbHighCut" },

    { ParameterId::TrackPanning,            "TrackPanning" },
    { ParameterId::TrackOutputGain,         "TrackOutputGain" },

    { ParameterId::MasterVolume,            "MasterVolume" }

    // { ParameterId::MatrixNoteIn,            "MatrixNoteIn" }
    // { ParameterId::Undefined,               "" } // Undefined must not be defined :-]
};

inline const std::map<ParameterId, std::string>
parameterCodeNameById = []() {
    std::map<ParameterId, std::string> result;
    for (const auto& [id, codeName] : parameterIdAndCodeNames) {
        result[id] = codeName;
    }
    return result;
}();

inline const std::map<std::string, ParameterId>
parameterIdByCodeName = []() {
    std::map<std::string, ParameterId> result;
    for (const auto& [id, codeName] : parameterIdAndCodeNames) {
        result[codeName] = id;
    }
    return result;
}();

inline const std::map<ParameterId, std::shared_ptr<paramData>>
parameterDefinitionById = []() {
    std::map<ParameterId, std::shared_ptr<paramData>> res;
    for (auto& d : parameterDefinitions) {
        res[d.id] = std::make_shared<paramData>(d);
    }
    return res;
}();

inline const std::map<int, std::shared_ptr<paramData>>
parameterDefinitionByCCNumber = []() {
    std::map<int, std::shared_ptr<paramData>> res;
    for (auto& d : parameterDefinitions) {
        if (const auto* cc = std::get_if<CCParam>(&d.data)) {
            res[cc->midiCCNumber] = std::make_shared<paramData>(d);
        }
    }
    return res;
}();

/*
static bool isButtonCC(ParameterId paramId) {
  return (paramId == ParameterId::AllSoundOff
          || paramId == ParameterId::ResetAllController
          || paramId == ParameterId::AllNoteOff);
}

static bool isButtonCC(int ccNumber) {
  if (parameterDefinitionsByCCNumber.contains(ccNumber)) {
    const auto paramId = parameterDefinitionsByCCNumber.at(ccNumber)->id;
    return isButtonCC(paramId);
  }
  return false;
}
//*/

/*
namespace RoutingMatrix {

    struct connectionKey {
        OneBasedMidiChannel outch;
        ParameterId id;
        OneBasedMidiChannel inch;
        bool operator<(const connectionKey& k) const {
            return outch < k.outch
                   || (outch == k.outch && inch < k.inch)
                   || (outch == k.outch && inch == k.inch && id < k.id);
        }
        static bool tryOf(const std::string& juceAudioParameterId, connectionKey& key);

        static std::string getValueTreeStateParameterNameForConnection(const connectionKey& key);
    };
}
//*/
