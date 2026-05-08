#include "parameterDefinitions.h"
// #include "ParameterUtilities.h"
#include <iostream>
#include <sstream>

// HOW TO maintain a bunch of similar var names / strings
// see : https://stackoverflow.com/a/238157
// (is-there-a-simple-way-to-convert-c-enum-to-string)
// => a generally useful technique for managing large amounts of parameters, see also :
// https://forum.juce.com/t/how-to-access-audioprocessorvaluetreestate-parameters-en-masse/20455/6?u=joseph

// #define SOME_ENUM(DO) \
// DO(Undefined) \
// DO(Transpose) \
// DO(AllSoundOff) \
// DO(ResetAllController) \
// DO(AllNoteOff) \
// DO(Volume) \
// DO(PitchBend) \
// DO(PitchBendRange) \
// DO(VibratoFrequency) \
// DO(VibratoAmplitude) \
// DO(VibratoAcceleration) \
// DO(Portamento) \
// DO(TremoloFrequency) \
// DO(TremoloAmplitude) \
// DO(AttackDuration) \
// DO(ReleaseDuration) \
// DO(Timbre) \
// DO(Mute) \
// DO(ReverbEnable) \
// DO(ReverbDryWet) \
// DO(ReverbDamping) \
// DO(ReverbRoomSize) \
// DO(ReverbWidth) \
// DO(ReverbLowCut) \
// DO(ReverbHighCut) \
// DO(TrackPanning) \
// DO(TrackOutputGain) \
// DO(MasterVolume) \
// DO(MatrixNoteIn)
//
// #define MAKE_ENUM(VAR) VAR,
// enum class ParameterId {
//     SOME_ENUM(MAKE_ENUM)
// };
//
// #define MAKE_STRINGS(VAR) #VAR,
// const char* const parameterCodeNames[] = {
//     SOME_ENUM(MAKE_STRINGS)
// };

ParameterClass ParameterIdGet::getClass(const ParameterId& parameter) {
    switch (parameter) {
    case ParameterId::Transpose:
        // case ParameterId::Tune:
    case ParameterId::AllSoundOff:
    case ParameterId::ResetAllController:
    case ParameterId::AllNoteOff:
    case ParameterId::Volume:
        // case ParameterId::AutoVolume:
    case ParameterId::PitchBend:
    case ParameterId::PitchBendRange:
    case ParameterId::VibratoFrequency:
    case ParameterId::VibratoAmplitude:
    case ParameterId::VibratoAcceleration:
        // case ParameterId::EffectOrderSwitch:
    case ParameterId::Portamento:
    case ParameterId::TremoloFrequency:
    case ParameterId::TremoloAmplitude:
        // case ParameterId::EnvSpeed:
        // case ParameterId::AttackReleaseLink:
    case ParameterId::AttackDuration:
    case ParameterId::ReleaseDuration:
    case ParameterId::Timbre:
    case ParameterId::Mute:
        return ParameterClass::SirenControl;
    case ParameterId::ReverbEnable:
    case ParameterId::ReverbDryWet:
    case ParameterId::ReverbRoomSize:
    case ParameterId::ReverbWidth:
    case ParameterId::ReverbDamping:
    case ParameterId::ReverbLowCut:
    case ParameterId::ReverbHighCut:
        return ParameterClass::ReverbControl;
    case ParameterId::TrackPanning:
    case ParameterId::TrackOutputGain:
        return ParameterClass::TrackControl;
    case ParameterId::MasterVolume:
        return ParameterClass::MasterControl;
    // case ParameterId::MatrixNoteIn:
    //     return ParameterClass::MatrixControl;
    case ParameterId::Undefined:
    default:
        // return ParameterClass::Undefined;
        assert(false);
    }
}

std::string ParameterIdGet::codeName(const ParameterId& p) {
    auto it = parameterCodeNameById.find(p);
    if (it != parameterCodeNameById.end()) {
        return it->second;
    }
    assert(false);
}

ParameterId ParameterIdGet::ofCodeName(const std::string& p) {
    auto it = parameterIdByCodeName.find(p);
    if (it != parameterIdByCodeName.end()) {
        return it->second;
    }
    assert(false);
}

bool ParameterIdGet::tryOfCodeName(const std::string& p, ParameterId& param) {
    auto it = parameterIdByCodeName.find(p);
    if (it != parameterIdByCodeName.end()) {
        param = it->second;
        return true;
    }
    return false;
}

std::string ParameterIdGet::toJuceParameterId(const std::string& groupId,
                                              const ParameterId& parameterId)
{
    return groupId + parameterGroupSeparator + ParameterIdGet::codeName(parameterId);
}

/*
bool ParameterIdGet::tryOfJuceAudioProcessorParameterIdString(const std::string& input,
                                                              ParameterMaps::FullParameterId& fullId) {
    std::string parameter;
    std::size_t pipeIndex = input.find('|');
    if (pipeIndex != std::string::npos)
    {
        std::string numberString = input.substr(1, pipeIndex - 1);
        int number = std::stoi(numberString);

        parameter = input.substr(pipeIndex + 1);
        parameter.erase(parameter.begin(),
                        std::find_if(parameter.begin(),
                                     parameter.end(),
                                     [](unsigned char ch) {
                                        return !std::isspace(ch);
                                     }));

        // midi channel index is 0 based in the parameter maps
        auto midiChannel = number - 1;
        auto parameterId = ParameterIdGet::ofCodeName(parameter);
        fullId = ParameterMaps::FullParameterId({.zeroBased=midiChannel}, parameterId);
        return true;
    }
    return false;
}

std::string ParameterIdGet::toJuceParameterId(std::pair<ZeroBasedMidiChannel,
                                               ParameterId> fullId) {
    // auto result = ParameterMaps::mkJuceFullParameterId(fullId);
    auto result = ParameterMaps::mkFullParameterIdString(fullId);
    return result;
}
//*/






/*
std::string
RoutingMatrix::connectionKey::getValueTreeStateParameterNameForConnection(const RoutingMatrix::connectionKey& key){
    return "routing_" + std::to_string(key.inch.oneBased) + "_" + std::to_string(key.outch.oneBased) + "_" + ParameterIdsGet::codeName(key.id);
}

bool RoutingMatrix::connectionKey::tryOf(const std::string &juceAudioParameterId, RoutingMatrix::connectionKey &key) {


#if DEBUG
    std::cout << "cpp:> RoutingMatrix::connectionKey::tryOf '" << juceAudioParameterId << "'" << std::endl;
#endif
    std::stringstream ss(juceAudioParameterId);
    std::string token;
    int inch;//,outch;
    std::string paramId;
    std::getline(ss, token, '_'); // "routing"
    if (token == "routing") {
        std::getline(ss, token, '_'); // "1"
        inch = std::stoi(token);
        std::getline(ss, token, '_'); // "2"
        //outch = std::stoi(token);
        //std::getline(ss, token, '_'); // "foo"
        paramId = token;
        auto outch = inch;
        if(
                (inch >= 1  && inch <= 7)
                && (outch >= 1 && outch <= 7)
                ) {
            auto p = ParameterId::Undefined;
            if (ParameterIdsGet::tryOfCodeName(paramId, p)) {
                key = RoutingMatrix::connectionKey {.outch = outch, .id = p, .inch = inch};
                return true;
            }
        }
    }
#if DEBUG
    std::cout << "cpp:> RoutingMatrix::tryOf couldn't parse " << juceAudioParameterId << std::endl;
#endif
    return false;
}
//*/
