#include <vector>

#include "ReturnToLobby.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

void ReturnToLobby()
{
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE lobbyStateType = 2;
    constexpr BYTE returnLobbyType = 42;

    gameState.phase = lobbyPhase;
    gameState.roundCnt = 0;
    gameState.roundWinnerId = 0;
    gameState.electricityActive = false;
    gameState.lastPreviewSequences.clear();

    // 다음 경기에는 처음 읽은 기본 맵의 객체를 전송한다.
    gameState.objects = gameState.baseMapObjects;
    gameState.nextObjectId = static_cast<std::uint32_t>(gameState.objects.size()) + 1;

    std::vector<BYTE> lobbyBody{ static_cast<BYTE>(gameState.players.size()) };
    for (auto& [playerId, player] : gameState.players) {
        const uint8_t color = player.info.color;
        player = {};
        player.info = { playerId, color, false };
        player.motion.playerId = playerId;

        lobbyBody.push_back(playerId);
        lobbyBody.push_back(color);
        lobbyBody.push_back(0);
    }

    for (const auto& entry : gameState.players) {
        const uint8_t playerId = entry.first;
        if (!QueueServerTcpPacket(playerId, returnLobbyType, {})) {
            RequestDisconnect(playerId);
            continue;
        }
        if (!QueueServerTcpPacket(playerId, lobbyStateType, lobbyBody))
            RequestDisconnect(playerId);
    }
}
