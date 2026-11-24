#include <cstdint>
#include <vector>

#include "EndRound.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

extern GameState gameState;

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

    constexpr BYTE racingPhase = 3;
    constexpr BYTE roundResultPhase = 4;
    constexpr BYTE gameResultPhase = 5;
    constexpr BYTE roundResultType = 40;
    constexpr BYTE gameResultType = 41;
    constexpr uint8_t winningScore = 5;
}

void EndRound(uint8_t winnerId)
{
    if (gameState.phase != racingPhase)
        return;

    const auto winner = gameState.players.find(winnerId);
    if (winnerId != 0 && winner == gameState.players.end())
        return;

    // 결과 단계로 먼저 전환하여 같은 라운드의 중복 득점을 막는다.
    gameState.phase = roundResultPhase;
    gameState.roundWinnerId = winnerId;
    const bool gameFinished = winnerId != 0 && ++winner->second.score >= winningScore;

    std::vector<BYTE> roundBody = { static_cast<BYTE>(gameState.roundCnt >> 8), static_cast<BYTE>(gameState.roundCnt), winnerId };
    roundBody.reserve(roundBody.size() + gameState.players.size() * 2);
    for (const auto& [playerId, player] : gameState.players) {
        roundBody.push_back(playerId);
        roundBody.push_back(player.score);
    }

    for (const auto& entry : gameState.players)
        SendPacket(entry.first, roundResultType, roundBody);

    if (gameFinished) {
        gameState.objects = gameState.baseMapObjects;
        gameState.nextObjectId = static_cast<std::uint32_t>(gameState.objects.size()) + 1;
        gameState.electricityActive = false;
        gameState.phase = gameResultPhase;
        // GAME_RESULT는 ROUND_RESULT에서 roundCnt 두 바이트를 뺀 형식이다.
        const std::vector<BYTE> gameBody(roundBody.begin() + 2, roundBody.end());
        for (const auto& entry : gameState.players)
            SendPacket(entry.first, gameResultType, gameBody);
    }
}
