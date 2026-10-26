//
// Created by joseph larralde on 28/04/2026.
//

#include "Reverb.h"
#include "lib/dsp/mareverbe.h"
#if COMPOSESIREN_RUST_DSP
#include "lib/wrappers/DspChoice.h"
#include "reverb_dsp.h"
#endif

Reverb::Reverb() {
    rvb = std::make_unique<mareverbe>();
    rvb->setdry(0.0f);
    rvb->setwet(1.0f);
#if COMPOSESIREN_RUST_DSP
    rustRvb = csr_reverb_new();
    useRust.store(!cs::currentDspChoice().isCpp(), std::memory_order_relaxed);
#endif
}

#if COMPOSESIREN_RUST_DSP
Reverb::Reverb(bool rust) : Reverb() {
    setRust(rust);
}

void Reverb::setRust(bool rust) {
    useRust.store(rust && rustRvb != nullptr, std::memory_order_relaxed);
}

bool Reverb::isRust() const {
    return useRust.load(std::memory_order_relaxed) && rustRvb != nullptr;
}

void Reverb::reloadDsp() {
    setRust(!cs::currentDspChoice().isCpp());
}
#endif

// cannot use default destructor because of mareverbe forward declaration :
// unique_ptr needs to know sizeof mareverbe
Reverb::~Reverb() {
#if COMPOSESIREN_RUST_DSP
    csr_reverb_free(rustRvb);
#endif
}

void Reverb::setSampleRate(double sr) {
    sampleRate = sr;
    updateHighPassCoefficients();
    updateLowPassCoefficients();
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_sample_rate(rustRvb, sr);
#endif
}

void Reverb::enable(bool e) {
    enabled = e;
#if COMPOSESIREN_RUST_DSP
    csr_reverb_enable(rustRvb, e);
#endif
}

void Reverb::setDryWet(float d) {
    dryWet.store(d, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_dry_wet(rustRvb, d);
#endif
}

void Reverb::setDamping(float d) {
    damping.store(d, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_damping(rustRvb, d);
#endif
}

void Reverb::setRoomSize(float s) {
    roomSize.store(s, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_room_size(rustRvb, s);
#endif
}

void Reverb::setWidth(float w) {
    width.store(w, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_width(rustRvb, w);
#endif
}

void Reverb::setLowCut(float l) {
    lowCut.store(l, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_low_cut(rustRvb, l);
#endif
}

void Reverb::setHighCut(float h) {
    highCut.store(h, std::memory_order_relaxed);
#if COMPOSESIREN_RUST_DSP
    csr_reverb_set_high_cut(rustRvb, h);
#endif
}

void Reverb::beginProcessBlock()
{
#if COMPOSESIREN_RUST_DSP
    if (isRust()) {
        csr_reverb_begin_process_block(rustRvb);
        return;
    }
#endif
    en = enabled.load(std::memory_order_relaxed);
    rdw.setTarget(dryWet.load(std::memory_order_relaxed));
    rd.setTarget(damping.load(std::memory_order_relaxed));
    rrs.setTarget(roomSize.load(std::memory_order_relaxed));
    rw.setTarget(width.load(std::memory_order_relaxed));

    float v = lowCut.load(std::memory_order_relaxed);
    if (v != highPassFreq) {
        highPassFreq = v;
        updateHighPassCoefficients();
    }
    v = highCut.load(std::memory_order_relaxed);
    if (v != lowPassFreq) {
        lowPassFreq = v;
        updateLowPassCoefficients();
    }
}

void Reverb::process(float *inL, float *inR,
                     float *outL, float *outR,
                     int n) {
#if COMPOSESIREN_RUST_DSP
    if (isRust()) {
        if (n > 0) {
            csr_reverb_process(rustRvb, inL, inR, outL, outR, static_cast<size_t>(n));
        }
        return;
    }
#endif
    if (en) {
        float v;
        if (rdw.process(v)) {
            dry = 1.0f - v;
            wet = v;
        }
        if (rd.process(v)) { rvb->setdamp(v); }
        if (rrs.process(v)) { rvb->setroomsize(v); }
        if (rw.process(v)) { rvb->setwidth(v); }

        for (int i = 0; i < n; ++i) {
            // filter out low frequencies in dry signal before reverberating
            outL[i] = reverbHighpassL.processSingleSampleRaw(inL[i]);
            outR[i] = reverbHighpassL.processSingleSampleRaw(inR[i]);
            // reverberate (full wet signal out)
            rvb->process_stereo(outL+i, outR+i, outL+i, outR+i, 1);
            // filter out high frequencies in reverberated signal
            outL[i] = reverbLowpassL.processSingleSampleRaw(outL[i]);
            outR[i] = reverbLowpassR.processSingleSampleRaw(outR[i]);
            // mix with dry input
            outL[i] = inL[i] * dry + outL[i] * wet;
            outR[i] = inR[i] * dry + outR[i] * wet;
        }
    } else {
        // just forward input signal
        for (int i = 0; i < n; i++) {
            outL[i] = inL[i];
            outR[i] = inR[i];
        }
    }
}

void Reverb::updateHighPassCoefficients() {
    juce::IIRCoefficients coeffs
        = juce::IIRCoefficients::makeHighPass(sampleRate, highPassFreq);
    reverbHighpassL.setCoefficients(coeffs);
    reverbHighpassR.setCoefficients(coeffs);
}

void Reverb::updateLowPassCoefficients() {
    juce::IIRCoefficients coeffs
        = juce::IIRCoefficients::makeLowPass(sampleRate, lowPassFreq);
    reverbLowpassL.setCoefficients(coeffs);
    reverbLowpassR.setCoefficients(coeffs);
}
