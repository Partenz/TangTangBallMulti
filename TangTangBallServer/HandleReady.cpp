#include <algorithm>
#include <vector>

#include "HandleReady.h"
#include "ServerConnections.h"
#include "ServerVariable.h"
#include "SendMap.h"

extern GameState gameState;

void HandleReady(std::uint8_t playerId, bool ready)
{
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE lobbyStateType = 2;
    constexpr BYTE readyResponseType = 6;

    const auto player = gameState.players.find(playerId);
    bool accepted = false;

    // 로비에 있는 참가자의 UDP 등록 여부를 확인한다.
    if (gameState.phase == lobbyPhase && player != gameState.players.end()) {
        if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
            return;
        }

        accepted = registeredUdpAddresses.contains(playerId);

        if (!SetEvent(hConnectionGate)) {
            return;
        }
    }

    // 요청자에게 준비 또는 준비 취소 요청의 승인 여부를 보낸다.
    const std::vector<BYTE> response{ static_cast<BYTE>(accepted) };
    if (!QueueServerTcpPacket(playerId, readyResponseType, response)) {
        RequestDisconnect(playerId);
        return;
    }

    if (!accepted) {
        return;
    }

    player->second.info.ready = ready;

    // 참가자 수와 참가자별 번호·색상·준비 상태를 담는다.
    std::vector<BYTE> lobbyBody{ static_cast<BYTE>(gameState.players.size()) };
    for (const auto& [id, state] : gameState.players) {
        lobbyBody.push_back(id);
        lobbyBody.push_back(state.info.color);
        lobbyBody.push_back(static_cast<BYTE>(state.info.ready));
    }

    bool queuedToEveryone = true;
    for (const auto& entry : gameState.players) {
        if (!QueueServerTcpPacket(entry.first, lobbyStateType, lobbyBody)) {
            RequestDisconnect(entry.first);
            queuedToEveryone = false;
        }
    }

    const bool allReady = std::all_of(gameState.players.begin(), gameState.players.end(),
        [](const auto& entry) { return entry.second.info.ready; });

    if (queuedToEveryone && allReady) {
        constexpr BYTE loadingPhase = 1;

        // 기본 맵이 준비된 뒤 전송을 시작한다.
        if (gameState.mapWidth == 0 || gameState.mapHeight == 0) {
            return;
        }

        gameState.phase = loadingPhase;

        for (auto& [id, player] : gameState.players) {
            player.mapReady = false;
        }

        for (const auto& [id, player] : gameState.players) {
            if (!SendMap(id)) {
                RequestDisconnect(id);
            }
        }
    }
}