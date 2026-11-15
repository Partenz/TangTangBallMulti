#include <bit>
#include "EnDePacket.h"
#include "EventQueue.h"
#include "ServerVariable.h"

void BroadcastPlayerStates()
{
    constexpr BYTE playersStatePacketType = 111;
    const std::uint32_t udpSequence = ++serverUdpSendSequence;
    const std::uint16_t roundCnt = gameState.roundCnt;

    // 라운드 번호 2바이트, 순서 번호 4바이트, 생존 인원 1바이트.
    std::vector<BYTE> packetBody{
        static_cast<BYTE>(roundCnt >> 8),
        static_cast<BYTE>(roundCnt),
        static_cast<BYTE>(udpSequence >> 24),
        static_cast<BYTE>(udpSequence >> 16),
        static_cast<BYTE>(udpSequence >> 8),
        static_cast<BYTE>(udpSequence),
        0
    };
    packetBody.reserve(7 + gameState.players.size() * 18);

    BYTE alivePlayerCount = 0;
    for (const auto& [playerId, player] : gameState.players) {
        if (!player.alive)
            continue;

        const PlayerMotion& motion = player.motion;
        packetBody.push_back(playerId);

        const float motionValues[] = { motion.x, motion.y, motion.vx, motion.vy };
        for (float motionValue : motionValues) {
            const auto motionBits = std::bit_cast<std::uint32_t>(motionValue);
            for (int bitShift = 24; bitShift >= 0; bitShift -= 8)
                packetBody.push_back(static_cast<BYTE>(motionBits >> bitShift));
        }

        packetBody.push_back(motion.falling ? 1 : 0);
        ++alivePlayerCount;
    }
    packetBody[6] = alivePlayerCount;

    std::vector<std::uint8_t> recipientPlayerIds;
    recipientPlayerIds.reserve(gameState.players.size());

    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
        return;

    for (const auto& [playerId, player] : gameState.players) {
        if (registeredUdpAddresses.contains(playerId))
            recipientPlayerIds.push_back(playerId);
    }

    if (!SetEvent(hConnectionGate))
        return;

    NetEvent sendEvent{};
    sendEvent.packetType = playersStatePacketType;
    sendEvent.packetData = EncodePacket(playersStatePacketType, packetBody);

    for (std::uint8_t recipientPlayerId : recipientPlayerIds) {
        sendEvent.playerId = recipientPlayerId;
        if (!PushEventQueue(serverUdpSendQueue, sendEvent,
            hServerUdpSendGate, hServerUdpSendReady))
            return;
    }
}