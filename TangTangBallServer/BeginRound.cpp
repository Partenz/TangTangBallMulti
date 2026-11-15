#include <cstdint>
#include <cstring>
#include <random>
#include <vector>

#include "BeginRound.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

extern GameState gameState;

namespace {
    void AppendUint32(std::vector<BYTE>& body, uint32_t value)
    {
        body.push_back(static_cast<BYTE>(value >> 24));
        body.push_back(static_cast<BYTE>(value >> 16));
        body.push_back(static_cast<BYTE>(value >> 8));
        body.push_back(static_cast<BYTE>(value));
    }

    void SendPacket(uint8_t playerId, BYTE packetType, const std::vector<BYTE>& body)
    {
        NetEvent event{};
        event.playerId = playerId;
        event.packetType = packetType;
        event.packetData = EncodePacket(packetType, body);
        if (event.packetData.empty() ||
            WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
            return;

        const auto connection = connections.find(playerId);
        if (connection == connections.end() ||
            connection->second.state != ConnectionState::Active ||
            !connection->second.sendContext) {
            SetEvent(hConnectionGate);
            return;
        }

        auto& context = *connection->second.sendContext;
        const bool queued = PushEventQueue(
            context.sendQueue, event,
            context.sendQueueGate, context.sendQueueReady);

        SetEvent(hConnectionGate);
        if (!queued)
            RequestDisconnect(playerId);
    }

    constexpr BYTE loadingPhase = 1;
    constexpr BYTE placingPhase = 2;
    constexpr BYTE roundResultPhase = 4;
    constexpr BYTE roundBeginType = 20;
    constexpr BYTE blockAssignType = 21;

    void AppendFloat(std::vector<BYTE>& body, float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        AppendUint32(body, bits);
    }
}

void BeginRound()
{
    if (gameState.players.empty() ||
        (gameState.phase != loadingPhase && gameState.phase != roundResultPhase))
        return;

    // 별은 맵에 하나만 두고, 전기 블록은 비활성 상태로 배정한다.
    constexpr BYTE blockTypes[] = { 0, 1, 2, 3, 5, 6, 8 };
    static std::mt19937 randomEngine{ std::random_device{}() };
    std::uniform_int_distribution<int> chooseBlock(0, 6);

    ++gameState.roundCnt;
    gameState.phase = placingPhase;
    gameState.roundWinnerId = 0;
    gameState.electricityActive = false;

    for (ObjectInfo& object : gameState.objects)
        object.used = false;

    std::vector<BYTE> roundBody = {
        static_cast<BYTE>(gameState.roundCnt >> 8),
        static_cast<BYTE>(gameState.roundCnt)
    };
    AppendFloat(roundBody, gameState.spawnX);
    AppendFloat(roundBody, gameState.spawnY);

    for (auto& [playerId, player] : gameState.players) {
        player.alive = true;
        player.placed = false;
        player.doubleJumpReady = false;
        player.motion = { playerId, gameState.spawnX, gameState.spawnY, 0.0f, 0.0f, false };
        player.lastmoveTime = 0;
        player.lastMoveSequence.reset();
        player.moveDirection = 0;
        player.previewGridX = 0;
        player.previewGridY = 0;
        player.previewVisible = false;
        player.assignedBlockType = blockTypes[chooseBlock(randomEngine)];

        SendPacket(playerId, roundBeginType, roundBody);
        SendPacket(playerId, blockAssignType, { player.assignedBlockType });
    }
}
