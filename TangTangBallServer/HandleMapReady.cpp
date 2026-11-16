#include <algorithm>

#include "HandleMapReady.h"
#include "BeginRound.h"
#include "ServerVariable.h"

extern GameState gameState;

void HandleMapReady(std::uint8_t playerId)
{
    constexpr BYTE loadingPhase = 1;

    if (gameState.phase != loadingPhase) {
        return;
    }

    const auto player = gameState.players.find(playerId);
    if (player == gameState.players.end()) {
        return;
    }

    player->second.mapReady = true;

    const bool allMapReady = std::all_of(gameState.players.begin(), gameState.players.end(),
        [](const auto& entry) { return entry.second.mapReady; });

    if (allMapReady) {
        BeginRound();
    }
}