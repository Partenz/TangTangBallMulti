#include "HandleMoveInput.h"
#include "IsNewerSequence.h"
#include "ServerVariable.h"

extern GameState gameState;

void HandleMoveInput(uint8_t playerId, uint16_t roundCnt, uint32_t sequence, uint8_t moveDirection)
{
    constexpr BYTE racingPhase = 3;
    constexpr uint8_t rightMoveDirection = 2;

    if (gameState.phase != racingPhase or
        roundCnt != gameState.roundCnt or
        moveDirection > rightMoveDirection) {
        return;
    }

    const auto playerEntry = gameState.players.find(playerId);
    if (playerEntry == gameState.players.end() or not playerEntry->second.alive) {
        return;
    }

    PlayerState& player = playerEntry->second;

    if (player.lastMoveSequence.has_value()
        and not IsNewerSequence(sequence, *player.lastMoveSequence)) {
        return;
    }

    player.lastMoveSequence = sequence;
    player.moveDirection = moveDirection;
    player.lastmoveTime = GetTickCount64();
}