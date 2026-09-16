#include "SirenLink.h"

#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using socklen_t = int;
  static constexpr long long kInvalidSocket = static_cast<long long>(INVALID_SOCKET);
#else
  #include <arpa/inet.h>
  #include <fcntl.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  static constexpr long long kInvalidSocket = -1;
#endif

namespace cs::net {

// UdpSocket /////////////////////////////////////////////////////////////////

#if defined(_WIN32)
// winsock exige une initialisation par processus ; comptée par socket.
static void winsockAcquire()
{
    static int count = 0;
    if (count++ == 0) {
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
    }
}
#endif

UdpSocket::UdpSocket()
{
#if defined(_WIN32)
    winsockAcquire();
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        return;
    u_long nonBlocking = 1;
    ioctlsocket(s, FIONBIO, &nonBlocking);
    handle = static_cast<long long>(s);
#else
    int s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
        return;
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
    handle = s;
#endif
}

UdpSocket::~UdpSocket()
{
    if (handle == kInvalidSocket)
        return;
#if defined(_WIN32)
    closesocket(static_cast<SOCKET>(handle));
#else
    ::close(static_cast<int>(handle));
#endif
}

bool UdpSocket::bind(uint16_t port)
{
    if (handle == kInvalidSocket)
        return false;
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    return ::bind(static_cast<int>(handle), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
}

int UdpSocket::sendTo(const std::string& ip, uint16_t port, const uint8_t* data, int len) noexcept
{
    if (handle == kInvalidSocket)
        return -1;
    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1)
        return -1;
    return static_cast<int>(::sendto(static_cast<int>(handle),
                                     reinterpret_cast<const char*>(data), static_cast<size_t>(len), 0,
                                     reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)));
}

int UdpSocket::receiveFrom(uint8_t* buffer, int capacity, std::string& senderIp) noexcept
{
    if (handle == kInvalidSocket)
        return -1;
    sockaddr_in from {};
    socklen_t fromLen = sizeof(from);
    const auto n = ::recvfrom(static_cast<int>(handle),
                              reinterpret_cast<char*>(buffer), static_cast<size_t>(capacity), 0,
                              reinterpret_cast<sockaddr*>(&from), &fromLen);
    if (n < 0) {
#if defined(_WIN32)
        return WSAGetLastError() == WSAEWOULDBLOCK ? 0 : -1;
#else
        return (errno == EAGAIN || errno == EWOULDBLOCK) ? 0 : -1;
#endif
    }
    char ip[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &from.sin_addr, ip, sizeof(ip));
    senderIp = ip;
    return static_cast<int>(n);
}

// Parc //////////////////////////////////////////////////////////////////////

Parc Parc::defaults()
{
    Parc p;
    for (int n = 1; n <= kNumSirens; ++n) {
        auto& e = p.sirens[static_cast<size_t>(n - 1)];
        e.cardIp = "192.168.1.1" + std::to_string(n);      // .11 .. .17
        e.midiPort = 8001;
        e.kebIp = "192.168.1." + std::to_string(69 + n);   // .70 .. .76
        e.kebPort = 8000;
        e.keb = (n == 4) ? KebType::F6 : KebType::F5;
    }
    return p;
}

// frames ////////////////////////////////////////////////////////////////////

namespace frames {

Frame midi(int siren, uint8_t status, uint8_t d1, uint8_t d2) noexcept
{
    return { 18, 10,
             static_cast<uint8_t>(status ^ d1 ^ d2),
             static_cast<uint8_t>(siren),
             status, d1, d2,
             0, 0, 0 };
}

std::array<uint8_t, 4> st(bool on) noexcept
{
    return { 4, 4, 0, static_cast<uint8_t>(on ? 1 : 0) };
}

bool parseKebReply(const uint8_t* reply, int len, StState& out) noexcept
{
    if (len == 11
        && reply[1] == '0' && reply[2] == '2'
        && reply[3] == '1' && reply[4] == '6') {
        // F5 : réponse à la lecture du paramètre 0216, état en reply[8]
        out = reply[8] == '9' ? StState::on
            : reply[8] == '8' ? StState::off
                              : StState::unknown;
        return true;
    }
    if (len == 13 && reply[0] == 0x02 && reply[1] == 'G') {
        // F6 : controlword DS402 en ASCII hex aux octets [3..10]
        char hex[9] = {};
        std::memcpy(hex, reply + 3, 8);
        const long value = std::strtol(hex, nullptr, 16);
        out = value == 15 ? StState::on
            : value == 7  ? StState::off
                          : StState::unknown;
        return true;
    }
    return false;
}

} // namespace frames

