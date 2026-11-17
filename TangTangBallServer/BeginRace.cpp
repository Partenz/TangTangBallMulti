#include <algorithm>
#include <cstdint>
#include <vector>

#include "BeginRace.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

namespace {
    void SendPacket(uint8_t playerId, BYTE packetType, const std::vector<BYTE>& body)
    {
        NetEvent event{};
        event.playerId = playerId;
        event.packetType = packetType;
        event.packetData = EncodePacket(packetType, body);
        if (event.packetData.empty() || WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
            return;

        const auto connection = connections.find(playerId);
        if (connection == connections.end() || connection->second.state != ConnectionState::Active || !connection->second.sendContext) {
            SetEvent(hConnectionGate);
            return;
        }

        auto& context = *connection->second.sendContext;
        const bool queued = PushEventQueue(context.sendQueue, event, context.sendQueueGate, context.sendQueueReady);

        SetEvent(hConnectionGate);
        if (!queued)
            RequestDisconnect(playerId);
    }

    constexpr BYTE placingPhase = 2;
    constexpr BYTE racingPhase = 3;
    constexpr BYTE lightningOnBlock = 7;
    constexpr BYTE lightningOffBlock = 8;
    constexpr BYTE raceBeginType = 25;

    bool IsLightningBlock(const ObjectInfo& object)
    {
        return object.blockType == lightningOnBlock || object.blockType == lightningOffBlock;
    }
}

void BeginRace()
{
    if (gameState.phase != placingPhase || gameState.players.empty())
        return;

    for (const auto& entry : gameState.players) {
        if (!entry.second.placed)
            return;
    }

    const auto lightningCount = std::count_if(gameState.objects.begin(), gameState.objects.end(), IsLightningBlock);
    gameState.electricityActive = lightningCount % 2 != 0;
    for (ObjectInfo& object : gameState.objects) {
        if (IsLightningBlock(object))
            object.blockType = gameState.electricityActive ? lightningOnBlock : lightningOffBlock;
    }

    gameState.phase = racingPhase;
    const std::vector<BYTE> body = { static_cast<BYTE>(gameState.electricityActive) };
    for (const auto& entry : gameState.players)
        SendPacket(entry.first, raceBeginType, body);
}
