#include "ClientVariable.h"
#include "EnDePacket.h"
#include "EventQueue.h"

bool SendMoveInput(std::uint16_t roundCnt, std::uint8_t moveDirection)
{
    constexpr BYTE moveInputPacketType = 110;
    const std::uint32_t udpSequence = ++clientUdpSendSequence;

    // 여러 바이트로 된 값은 큰 자리 바이트부터 저장한다.
    const std::vector<BYTE> moveInputBody = {
        static_cast<BYTE>(roundCnt >> 8),
        static_cast<BYTE>(roundCnt),
        static_cast<BYTE>(udpSequence >> 24),
        static_cast<BYTE>(udpSequence >> 16),
        static_cast<BYTE>(udpSequence >> 8),
        static_cast<BYTE>(udpSequence),
        moveDirection
    };

    NetEvent sendEvent{};
    sendEvent.packetType = moveInputPacketType;
    sendEvent.packetData = EncodePacket(moveInputPacketType, moveInputBody);

    return PushEventQueue(clientUdpSendQueue, sendEvent,
        hClientUdpSendGate, hClientUdpSendReady);
}