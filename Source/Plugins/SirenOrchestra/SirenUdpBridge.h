//
// Pont UDP vers les sirènes physiques — réplique le protocole du patch Pd
// sirenMidi2Udp (repo puredata-abstractions, firmwares/v1/routing) :
//
//   - transport : UDP binaire, sirène N -> 192.168.1.1N:8001
//   - trame 10 octets : [18, 10, checksum, N, status, d1, d2, 0, 0, 0]
//     avec checksum = status ^ d1 ^ d2
//     et status = status byte MIDI standard, canal = sirène (note-on 143+N,
//     note-off 127+N, CC 175+N, pitch bend 223+N — d1/d2 = LSB/MSB)
//   - handshake à la connexion : [8, 10, 0, 0, 0, 0, 0, 0, 0, 0]
//   - commande ST (standby) : [4, 4, 0, état] (CMD_ST du firmware)
//
// Le thread audio pousse les messages dans une FIFO lock-free (pushMidi) ;
// un thread dédié draine la FIFO et fait les envois réseau (jamais de socket
// sur le thread audio).
//
// État ST : lu directement sur les variateurs KEB (192.168.1.70+N-1:8000,
// protocole DIN 66019 sur UDP), à 1 Hz. Deux variantes selon le variateur
// (cf. firmwares-artila-F6/docs/MIGRATION_KEB_F5_F6.md) :
//   - KEB F5 (toutes sauf S4) : lecture du paramètre 0216 comme le démon
//     testkebreponse du firmware ; la réponse de 11 octets porte l'état en
//     buf[8] ('8' = OFF, '9' = ON).
//   - KEB F6 (S4, avril 2026) : lecture DIN 66019 II du controlword DS402
//     (sy50, registre 2500) ; réponse de 13 octets, valeur int32 en ASCII
//     hex aux octets [3..10] : 15 = ON (Enable Operation), 7 = OFF (veille).
// Pas de réponse -> inconnu.
//

#ifndef SIRENORCHESTRA_SIRENUDPBRIDGE_H
#define SIRENORCHESTRA_SIRENUDPBRIDGE_H

#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <juce_core/juce_core.h>

class SirenUdpBridge : private juce::Thread
{
public:
    static constexpr int kNumSirens = 7;
    static constexpr int kPort = 8001;

    SirenUdpBridge() : juce::Thread("SirenUdpBridge"), fifo(kFifoSize)
    {
        startThread();
    }

    ~SirenUdpBridge() override
    {
        stopThread(1000);
    }

    // Appelé depuis le thread audio : lock-free, ne touche pas au socket.
    // Ne retient que les types gérés par le patch Pd (note on/off, CC, bend)
    // sur les canaux 1..7.
    void pushMidi(juce::uint8 status, juce::uint8 d1, juce::uint8 d2) noexcept
    {
        const int type = status & 0xF0;
        if (type != 0x80 && type != 0x90 && type != 0xB0 && type != 0xE0)
            return;

        const int siren = (status & 0x0F) + 1;
        if (siren > kNumSirens)
            return;

        // comme dans le patch Pd : note-on de vélocité 0 -> trame note-off
        if (type == 0x90 && d2 == 0)
            status = static_cast<juce::uint8>(0x80 | (status & 0x0F));

        const auto scope = fifo.write(1);
        if (scope.blockSize1 == 1) {
            frames[static_cast<size_t>(scope.startIndex1)] = {
                18, 10,
                static_cast<juce::uint8>(status ^ d1 ^ d2),
                static_cast<juce::uint8>(siren),
                status, d1, d2,
                0, 0, 0
            };
        }
        notify();
    }

    // Envoie la trame de reset [8, 10, 0...] à une sirène (cf. "route connect
    // reset" du patch Pd). Appelable depuis n'importe quel thread (la FIFO est
    // réservée au thread audio : ici on passe par un masque atomique).
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
    enum class StState { unknown = -1, off = 0, on = 1 };

    StState getStState(int siren) const noexcept
    {
        if (siren < 1 || siren > kNumSirens)
            return StState::unknown;
        return static_cast<StState>(
            stStates[static_cast<size_t>(siren - 1)].load(std::memory_order_relaxed));
    }

    // Active/désactive le ST de toutes les sirènes (trame CMD_ST [4, 4, 0, état]
    // vers chaque carte, comme le message "st" du patch Pd). Thread-safe.
    void setStAll(bool on) noexcept
    {
        pendingStAll.store(on ? 1 : 0, std::memory_order_relaxed);
        notify();
    }

private:
    static constexpr int kFifoSize = 1024;

    using Frame = std::array<juce::uint8, 10>;

    static juce::String addressForSiren(int siren)
    {
        // 192.168.1.11 .. 192.168.1.17, comme "connect 192.168.1.1$1 8001"
        return "192.168.1.1" + juce::String(siren);
    }

    static juce::String kebAddressForSiren(int siren)
    {
        // variateurs KEB : 192.168.1.70 (S1) .. 192.168.1.76 (S7)
        return "192.168.1." + juce::String(69 + siren);
    }

    static constexpr bool isF6Siren(int siren)
    {
        // état du parc avril 2026 : seule S4 est équipée d'un KEB F6
        return siren == 4;
    }

