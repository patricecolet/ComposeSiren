#include "StandaloneAudio.h"

#include <atomic>

namespace cs::standalone {

namespace {
std::function<Keeper()>& maker()
{
    static std::function<Keeper()> make;
    return make;
}

std::atomic<AudioState> state{AudioState::unknown};
} // namespace

void installKeeper(std::function<Keeper()> make)
{
    maker() = std::move(make);
}

Keeper keepRunningWithoutAudio()
{
    return maker() ? maker()() : nullptr;
}

AudioState audioState()
{
    return state.load(std::memory_order_relaxed);
}

void setAudioState(AudioState s)
{
    state.store(s, std::memory_order_relaxed);
}

} // namespace cs::standalone
