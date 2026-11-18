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

    // SendMap이 다시 사용할 맵 크기, 시작 위치, 객체 목록은 보관한다.
    for (ObjectInfo& object : gameState.objects) {
        object.used = false;
        if (object.blockType == 7)
            object.blockType = 8;
    }

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