// SirenLink /////////////////////////////////////////////////////////////////

SirenLink::SirenLink(Parc parc) : table(std::move(parc))
{
    states.fill(StState::unknown);
    kebSocket.bind(0);
}

void SirenLink::sendToCard(int siren, const uint8_t* data, int len) noexcept
{
    const auto& e = table.siren(siren);
    cardSocket.sendTo(e.cardIp, e.midiPort, data, len);
}

bool SirenLink::sendMidi(uint8_t status, uint8_t d1, uint8_t d2) noexcept
{
    const int type = status & 0xF0;
    if (type != 0x80 && type != 0x90 && type != 0xB0 && type != 0xE0)
        return false;

    const int siren = (status & 0x0F) + 1;
    if (siren > kNumSirens)
        return false;

    // comme dans le patch Pd : note-on de vélocité 0 -> trame note-off
    if (type == 0x90 && d2 == 0)
        status = static_cast<uint8_t>(0x80 | (status & 0x0F));

    const auto f = frames::midi(siren, status, d1, d2);
    sendToCard(siren, f.data(), static_cast<int>(f.size()));
    return true;
}

void SirenLink::sendReset(int siren) noexcept
{
    if (siren < 1 || siren > kNumSirens)
        return;
    sendToCard(siren, frames::reset.data(), static_cast<int>(frames::reset.size()));
}

void SirenLink::sendResetAll() noexcept
{
    for (int siren = 1; siren <= kNumSirens; ++siren)
        sendReset(siren);
}

void SirenLink::sendStAll(bool on) noexcept
{
    const auto f = frames::st(on);
    for (int siren = 1; siren <= kNumSirens; ++siren)
        sendToCard(siren, f.data(), static_cast<int>(f.size()));
}

void SirenLink::sendKebRequests() noexcept
{
    // les sirènes restées muettes depuis la dernière salve passent à inconnu
    for (int siren = 1; siren <= kNumSirens; ++siren)
        if (awaitingReplies & (1 << (siren - 1)))
            states[static_cast<size_t>(siren - 1)] = StState::unknown;

    awaitingReplies = 0;
    for (int siren = 1; siren <= kNumSirens; ++siren) {
        const auto& e = table.siren(siren);
        const uint8_t* req = e.keb == KebType::F6 ? frames::kebRequestF6.data() : frames::kebRequestF5.data();
        const int len = e.keb == KebType::F6 ? static_cast<int>(frames::kebRequestF6.size())
                                             : static_cast<int>(frames::kebRequestF5.size());
        if (kebSocket.sendTo(e.kebIp, e.kebPort, req, len) >= 0)
            awaitingReplies |= 1 << (siren - 1);
    }
}

int SirenLink::sirenForKebIp(const std::string& ip) const noexcept
{
    for (int siren = 1; siren <= kNumSirens; ++siren)
        if (table.siren(siren).kebIp == ip)
            return siren;
    return 0;
}

void SirenLink::pumpKebReplies() noexcept
{
    uint8_t reply[32];
    std::string sender;
    for (;;) {
        const int len = kebSocket.receiveFrom(reply, sizeof(reply), sender);
        if (len <= 0)
            break;
        const int siren = sirenForKebIp(sender);
        if (siren == 0)
            continue;
        StState state;
        if (frames::parseKebReply(reply, len, state)) {
            states[static_cast<size_t>(siren - 1)] = state;
            awaitingReplies &= ~(1 << (siren - 1));
        }
    }
}

StState SirenLink::stState(int siren) const noexcept
{
    if (siren < 1 || siren > kNumSirens)
        return StState::unknown;
    return states[static_cast<size_t>(siren - 1)];
}

} // namespace cs::net
