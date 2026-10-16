#ifndef COMPOSESIREN_STANDALONEAUDIO_H
#define COMPOSESIREN_STANDALONEAUDIO_H

#include <functional>
#include <memory>

// A standalone app runs its processor only while an audio device calls it. With no
// audio output chosen (a valid setup: SirenOrchestra driving the park through the
// bridge's MIDI port, for example), nothing would play the MIDI that comes in or the
// on-screen keyboard, and no MIDI would go out.
//
// StandaloneAudioless.cpp, compiled into the Standalone targets only, installs a keeper
// that drives the processor from a clock of its own while there is no audio device, and
// hands it back to the device as soon as one is chosen. In a plugin nothing is
// installed: the host runs the processor.

namespace cs::standalone {

enum class AudioState
{
    unknown,  // a plugin, or not started
    device,   // an audio device runs the processor
    noDevice, // no audio device: the keeper's clock runs it, without sound
};

using Keeper = std::shared_ptr<void>;

// Called by the standalone at static initialisation.
void installKeeper(std::function<Keeper()> make);

// From the editor, on the message thread: keeps the processor running while there is
// no audio device, until the result is released. Null in a plugin.
Keeper keepRunningWithoutAudio();

AudioState audioState();
void setAudioState(AudioState);

} // namespace cs::standalone

#endif // COMPOSESIREN_STANDALONEAUDIO_H
