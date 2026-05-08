//
// Created by joseph larralde on 28/04/2026.
//

#ifndef COMPOSESIREN_REVERB_H
#define COMPOSESIREN_REVERB_H

#include <memory>
#include <juce_dsp/juce_dsp.h>

#include "lib/dsp/rampe.h"

class mareverbe;

class Reverb {
    std::unique_ptr<mareverbe> rvb;

    std::atomic<bool> enabled{true};
    bool en{true};

    std::atomic<float> dryWet{0.0f};
    rampe<float> rdw{0, 10};
    std::atomic<float> damping{0.0f};
    rampe<float> rd{0, 10};
    std::atomic<float> roomSize{0.0f};
    rampe<float> rrs{0, 10};
    std::atomic<float> width{0.0f};
    rampe<float> rw{0, 10};

    std::atomic<float> lowCut{0.0f};
    std::atomic<float> highCut{0.0f};

    float dry = 0.0f;
    float wet = 1.0f;
    double sampleRate = 44100.0;

    float highPassFreq = 1000.0f;
    juce::IIRFilter reverbHighpassL;
    juce::IIRFilter reverbHighpassR;

    float lowPassFreq = 10000.0f;
    juce::IIRFilter reverbLowpassL;
    juce::IIRFilter reverbLowpassR;

public:
    Reverb();
    ~Reverb();

    void setSampleRate(double sr);
    void enable(bool e);
    void setDryWet(float d);
    void setDamping(float d);
    void setRoomSize(float s);
    void setWidth(float w);
    void setLowCut(float l);
    void setHighCut(float h);

    void beginProcessBlock();
    void process(float *inL, float *inR, float *outL, float *outR, int n);

private:
    void updateHighPassCoefficients();
    void updateLowPassCoefficients();
};

#endif //COMPOSESIREN_REVERB_H