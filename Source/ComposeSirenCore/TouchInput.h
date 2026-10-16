#ifndef COMPOSESIREN_TOUCHINPUT_H
#define COMPOSESIREN_TOUCHINPUT_H

#include <memory>
#include <juce_gui_basics/juce_gui_basics.h>

namespace cs {

// Multi-touch on Linux: a finger on the keyboard and others on the knobs at once.
//
// JUCE reads touch on Windows, iOS and Android, not on X11, where the X server turns
// one finger into the mouse pointer and drops the others. TouchInput reads the touches
// of the component's window itself (XInput 2.2, on a display connection of its own) and
// passes each finger to the window's ComponentPeer as its own touch source, as JUCE's
// Windows code does. JUCE creates touch sources on Linux only with
// Dependencies/patches/juce-linux-touch.patch, which CMake applies.
//
// Elsewhere, or when the X server has no XInput 2.2, it does nothing.
class TouchInput
{
public:
    explicit TouchInput(juce::Component& component);
    ~TouchInput();

    TouchInput(const TouchInput&) = delete;
    TouchInput& operator=(const TouchInput&) = delete;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace cs

#endif // COMPOSESIREN_TOUCHINPUT_H
