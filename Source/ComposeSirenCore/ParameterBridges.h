//
// Created by joseph larralde on 28/04/2026.
//

#ifndef COMPOSESIREN_PARAMETERBRIDGES_H
#define COMPOSESIREN_PARAMETERBRIDGES_H

#include <juce_audio_processors/juce_audio_processors.h>
#include "lib/definitions/parameterDefinitions.h"
#include "lib/wrappers/SirenEnsemble.h"
#include "Reverb.h"

class ParameterBridge : public juce::AudioProcessorParameter::Listener
{
    juce::RangedAudioParameter* param;
    std::atomic<float> value{0.0f};
    std::function<void(float)> callback;

public:
    ParameterBridge(const juce::AudioProcessorValueTreeState& vts,
                    const std::string& groupId,
                    const ParameterId paramId,
                    std::function<void(float)> cb) : callback(std::move(cb)) {
        param = vts.getParameter(
            ParameterIdGet::toJuceParameterId(groupId, paramId)
        );
        param->addListener(this);
    }

    ~ParameterBridge() override {
        param->removeListener(this);
    }

    void sendParameterValue() {
        value.store(param->getValue(), std::memory_order_relaxed);
        float v = param->convertFrom0to1(value.load(std::memory_order_relaxed));
        callback(v);
    }

    void parameterValueChanged(int, float newValue) override {
        value.store(newValue, std::memory_order_relaxed);
        float v = param->convertFrom0to1(value.load(std::memory_order_relaxed));
        callback(v);
    }

    void parameterGestureChanged(int, bool) override {}
};

//==============================================================================

class SirenTrackParameterBridges
{
    std::unique_ptr<ParameterBridge> trackPanning;
    std::unique_ptr<ParameterBridge> trackOutputGain;

public:
    SirenTrackParameterBridges(juce::AudioProcessorValueTreeState& vts,
                               sirenId sid,
                               SirenEnsemble* ensemble) {
        std::function<void(float)> setPanning
            = [sid, ensemble](float f) { ensemble->setPanning(sid, f); };
        std::function<void(float)> setOutputGain
            = [sid, ensemble](float f) { ensemble->setOutputGain(sid, f); };

        trackPanning = std::make_unique<ParameterBridge>(
            vts,
            sirenStrIdById.at(sid),
            ParameterId::TrackPanning,
            std::move(setPanning)
        );

        trackOutputGain = std::make_unique<ParameterBridge>(
            vts,
            sirenStrIdById.at(sid),
            ParameterId::TrackOutputGain,
            std::move(setOutputGain)
        );
    }

    void sendParameterValues() const {
        trackPanning->sendParameterValue();
        trackOutputGain->sendParameterValue();
    }
};

//==============================================================================

class SirenEnsembleParameterBridges
{
    std::vector<std::unique_ptr<SirenTrackParameterBridges>> sirenBridges;
    std::unique_ptr<ParameterBridge> masterVolume;

public:
    SirenEnsembleParameterBridges(juce::AudioProcessorValueTreeState& vts,
                                  SirenEnsemble* e) {
        for (const auto& sid : allSirenIds) {
            sirenBridges.push_back(
                std::make_unique<SirenTrackParameterBridges>(vts, sid, e)
            );
        }

        std::function<void(float)> setMasterVolume
            = [e](float f) { e->setMasterVolume(f); };
        masterVolume = std::make_unique<ParameterBridge>(
            vts, "M", ParameterId::MasterVolume, std::move(setMasterVolume)
        );
    }

    void sendParameterValues() const {
        for (const auto& bridge : sirenBridges) {
            bridge->sendParameterValues();
        }
        masterVolume->sendParameterValue();
    }
};

//==============================================================================

class ReverbParameterBridges
{
    std::unique_ptr<ParameterBridge> reverbEnable;
    std::unique_ptr<ParameterBridge> reverbDryWet;
    std::unique_ptr<ParameterBridge> reverbDamping;
    std::unique_ptr<ParameterBridge> reverbRoomSize;
    std::unique_ptr<ParameterBridge> reverbWidth;
    std::unique_ptr<ParameterBridge> reverbLowCut;
    std::unique_ptr<ParameterBridge> reverbHighCut;

public:
    ReverbParameterBridges(juce::AudioProcessorValueTreeState& vts,
                           Reverb* reverb)
    {
        std::function<void(float)> enable
            = [reverb](float e) { reverb->enable(e > 0.5f); };
        std::function<void(float)> setDryWet
            = [reverb](float d) { reverb->setDryWet(d); };
        std::function<void(float)> setDamping
            = [reverb](float d) { reverb->setDamping(d); };
        std::function<void(float)> setRoomSize
            = [reverb](float s) { reverb->setRoomSize(s); };
        std::function<void(float)> setWidth
            = [reverb](float w) { reverb->setWidth(w); };
        std::function<void(float)> setLowCut
            = [reverb](float l) { reverb->setLowCut(l); };
        std::function<void(float)> setHighCut
            = [reverb](float h) { reverb->setHighCut(h); };

        reverbEnable = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbEnable, std::move(enable)
        );

        reverbDryWet = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbDryWet, std::move(setDryWet)
        );

        reverbDamping = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbDamping, std::move(setDamping)
        );

        reverbRoomSize = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbRoomSize, std::move(setRoomSize)
        );

        reverbWidth = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbWidth, std::move(setWidth)
        );

        reverbLowCut = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbLowCut, std::move(setLowCut)
        );

        reverbHighCut = std::make_unique<ParameterBridge>(
            vts, "R", ParameterId::ReverbHighCut, std::move(setHighCut)
        );
    }

    void sendParameterValues() const {
        reverbEnable->sendParameterValue();
        reverbDryWet->sendParameterValue();
        reverbDamping->sendParameterValue();
        reverbRoomSize->sendParameterValue();
        reverbWidth->sendParameterValue();
        reverbLowCut->sendParameterValue();
        reverbHighCut->sendParameterValue();
    }
};

#endif //COMPOSESIREN_PARAMETERBRIDGES_H