//
// Pont UDP vers les sirènes physiques — wrapper JUCE de cs::net::SirenLink,
// qui porte le protocole (trames, KEB, table du parc). Ici, seulement le
// modèle de threads du plugin :
//
// Le thread audio pousse les messages dans une FIFO lock-free (pushMidi) ;
// un thread dédié draine la FIFO et fait les envois réseau (jamais de socket
// sur le thread audio). Les resets et la commande ST, demandés depuis l'UI,
// passent par des masques atomiques. L'état ST est lu sur les KEB à 1 Hz.
//

#ifndef SIRENORCHESTRA_SIRENUDPBRIDGE_H
#define SIRENORCHESTRA_SIRENUDPBRIDGE_H

#include <array>
#include <atomic>
#include <juce_core/juce_core.h>
#include "lib/net/SirenLink.h"

class SirenUdpBridge : private juce::Thread
{
public:
    static constexpr int kNumSirens = cs::net::SirenLink::kNumSirens;

    using StState = cs::net::StState;

    SirenUdpBridge() : juce::Thread("SirenUdpBridge"), fifo(kFifoSize)
    {
        startThread();
    }

    ~SirenUdpBridge() override
    {
        stopThread(1000);
    }

    // Appelé depuis le thread audio : lock-free, ne touche pas au socket.
    // Le tri des messages (types, canaux 1..7, note-on de vélocité 0) est
    // fait par SirenLink au moment de l'envoi.
    void pushMidi(juce::uint8 status, juce::uint8 d1, juce::uint8 d2) noexcept
    {
        const auto scope = fifo.write(1);
        if (scope.blockSize1 == 1)
            pending[static_cast<size_t>(scope.startIndex1)] = { status, d1, d2 };
        notify();
    }

    // Trame de reset vers une sirène (cf. "route connect reset" du patch Pd).
    // Appelable depuis n'importe quel thread (la FIFO est réservée au thread
    // audio : ici on passe par un masque atomique).
    void pushReset(int siren) noexcept
    {
        if (siren < 1 || siren > kNumSirens)
            return;
        pendingResets.fetch_or(1 << (siren - 1), std::memory_order_relaxed);
        notify();
    }

    void pushResetAll() noexcept
    {
        pendingResets.fetch_or((1 << kNumSirens) - 1, std::memory_order_relaxed);
        notify();
    }

    // État ST d'une sirène (1..7), tel que lu sur son variateur KEB
    StState getStState(int siren) const noexcept
    {
        if (siren < 1 || siren > kNumSirens)
            return StState::unknown;
        return static_cast<StState>(
            stStates[static_cast<size_t>(siren - 1)].load(std::memory_order_relaxed));
    }

    // Active/désactive le ST de toutes les sirènes. Thread-safe.
    void setStAll(bool on) noexcept
    {
        pendingStAll.store(on ? 1 : 0, std::memory_order_relaxed);
        notify();
    }

private:
    static constexpr int kFifoSize = 1024;
    static constexpr juce::uint32 kPollIntervalMs = 1000;

    struct MidiBytes { juce::uint8 status, d1, d2; };

    void run() override
    {
        cs::net::SirenLink link; // les sockets vivent sur ce thread

        // handshake, comme le message "connect" du patch Pd
        link.sendResetAll();

        juce::uint32 lastPollTime = 0;

        while (!threadShouldExit()) {
            // réveil immédiat sur notify() (MIDI/reset/ST), sinon tick de 100 ms
            // pour cadencer le polling KEB et ramasser les réponses
            wait(100);

            if (int mask = pendingResets.exchange(0, std::memory_order_relaxed))
                for (int siren = 1; siren <= kNumSirens; ++siren)
                    if (mask & (1 << (siren - 1)))
                        link.sendReset(siren);

            if (int st = pendingStAll.exchange(-1, std::memory_order_relaxed); st >= 0)
                link.sendStAll(st != 0);

            while (fifo.getNumReady() > 0 && !threadShouldExit()) {
                const auto scope = fifo.read(1);
                if (scope.blockSize1 == 1) {
                    const auto& m = pending[static_cast<size_t>(scope.startIndex1)];
                    link.sendMidi(m.status, m.d1, m.d2);
                }
            }

            link.pumpKebReplies();

            const auto now = juce::Time::getMillisecondCounter();
            if (now - lastPollTime >= kPollIntervalMs) {
                link.sendKebRequests();
                lastPollTime = now;
            }

            for (int siren = 1; siren <= kNumSirens; ++siren)
                stStates[static_cast<size_t>(siren - 1)].store(
                    static_cast<int>(link.stState(siren)), std::memory_order_relaxed);
        }
    }

    juce::AbstractFifo fifo;
    std::array<MidiBytes, kFifoSize> pending;
    std::atomic<int> pendingResets { 0 };
    std::atomic<int> pendingStAll { -1 };
    std::array<std::atomic<int>, kNumSirens> stStates {
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown) };
};

#endif //SIRENORCHESTRA_SIRENUDPBRIDGE_H