    void run() override
    {
        juce::DatagramSocket socket;
        juce::DatagramSocket kebSocket; // socket dédié au polling des KEB

        // JUCE exige un bind explicite pour pouvoir lire (read() retourne -1
        // sinon, cf. juce_Socket.cpp). Port 0 = éphémère ; les variateurs
        // répondent au port source de la requête.
        kebSocket.bindToPort(0);

        // handshake, comme le message "connect" du patch Pd
        // (même trame que le reset)
        for (int siren = 1; siren <= kNumSirens; ++siren)
            socket.write(addressForSiren(siren), kPort,
                         kResetFrame.data(), static_cast<int>(kResetFrame.size()));

        juce::uint32 lastPollTime = 0;
        int awaitingReplies = 0; // masque des sirènes interrogées sans réponse

        while (!threadShouldExit()) {
            // réveil immédiat sur notify() (MIDI/reset/ST), sinon tick de 100 ms
            // pour cadencer le polling KEB et ramasser les réponses
            wait(100);

            // resets en attente (demandés depuis l'UI)
            if (int mask = pendingResets.exchange(0, std::memory_order_relaxed))
                for (int siren = 1; siren <= kNumSirens; ++siren)
                    if (mask & (1 << (siren - 1)))
                        socket.write(addressForSiren(siren), kPort,
                                     kResetFrame.data(),
                                     static_cast<int>(kResetFrame.size()));

            // commande ST globale en attente (switch de l'UI)
            if (int st = pendingStAll.exchange(-1, std::memory_order_relaxed); st >= 0) {
                const std::array<juce::uint8, 4> stFrame {
                    4, 4, 0, static_cast<juce::uint8>(st) };
                for (int siren = 1; siren <= kNumSirens; ++siren)
                    socket.write(addressForSiren(siren), kPort,
                                 stFrame.data(), static_cast<int>(stFrame.size()));
            }

            while (fifo.getNumReady() > 0 && !threadShouldExit()) {
                const auto scope = fifo.read(1);
                if (scope.blockSize1 == 1) {
                    const auto& f = frames[static_cast<size_t>(scope.startIndex1)];
                    socket.write(addressForSiren(f[3]), kPort,
                                 f.data(), static_cast<int>(f.size()));
                }
            }

            // ramasser les réponses KEB éventuelles (non bloquant)
            while (kebSocket.waitUntilReady(true, 0) == 1) {
                juce::uint8 reply[32] {};
                juce::String senderIp;
                int senderPort = 0;
                const int len = kebSocket.read(reply, sizeof(reply), false,
                                               senderIp, senderPort);
                if (len <= 0)
                    break;
                const int siren = senderIp.getTrailingIntValue() - 69;
                if (siren < 1 || siren > kNumSirens)
                    continue;
                auto state = StState::unknown;
                bool parsed = false;
                if (len == 11
                    && reply[1] == '0' && reply[2] == '2'
                    && reply[3] == '1' && reply[4] == '6') {
                    // F5 : réponse à la lecture du paramètre 0216, état en reply[8]
                    state = reply[8] == '9' ? StState::on
                          : reply[8] == '8' ? StState::off
                                            : StState::unknown;
                    parsed = true;
                } else if (len == 13 && reply[0] == 0x02 && reply[1] == 'G') {
                    // F6 : controlword DS402 en ASCII hex aux octets [3..10]
                    char hex[9] = {};
                    std::memcpy(hex, reply + 3, 8);
                    const long value = std::strtol(hex, nullptr, 16);
                    state = value == 15 ? StState::on
                          : value == 7  ? StState::off
                                        : StState::unknown;
                    parsed = true;
                }
                if (parsed) {
                    stStates[static_cast<size_t>(siren - 1)].store(
                        static_cast<int>(state), std::memory_order_relaxed);
                    awaitingReplies &= ~(1 << (siren - 1));
                }
            }

            const auto now = juce::Time::getMillisecondCounter();
            if (now - lastPollTime >= kPollIntervalMs) {
                // les sirènes restées muettes depuis la dernière requête
                // passent à l'état inconnu
                for (int siren = 1; siren <= kNumSirens; ++siren)
                    if (awaitingReplies & (1 << (siren - 1)))
                        stStates[static_cast<size_t>(siren - 1)].store(
                            static_cast<int>(StState::unknown),
                            std::memory_order_relaxed);

                // nouvelle salve de lectures
                // F5 : paramètre 0216 (EOT, ADR "01", "0216", ENQ),
                // cf. testkebreponse/main.c
                static constexpr juce::uint8 kStRequestF5[8] = {
                    0x04, '0', '1', '0', '2', '1', '6', 0x05 };
                // F6 : lecture DIN 66019 II du registre 2500 (sy50), set 0
                // ('G', IID '1', "2500", '0', set '1', ENQ, BCC)
                static constexpr juce::uint8 kStRequestF6[10] = {
                    'G', '1', '2', '5', '0', '0', '0', '1', 0x05, 0x75 };
                awaitingReplies = 0;
                for (int siren = 1; siren <= kNumSirens; ++siren) {
                    const auto* req = isF6Siren(siren) ? kStRequestF6 : kStRequestF5;
                    const int reqLen = isF6Siren(siren) ? 10 : 8;
                    if (kebSocket.write(kebAddressForSiren(siren), kKebPort,
                                        req, reqLen) >= 0)
                        awaitingReplies |= 1 << (siren - 1);
                }
                lastPollTime = now;
            }
        }
    }

    static constexpr Frame kResetFrame { 8, 10, 0, 0, 0, 0, 0, 0, 0, 0 };
    static constexpr int kKebPort = 8000;
    static constexpr juce::uint32 kPollIntervalMs = 1000;

    juce::AbstractFifo fifo;
    std::array<Frame, kFifoSize> frames;
    std::atomic<int> pendingResets { 0 };
    std::atomic<int> pendingStAll { -1 };
    std::array<std::atomic<int>, kNumSirens> stStates {
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown), static_cast<int>(StState::unknown),
        static_cast<int>(StState::unknown) };
};

#endif //SIRENORCHESTRA_SIRENUDPBRIDGE_H
