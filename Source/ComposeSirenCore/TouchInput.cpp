#include "TouchInput.h"

#if JUCE_LINUX

#include <juce_events/juce_events.h>
#include <map>

// After every JUCE header: Xlib defines macros (None, Bool, Status…) that clash with them.
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

namespace cs {

struct TouchInput::Impl : private juce::Timer
{
    explicit Impl(juce::Component& c) : component(c)
    {
        display = XOpenDisplay(nullptr);
        if (display == nullptr)
            return;

        int event = 0, error = 0;
        int major = 2, minor = 2;
        if (!XQueryExtension(display, "XInputExtension", &opcode, &event, &error)
            || XIQueryVersion(display, &major, &minor) != Success
            || major * 100 + minor < 202) {
            XCloseDisplay(display);
            display = nullptr;
            return;
        }

        fd = ConnectionNumber(display);
        juce::LinuxEventLoop::registerFdCallback(fd, [this](int) { readEvents(); });
        // The window comes and goes with the component's peer.
        timerCallback();
        startTimer(500);
    }

    ~Impl() override
    {
        stopTimer();
        if (display == nullptr)
            return;
        juce::LinuxEventLoop::unregisterFdCallback(fd);
        // Closing the connection also drops its selection on the window.
        XCloseDisplay(display);
    }

private:
    // Selects the touches of the peer's window, when it changes.
    void timerCallback() override
    {
        auto* peer = component.getPeer();
        const auto handle = peer != nullptr
            ? reinterpret_cast<::Window>(peer->getNativeHandle())
            : ::Window{};
        if (handle == window)
            return;

        window = handle;
        fingers.clear();
        if (window == ::Window{})
            return;

        unsigned char bits[XIMaskLen(XI_LASTEVENT)] = {};
        XISetMask(bits, XI_TouchBegin);
        XISetMask(bits, XI_TouchUpdate);
        XISetMask(bits, XI_TouchEnd);
        XIEventMask mask;
        mask.deviceid = XIAllMasterDevices;
        mask.mask_len = sizeof(bits);
        mask.mask = bits;
        // From now on the touches over this window come here instead of being
        // emulated as the mouse pointer.
        XISelectEvents(display, window, &mask, 1);
        XFlush(display);
    }

    void readEvents()
    {
        while (XPending(display) > 0) {
            XEvent event;
            XNextEvent(display, &event);
            auto* cookie = &event.xcookie;
            if (cookie->type != GenericEvent || cookie->extension != opcode)
                continue;
            if (XGetEventData(display, cookie)) {
                touch(cookie->evtype, *static_cast<XIDeviceEvent*>(cookie->data));
                XFreeEventData(display, cookie);
            }
        }
    }

    // The touch index JUCE gets for an X touch: the lowest one not in use.
    int indexFor(int touchId)
    {
        if (const auto it = fingers.find(touchId); it != fingers.end())
            return it->second;
        int index = 0;
        for (bool used = true; used; ) {
            used = false;
            for (const auto& [id, i] : fingers)
                if (i == index) { used = true; ++index; break; }
        }
        fingers[touchId] = index;
        return index;
    }

    void touch(int type, const XIDeviceEvent& e)
    {
        auto* peer = component.getPeer();
        if (peer == nullptr || e.event != window)
            return;
        if (type != XI_TouchBegin && type != XI_TouchUpdate && type != XI_TouchEnd)
            return;

        const auto index = indexFor(e.detail);
        const auto scale = static_cast<float>(peer->getPlatformScaleFactor());
        const juce::Point<float> pos{ static_cast<float>(e.event_x) / scale,
                                      static_cast<float>(e.event_y) / scale };
        const auto time = juce::Time::currentTimeMillis();
        const auto up = juce::ModifierKeys::getCurrentModifiers().withoutMouseButtons();
        const auto down = up.withFlags(juce::ModifierKeys::leftButtonModifier);
        constexpr auto source = juce::MouseInputSource::InputSourceType::touch;
        const auto pressure = juce::MouseInputSource::defaultPressure;
        const auto orientation = juce::MouseInputSource::defaultOrientation;

        if (type == XI_TouchBegin) {
            // Enter at the finger, then press (as JUCE's Windows code does).
            peer->handleMouseEvent(source, pos, up, pressure, orientation, time, {}, index);
            peer->handleMouseEvent(source, pos, down, pressure, orientation, time, {}, index);
        } else if (type == XI_TouchUpdate) {
            peer->handleMouseEvent(source, pos, down, pressure, orientation, time, {}, index);
        } else {
            peer->handleMouseEvent(source, pos, up, pressure, orientation, time, {}, index);
            // Leave, so the component under the finger gets its mouseExit.
            peer->handleMouseEvent(source, juce::MouseInputSource::offscreenMousePos, up,
                                   pressure, orientation, time, {}, index);
            fingers.erase(e.detail);
        }
    }

    juce::Component& component;
    Display* display = nullptr;
    int opcode = 0;
    int fd = -1;
    ::Window window{};
    std::map<int, int> fingers; // X touch id -> JUCE touch index
};

TouchInput::TouchInput(juce::Component& component) : impl(std::make_unique<Impl>(component)) {}
TouchInput::~TouchInput() = default;

} // namespace cs

#else

namespace cs {

struct TouchInput::Impl {};

TouchInput::TouchInput(juce::Component&) {}
TouchInput::~TouchInput() = default;

} // namespace cs

#endif
