//
// Created by joseph larralde on 03/03/2026.
//

#ifndef COMPOSESIREN_APVTS_UTILITIES_H
#define COMPOSESIREN_APVTS_UTILITIES_H

#include <juce_audio_processors/juce_audio_processors.h>
#include "lib/definitions/parameterDefinitions.h"
// #include "lib/definitions/ParameterUtilities.h"

struct parameterLayoutGroupData {
    std::string id;
    std::string name;
    std::vector<std::shared_ptr<const paramData>> params;
};

template<class... C>
parameterLayoutGroupData mkLayoutGroupData(const std::string& groupId,
                                           const std::string& groupName,
                                           const C... pc)
{
    // is the compiler detecting that C should be a ParameterClass from here ?
    std::array<ParameterClass, sizeof...(C)> allowedClasses = {pc...};

    return {
        .id = groupId,
        .name = groupName,
        .params = [&]() {
            std::vector<std::shared_ptr<const paramData>> res;
            for (const auto& p : parameterDefinitions) {
                // CLion sees an error in the embedded fold expression but
                // this is valid C++20 and should compile :
                // if ((ParameterIdsGet::getClass(p.id) == pc || ...)) {
                if (std::find(allowedClasses.begin(),
                              allowedClasses.end(),
                              ParameterIdGet::getClass(p.id))
                              != allowedClasses.end())
                {
                    res.push_back(std::make_shared<paramData>(p));
                }
            }
            return res;
        }()
    };
}

inline std::unique_ptr<juce::AudioParameterFloat>
createParameterFromDefinition(const paramData& d, const std::string& juceParameterId)
{
    return std::make_unique<juce::AudioParameterFloat>(
        // must specify version hint for AU (and standalone)
        // see https://forum.juce.com/t/how-to-set-a-parameter-version-hint
        juce::ParameterID { juceParameterId, 1 },
        englishLabels.at(d.id),
        juce::NormalisableRange<float>(
                std::get<0>(d.bounds.minMaxValue),
                std::get<1>(d.bounds.minMaxValue),
                d.bounds.incValue,
                d.bounds.skewValue
        ),
        d.bounds.defaultValue,
        englishUnits.at(d.id), // is Label supposed to be Unit ?
        juce::AudioProcessorParameter::genericParameter,
        d.bounds.incValue == 0.0f
            // always display 2 decimal places
            ? [](float v, int maxlen) { return juce::String(v, 2); }
            // we assume incValue is an Int
            : [](float v, int maxlen) { return juce::String(v, 0); },
        [](juce::String s) { return s.getFloatValue(); }
    );
}


inline juce::AudioProcessorValueTreeState::ParameterLayout
createParameterLayout(const std::vector<parameterLayoutGroupData>& groups)
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& g : groups) {
        auto group = std::make_unique<juce::AudioProcessorParameterGroup>(
                g.id, g.name, parameterGroupSeparator
        );
        for (const auto& d : g.params) {
            std::string fullParameterId = g.id + parameterGroupSeparator + ParameterIdGet::codeName(d->id);
            group->addChild(createParameterFromDefinition(*d, fullParameterId));
        }
        layout.add(std::move(group));
    }
    return layout;
}

#endif //COMPOSESIREN_APVTS_UTILITIES_H