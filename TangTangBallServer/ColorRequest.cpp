#include <vector>

#include "ColorRequest.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

extern GameState gameState;

void ColorRequest(std::uint8_t playerId, std::uint8_t color)
{
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE lobbyStateType = 2;
    constexpr BYTE colorResponseType = 4;
    constexpr BYTE maxColor = 5;

    const auto player = gameState.players.find(playerId);
    const bool accepted = gameState.phase == lobbyPhase &&
        player != gameState.players.end() && color <= maxColor;

    // 요청자에게 색상 변경 승인 여부를 보낸다.
    const std::vector<BYTE> response{ static_cast<BYTE>(accepted) };
    if (!QueueServerTcpPacket(playerId, colorResponseType, response)) {
        RequestDisconnect(playerId);
        return;
    }

    if (!accepted) {
        return;
    }

    player->second.info.color = color;

    // 참가자 수에 이어 각 참가자의 번호, 색상, 준비 상태를 담는다.
    std::vector<BYTE> lobbyBody{ static_cast<BYTE>(gameState.players.size()) };
    for (const auto& [id, state] : gameState.players) {
        lobbyBody.push_back(id);
        lobbyBody.push_back(state.info.color);
        lobbyBody.push_back(static_cast<BYTE>(state.info.ready));
    }

    for (const auto& entry : gameState.players) {
        if (!QueueServerTcpPacket(entry.first, lobbyStateType, lobbyBody)) {
            RequestDisconnect(entry.first);
        }
    }
}