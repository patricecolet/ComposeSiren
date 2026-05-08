//
// Created by joseph larralde on 01/05/2026.
//

#ifndef COMPOSESIREN_RAMPE_H
#define COMPOSESIREN_RAMPE_H

#include <concepts>

template<std::floating_point T>
class rampe
{
    bool ramping{false};
    T target{0};
    T duration{0};
    T increment{1}; // in samples
    T current{0};
    double sampleRate{44100}; // in samples per second

    static constexpr T epsilon{0.001};

public:
    rampe(T defaultValue, T defaultDuration) :
        current(defaultValue), duration(defaultDuration) {}

    ~rampe() = default;

    void setSampleRate(double sr) {
        if (ramping) { increment *= static_cast<T>(sampleRate / sr); }
        sampleRate = sr;
    }

    void setDuration(T d) { // ramp duration in ms
        duration = std::max(d, epsilon);
    }

    void setTarget(T t) { // ramp target
        target = t;
        increment = (t - current) / (1000.0 * duration / sampleRate);
        ramping = true;
    }

    bool process(T& v) {
        if (ramping) {
            current += increment;
            if ((increment < 0 && current <= target) ||
                (increment > 0 && current >= target)) {
                ramping = false;
                current = target;
            }
            v = current;
            return true;
        }
        v = current;
        return false;
    }
};

#endif //COMPOSESIREN_RAMPE_H