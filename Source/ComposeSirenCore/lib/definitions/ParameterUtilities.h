#pragma once
#include "../utilities/cpp.stdlib.is.crippled.h"
#include <optional>
#include <set>
#include "sirenProperties.h"
#include "parameterDefinitions.h"

inline const std::vector<std::pair<ParameterId, const char*>> idsAndLesNomsEnFrancais = {
        // { ParameterId::MatrixNoteIn, "Notes" },
        { ParameterId::Volume, "Volume" },
        { ParameterId::PitchBend, "Pitch Bend" },
        { ParameterId::PitchBendRange, "Pitch Bend Range" },
        { ParameterId::Portamento, "Portamento" },
        { ParameterId::VibratoAmplitude, "Vibrato - Profondeur" },
        { ParameterId::VibratoFrequency, "Vibrato - Vitesse" },
        { ParameterId::VibratoAcceleration, "Vibrato - Acceleration" },
        { ParameterId::AttackDuration, "Attack" },
        { ParameterId::ReleaseDuration, "Release" },
        { ParameterId::TremoloAmplitude, "Tremolo - Profondeur" },
        { ParameterId::TremoloFrequency, "Tremolo - Vitesse" },
        { ParameterId::Timbre, "Timbre" },
        { ParameterId::Mute, "Sourdine" }
};

class ParameterMaps {
public:
  // using ParameterId = ParameterId;
  using CC = int;
  using FullParameterId = std::pair<ZeroBasedMidiChannel, ParameterId>;
  using ChannelCCPair = std::pair<OneBasedMidiChannel, CC>;
  // using JuceFullParameterId = std::string;
  using FullParameterIdString = std::string;
private:
  std::map<ChannelCCPair,   FullParameterId>    fullIdByChannelCCPair;
  std::map<FullParameterId, ChannelCCPair>      channelCCPairByFullId;
  std::map<FullParameterId, ParameterId>        idByFullId;
  std::set<OneBasedMidiChannel>                 channels;
  std::vector<OneBasedMidiChannel>              orderedChannels;
  // std::map<JuceFullParameterId,FullParameterId> fullParameterIdByJuceFullParameterIdString;
  std::map<FullParameterIdString,FullParameterId> fullParameterIdByFullParameterIdString;

  // template<typename K, typename V>
  // std::optional<V> getOptionalValueFromKey(const std::map<K, V>& m, K arg) const {
  //   if (m.contains(arg)) {
  //     return {m.at(arg)};
  //   }
  //   return std::nullopt;
  // }

  template<typename K, typename V>
  const V& getValueFromKey(const std::map<K, V>& m, K arg) const {
    if (m.contains(arg)) {
      return {m.at(arg)};
    }

    // std::cout << "can't retrieve key: " << arg << std::endl;
    assert(false); // non existent key in map is an error case
  }

public:
  /*
  static JuceFullParameterId mkJuceFullParameterId(std::pair<ZeroBasedMidiChannel, ParameterIds> fullId){
      auto p = sirenDataGet::getForFullId(fullId);
      auto paramData = *parameterDefinitionsById.at(fullId.second);
      return mkJuceFullParameterId(p, paramData);
  }
  static JuceFullParameterId mkJuceFullParameterId(const sirenData& p, const paramData& d){
      auto sirenPart(sirenDataGet::id(p));
      auto result = sirenPart + juce_parameterGroupSeparator + ParameterIdsGet::codeName(d.id);
      return result;
  }
  //*/

  static FullParameterIdString mkFullParameterIdString(std::pair<ZeroBasedMidiChannel, ParameterId> fullId){
      auto p = sirenDataGet::getForFullId(fullId);
      auto paramData = *parameterDefinitionById.at(fullId.second);
      return mkFullParameterIdString(p, paramData);
  }
  static FullParameterIdString mkFullParameterIdString(const sirenData& p, const paramData& d){
      auto sirenPart(sirenDataGet::id(p));
      auto result = sirenPart + parameterGroupSeparator + ParameterIdGet::codeName(d.id);
      return result;
  }


  ParameterMaps() {
    for (const auto& p : sirenProperties) {
      channels.insert(p.oneBasedMidiChannel);
      orderedChannels.push_back(p.oneBasedMidiChannel);
      for (const auto& d : parameterDefinitions) {
        // if (d.id == ParameterIds::MidiChannel) continue;
        auto jucefullId = mkFullParameterIdString(p,d);
        fullParameterIdByFullParameterIdString[jucefullId] = std::pair((ZeroBasedMidiChannel)p.oneBasedMidiChannel, d.id);
        auto fullId = std::pair((ZeroBasedMidiChannel)p.oneBasedMidiChannel, d.id);
        if (const auto* cc = std::get_if<CCParam>(&d.data)) {
          fullIdByChannelCCPair[{p.oneBasedMidiChannel, cc->midiCCNumber}] = fullId;
          channelCCPairByFullId[fullId] = {p.oneBasedMidiChannel, cc->midiCCNumber};
        }
        else if (const auto* pb = std::get_if<PitchBendParam>(&d.data)) {

        }
        idByFullId[fullId] = d.id;
      }
    }
  }

  bool isValidChannel(ZeroBasedMidiChannel ch) const {
    return channels.contains((OneBasedMidiChannel)ch);
  }

  bool isValidChannel(OneBasedMidiChannel ch) const {
    return channels.contains(ch);
  }

  std::set<OneBasedMidiChannel> getChannels() const {
    return channels;
  }

  std::vector<OneBasedMidiChannel> getOrderedChannels() const {
    return orderedChannels;
  }

  FullParameterId getFullParameterId(OneBasedMidiChannel ch, CC cc) const {
    return getValueFromKey<ChannelCCPair, FullParameterId>(fullIdByChannelCCPair, {ch, cc});
  }

  ChannelCCPair getChannelCCPair(FullParameterId id) const {
    return getValueFromKey<FullParameterId, ChannelCCPair>(channelCCPairByFullId, id);
  }
  ParameterId getParameterId(FullParameterId id) const {
      if (!idByFullId.contains(id)) {
;
      }
    return getValueFromKey<FullParameterId, ParameterId>(idByFullId, id);
  }

  bool tryGetFullParameterId(const OneBasedMidiChannel &ch, const ParameterId &id, FullParameterId &fullId) const {
    FullParameterId idToTry(ch,id);
    ParameterId parameterId;
    if(mapTryGet(idByFullId, idToTry, parameterId)) {
      fullId = idToTry;
      return true;
    }
    return false;
  }
};

//==============================================================================
// wrapping class for ParameterMaps

namespace ParameterUtilities {
  // C++17 minimum :
  inline const ParameterMaps maps;
}

/*
static std::unique_ptr<juce::XmlElement> filterXmlChildren(
        juce::XmlElement* state,
        std::function<bool(std::string)> shouldDiscardParameter,
        std::function<std::string(std::string)> translateId
);

static std::unique_ptr<juce::XmlElement> mergeXmlChildren(
        juce::XmlElement* target,
        const juce::XmlElement* newValues
);

static void printXmlElement(juce::XmlElement& e, int indent = 0);
*/
