#include <algorithm>
#include <cstdint>
#include <vector>

#include "ServerConnections.h"
#include "BeginRace.h"
#include "BeginRound.h"
#include "EndRound.h"
#include "ReturnToLobby.h"
#include "SendMap.h"
#include "ServerVariable.h"

void HandleDisconnect(uint8_t playerId)
{
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE loadingPhase = 1;
    constexpr BYTE placingPhase = 2;
    constexpr BYTE racingPhase = 3;
    constexpr BYTE lobbyStateType = 2;
    constexpr BYTE playerLeaveType = 8;

    if (gameState.players.erase(playerId) == 0)
        return;

    gameState.lastPreviewSequences.erase(playerId);

    const std::vector<BYTE> leaveBody{ playerId };
    for (const auto& entry : gameState.players) {
        if (!QueueServerTcpPacket(entry.first, playerLeaveType, leaveBody))
            RequestDisconnect(entry.first);
    }

    if (gameState.players.empty()) {
        ReturnToLobby();
        return;
    }

    switch (gameState.phase) {
    case lobbyPhase: {
        std::vector<BYTE> lobbyBody{ static_cast<BYTE>(gameState.players.size()) };
        bool queuedToEveryone = true;
        for (const auto& [id, player] : gameState.players) {
            lobbyBody.push_back(id);
            lobbyBody.push_back(player.info.color);
            lobbyBody.push_back(static_cast<BYTE>(player.info.ready));
        }
        for (const auto& entry : gameState.players) {
            if (!QueueServerTcpPacket(entry.first, lobbyStateType, lobbyBody)) {
                RequestDisconnect(entry.first);
                queuedToEveryone = false;
            }
        }

        const bool allReady = std::all_of(gameState.players.begin(), gameState.players.end(), [](const auto& entry) { return entry.second.info.ready; });
        if (queuedToEveryone && allReady) {
            for (const auto& entry : gameState.players) {
                if (!SendMap(entry.first)) {
                    if (gameState.phase == lobbyPhase)
                        break;
                    RequestDisconnect(entry.first);
                }
            }
        }
        break;
    }
    case loadingPhase: {
        const bool allMapReady = std::all_of(gameState.players.begin(), gameState.players.end(), [](const auto& entry) { return entry.second.mapReady; });
        if (allMapReady)
            BeginRound();
        break;
    }
    case placingPhase:
        BeginRace();
        break;
    case racingPhase: {
        const bool anyoneAlive = std::any_of(gameState.players.begin(), gameState.players.end(), [](const auto& entry) { return entry.second.alive; });
        if (!anyoneAlive)
            EndRound(0);
        break;
    }
    }
}
