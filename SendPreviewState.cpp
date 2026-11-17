#include "SendPreviewState.h"
#include "ClientVariable.h"
#include "EnDePacket.h"
#include "EventQueue.h"

bool SendPreviewState(std::uint16_t roundCnt, std::int16_t gridX, std::int16_t gridY, bool visible)
{
    constexpr BYTE previewStatePacketType = 120;
    const std::uint32_t udpSequence = ++clientUdpSendSequence;
    const auto gridXBits = static_cast<std::uint16_t>(gridX);
    const auto gridYBits = static_cast<std::uint16_t>(gridY);

    const std::vector<BYTE> previewBody = {
        static_cast<BYTE>(roundCnt >> 8),
        static_cast<BYTE>(roundCnt),
        static_cast<BYTE>(udpSequence >> 24),
        static_cast<BYTE>(udpSequence >> 16),
        static_cast<BYTE>(udpSequence >> 8),
        static_cast<BYTE>(udpSequence),
        static_cast<BYTE>(gridXBits >> 8),
        static_cast<BYTE>(gridXBits),
        static_cast<BYTE>(gridYBits >> 8),
        static_cast<BYTE>(gridYBits),
        static_cast<BYTE>(visible)
    };

    NetEvent sendEvent{};
    sendEvent.packetType = previewStatePacketType;
    sendEvent.packetData = EncodePacket(previewStatePacketType, previewBody);

    return PushEventQueue(clientUdpSendQueue, sendEvent, hClientUdpSendGate, hClientUdpSendReady);
}