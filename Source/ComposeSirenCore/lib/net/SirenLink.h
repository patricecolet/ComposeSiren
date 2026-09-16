//
// SirenLink — le protocole réseau du parc, sans JUCE.
//
// Une seule implémentation pour les plugins (SirenUdpBridge), l'external Pd
// (c-siren~) et tout autre client. Réplique le patch Pd sirenMidi2Udp
// (puredata-abstractions, firmwares/v1/routing) :
//
//   - transport : UDP binaire, sirène N -> carte 192.168.1.1N:8001
//   - trame MIDI 10 octets : [18, 10, checksum, N, status, d1, d2, 0, 0, 0]
//     avec checksum = status ^ d1 ^ d2
//     et status = status byte MIDI standard, canal = sirène (note-on 143+N,
//     note-off 127+N, CC 175+N, pitch bend 223+N — d1/d2 = LSB/MSB)
//   - handshake à la connexion : [8, 10, 0, 0, 0, 0, 0, 0, 0, 0] (= reset)
//   - commande ST (standby) : [4, 4, 0, état] (CMD_ST du firmware)
//
// État ST : lu directement sur les variateurs KEB (192.168.1.70+N-1:8000,
// protocole DIN 66019 sur UDP). Deux variantes selon le variateur
// (cf. firmwares-artila-F6/docs/MIGRATION_KEB_F5_F6.md) :
//   - KEB F5 (toutes sauf S4) : lecture du paramètre 0216 comme le démon
//     testkebreponse du firmware ; la réponse de 11 octets porte l'état en
//     buf[8] ('8' = OFF, '9' = ON).
//   - KEB F6 (S4, avril 2026) : lecture DIN 66019 II du controlword DS402
//     (sy50, registre 2500) ; réponse de 13 octets, valeur int32 en ASCII
//     hex aux octets [3..10] : 15 = ON (Enable Operation), 7 = OFF (veille).
// Pas de réponse -> inconnu.
//
// Aucun thread ici : toutes les méthodes sont synchrones et non bloquantes.
// L'appelant choisit son modèle — thread dédié + FIFO côté JUCE (on part du
// thread audio), clock_delay côté Pd. Jamais de connect() UDP : sendto() par
// paquet sur un socket non connecté, pour qu'un réseau en retard au démarrage
// ne condamne pas la session (cf. wait-network.sh du pédalier).
//
// La table du parc (Parc) a des valeurs par défaut égales à l'état d'avril
// 2026 ; l'appelant peut la remplacer par celle lue dans config.json
// (mecaviv-qml-ui/SirenManager/backend/config.json, la source de vérité).
//

#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace cs::net {

class UdpSocket
{
public:
    UdpSocket();
    ~UdpSocket();
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    // port 0 = éphémère. Nécessaire pour recevoir : les KEB répondent au port
    // source de la requête.
    bool bind(uint16_t port);
    bool isOpen() const noexcept { return handle >= 0; }

    // Non bloquant. Retourne le nombre d'octets envoyés, -1 en cas d'erreur.
    int sendTo(const std::string& ip, uint16_t port, const uint8_t* data, int len) noexcept;

    // Non bloquant. Retourne le nombre d'octets lus, 0 si rien n'attend, -1
    // en cas d'erreur. senderIp reçoit l'adresse de l'émetteur.
    int receiveFrom(uint8_t* buffer, int capacity, std::string& senderIp) noexcept;

private:
    long long handle { -1 }; // int POSIX ou SOCKET winsock, stocké large
};

enum class KebType { F5, F6 };

enum class StState { unknown = -1, off = 0, on = 1 };

struct SirenEntry
{
    std::string cardIp;   // carte de la sirène (trames MIDI, reset, ST)
    uint16_t midiPort;
    std::string kebIp;    // variateur
    uint16_t kebPort;
    KebType keb;
};

struct Parc
{
    static constexpr int kNumSirens = 7;
    std::array<SirenEntry, kNumSirens> sirens; // index 0 = S1

    // état du parc d'avril 2026 : cartes 192.168.1.11..17:8001,
    // KEB 192.168.1.70..76:8000, seule S4 en F6
    static Parc defaults();

    const SirenEntry& siren(int oneBased) const { return sirens[static_cast<size_t>(oneBased - 1)]; }
};

using Frame = std::array<uint8_t, 10>;

// Trames, indépendantes de tout socket — utilisables pour un print, un test,
// ou une outlet miroir dans Pd.
namespace frames {
    Frame midi(int siren, uint8_t status, uint8_t d1, uint8_t d2) noexcept;
    constexpr Frame reset { 8, 10, 0, 0, 0, 0, 0, 0, 0, 0 };
    std::array<uint8_t, 4> st(bool on) noexcept;

    // requêtes de lecture ST sur le KEB
    constexpr std::array<uint8_t, 8>  kebRequestF5 { 0x04, '0', '1', '0', '2', '1', '6', 0x05 };
    constexpr std::array<uint8_t, 10> kebRequestF6 { 'G', '1', '2', '5', '0', '0', '0', '1', 0x05, 0x75 };

    // Décode une réponse KEB ; retourne false si la trame n'est pas reconnue.
    bool parseKebReply(const uint8_t* reply, int len, StState& out) noexcept;
}

class SirenLink
{
public:
    static constexpr int kNumSirens = Parc::kNumSirens;

    explicit SirenLink(Parc parc = Parc::defaults());

    // Ne retient que les types gérés par le patch Pd (note on/off, CC, bend)
    // sur les canaux 1..7 ; note-on de vélocité 0 -> trame note-off.
    // Retourne false si le message est ignoré.
    bool sendMidi(uint8_t status, uint8_t d1, uint8_t d2) noexcept;

    void sendReset(int siren) noexcept;      // = handshake "connect" du patch Pd
    void sendResetAll() noexcept;
    void sendStAll(bool on) noexcept;

    // Lecture de l'état ST : sendKebRequests() envoie une salve aux 7 KEB et
    // marque les sirènes muettes depuis la salve précédente comme inconnues ;
    // pumpKebReplies() ramasse ce qui est arrivé. À appeler respectivement à
    // 1 Hz et aussi souvent que possible (le socket ne garde pas indéfiniment).
    void sendKebRequests() noexcept;
    void pumpKebReplies() noexcept;
    StState stState(int siren) const noexcept;
    void forgetStates() noexcept; // tout repasse à inconnu (ex. pont désactivé)

    const Parc& parc() const noexcept { return table; }

private:
    void sendToCard(int siren, const uint8_t* data, int len) noexcept;
    int sirenForKebIp(const std::string& ip) const noexcept;

    Parc table;
    UdpSocket cardSocket;  // envoi seul
    UdpSocket kebSocket;   // lié à un port éphémère pour recevoir les réponses
    std::array<StState, kNumSirens> states;
    int awaitingReplies { 0 }; // masque des sirènes interrogées sans réponse
};

} // namespace cs::net
