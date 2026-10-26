// Minimal shim of juce::IIRCoefficients and juce::IIRFilter (verbatim from
// Dependencies/JUCE/modules/juce_audio_basics/utilities/juce_IIRFilter.h/.cpp) so
// Source/ComposeSirenCore/Reverb.cpp can be compiled in siren-dsp-oracle without JUCE.
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

namespace juce {

template <typename FloatType>
struct MathConstants {
    static constexpr FloatType pi = static_cast<FloatType>(3.141592653589793238L);
    static constexpr FloatType sqrt2 = static_cast<FloatType>(1.4142135623730950488L);
};

class IIRCoefficients {
public:
    IIRCoefficients() noexcept {
        std::memset(coefficients, 0, sizeof(coefficients));
    }

    IIRCoefficients(double c1, double c2, double c3,
                    double c4, double c5, double c6) noexcept {
        const double a = 1.0 / c4;
        coefficients[0] = (float)(c1 * a);
        coefficients[1] = (float)(c2 * a);
        coefficients[2] = (float)(c3 * a);
        coefficients[3] = (float)(c5 * a);
        coefficients[4] = (float)(c6 * a);
    }

    static IIRCoefficients makeLowPass(double sampleRate, double cutoffFrequency) noexcept {
        return makeLowPass(sampleRate, cutoffFrequency, 1.0 / MathConstants<double>::sqrt2);
    }

    static IIRCoefficients makeLowPass(double sampleRate,
                                       double cutoffFrequency,
                                       double Q) noexcept {
        const auto n = 1.0 / std::tan(MathConstants<double>::pi * cutoffFrequency / sampleRate);
        const auto n2 = n * n;
        const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + n2);

        return IIRCoefficients(c1, c1 * 2.0, c1,
                               1.0,
                               c1 * 2.0 * (1.0 - n2),
                               c1 * (1.0 - 1.0 / Q * n + n2));
    }

    static IIRCoefficients makeHighPass(double sampleRate, double cutoffFrequency) noexcept {
        return makeHighPass(sampleRate, cutoffFrequency, 1.0 / std::sqrt(2.0));
    }

    static IIRCoefficients makeHighPass(double sampleRate,
                                        double cutoffFrequency,
                                        double Q) noexcept {
        const auto n = std::tan(MathConstants<double>::pi * cutoffFrequency / sampleRate);
        const auto n2 = n * n;
        const auto c1 = 1.0 / (1.0 + 1.0 / Q * n + n2);

        return IIRCoefficients(c1, c1 * -2.0, c1,
                               1.0,
                               c1 * 2.0 * (n2 - 1.0),
                               c1 * (1.0 - 1.0 / Q * n + n2));
    }

    float coefficients[5];
};

class IIRFilter {
public:
    IIRFilter() noexcept = default;

    void reset() noexcept {
        v1 = 0.0f;
        v2 = 0.0f;
    }

    void setCoefficients(const IIRCoefficients& newCoefficients) noexcept {
        coefficients = newCoefficients;
        active = true;
    }

    float processSingleSampleRaw(const float in) noexcept {
        auto out = coefficients.coefficients[0] * in + v1;

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
        if (!(out < -1.0e-8f || out > 1.0e-8f))
            out = 0;
#endif

        v1 = coefficients.coefficients[1] * in - coefficients.coefficients[3] * out + v2;
        v2 = coefficients.coefficients[2] * in - coefficients.coefficients[4] * out;

        return out;
    }

protected:
    IIRCoefficients coefficients;
    float v1 = 0, v2 = 0;
    bool active = false;
};

} // namespace juce
