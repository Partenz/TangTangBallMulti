#include "ServerVariable.h"

extern GameState gameState;

void HandleDoubleJump(std::uint8_t playerId, std::uint16_t roundCnt, std::uint8_t direction)
{
    constexpr BYTE racingPhase = 3;
    constexpr std::uint8_t leftDirection = 1;
    constexpr std::uint8_t rightDirection = 2;
    constexpr float horizontalSpeed = 140.0f;
    constexpr float doubleJumpSpeed = 500.0f;

    if (gameState.phase != racingPhase or 
        gameState.roundWinnerId != 0 or 
        roundCnt != gameState.roundCnt or
        (direction != leftDirection and direction != rightDirection)) {
        return;
    }

    const auto playerEntry = gameState.players.find(playerId);
    if (playerEntry == gameState.players.end()) {
        return;
    }

    PlayerState& player = playerEntry->second;
    if (not player.alive or not player.doubleJumpReady) {
        return;
    }

    // 다음 15ms 물리 갱신에서도 더블 클릭한 방향을 사용한다.
    player.moveDirection = direction;
    player.motion.vx = direction == leftDirection ? -horizontalSpeed : horizontalSpeed;
    player.motion.vy = -doubleJumpSpeed;
    player.motion.falling = false;
    player.doubleJumpReady = false;
}